#include "PluginEditor.h"

//==============================================================================
MyPluginEditor::MyPluginEditor(MyPluginProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // --- Gain slider ---
    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible(gainSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "gain_db", gainSlider);

    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gainLabel);

    // --- Dry/Wet slider ---
    dryWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryWetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible(dryWetSlider);
    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "dry_wet", dryWetSlider);

    dryWetLabel.setText("Dry/Wet", juce::dontSendNotification);
    dryWetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dryWetLabel);

    // --- Bypass button ---
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "bypass", bypassButton);

    // TODO: Adjust window size
    setSize(400, 300);
}

MyPluginEditor::~MyPluginEditor() {}

//==============================================================================
void MyPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("My Plugin", getLocalBounds().removeFromTop(40),
                     juce::Justification::centred, 1);
}

void MyPluginEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(40); // title space

    auto knobArea = area.removeFromTop(180);
    auto knobWidth = knobArea.getWidth() / 2;

    // Gain knob
    auto gainArea = knobArea.removeFromLeft(knobWidth);
    gainLabel.setBounds(gainArea.removeFromTop(20));
    gainSlider.setBounds(gainArea);

    // Dry/Wet knob
    dryWetLabel.setBounds(knobArea.removeFromTop(20));
    dryWetSlider.setBounds(knobArea);

    // Bypass
    bypassButton.setBounds(area.removeFromTop(30).reduced(100, 0));
}
