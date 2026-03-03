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
        juce::ParameterID{"warmth", 1},
        "Warmth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

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

    // Pre-allocate dry buffer so processBlock never touches the heap
    dryBuffer.setSize(2, samplesPerBlock, false, false, true);

    // ~5 ms smoothing time constant for dry/wet mix
    dryWetSmoothingCoeff = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.005)));
    smoothedDryWet = apvts.getRawParameterValue("dry_wet")->load() / 100.0f;
    dryWetNeedsSnap = false;
}

void HendrixFlangerProcessor::releaseResources() {}

void HendrixFlangerProcessor::reset()
{
    flanger.reset();
    dryWetNeedsSnap = true;
}

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
    float warmthPct   = apvts.getRawParameterValue("warmth")->load();
    float dryWetTarget = apvts.getRawParameterValue("dry_wet")->load() / 100.0f;

    // Update flanger parameters
    flanger.setRate(rateHz);
    flanger.setDepth(depth);
    flanger.setManualMs(manualMs);
    flanger.setFeedback(feedbackPct);
    flanger.setStereoSpread(stereo);
    flanger.setThroughZero(tzEnabled);
    flanger.setEnvAmount(envAmt);
    flanger.setLfoShape(lfoShape);
    flanger.setWarmth(warmthPct);

    // Snap dry/wet smoothing after reset (deferred so target is current)
    if (dryWetNeedsSnap)
    {
        smoothedDryWet = dryWetTarget;
        dryWetNeedsSnap = false;
    }

    // Store dry signal for mix (uses pre-allocated buffer — no heap alloc)
    int numCh = buffer.getNumChannels();
    int numSamp = buffer.getNumSamples();
    if (dryBuffer.getNumChannels() < numCh || dryBuffer.getNumSamples() < numSamp)
        dryBuffer.setSize(numCh, numSamp, false, false, true);
    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamp);

    // Process through flanger
    flanger.process(buffer);

    // Dry/wet mix with per-sample smoothing (prevents pops/clicks)
    const float coeff = dryWetSmoothingCoeff;
    const float oneMinusCoeff = 1.0f - coeff;
    float dw = smoothedDryWet;
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        auto* dry = dryBuffer.getReadPointer(ch);
        float dwCh = dw;  // Both channels use same starting point
        for (int s = 0; s < numSamp; ++s)
        {
            dwCh = dwCh * coeff + dryWetTarget * oneMinusCoeff;
            wet[s] = dry[s] * (1.0f - dwCh) + wet[s] * dwCh;
        }
        dw = dwCh;  // After first channel, second channel continues from same state
    }
    smoothedDryWet = dw;
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

void HendrixFlangerProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= numFactoryPresets)
        return;

    currentPreset = index;
    const auto& p = factoryPresets[index];

    auto* rate    = apvts.getParameter("rate_hz");
    auto* dep     = apvts.getParameter("depth");
    auto* manual  = apvts.getParameter("manual_ms");
    auto* fb      = apvts.getParameter("feedback");
    auto* st      = apvts.getParameter("stereo_spread");
    auto* tz      = apvts.getParameter("through_zero");
    auto* env     = apvts.getParameter("env_amount");
    auto* shape   = apvts.getParameter("lfo_shape");
    auto* warm    = apvts.getParameter("warmth");
    auto* mix     = apvts.getParameter("dry_wet");

    rate->setValueNotifyingHost(rate->convertTo0to1(p.rate_hz));
    dep->setValueNotifyingHost(dep->convertTo0to1(p.depth));
    manual->setValueNotifyingHost(manual->convertTo0to1(p.manual_ms));
    fb->setValueNotifyingHost(fb->convertTo0to1(p.feedback));
    st->setValueNotifyingHost(st->convertTo0to1(p.stereo_spread));
    tz->setValueNotifyingHost(p.through_zero ? 1.0f : 0.0f);
    env->setValueNotifyingHost(env->convertTo0to1(p.env_amount));
    shape->setValueNotifyingHost(shape->convertTo0to1(static_cast<float>(p.lfo_shape)));
    warm->setValueNotifyingHost(warm->convertTo0to1(p.warmth));
    mix->setValueNotifyingHost(mix->convertTo0to1(p.dry_wet));
}

const juce::String HendrixFlangerProcessor::getProgramName(int index)
{
    if (index >= 0 && index < numFactoryPresets)
        return factoryPresets[index].name;
    return {};
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HendrixFlangerProcessor();
}
