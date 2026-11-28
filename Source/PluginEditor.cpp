
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::Colour backgroundColour(10, 10, 12);
    const juce::Colour panelColour(25, 23, 22);
    const juce::Colour orangeColour(235, 105, 25);
    const juce::Colour amberColour(255, 174, 65);
    const juce::Colour textColour(245, 235, 220);

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
    title.setColour(juce::Label::textColourId, amberColour);
    addAndMakeVisible(title);

    subtitle.setText("BLACKOUT ORANGE  |  CAST YOUR SOUND",
                     juce::dontSendNotification);
    subtitle.setFont(juce::Font(12.0f));
    subtitle.setJustificationType(juce::Justification::centred);
    subtitle.setColour(juce::Label::textColourId, orangeColour);
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
    waveform.setColour(juce::ComboBox::backgroundColourId, panelColour);
    waveform.setColour(juce::ComboBox::textColourId, amberColour);
    waveform.setColour(juce::ComboBox::outlineColourId, orangeColour);
    addAndMakeVisible(waveform);

    waveformLabel.setText("WAVEFORM", juce::dontSendNotification);
    waveformLabel.setJustificationType(juce::Justification::centred);
    waveformLabel.setColour(juce::Label::textColourId, textColour);
    addAndMakeVisible(waveformLabel);

    waveformAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
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

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 65, 19);

    slider.setColour(juce::Slider::rotarySliderFillColourId, orangeColour);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId,
                     juce::Colour(75, 60, 48));
    slider.setColour(juce::Slider::thumbColourId, amberColour);
    slider.setColour(juce::Slider::textBoxTextColourId, textColour);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, panelColour);
    slider.setColour(juce::Slider::textBoxOutlineColourId, orangeColour);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(11.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, amberColour);

    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

void SonicLureAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour);

    auto bounds = getLocalBounds().toFloat().reduced(10.0f);

    g.setColour(panelColour);
    g.fillRoundedRectangle(bounds, 16.0f);

    g.setColour(orangeColour);
    g.drawRoundedRectangle(bounds, 16.0f, 2.0f);

    g.setColour(juce::Colour(65, 43, 29));
    g.drawHorizontalLine(85, 25.0f, getWidth() - 25.0f);

    g.setColour(orangeColour.withAlpha(0.35f));
    g.drawHorizontalLine(365, 25.0f, getWidth() - 25.0f);

    g.setColour(amberColour);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("OSCILLATOR & SOUND ENGINE",
               30, 105, getWidth() - 60, 22,
               juce::Justification::centredLeft);

    g.drawText("MOVEMENT & PERFORMANCE",
               30, 385, getWidth() - 60, 22,
               juce::Justification::centredLeft);

    g.setColour(orangeColour.withAlpha(0.12f));
    g.fillEllipse(0.0f, 0.0f, 180.0f, 180.0f);
}

void SonicLureAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(28);

    title.setBounds(area.removeFromTop(42));
    subtitle.setBounds(area.removeFromTop(27));

    area.removeFromTop(38);

    const int gap = 12;
    const int cellWidth = (area.getWidth() - gap * 3) / 4;
    const int cellHeight = 125;

    for (int i = 0; i < controlCount; ++i)
    {
        if (i == 6)
            area.removeFromTop(35);

        const int row = i / 4;
        const int column = i % 4;

        auto cell = juce::Rectangle<int>(
            area.getX() + column * (cellWidth + gap),
            area.getY() + row * cellHeight,
            cellWidth,
            cellHeight - 5);

        labels[static_cast<size_t>(i)].setBounds(
            cell.removeFromTop(22));

        sliders[static_cast<size_t>(i)].setBounds(cell.reduced(4));
    }

    waveformLabel.setBounds(
        getWidth() / 2 - 145, getHeight() - 47, 110, 25);

    waveform.setBounds(
        getWidth() / 2 - 30, getHeight() - 48, 150, 28);
}
