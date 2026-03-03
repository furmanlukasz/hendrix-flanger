#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>

/**
 * Fractional delay line with allpass interpolation.
 *
 * Uses a first-order allpass filter for fractional-sample interpolation,
 * which preserves high-frequency content better than linear interpolation.
 * The allpass has flat magnitude response — only phase shifts, no HF rolloff.
 *
 * Supports delay times from 0 to maxDelayMs. Used as the core building
 * block for the through-zero flanger.
 */
class DelayLine
{
public:
    void prepare(double sampleRate, float maxDelayMs)
    {
        sr = sampleRate;
        int maxSamples = static_cast<int>(std::ceil(maxDelayMs * 0.001 * sr)) + 2;
        buffer.assign(static_cast<size_t>(maxSamples), 0.0f);
        writePos = 0;
        maxDelay = maxDelayMs;
        apState = 0.0f;
    }

    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        apState = 0.0f;
    }

    void pushSample(float sample)
    {
        buffer[static_cast<size_t>(writePos)] = sample;
        writePos = (writePos + 1) % static_cast<int>(buffer.size());
    }

    float readSample(float delayMs) const
    {
        float delaySamples = delayMs * 0.001f * static_cast<float>(sr);
        delaySamples = juce::jlimit(0.0f, static_cast<float>(buffer.size() - 2), delaySamples);

        int intDelay = static_cast<int>(delaySamples);
        float frac = delaySamples - static_cast<float>(intDelay);

        int size = static_cast<int>(buffer.size());

        int index0 = writePos - 1 - intDelay;
        index0 = ((index0 % size) + size) % size;
        int index1 = ((index0 - 1) % size + size) % size;

        float s0 = buffer[static_cast<size_t>(index0)];
        float s1 = buffer[static_cast<size_t>(index1)];

        // Allpass interpolation: flat magnitude, phase-only shift.
        // Always run the allpass to keep apState continuously updated —
        // skipping the state update (e.g., via a linear fallback) causes
        // clicks at every integer-delay boundary crossing during LFO sweeps.
        // coeff ranges from 0 (frac=1) to 1 (frac=0), always stable.
        float coeff = (1.0f - frac) / (1.0f + frac);
        float out = s1 + coeff * (s0 - apState);
        apState = out;
        return out;
    }

    float getMaxDelayMs() const { return maxDelay; }

private:
    std::vector<float> buffer;
    int writePos = 0;
    double sr = 44100.0;
    float maxDelay = 20.0f;
    mutable float apState = 0.0f;  // allpass filter state
};
