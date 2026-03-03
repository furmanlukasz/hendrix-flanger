#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>

/**
 * Fractional delay line with linear interpolation.
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
    }

    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
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

        int index0 = writePos - 1 - static_cast<int>(delaySamples);
        float frac = delaySamples - std::floor(delaySamples);

        int size = static_cast<int>(buffer.size());
        index0 = ((index0 % size) + size) % size;
        int index1 = ((index0 - 1) % size + size) % size;

        return buffer[static_cast<size_t>(index0)] * (1.0f - frac)
             + buffer[static_cast<size_t>(index1)] * frac;
    }

    float getMaxDelayMs() const { return maxDelay; }

private:
    std::vector<float> buffer;
    int writePos = 0;
    double sr = 44100.0;
    float maxDelay = 20.0f;
};
