
#include "PluginProcessor.h"
#include "PluginEditor.h"

SonicLureAudioProcessorEditor::SonicLureAudioProcessorEditor(
    SonicLureAudioProcessor& p)
    : juce::AudioProcessorEditor(&p),
      audioProcessor(p)
{
    title.setText("SONICLURE", juce::dontSendNotification);
    title.setFont(juce::Font(30.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centred);
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    subtitle.setText("CAST YOUR SOUND", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    subtitle.setColour(juce::Label::textColourId, juce::Colours::lightblue);
    addAndMakeVisible(subtitle);

    setupSlider(attack, attackLabel, "ATTACK");
    setupSlider(release, releaseLabel, "RELEASE");
    setupSlider(brightness, brightnessLabel, "BRIGHTNESS");
    setupSlider(drift, driftLabel, "DRIFT");
    setupSlider(tide, tideLabel, "TIDE");
    setupSlider(output, outputLabel, "OUTPUT");

    attackAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.parameters, "ATTACK", attack);
    releaseAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.parameters, "RELEASE", release);
    brightnessAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.parameters, "BRIGHTNESS", brightness);
    driftAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.parameters, "DRIFT", drift);
    tideAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.parameters, "TIDE", tide);
    outputAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.parameters, "OUTPUT", output);

    waveform.addItem("Sine", 1);
    waveform.addItem("Saw", 2);
    waveform.addItem("Square", 3);
    addAndMakeVisible(waveform);

    waveformLabel.setText("WAVEFORM", juce::dontSendNotification);
    waveformLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(waveformLabel);

    waveformAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.parameters, "WAVEFORM", waveform);

    setSize(720, 480);
}

SonicLureAudioProcessorEditor::~SonicLureAudioProcessorEditor() = default;

void SonicLureAudioProcessorEditor::setupSlider(
    juce::Slider& slider,
    juce::Label& label,
    const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 70, 20);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

void SonicLureAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(18, 29, 43));

    g.setColour(juce::Colour(35, 65, 85));
    g.fillRoundedRectangle(
        getLocalBounds().toFloat().reduced(10.0f), 15.0f);

    g.setColour(juce::Colour(90, 190, 220));
    g.drawRoundedRectangle(
        getLocalBounds().toFloat().reduced(10.0f), 15.0f, 2.0f);
}

void SonicLureAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(25);

    title.setBounds(area.removeFromTop(45));
    subtitle.setBounds(area.removeFromTop(25));

    area.removeFromTop(15);

    const int columnWidth = area.getWidth() / 3;
    const int rowHeight = 145;

    auto topRow = area.removeFromTop(rowHeight);
    auto bottomRow = area.removeFromTop(rowHeight);

    auto placeControl = [](
        juce::Rectangle<int> cell,
        juce::Slider& slider,
        juce::Label& label)
    {
        label.setBounds(cell.removeFromTop(25));
        slider.setBounds(cell.reduced(8));
    };

    placeControl(topRow.removeFromLeft(columnWidth), attack, attackLabel);
    placeControl(topRow.removeFromLeft(columnWidth), release, releaseLabel);
    placeControl(topRow, brightness, brightnessLabel);

    placeControl(bottomRow.removeFromLeft(columnWidth), drift, driftLabel);
    placeControl(bottomRow.removeFromLeft(columnWidth), tide, tideLabel);
    placeControl(bottomRow, output, outputLabel);

    auto waveformArea = area.removeFromTop(45);
    waveformLabel.setBounds(waveformArea.removeFromLeft(120));
    waveform.setBounds(waveformArea.removeFromLeft(180));
}
