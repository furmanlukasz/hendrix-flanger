#pragma once

#include <JuceHeader.h>
#include "dsp/ThroughZeroFlanger.h"
#include "dsp/DubEcho.h"

// Note division labels and multipliers (relative to quarter note)
static const juce::StringArray noteDivisionLabels {
    "1/1", "1/2d", "1/2", "1/4d", "1/4",
    "1/8d", "1/8", "1/16d", "1/16", "1/32"
};

static constexpr float noteDivisionMultipliers[] = {
    4.0f, 3.0f, 2.0f, 1.5f, 1.0f,
    0.75f, 0.5f, 0.375f, 0.25f, 0.125f
};

static constexpr int numNoteDivisions = 10;

struct FactoryPreset
{
    const char* name;
    float rate_hz;
    float depth;
    float manual_ms;
    float feedback;
    float stereo_spread;
    bool  through_zero;
    float env_amount;
    int   lfo_shape;
    float warmth;
    float dry_wet;
    // Dub Echo params
    bool  dub_enabled;
    int   dub_echo_div;   // note division index (0=1/1 .. 9=1/32)
    float dub_reverb;
    float dub_feedback;
    float dub_offset;
    float dub_autopan;
    float dub_fullness;
    float dub_space;
    float dub_dry;
    float dub_wet;
    float dub_volume;
};

static const FactoryPreset factoryPresets[] =
{
    //                      rate   depth  manual  fb     stereo  tz     env    lfo warm  mix    dub?  div  rev    fb     off    pan    full   space  dry    wet    vol
    // "Bold as Love" — slow deep TZF sweep, negative feedback
    { "Bold as Love",       0.18f, 85.0f, 4.5f, -60.0f, 120.0f, true,  0.0f, 0, 15.0f, 55.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Voodoo Child" — faster rate, moderate depth, positive feedback
    { "Voodoo Child",       0.8f,  65.0f, 3.5f,  55.0f,  90.0f, true,  0.0f, 0,  0.0f, 60.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Electric Ladyland" — extreme TZF with envelope follower
    { "Electric Ladyland",  0.4f,  90.0f, 5.0f,  40.0f, 150.0f, true, 70.0f, 0, 25.0f, 65.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Tape Machine" — subtle slow sweep, low feedback
    { "Tape Machine",       0.12f, 35.0f, 2.5f,  20.0f,  60.0f, false, 0.0f, 1, 40.0f, 45.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Jet Engine" — max depth TZF, high feedback, fast rate
    { "Jet Engine",         3.5f, 100.0f, 6.0f,  85.0f, 180.0f, true,  0.0f, 0,  0.0f, 75.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Dub Plate" — user favorite: very slow TZF, high feedback, warm, env-reactive, wide stereo
    { "Dub Plate",          0.10f, 65.0f, 3.5f,  75.0f, 180.0f, true, 50.0f, 0, 36.0f, 61.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Lead Shimmer" — crawling TZF, max feedback edge, warmth pushes harmonics, less env
    { "Lead Shimmer",       0.07f, 80.0f, 4.0f,  88.0f, 150.0f, true, 15.0f, 0, 20.0f, 68.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Dub Siren" — very slow with triangle LFO, high feedback resonance, warm and deep
    { "Dub Siren",          0.05f, 70.0f, 5.5f,  60.0f, 120.0f, true,  0.0f, 1, 45.0f, 65.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Warm Nebula" — glacial sweep, moderate feedback, max warmth saturation, wide
    { "Warm Nebula",        0.08f, 50.0f, 3.0f,  60.0f, 180.0f, true, 30.0f, 0, 55.0f, 58.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Resonant Space" — slow TZF, negative feedback for hollow tone, warm overtones
    { "Resonant Space",     0.12f, 75.0f, 4.5f, -70.0f, 140.0f, true, 20.0f, 0, 40.0f, 62.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Random Chop" — fast S&H, long delay, positive feedback, warm and env-reactive
    { "Random Chop",        6.91f, 80.0f, 7.59f, 49.0f, 150.0f, true, 53.4f, 2, 68.3f, 44.4f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Glitch Hollow" — fast S&H, short delay, negative feedback, wide stereo
    { "Glitch Hollow",      6.91f, 63.2f, 2.48f,-50.7f, 180.0f, true, 53.4f, 2, 41.6f, 45.0f, false, 6, 30.0f, 40.0f, 50.0f, 0.0f, 20.0f, 40.0f, 100.0f, 50.0f, 80.0f },

    // "Dub Tape" — slow TZF flanger with warm echo, reverb, and stereo movement
    { "Dub Tape",           0.15f, 60.0f, 3.5f,  50.0f, 140.0f, true, 20.0f, 0, 35.0f, 58.0f, true, 6, 45.0f, 55.0f, 60.0f, 25.0f, 30.0f, 50.0f, 80.0f, 45.0f, 75.0f },

    // "Space Echo" — medium TZF with long echo, deep reverb, wide stereo
    { "Space Echo",         0.25f, 70.0f, 4.0f,  40.0f, 180.0f, true, 10.0f, 0, 25.0f, 62.0f, true, 4, 60.0f, 70.0f, 40.0f, 15.0f, 45.0f, 65.0f, 70.0f, 55.0f, 80.0f },
};

static constexpr int numFactoryPresets = static_cast<int>(sizeof(factoryPresets) / sizeof(factoryPresets[0]));

class HendrixFlangerProcessor : public juce::AudioProcessor
{
public:
    HendrixFlangerProcessor();
    ~HendrixFlangerProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return numFactoryPresets; }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    ThroughZeroFlanger flanger;
    DubEcho dubEcho;
    int currentPreset = 0;

    // Pre-allocated buffer for dry signal (avoids heap alloc on audio thread)
    juce::AudioBuffer<float> dryBuffer;

    // Per-sample smoothing for dry/wet mix (avoids pops/clicks on knob moves)
    float smoothedDryWet = 0.5f;
    float dryWetSmoothingCoeff = 0.99f;  // ~5 ms, set in prepareToPlay
    bool  dryWetNeedsSnap = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HendrixFlangerProcessor)
};
