#include "PluginEditor.h"

HendrixFlangerEditor::HendrixFlangerEditor(HendrixFlangerProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    auto& apvts = processorRef.getAPVTS();

    // Row 1: Rate, Depth, Manual
    setupSlider(rateSlider, rateLabel, "Speed");
    rateAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "rate_hz", rateSlider);

    setupSlider(depthSlider, depthLabel, "Depth");
    depthAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "depth", depthSlider);

    setupSlider(manualSlider, manualLabel, "Manual");
    manualAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "manual_ms", manualSlider);

    // Row 2: Feedback, Stereo, Mix
    setupSlider(feedbackSlider, feedbackLabel, "Feedback");
    feedbackAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "feedback", feedbackSlider);

    setupSlider(stereoSlider, stereoLabel, "Stereo");
    stereoAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "stereo_spread", stereoSlider);

    setupSlider(mixSlider, mixLabel, "Mix");
    mixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "dry_wet", mixSlider);

    // Row 3: Through Zero, LFO Shape, Envelope
    addAndMakeVisible(throughZeroButton);
    throughZeroButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    throughZeroButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff6b35));
    tzAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, "through_zero", throughZeroButton);

    lfoShapeBox.addItemList({"Sine", "Triangle", "S&H"}, 1);
    lfoShapeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a3e));
    lfoShapeBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    lfoShapeBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff555577));
    addAndMakeVisible(lfoShapeBox);
    lfoShapeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, "lfo_shape", lfoShapeBox);

    setupSlider(envSlider, envLabel, "Envelope");
    envAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "env_amount", envSlider);

    setSize(540, 400);
}

HendrixFlangerEditor::~HendrixFlangerEditor() {}

void HendrixFlangerEditor::setupSlider(juce::Slider& slider, juce::Label& label,
                                        const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff6b35));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffaa00));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    label.setFont(juce::Font(13.0f));
    addAndMakeVisible(label);
}

void HendrixFlangerEditor::paint(juce::Graphics& g)
{
    // Dark background with subtle gradient
    auto bounds = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff0d0d1a), bounds.getCentreX(), 0.0f,
        juce::Colour(0xff1a1a2e), bounds.getCentreX(), bounds.getHeight(),
        false));
    g.fillRect(bounds);

    // Title
    g.setColour(juce::Colour(0xffff6b35));
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawFittedText("HENDRIX FLANGER", getLocalBounds().removeFromTop(45),
                     juce::Justification::centred, 1);

    // Subtle separator lines
    g.setColour(juce::Colour(0xff333355));
    int row1Bottom = 45 + 130;
    int row2Bottom = row1Bottom + 130;
    g.drawHorizontalLine(row1Bottom + 2, 20.0f, static_cast<float>(getWidth() - 20));
    g.drawHorizontalLine(row2Bottom + 2, 20.0f, static_cast<float>(getWidth() - 20));
}

void HendrixFlangerEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(45); // title

    int knobH = 130;
    int labelH = 18;

    // Row 1: Rate, Depth, Manual
    auto row1 = area.removeFromTop(knobH);
    int colW = row1.getWidth() / 3;

    auto rateArea = row1.removeFromLeft(colW);
    rateLabel.setBounds(rateArea.removeFromTop(labelH));
    rateSlider.setBounds(rateArea);

    auto depthArea = row1.removeFromLeft(colW);
    depthLabel.setBounds(depthArea.removeFromTop(labelH));
    depthSlider.setBounds(depthArea);

    manualLabel.setBounds(row1.removeFromTop(labelH));
    manualSlider.setBounds(row1);

    area.removeFromTop(8); // spacing

    // Row 2: Feedback, Stereo, Mix
    auto row2 = area.removeFromTop(knobH);
    colW = row2.getWidth() / 3;

    auto fbArea = row2.removeFromLeft(colW);
    feedbackLabel.setBounds(fbArea.removeFromTop(labelH));
    feedbackSlider.setBounds(fbArea);

    auto stArea = row2.removeFromLeft(colW);
    stereoLabel.setBounds(stArea.removeFromTop(labelH));
    stereoSlider.setBounds(stArea);

    mixLabel.setBounds(row2.removeFromTop(labelH));
    mixSlider.setBounds(row2);

    area.removeFromTop(8);

    // Row 3: Through Zero, LFO Shape, Envelope
    auto row3 = area.removeFromTop(80);
    colW = row3.getWidth() / 3;

    auto tzArea = row3.removeFromLeft(colW);
    throughZeroButton.setBounds(tzArea.reduced(10, 20));

    auto shapeArea = row3.removeFromLeft(colW);
    lfoShapeBox.setBounds(shapeArea.reduced(20, 25));

    envLabel.setBounds(row3.removeFromTop(labelH));
    envSlider.setBounds(row3);
}
