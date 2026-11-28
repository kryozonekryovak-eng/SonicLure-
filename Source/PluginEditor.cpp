
#include "PluginEditor.h"

//==============================================================
// XLURE EDITOR
//==============================================================

XLUREAudioProcessorEditor::XLUREAudioProcessorEditor(
    XLUREAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setSize(650, 420);

    setResizable(true, true);
    setResizeLimits(500, 350, 1100, 800);

    // Title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("XLURE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(38.0f, juce::Font::bold));
    titleLabel.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(90, 220, 255));
    titleLabel.setJustificationType(
        juce::Justification::centred);

    // Sample information
    addAndMakeVisible(sampleLabel);
    sampleLabel.setText(
        "NO WAV SAMPLE LOADED",
        juce::dontSendNotification);
    sampleLabel.setFont(juce::Font(15.0f));
    sampleLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::lightgrey);
    sampleLabel.setJustificationType(
        juce::Justification::centred);

    // Status
    addAndMakeVisible(statusLabel);
    statusLabel.setText(
        "READY",
        juce::dontSendNotification);
    statusLabel.setFont(juce::Font(13.0f));
    statusLabel.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(100, 230, 180));
    statusLabel.setJustificationType(
        juce::Justification::centred);

    // Buttons
    addAndMakeVisible(loadButton);
    addAndMakeVisible(playButton);

    loadButton.onClick = [this]
    {
        loadWavFile();
    };

    playButton.onClick = [this]
    {
        audioProcessor.playPreview();
        statusLabel.setText(
            "PLAYING SAMPLE PREVIEW",
            juce::dontSendNotification);
    };

    loadButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour::fromRGB(10, 90, 125));

    playButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour::fromRGB(15, 115, 90));

    loadButton.setColour(
        juce::TextButton::textColourOffId,
        juce::Colours::white);

    playButton.setColour(
        juce::TextButton::textColourOffId,
        juce::Colours::white);
}

XLUREAudioProcessorEditor::~XLUREAudioProcessorEditor() = default;

//==============================================================
// BACKGROUND
//==============================================================

void XLUREAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(5, 12, 24));

    auto area = getLocalBounds().toFloat().reduced(10.0f);

    g.setColour(juce::Colour::fromRGB(15, 40, 58));
    g.drawRoundedRectangle(area, 18.0f, 2.0f);

    g.setColour(
        juce::Colour::fromRGB(0, 170, 220).withAlpha(0.12f));

    g.fillRoundedRectangle(
        area.reduced(8.0f),
        14.0f);
}

//==============================================================
// LAYOUT
//==============================================================

void XLUREAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(35);

    titleLabel.setBounds(
        area.removeFromTop(75));

    area.removeFromTop(35);

    sampleLabel.setBounds(
        area.removeFromTop(45));

    statusLabel.setBounds(
        area.removeFromTop(35));

    area.removeFromTop(35);

    const int buttonHeight = 55;

    loadButton.setBounds(
        area.removeFromTop(buttonHeight)
            .reduced(45, 3));

    area.removeFromTop(15);

    playButton.setBounds(
        area.removeFromTop(buttonHeight)
            .reduced(45, 3));
}

//==============================================================
// WAV FILE LOADER
//==============================================================

void XLUREAudioProcessorEditor::loadWavFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose a WAV sample",
        juce::File{},
        "*.wav");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles,

        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();

            if (!file.existsAsFile())
                return;

            if (audioProcessor.loadSampleFile(file))
            {
                sampleLabel.setText(
                    audioProcessor.getLoadedSampleName(),
                    juce::dontSendNotification);

                statusLabel.setText(
                    "SAMPLE LOADED SUCCESSFULLY",
                    juce::dontSendNotification);
            }
            else
            {
                statusLabel.setText(
                    "COULD NOT LOAD SAMPLE",
                    juce::dontSendNotification);
            }
        });
}
