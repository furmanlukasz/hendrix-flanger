#include "PluginEditor.h"

HendrixFlangerEditor::HendrixFlangerEditor(HendrixFlangerProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    auto& apvts = processorRef.getAPVTS();

    // --- Preset selector ---
    for (int i = 0; i < numFactoryPresets; ++i)
        presetBox.addItem(factoryPresets[i].name, i + 1);

    presetBox.setSelectedId(processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a3e));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffffaa00));
    presetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff555577));
    presetBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffff6b35));
    presetBox.onChange = [this]()
    {
        int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0)
            processorRef.setCurrentProgram(idx);
    };
    addAndMakeVisible(presetBox);

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

    // Row 3: Through Zero, LFO Shape, Envelope, Warmth
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

    setupSlider(warmthSlider, warmthLabel, "Warmth");
    warmthAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "warmth", warmthSlider);

    setSize(540, 440);
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

    // Title (left-aligned to make room for preset dropdown)
    g.setColour(juce::Colour(0xffff6b35));
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    auto titleArea = getLocalBounds().removeFromTop(45);
    g.drawFittedText("HENDRIX FLANGER", titleArea.removeFromLeft(220).reduced(15, 0),
                     juce::Justification::centredLeft, 1);

    // Subtle separator lines
    g.setColour(juce::Colour(0xff333355));
    int headerH = 45;
    int row1Bottom = headerH + 130;
    int row2Bottom = row1Bottom + 130;
    g.drawHorizontalLine(row1Bottom + 2, 20.0f, static_cast<float>(getWidth() - 20));
    g.drawHorizontalLine(row2Bottom + 2, 20.0f, static_cast<float>(getWidth() - 20));
}

void HendrixFlangerEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Header: title (painted) + preset dropdown
    auto header = area.removeFromTop(45);
    header.removeFromLeft(210); // space for painted title
    presetBox.setBounds(header.reduced(5, 8));

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

    // Row 3: Through Zero, LFO Shape, Envelope, Warmth (4 columns)
    auto row3 = area.removeFromTop(90);
    colW = row3.getWidth() / 4;

    auto tzArea = row3.removeFromLeft(colW);
    throughZeroButton.setBounds(tzArea.reduced(5, 25));

    auto shapeArea = row3.removeFromLeft(colW);
    lfoShapeBox.setBounds(shapeArea.reduced(10, 28));

    auto envArea = row3.removeFromLeft(colW);
    envLabel.setBounds(envArea.removeFromTop(labelH));
    envSlider.setBounds(envArea);

    warmthLabel.setBounds(row3.removeFromTop(labelH));
    warmthSlider.setBounds(row3);
}
