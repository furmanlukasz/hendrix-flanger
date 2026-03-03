#pragma once

#include <JuceHeader.h>
#include "LFO.h"
#include "DCBlocker.h"
#include <cmath>
#include <vector>
#include <algorithm>

/**
 * Dub Echo DSP module — stereo delay + Schroeder reverb + autopan.
 *
 * Designed to be placed BEFORE the flanger in the signal chain,
 * adding dub-style echo, reverb, and stereo movement.
 *
 * Architecture:
 *   input → stereo delay (with feedback + fullness saturation)
 *         → reverb send (Schroeder: 4 comb || 2 allpass series)
 *         → autopan LFO
 *         → dry/wet mix → volume → output
 *
 * All parameters are smoothed per-sample (~5 ms time constant)
 * with deferred snap on reset.
 */
class DubEcho
{
public:
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        smoothingCoeff = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.005)));

        // Echo delay lines — max 1100 ms
        int maxEchoSamples = static_cast<int>(std::ceil(1.1 * sampleRate)) + 2;
        for (int ch = 0; ch < 2; ++ch)
        {
            echoBuffer[ch].assign(static_cast<size_t>(maxEchoSamples), 0.0f);
            echoWritePos[ch] = 0;
            echoFeedbackState[ch] = 0.0f;
        }

        // Comb filter delay lines for reverb
        // Schroeder comb times: 29.7, 37.1, 41.1, 43.7 ms
        static constexpr float combTimesMs[NUM_COMBS] = { 29.7f, 37.1f, 41.1f, 43.7f };
        for (int i = 0; i < NUM_COMBS; ++i)
        {
            int combSamples = static_cast<int>(std::ceil(combTimesMs[i] * 0.001 * sampleRate)) + 2;
            for (int ch = 0; ch < 2; ++ch)
            {
                combBuffer[i][ch].assign(static_cast<size_t>(combSamples), 0.0f);
                combWritePos[i][ch] = 0;
                combDelaySamples[i] = static_cast<int>(combTimesMs[i] * 0.001f * static_cast<float>(sampleRate));
            }
        }

        // Allpass delay lines for reverb
        // Schroeder allpass times: 5.0, 1.7 ms
        static constexpr float apTimesMs[NUM_ALLPASS] = { 5.0f, 1.7f };
        for (int i = 0; i < NUM_ALLPASS; ++i)
        {
            int apSamples = static_cast<int>(std::ceil(apTimesMs[i] * 0.001 * sampleRate)) + 2;
            for (int ch = 0; ch < 2; ++ch)
            {
                apBuffer[i][ch].assign(static_cast<size_t>(apSamples), 0.0f);
                apWritePos[i][ch] = 0;
                apDelaySamples[i] = static_cast<int>(apTimesMs[i] * 0.001f * static_cast<float>(sampleRate));
            }
        }

        // Autopan LFO
        for (int ch = 0; ch < 2; ++ch)
        {
            autopanLfo[ch].prepare(sampleRate);
            dcBlocker[ch].prepare(sampleRate);
        }

        // Init smoothed values
        smoothEcho     = echoMs;
        smoothReverb   = reverb;
        smoothFeedback = feedback;
        smoothOffset   = offset;
        smoothAutopan  = autopan;
        smoothFullness = fullness;
        smoothSpace    = space;
        smoothDry      = dry;
        smoothWet      = wet;
        smoothVolume   = volume;
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            std::fill(echoBuffer[ch].begin(), echoBuffer[ch].end(), 0.0f);
            echoWritePos[ch] = 0;
            echoFeedbackState[ch] = 0.0f;
            dcBlocker[ch].reset();
            autopanLfo[ch].reset();
        }

        for (int i = 0; i < NUM_COMBS; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                std::fill(combBuffer[i][ch].begin(), combBuffer[i][ch].end(), 0.0f);
                combWritePos[i][ch] = 0;
            }

        for (int i = 0; i < NUM_ALLPASS; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                std::fill(apBuffer[i][ch].begin(), apBuffer[i][ch].end(), 0.0f);
                apWritePos[i][ch] = 0;
            }

        smoothingNeedsSnap = true;
    }

    // --- Parameter setters ---
    void setEcho(float ms)          { echoMs   = juce::jlimit(0.0f, 1000.0f, ms); }
    void setReverb(float pct)       { reverb   = pct * 0.01f; }
    void setFeedback(float pct)     { feedback = juce::jlimit(0.0f, 0.95f, pct * 0.01f); }
    void setOffset(float pct)       { offset   = pct * 0.01f; }
    void setAutopan(float pct)      { autopan  = pct * 0.01f; }
    void setFullness(float pct)     { fullness = pct * 0.01f; }
    void setSpace(float pct)        { space    = pct * 0.01f; }
    void setDry(float pct)          { dry      = pct * 0.01f; }
    void setWet(float pct)          { wet      = pct * 0.01f; }
    void setVolume(float pct)       { volume   = pct * 0.01f; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        int numChannels = buffer.getNumChannels();
        int numSamples  = buffer.getNumSamples();

        // Snap smoothed values after reset
        if (smoothingNeedsSnap)
        {
            smoothEcho     = echoMs;
            smoothReverb   = reverb;
            smoothFeedback = feedback;
            smoothOffset   = offset;
            smoothAutopan  = autopan;
            smoothFullness = fullness;
            smoothSpace    = space;
            smoothDry      = dry;
            smoothWet      = wet;
            smoothVolume   = volume;
            smoothingNeedsSnap = false;
        }

        for (int s = 0; s < numSamples; ++s)
        {
            // Smooth all parameters
            const float c = smoothingCoeff;
            const float ic = 1.0f - c;
            smoothEcho     = smoothEcho     * c + echoMs   * ic;
            smoothReverb   = smoothReverb   * c + reverb   * ic;
            smoothFeedback = smoothFeedback * c + feedback  * ic;
            smoothOffset   = smoothOffset   * c + offset   * ic;
            smoothAutopan  = smoothAutopan  * c + autopan  * ic;
            smoothFullness = smoothFullness * c + fullness  * ic;
            smoothSpace    = smoothSpace    * c + space    * ic;
            smoothDry      = smoothDry      * c + dry      * ic;
            smoothWet      = smoothWet      * c + wet      * ic;
            smoothVolume   = smoothVolume   * c + volume   * ic;

            // Autopan LFO — get L/R gain
            autopanLfo[0].setRate(0.5f);  // fixed slow rate
            autopanLfo[0].setShape(LFO::Shape::Sine);
            float panLfo = autopanLfo[0].getNextSample();  // [-1, +1]
            float panAmount = smoothAutopan;
            // L gets louder when panLfo > 0, R when panLfo < 0
            float panL = 1.0f - panAmount * juce::jmax(0.0f, -panLfo);
            float panR = 1.0f - panAmount * juce::jmax(0.0f,  panLfo);

            for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                float input = data[s];

                // --- Echo delay ---
                // Per-channel delay: L uses echoMs, R uses echoMs + offset
                float delayMs = smoothEcho;
                if (ch == 1)
                    delayMs += smoothOffset * 30.0f;  // up to 30ms offset

                float delaySamples = delayMs * 0.001f * static_cast<float>(sr);
                delaySamples = juce::jlimit(0.0f,
                    static_cast<float>(echoBuffer[ch].size() - 2), delaySamples);

                // Read from echo delay (linear interpolation for long delays)
                int intDelay = static_cast<int>(delaySamples);
                float frac = delaySamples - static_cast<float>(intDelay);
                int size = static_cast<int>(echoBuffer[ch].size());
                int idx0 = ((echoWritePos[ch] - 1 - intDelay) % size + size) % size;
                int idx1 = ((idx0 - 1) % size + size) % size;
                float echoOut = echoBuffer[ch][static_cast<size_t>(idx0)] * (1.0f - frac)
                              + echoBuffer[ch][static_cast<size_t>(idx1)] * frac;

                // Fullness saturation on feedback path
                float fb = echoFeedbackState[ch] * smoothFeedback;
                if (smoothFullness > 0.001f)
                {
                    float saturated = std::tanh(fb * (1.0f + smoothFullness * 2.0f));
                    fb = fb * (1.0f - smoothFullness) + saturated * smoothFullness;
                }

                // Write to echo delay
                echoBuffer[ch][static_cast<size_t>(echoWritePos[ch])] = input + fb;
                echoWritePos[ch] = (echoWritePos[ch] + 1) % size;

                // Store feedback state (soft clip)
                echoFeedbackState[ch] = std::tanh(echoOut);

                // --- Schroeder reverb ---
                float reverbIn = echoOut * smoothSpace;
                float reverbOut = processReverb(ch, reverbIn, smoothReverb);

                // DC block the reverb output
                reverbOut = dcBlocker[ch].processSample(reverbOut);

                // --- Mix echo + reverb ---
                float wetSignal = echoOut + reverbOut;

                // Autopan
                float panGain = (ch == 0) ? panL : panR;
                wetSignal *= panGain;

                // Dry/wet mix
                float output = input * smoothDry + wetSignal * smoothWet;

                // Master volume
                output *= smoothVolume;

                data[s] = output;
            }
        }
    }

private:
    static constexpr int NUM_COMBS = 4;
    static constexpr int NUM_ALLPASS = 2;

    double sr = 44100.0;

    // --- Target parameter values ---
    float echoMs   = 300.0f;
    float reverb   = 0.3f;
    float feedback  = 0.4f;
    float offset   = 0.5f;
    float autopan  = 0.0f;
    float fullness  = 0.2f;
    float space    = 0.4f;
    float dry      = 1.0f;
    float wet      = 0.5f;
    float volume   = 0.8f;

    // --- Smoothed values ---
    float smoothEcho     = 300.0f;
    float smoothReverb   = 0.3f;
    float smoothFeedback = 0.4f;
    float smoothOffset   = 0.5f;
    float smoothAutopan  = 0.0f;
    float smoothFullness = 0.2f;
    float smoothSpace    = 0.4f;
    float smoothDry      = 1.0f;
    float smoothWet      = 0.5f;
    float smoothVolume   = 0.8f;
    float smoothingCoeff = 0.99f;
    bool  smoothingNeedsSnap = true;

    // --- Echo delay lines (stereo) ---
    std::vector<float> echoBuffer[2];
    int echoWritePos[2] = { 0, 0 };
    float echoFeedbackState[2] = { 0.0f, 0.0f };

    // --- Schroeder reverb ---
    // Comb filters
    std::vector<float> combBuffer[NUM_COMBS][2];
    int combWritePos[NUM_COMBS][2] = {};
    int combDelaySamples[NUM_COMBS] = {};

    // Allpass filters
    std::vector<float> apBuffer[NUM_ALLPASS][2];
    int apWritePos[NUM_ALLPASS][2] = {};
    int apDelaySamples[NUM_ALLPASS] = {};

    // Autopan + DC blocker
    LFO autopanLfo[2];
    DCBlocker dcBlocker[2];

    // --- Comb filter: y[n] = x[n] + g * y[n-M] ---
    float processComb(int combIdx, int ch, float input, float g)
    {
        auto& buf = combBuffer[combIdx][ch];
        int& wp = combWritePos[combIdx][ch];
        int M = combDelaySamples[combIdx];
        int sz = static_cast<int>(buf.size());

        int readPos = ((wp - M) % sz + sz) % sz;
        float delayed = buf[static_cast<size_t>(readPos)];
        float output = delayed;

        buf[static_cast<size_t>(wp)] = input + g * delayed;
        wp = (wp + 1) % sz;

        return output;
    }

    // --- Allpass filter: y[n] = -g*x[n] + x[n-M] + g*y[n-M] ---
    float processAllpass(int apIdx, int ch, float input, float g)
    {
        auto& buf = apBuffer[apIdx][ch];
        int& wp = apWritePos[apIdx][ch];
        int M = apDelaySamples[apIdx];
        int sz = static_cast<int>(buf.size());

        int readPos = ((wp - M) % sz + sz) % sz;
        float delayed = buf[static_cast<size_t>(readPos)];
        float output = -g * input + delayed;

        buf[static_cast<size_t>(wp)] = input + g * delayed;
        wp = (wp + 1) % sz;

        return output;
    }

    float processReverb(int ch, float input, float reverbAmount)
    {
        if (reverbAmount < 0.001f)
            return 0.0f;

        // Feedback gain for comb filters — controls decay time
        float combG = 0.7f * reverbAmount;

        // 4 parallel comb filters
        float combSum = 0.0f;
        for (int i = 0; i < NUM_COMBS; ++i)
            combSum += processComb(i, ch, input, combG);
        combSum *= 0.25f;  // average

        // 2 series allpass filters (fixed g = 0.7)
        float ap = combSum;
        for (int i = 0; i < NUM_ALLPASS; ++i)
            ap = processAllpass(i, ch, ap, 0.7f);

        return ap * reverbAmount;
    }
};
