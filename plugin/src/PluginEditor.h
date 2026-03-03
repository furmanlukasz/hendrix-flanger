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

    // --- Knobs ---
    juce::Slider rateSlider, depthSlider, manualSlider;
    juce::Slider feedbackSlider, stereoSlider, mixSlider;
    juce::Slider envSlider;

    juce::Label rateLabel, depthLabel, manualLabel;
    juce::Label feedbackLabel, stereoLabel, mixLabel;
    juce::Label envLabel;

    // --- Toggles / Combos ---
    juce::ToggleButton throughZeroButton { "Through Zero" };
    juce::ComboBox lfoShapeBox;

    // --- Attachments ---
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        rateAtt, depthAtt, manualAtt, feedbackAtt, stereoAtt, mixAtt, envAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tzAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoShapeAtt;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HendrixFlangerEditor)
};
