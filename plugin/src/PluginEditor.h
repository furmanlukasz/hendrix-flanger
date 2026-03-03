#pragma once

#include "PluginProcessor.h"

//==============================================================================
class MyPluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit MyPluginEditor(MyPluginProcessor&);
    ~MyPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MyPluginProcessor& processorRef;

    // UI controls — add yours here
    juce::Slider gainSlider;
    juce::Label  gainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    juce::Slider dryWetSlider;
    juce::Label  dryWetLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;

    juce::ToggleButton bypassButton { "Bypass" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyPluginEditor)
};
