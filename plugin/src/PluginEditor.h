#pragma once

#include "PluginProcessor.h"

class HendrixFlangerEditor : public juce::AudioProcessorEditor,
                              private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit HendrixFlangerEditor(HendrixFlangerProcessor&);
    ~HendrixFlangerEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void updateDubVisibility();
    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);

    static constexpr int kHeightDubOn  = 740;
    static constexpr int kHeightDubOff = 500;

    HendrixFlangerProcessor& processorRef;

    // --- Preset selector ---
    juce::ComboBox presetBox;

    // --- Flanger Knobs ---
    juce::Slider rateSlider, depthSlider, manualSlider;
    juce::Slider feedbackSlider, stereoSlider, mixSlider;
    juce::Slider envSlider, warmthSlider;

    juce::Label rateLabel, depthLabel, manualLabel;
    juce::Label feedbackLabel, stereoLabel, mixLabel;
    juce::Label envLabel, warmthLabel;

    // --- Flanger Toggles / Combos ---
    juce::ToggleButton throughZeroButton { "Through Zero" };
    juce::ComboBox lfoShapeBox;

    // --- Dub Echo Controls ---
    juce::ToggleButton dubEnabledButton { "Dub Echo" };

    juce::ComboBox dubEchoDivBox;
    juce::Label dubEchoDivLabel;
    juce::Slider dubBpmSlider;
    juce::Label dubBpmLabel;

    juce::Slider dubReverbSlider, dubFeedbackSlider;
    juce::Slider dubOffsetSlider, dubAutopanSlider;
    juce::Slider dubFullnessSlider, dubSpaceSlider;
    juce::Slider dubDrySlider, dubWetSlider, dubVolumeSlider;

    juce::Label dubReverbLabel, dubFeedbackLabel;
    juce::Label dubOffsetLabel, dubAutopanLabel;
    juce::Label dubFullnessLabel, dubSpaceLabel;
    juce::Label dubDryLabel, dubWetLabel, dubVolumeLabel;

    // --- Flanger Attachments ---
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        rateAtt, depthAtt, manualAtt, feedbackAtt, stereoAtt, mixAtt, envAtt, warmthAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tzAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoShapeAtt;

    // --- Dub Echo Attachments ---
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dubEnabledAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> dubEchoDivAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        dubBpmAtt, dubReverbAtt, dubFeedbackAtt, dubOffsetAtt, dubAutopanAtt,
        dubFullnessAtt, dubSpaceAtt, dubDryAtt, dubWetAtt, dubVolumeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HendrixFlangerEditor)
};
