#include "PluginProcessor.h"
#include "PluginEditor.h"

HendrixFlangerProcessor::HendrixFlangerProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

HendrixFlangerProcessor::~HendrixFlangerProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
HendrixFlangerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"rate_hz", 1},
        "Speed",
        juce::NormalisableRange<float>(0.05f, 10.0f, 0.01f, 0.4f),
        0.5f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"depth", 1},
        "Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"manual_ms", 1},
        "Manual",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f),
        3.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"feedback", 1},
        "Feedback",
        juce::NormalisableRange<float>(-95.0f, 95.0f, 0.1f),
        30.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"stereo_spread", 1},
        "Stereo",
        juce::NormalisableRange<float>(0.0f, 180.0f, 1.0f),
        90.0f,
        juce::AudioParameterFloatAttributes().withLabel("\xc2\xb0")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"through_zero", 1},
        "Through Zero",
        true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"env_amount", 1},
        "Envelope",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo_shape", 1},
        "LFO Shape",
        juce::StringArray{"Sine", "Triangle", "S&H"},
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"dry_wet", 1},
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}

void HendrixFlangerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    flanger.prepare(sampleRate, samplesPerBlock);
}

void HendrixFlangerProcessor::releaseResources() {}

bool HendrixFlangerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void HendrixFlangerProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters
    float rateHz      = apvts.getRawParameterValue("rate_hz")->load();
    float depth       = apvts.getRawParameterValue("depth")->load();
    float manualMs    = apvts.getRawParameterValue("manual_ms")->load();
    float feedbackPct = apvts.getRawParameterValue("feedback")->load();
    float stereo      = apvts.getRawParameterValue("stereo_spread")->load();
    bool  tzEnabled   = apvts.getRawParameterValue("through_zero")->load() > 0.5f;
    float envAmt      = apvts.getRawParameterValue("env_amount")->load();
    int   lfoShape    = static_cast<int>(apvts.getRawParameterValue("lfo_shape")->load());
    float dryWet      = apvts.getRawParameterValue("dry_wet")->load() / 100.0f;

    // Update flanger parameters
    flanger.setRate(rateHz);
    flanger.setDepth(depth);
    flanger.setManualMs(manualMs);
    flanger.setFeedback(feedbackPct);
    flanger.setStereoSpread(stereo);
    flanger.setThroughZero(tzEnabled);
    flanger.setEnvAmount(envAmt);
    flanger.setLfoShape(lfoShape);

    // Store dry signal for mix
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Process through flanger
    flanger.process(buffer);

    // Dry/wet mix
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        auto* dry = dryBuffer.getReadPointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s)
            wet[s] = dry[s] * (1.0f - dryWet) + wet[s] * dryWet;
    }
}

juce::AudioProcessorEditor* HendrixFlangerProcessor::createEditor()
{
    return new HendrixFlangerEditor(*this);
}

void HendrixFlangerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HendrixFlangerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HendrixFlangerProcessor();
}
