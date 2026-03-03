#pragma once

#include <cmath>

/**
 * First-order DC blocking filter (high-pass at ~5 Hz).
 *
 * Transfer function: H(z) = (1 - z^-1) / (1 - R * z^-1)
 * where R controls the cutoff frequency: R = 1 - (2*pi*fc/fs)
 *
 * Removes DC offset that can accumulate in the feedback path
 * without audibly affecting the signal.
 */
class DCBlocker
{
public:
    void prepare(double sampleRate)
    {
        // Cutoff ~5 Hz: R approaches 1.0 for low cutoff
        float fc = 5.0f;
        R = 1.0f - (2.0f * 3.14159265f * fc / static_cast<float>(sampleRate));
        xPrev = 0.0f;
        yPrev = 0.0f;
    }

    void reset()
    {
        xPrev = 0.0f;
        yPrev = 0.0f;
    }

    float processSample(float x)
    {
        float y = x - xPrev + R * yPrev;
        xPrev = x;
        yPrev = y;
        return y;
    }

private:
    float R = 0.9993f;
    float xPrev = 0.0f;
    float yPrev = 0.0f;
};
