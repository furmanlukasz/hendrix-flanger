#pragma once

#include <JuceHeader.h>
#include <cmath>

/**
 * Low-frequency oscillator for modulation.
 *
 * Supports sine, triangle, and sample-and-hold waveforms.
 * Output range is [-1, +1]. Phase offset allows stereo spread.
 */
class LFO
{
public:
    enum class Shape
    {
        Sine = 0,
        Triangle,
        SampleAndHold
    };

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        phase = 0.0;
        sAndHValue = 0.0f;
        sAndHPhase = 0.0;
    }

    void reset()
    {
        phase = 0.0;
        sAndHValue = 0.0f;
        sAndHPhase = 0.0;
    }

    void setRate(float hz)        { rateHz = hz; }
    void setShape(Shape s)        { shape = s; }
    void setPhaseOffset(float deg) { phaseOffsetRad = deg * (juce::MathConstants<float>::pi / 180.0f); }

    float getNextSample()
    {
        double inc = rateHz / sr;
        phase += inc;
        if (phase >= 1.0)
            phase -= 1.0;

        double p = phase + static_cast<double>(phaseOffsetRad) / (2.0 * juce::MathConstants<double>::pi);
        if (p >= 1.0) p -= 1.0;
        if (p < 0.0) p += 1.0;

        float out = 0.0f;

        switch (shape)
        {
            case Shape::Sine:
                out = static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * p));
                break;

            case Shape::Triangle:
                out = static_cast<float>(2.0 * std::abs(2.0 * p - 1.0) - 1.0);
                break;

            case Shape::SampleAndHold:
            {
                sAndHPhase += inc;
                if (sAndHPhase >= 1.0)
                {
                    sAndHPhase -= 1.0;
                    sAndHValue = static_cast<float>(rng.nextFloat() * 2.0f - 1.0f);
                }
                out = sAndHValue;
                break;
            }
        }

        return out;
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
    float rateHz = 0.5f;
    Shape shape = Shape::Sine;
    float phaseOffsetRad = 0.0f;

    // Sample-and-hold state
    float sAndHValue = 0.0f;
    double sAndHPhase = 0.0;
    juce::Random rng;
};
