#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SonicLureAudioProcessorEditor
    : public juce::AudioProcessorEditor
{
public:
    explicit SonicLureAudioProcessorEditor(
        SonicLureAudioProcessor& processor);

    ~SonicLureAudioProcessorEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    SonicLureAudioProcessor& audioProcessor;

    juce::TextButton loadButton { "LOAD WAV" };
    juce::TextButton playButton { "PLAY" };

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label sampleLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        SonicLureAudioProcessorEditor)
};
