
#pragma once

#include <JuceHeader.h>
#include <memory>
#include "PluginProcessor.h"

class PURELUREAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PURELUREAudioProcessorEditor(PURELUREAudioProcessor&);
    ~PURELUREAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class LiquidWindow : public juce::Component,
                         private juce::Timer
    {
    public:
        LiquidWindow();
        void setTheme(int newTheme);
        void paint(juce::Graphics&) override;

    private:
        void timerCallback() override;

        int theme = 0;
        float animationPhase = 0.0f;

        juce::Colour backgroundColour;
        juce::Colour liquidColour;
        juce::Colour highlightColour;
    };

    PURELUREAudioProcessor& audioProcessor;

    LiquidWindow liquidWindow;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label sampleLabel;
    juce::Label statusLabel;

    juce::TextButton oceanButton { "OCEAN" };
    juce::TextButton muddyButton { "MUDDY PUDDLE" };
    juce::TextButton riverButton { "RIVER" };
    juce::TextButton lilyButton { "LILY POND" };

    juce::TextButton loadButton { "LOAD WAV" };
    juce::TextButton playButton { "PLAY SAMPLE" };

    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;
    juce::Slider masterSlider;

    juce::Label cutoffLabel;
    juce::Label resonanceLabel;
    juce::Label masterLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void styleButton(juce::TextButton& button);

    void setupSlider(
        juce::Slider& slider,
        juce::Label& label,
        const juce::String& name);

    void selectTheme(int themeIndex);
    void loadWavFile();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        PURELUREAudioProcessorEditor)
};
