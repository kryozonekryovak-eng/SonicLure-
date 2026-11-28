
#pragma once

#include "PluginProcessor.h"

class XLUREAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit XLUREAudioProcessorEditor(XLUREAudioProcessor&);
    ~XLUREAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    XLUREAudioProcessor& audioProcessor;

    juce::TextButton loadButton { "LOAD WAV" };
    juce::TextButton playButton { "PLAY" };

    juce::Label titleLabel;
    juce::Label sampleLabel;
    juce::Label statusLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void loadWavFile();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        XLUREAudioProcessorEditor)
};
