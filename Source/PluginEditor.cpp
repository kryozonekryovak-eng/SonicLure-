
#include "PluginProcessor.h"
#include "PluginEditor.h"

XLUREAudioProcessorEditor::XLUREAudioProcessorEditor(
    XLUREAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p)
{
    setSize(520, 330);

    titleLabel.setText("XLURE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(32.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xffe9f4ff));
    addAndMakeVisible(titleLabel);

    loadButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour(0xff176b91));
    loadButton.setColour(
        juce::TextButton::textColourOffId,
        juce::Colours::white);
    loadButton.onClick = [this]
    {
        loadWavFile();
    };
    addAndMakeVisible(loadButton);

    playButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour(0xff23875e));
    playButton.setColour(
        juce::TextButton::textColourOffId,
        juce::Colours::white);
    playButton.onClick = [this]
    {
        audioProcessor.playPreview();
    };
    addAndMakeVisible(playButton);

    sampleLabel.setText(
        "No WAV sample loaded",
        juce::dontSendNotification);
    sampleLabel.setJustificationType(
        juce::Justification::centred);
    sampleLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xffc5d5e5));
    addAndMakeVisible(sampleLabel);

    statusLabel.setText(
        "Load a WAV sample to begin",
        juce::dontSendNotification);
    statusLabel.setJustificationType(
        juce::Justification::centred);
    statusLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xff91a8bd));
    addAndMakeVisible(statusLabel);
}

XLUREAudioProcessorEditor::~XLUREAudioProcessorEditor()
{
}

void XLUREAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101c2a));

    auto area = getLocalBounds().toFloat();

    g.setColour(juce::Colour(0xff1c3145));
    g.drawRoundedRectangle(
        area.reduced(8.0f),
        14.0f,
        2.0f);

    g.setColour(juce::Colour(0xff176b91));
    g.fillRoundedRectangle(
        18.0f,
        18.0f,
        static_cast<float>(getWidth() - 36),
        5.0f,
        2.0f);
}

void XLUREAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(28);

    titleLabel.setBounds(
        area.removeFromTop(75));

    area.removeFromTop(20);

    loadButton.setBounds(
        area.removeFromTop(48).reduced(35, 2));

    area.removeFromTop(12);

    playButton.setBounds(
        area.removeFromTop(48).reduced(35, 2));

    area.removeFromTop(22);

    sampleLabel.setBounds(
        area.removeFromTop(32));

    statusLabel.setBounds(
        area.removeFromTop(28));
}

void XLUREAudioProcessorEditor::loadWavFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose a WAV sample",
        juce::File::getSpecialLocation(
            juce::File::userHomeDirectory),
        "*.wav");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const juce::File file = chooser.getResult();

            if (file.existsAsFile())
            {
                if (audioProcessor.loadSampleFile(file))
                {
                    sampleLabel.setText(
                        audioProcessor.getLoadedSampleName(),
                        juce::dontSendNotification);

                    statusLabel.setText(
                        "SAMPLE LOADED - READY TO PLAY",
                        juce::dontSendNotification);
                }
                else
                {
                    statusLabel.setText(
                        "Could not load this WAV file",
                        juce::dontSendNotification);
                }
            }
        });
}
