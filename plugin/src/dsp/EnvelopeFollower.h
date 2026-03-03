#pragma once

#include <JuceHeader.h>
#include <cmath>

/**
 * Simple envelope follower for dynamic modulation.
 *
 * Tracks the amplitude envelope of the input signal.
 * Output is smoothed and normalized to [0, 1] range.
 * Used to modulate flanger depth from input dynamics.
 */
class EnvelopeFollower
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        envelope = 0.0f;
        updateCoefficients();
    }

    void reset()
    {
        envelope = 0.0f;
    }

    void setAttackMs(float ms)
    {
        attackMs = ms;
        updateCoefficients();
    }

    void setReleaseMs(float ms)
    {
        releaseMs = ms;
        updateCoefficients();
    }

    float processSample(float input)
    {
        float rectified = std::abs(input);

        if (rectified > envelope)
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * rectified;
        else
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * rectified;

        return envelope;
    }

private:
    void updateCoefficients()
    {
        if (sr <= 0.0) return;
        attackCoeff  = std::exp(-1.0f / (static_cast<float>(sr) * attackMs  * 0.001f));
        releaseCoeff = std::exp(-1.0f / (static_cast<float>(sr) * releaseMs * 0.001f));
    }

    double sr = 44100.0;
    float envelope = 0.0f;
    float attackMs = 5.0f;
    float releaseMs = 50.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
};
