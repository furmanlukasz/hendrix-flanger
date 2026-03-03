#pragma once

#include <JuceHeader.h>
#include "DelayLine.h"
#include "LFO.h"
#include "EnvelopeFollower.h"
#include "DCBlocker.h"

/**
 * Through-Zero Flanger DSP module.
 *
 * Inspired by tape-based flanging techniques used on classic Hendrix
 * recordings and the Foxrox Paradox TZF pedal.
 *
 * In through-zero mode, the dry signal is also delayed by the base amount
 * so the modulated delay can sweep *through* the dry signal's time position,
 * causing deep phase cancellations at the zero-crossing point.
 *
 * Architecture:
 *   input ──┬── dryDelay (fixed = manual_ms) ──── dry path
 *           └── wetDelay (modulated by LFO) ──── DC blocker ──── wet path
 *           └── feedback ◄─── [optional warmth] ◄─── wet output
 *
 * v1.1 improvements:
 *   - Allpass interpolation in delay lines (less HF rolloff)
 *   - DC blocking filter on wet path (~5 Hz high-pass)
 *   - Exponential smoothing on all DSP parameters (no zipper noise)
 *   - Optional subtle saturation in feedback path
 */
class ThroughZeroFlanger
{
public:
    static constexpr float MAX_DELAY_MS = 20.0f;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        smoothingCoeff = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.005)));

        for (int ch = 0; ch < 2; ++ch)
        {
            dryDelay[ch].prepare(sampleRate, MAX_DELAY_MS);
            wetDelay[ch].prepare(sampleRate, MAX_DELAY_MS);
            lfo[ch].prepare(sampleRate);
            envFollower[ch].prepare(sampleRate);
            dcBlocker[ch].prepare(sampleRate);
            feedbackState[ch] = 0.0f;
        }

        // Initialize smoothed values to current targets
        smoothRate = rateHz;
        smoothDepth = depth;
        smoothManual = manualMs;
        smoothFeedback = feedback;
        smoothStereo = stereoSpreadDeg;
        smoothEnv = envAmount;
        smoothWarmth = warmth;
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            dryDelay[ch].reset();
            wetDelay[ch].reset();
            lfo[ch].reset();
            envFollower[ch].reset();
            dcBlocker[ch].reset();
            feedbackState[ch] = 0.0f;
        }

        // Defer smoothing snap to next process() call, when targets will be current
        smoothingNeedsSnap = true;
    }

    // --- Parameter setters ---
    void setRate(float hz)                { rateHz = hz; }
    void setDepth(float pct)              { depth = pct * 0.01f; }
    void setManualMs(float ms)            { manualMs = juce::jlimit(0.0f, MAX_DELAY_MS, ms); }
    void setFeedback(float pct)           { feedback = juce::jlimit(-0.95f, 0.95f, pct * 0.01f); }
    void setStereoSpread(float degrees)   { stereoSpreadDeg = degrees; }
    void setThroughZero(bool enabled)     { throughZeroEnabled = enabled; }
    void setEnvAmount(float pct)          { envAmount = pct * 0.01f; }
    void setLfoShape(int shapeIndex)      { lfoShape = static_cast<LFO::Shape>(shapeIndex); }
    void setWarmth(float pct)             { warmth = pct * 0.01f; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        int numChannels = buffer.getNumChannels();
        int numSamples  = buffer.getNumSamples();

        // Snap smoothed values to current targets after reset (deferred so
        // targets are up-to-date from processBlock's parameter reads)
        if (smoothingNeedsSnap)
        {
            smoothRate     = rateHz;
            smoothDepth    = depth;
            smoothManual   = manualMs;
            smoothFeedback = feedback;
            smoothStereo   = stereoSpreadDeg;
            smoothEnv      = envAmount;
            smoothWarmth   = warmth;
            smoothingNeedsSnap = false;
        }

        for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);

            for (int s = 0; s < numSamples; ++s)
            {
                // Smooth all parameters (exponential one-pole)
                smoothRate     = smoothRate     * smoothingCoeff + rateHz          * (1.0f - smoothingCoeff);
                smoothDepth    = smoothDepth    * smoothingCoeff + depth           * (1.0f - smoothingCoeff);
                smoothManual   = smoothManual   * smoothingCoeff + manualMs        * (1.0f - smoothingCoeff);
                smoothFeedback = smoothFeedback * smoothingCoeff + feedback        * (1.0f - smoothingCoeff);
                smoothStereo   = smoothStereo   * smoothingCoeff + stereoSpreadDeg * (1.0f - smoothingCoeff);
                smoothEnv      = smoothEnv      * smoothingCoeff + envAmount       * (1.0f - smoothingCoeff);
                smoothWarmth   = smoothWarmth   * smoothingCoeff + warmth          * (1.0f - smoothingCoeff);

                // Update LFO for this channel
                lfo[ch].setRate(smoothRate);
                lfo[ch].setShape(lfoShape);
                lfo[ch].setPhaseOffset(ch == 0 ? 0.0f : smoothStereo);

                float input = data[s];

                // Envelope follower modulation
                float envMod = envFollower[ch].processSample(input) * smoothEnv;

                // LFO output [-1, +1] scaled to delay modulation
                float lfoVal = lfo[ch].getNextSample();
                float modulatedDepth = smoothDepth + envMod;
                modulatedDepth = juce::jlimit(0.0f, 1.0f, modulatedDepth);

                // Wet delay = manual +/- (depth * manual) modulated by LFO
                float sweepRange = smoothManual * modulatedDepth;
                float wetDelayMs = smoothManual + lfoVal * sweepRange;
                wetDelayMs = juce::jlimit(0.0f, MAX_DELAY_MS, wetDelayMs);

                // Feedback path with optional warmth (soft saturation)
                float fb = feedbackState[ch] * smoothFeedback;
                if (smoothWarmth > 0.001f)
                {
                    // Blend between clean and saturated feedback
                    float saturated = std::tanh(fb * (1.0f + smoothWarmth * 2.0f));
                    fb = fb * (1.0f - smoothWarmth) + saturated * smoothWarmth;
                }

                // Push input + feedback into wet delay line
                float wetInput = input + fb;
                wetDelay[ch].pushSample(wetInput);
                float wetOut = wetDelay[ch].readSample(wetDelayMs);

                // DC blocking on wet path
                wetOut = dcBlocker[ch].processSample(wetOut);

                // Store feedback (soft clip to prevent runaway)
                feedbackState[ch] = std::tanh(wetOut);

                // Dry path
                float dryOut;
                if (throughZeroEnabled)
                {
                    dryDelay[ch].pushSample(input);
                    dryOut = dryDelay[ch].readSample(smoothManual);
                }
                else
                {
                    dryOut = input;
                }

                // Output is sum of dry and wet (flanging = comb filter)
                data[s] = dryOut + wetOut;
            }
        }
    }

private:
    double sr = 44100.0;

    // Parameters (target values — smoothed per-sample)
    float rateHz = 0.5f;
    float depth = 0.5f;           // 0..1
    float manualMs = 3.0f;
    float feedback = 0.3f;        // -0.95..+0.95
    float stereoSpreadDeg = 90.0f;
    bool  throughZeroEnabled = true;
    float envAmount = 0.0f;       // 0..1
    LFO::Shape lfoShape = LFO::Shape::Sine;
    float warmth = 0.0f;          // 0..1

    // Smoothed parameter values
    float smoothRate = 0.5f;
    float smoothDepth = 0.5f;
    float smoothManual = 3.0f;
    float smoothFeedback = 0.3f;
    float smoothStereo = 90.0f;
    float smoothEnv = 0.0f;
    float smoothWarmth = 0.0f;
    float smoothingCoeff = 0.99f;  // ~5 ms time constant
    bool  smoothingNeedsSnap = true;

    // Per-channel DSP
    DelayLine dryDelay[2];
    DelayLine wetDelay[2];
    LFO       lfo[2];
    EnvelopeFollower envFollower[2];
    DCBlocker dcBlocker[2];
    float feedbackState[2] = { 0.0f, 0.0f };
};
