
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SonicLureAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SonicLureAudioProcessorEditor(SonicLureAudioProcessor&);
    ~SonicLureAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SonicLureAudioProcessor& audioProcessor;

    juce::Label title;
    juce::Label subtitle;

    juce::Slider attack;
    juce::Slider release;
    juce::Slider brightness;
    juce::Slider drift;
    juce::Slider tide;
    juce::Slider output;

    juce::ComboBox waveform;

    juce::Label attackLabel;
    juce::Label releaseLabel;
    juce::Label brightnessLabel;
    juce::Label driftLabel;
    juce::Label tideLabel;
    juce::Label outputLabel;
    juce::Label waveformLabel;

    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> brightnessAttachment;
    std::unique_ptr<SliderAttachment> driftAttachment;
    std::unique_ptr<SliderAttachment> tideAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        waveformAttachment;

    void setupSlider(juce::Slider&, juce::Label&, const juce::String&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        SonicLureAudioProcessorEditor)
};
