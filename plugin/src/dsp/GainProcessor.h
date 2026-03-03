#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Example DSP module: simple gain processor.
 *
 * This demonstrates the pattern for self-contained DSP modules:
 *   - prepare()  — called once when sample rate / block size change
 *   - process()  — called per audio block
 *   - setXxx()   — parameter setters (called from processBlock)
 *
 * TODO: Replace or extend this with your own DSP modules.
 */
class GainProcessor
{
public:
    void prepare(double /*sampleRate*/, int /*maxBlockSize*/)
    {
        smoothedGain.reset(64); // 64-sample smoothing
        smoothedGain.setCurrentAndTargetValue(1.0f);
    }

    void setGainDb(float db)
    {
        smoothedGain.setTargetValue(juce::Decibels::decibelsToGain(db));
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        for (int s = 0; s < buffer.getNumSamples(); ++s)
        {
            float g = smoothedGain.getNextValue();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.getWritePointer(ch)[s] *= g;
        }
    }

private:
    juce::SmoothedValue<float> smoothedGain { 1.0f };
};
