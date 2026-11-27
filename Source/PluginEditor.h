#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

class SonicLureAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SonicLureAudioProcessorEditor(SonicLureAudioProcessor&);
    ~SonicLureAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SonicLureAudioProcessor& audioProcessor;

    static constexpr int controlCount = 14;

    juce::Label title;
    juce::Label subtitle;

    std::array<juce::Slider, controlCount> sliders;
    std::array<juce::Label, controlCount> labels;

    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    std::array<std::unique_ptr<SliderAttachment>, controlCount>
        sliderAttachments;

    juce::ComboBox waveform;
    juce::Label waveformLabel;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        waveformAttachment;

    void setupSlider(int index, const juce::String& labelText);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        SonicLureAudioProcessorEditor)
};
