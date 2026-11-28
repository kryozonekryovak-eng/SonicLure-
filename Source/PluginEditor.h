
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PURELUREAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PURELUREAudioProcessorEditor(PURELUREAudioProcessor&);
    ~PURELUREAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    PURELUREAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PURELUREAudioProcessorEditor)
};
