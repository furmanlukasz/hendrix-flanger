#pragma once

#include <JuceHeader.h>
#include "DelayLine.h"
#include "LFO.h"
#include "EnvelopeFollower.h"

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
 *           └── wetDelay (modulated by LFO) ──── wet path
 *           └── feedback ◄─── wet output
 *
 * When LFO sweeps the wet delay through manual_ms, the two paths align
 * and then cross, producing the characteristic "jet engine" sweep.
 */
class ThroughZeroFlanger
{
public:
    static constexpr float MAX_DELAY_MS = 20.0f;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        for (int ch = 0; ch < 2; ++ch)
        {
            dryDelay[ch].prepare(sampleRate, MAX_DELAY_MS);
            wetDelay[ch].prepare(sampleRate, MAX_DELAY_MS);
            lfo[ch].prepare(sampleRate);
            envFollower[ch].prepare(sampleRate);
            feedbackState[ch] = 0.0f;
        }
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            dryDelay[ch].reset();
            wetDelay[ch].reset();
            lfo[ch].reset();
            envFollower[ch].reset();
            feedbackState[ch] = 0.0f;
        }
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

    void process(juce::AudioBuffer<float>& buffer)
    {
        int numChannels = buffer.getNumChannels();
        int numSamples  = buffer.getNumSamples();

        // Update LFO parameters
        for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
        {
            lfo[ch].setRate(rateHz);
            lfo[ch].setShape(lfoShape);
            lfo[ch].setPhaseOffset(ch == 0 ? 0.0f : stereoSpreadDeg);
        }

        for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);

            for (int s = 0; s < numSamples; ++s)
            {
                float input = data[s];

                // Envelope follower modulation
                float envMod = envFollower[ch].processSample(input) * envAmount;

                // LFO output [-1, +1] scaled to delay modulation
                float lfoVal = lfo[ch].getNextSample();
                float modulatedDepth = depth + envMod;
                modulatedDepth = juce::jlimit(0.0f, 1.0f, modulatedDepth);

                // Wet delay = manual +/- (depth * manual) modulated by LFO
                float sweepRange = manualMs * modulatedDepth;
                float wetDelayMs = manualMs + lfoVal * sweepRange;
                wetDelayMs = juce::jlimit(0.0f, MAX_DELAY_MS, wetDelayMs);

                // Push input + feedback into wet delay line
                float wetInput = input + feedbackState[ch] * feedback;
                wetDelay[ch].pushSample(wetInput);
                float wetOut = wetDelay[ch].readSample(wetDelayMs);

                // Store feedback (soft clip to prevent runaway)
                feedbackState[ch] = std::tanh(wetOut);

                // Dry path
                float dryOut;
                if (throughZeroEnabled)
                {
                    // In TZF mode, dry is delayed by manual_ms so the wet
                    // path can sweep through it
                    dryDelay[ch].pushSample(input);
                    dryOut = dryDelay[ch].readSample(manualMs);
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

    // Parameters
    float rateHz = 0.5f;
    float depth = 0.5f;           // 0..1
    float manualMs = 3.0f;
    float feedback = 0.3f;        // -0.95..+0.95
    float stereoSpreadDeg = 90.0f;
    bool  throughZeroEnabled = true;
    float envAmount = 0.0f;       // 0..1
    LFO::Shape lfoShape = LFO::Shape::Sine;

    // Per-channel DSP
    DelayLine dryDelay[2];
    DelayLine wetDelay[2];
    LFO       lfo[2];
    EnvelopeFollower envFollower[2];
    float feedbackState[2] = { 0.0f, 0.0f };
};
