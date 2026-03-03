#pragma once

#include "PluginProcessor.h"

class HendrixFlangerEditor : public juce::AudioProcessorEditor
{
public:
    explicit HendrixFlangerEditor(HendrixFlangerProcessor&);
    ~HendrixFlangerEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
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

    juce::Slider dubEchoSlider, dubReverbSlider, dubFeedbackSlider;
    juce::Slider dubOffsetSlider, dubAutopanSlider;
    juce::Slider dubFullnessSlider, dubSpaceSlider;
    juce::Slider dubDrySlider, dubWetSlider, dubVolumeSlider;

    juce::Label dubEchoLabel, dubReverbLabel, dubFeedbackLabel;
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
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        dubEchoAtt, dubReverbAtt, dubFeedbackAtt, dubOffsetAtt, dubAutopanAtt,
        dubFullnessAtt, dubSpaceAtt, dubDryAtt, dubWetAtt, dubVolumeAtt;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HendrixFlangerEditor)
};
