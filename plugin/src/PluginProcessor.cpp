#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MyPluginProcessor::MyPluginProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

MyPluginProcessor::~MyPluginProcessor() {}

//==============================================================================
// TODO: Define your plugin parameters here
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
MyPluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Example: gain parameter in dB
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"gain_db", 1},
        "Gain",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f),
        0.0f,   // default
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // Example: dry/wet mix
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"dry_wet", 1},
        "Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Example: bypass toggle
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypass", 1},
        "Bypass",
        false));

    return { params.begin(), params.end() };
}

//==============================================================================
void MyPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP modules
    gainProcessor.prepare(sampleRate, samplesPerBlock);
}

void MyPluginProcessor::releaseResources() {}

bool MyPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Support mono and stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

//==============================================================================
// TODO: Implement your audio processing here
//==============================================================================
void MyPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters
    float gainDb = apvts.getRawParameterValue("gain_db")->load();
    float dryWet = apvts.getRawParameterValue("dry_wet")->load() / 100.0f;
    bool bypass  = apvts.getRawParameterValue("bypass")->load() > 0.5f;

    if (bypass)
        return;

    // Store dry signal for mix
    juce::AudioBuffer<float> dryBuffer;
    if (dryWet < 1.0f)
    {
        dryBuffer.makeCopyOf(buffer);
    }

    // --- Apply DSP chain ---
    gainProcessor.setGainDb(gainDb);
    gainProcessor.process(buffer);

    // --- Dry/wet mix ---
    if (dryWet < 1.0f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            auto* dry = dryBuffer.getReadPointer(ch);
            for (int s = 0; s < buffer.getNumSamples(); ++s)
                wet[s] = dry[s] * (1.0f - dryWet) + wet[s] * dryWet;
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* MyPluginProcessor::createEditor()
{
    return new MyPluginEditor(*this);
}

//==============================================================================
void MyPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MyPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyPluginProcessor();
}
