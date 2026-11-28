
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::Colour background(10, 10, 12);
    const juce::Colour panel(27, 24, 22);
    const juce::Colour orange(235, 105, 25);
    const juce::Colour amber(255, 174, 65);
    const juce::Colour cream(245, 235, 220);

    const std::array<const char*, 14> parameterIDs =
    {
        "ATTACK", "RELEASE", "BRIGHTNESS", "DRIFT",
        "TIDE", "OUTPUT", "OSC1_LEVEL", "OSC2_LEVEL",
        "OSC3_LEVEL", "OSC2_DETUNE", "OSC3_DETUNE",
        "SUB_LEVEL", "NOISE_LEVEL", "FILTER_CUTOFF"
    };

    const std::array<const char*, 14> controlNames =
    {
        "ATTACK", "RELEASE", "BRIGHTNESS", "DRIFT",
        "TIDE", "MASTER", "OSC 1 LEVEL", "OSC 2 LEVEL",
        "OSC 3 LEVEL", "OSC 2 DETUNE", "OSC 3 DETUNE",
        "SUB BASS", "NOISE", "FILTER CUTOFF"
    };
}

SonicLureAudioProcessorEditor::SonicLureAudioProcessorEditor(
    SonicLureAudioProcessor& p)
    : juce::AudioProcessorEditor(&p),
      audioProcessor(p)
{
    title.setText("SONICLURE", juce::dontSendNotification);
    title.setFont(juce::Font(32.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centred);
    title.setColour(juce::Label::textColourId, amber);
    addAndMakeVisible(title);

    subtitle.setText("BLACKOUT ORANGE  |  CAST YOUR SOUND",
                     juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    subtitle.setColour(juce::Label::textColourId, orange);
    addAndMakeVisible(subtitle);

    for (int i = 0; i < controlCount; ++i)
    {
        setupSlider(i, controlNames[static_cast<size_t>(i)]);

        sliderAttachments[static_cast<size_t>(i)] =
            std::make_unique<SliderAttachment>(
                audioProcessor.parameters,
                parameterIDs[static_cast<size_t>(i)],
                sliders[static_cast<size_t>(i)]);
    }

    waveform.addItem("Sine", 1);
    waveform.addItem("Saw", 2);
    waveform.addItem("Square", 3);

    waveform.setColour(
        juce::ComboBox::backgroundColourId, panel);
    waveform.setColour(
        juce::ComboBox::textColourId, amber);
    waveform.setColour(
        juce::ComboBox::outlineColourId, orange);

    addAndMakeVisible(waveform);

    waveformLabel.setText("WAVEFORM", juce::dontSendNotification);
    waveformLabel.setJustificationType(juce::Justification::centred);
    waveformLabel.setColour(
        juce::Label::textColourId, cream);
    addAndMakeVisible(waveformLabel);

    waveformAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                audioProcessor.parameters, "WAVEFORM", waveform);

    setSize(900, 700);
}

SonicLureAudioProcessorEditor::~SonicLureAudioProcessorEditor() = default;

void SonicLureAudioProcessorEditor::setupSlider(
    int index,
    const juce::String& labelText)
{
    auto& slider = sliders[static_cast<size_t>(index)];
    auto& label = labels[static_cast<size_t>(index)];

    slider.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 70, 19);

    slider.setColour(
        juce::Slider::rotarySliderFillColourId, orange);
    slider.setColour(
        juce::Slider::rotarySliderOutlineColourId,
        juce::Colour(75, 60, 48));
    slider.setColour(
        juce::Slider::thumbColourId, amber);
    slider.setColour(
        juce::Slider::textBoxTextColourId, cream);
    slider.setColour(
        juce::Slider::textBoxBackgroundColourId, panel);
    slider.setColour(
        juce::Slider::textBoxOutlineColourId, orange);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(11.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, amber);

    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

void SonicLureAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    auto bounds = getLocalBounds().toFloat().reduced(10.0f);

    g.setColour(panel);
    g.fillRoundedRectangle(bounds, 16.0f);

    g.setColour(orange);
    g.drawRoundedRectangle(bounds, 16.0f, 2.0f);

    g.setColour(orange.withAlpha(0.35f));
    g.drawHorizontalLine(95, 25.0f, getWidth() - 25.0f);

    g.setColour(amber);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(
        "SOUND ENGINE  /  OSCILLATORS  /  MOVEMENT",
        30, 105, getWidth() - 60, 22,
        juce::Justification::centredLeft);

    g.setColour(orange.withAlpha(0.12f));
    g.fillEllipse(0.0f, 0.0f, 180.0f, 180.0f);
}

void SonicLureAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(28);

    title.setBounds(area.removeFromTop(42));
    subtitle.setBounds(area.removeFromTop(27));

    const int gridTop = 125;
    const int gap = 12;
    const int columns = 4;
    const int cellWidth =
        (getWidth() - 56 - gap * (columns - 1)) / columns;
    const int cellHeight = 119;

    for (int i = 0; i < controlCount; ++i)
    {
        const int row = i / columns;
        const int column = i % columns;

        juce::Rectangle<int> cell(
            28 + column * (cellWidth + gap),
            gridTop + row * cellHeight,
            cellWidth,
            cellHeight - 4);

        labels[static_cast<size_t>(i)].setBounds(
            cell.removeFromTop(23));

        sliders[static_cast<size_t>(i)].setBounds(
            cell.reduced(5));
    }

    waveformLabel.setBounds(
        getWidth() / 2 - 145,
        getHeight() - 48, 110, 25);

    waveform.setBounds(
        getWidth() / 2 - 30,
        getHeight() - 49, 150, 28);
}
