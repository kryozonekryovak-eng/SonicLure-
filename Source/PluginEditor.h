
#pragma once

#include "PluginProcessor.h"

class XLUREAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit XLUREAudioProcessorEditor(XLUREAudioProcessor& processor);
    ~XLUREAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Animated circular liquid display
    class LiquidWindow : public juce::Component,
                         private juce::Timer
    {
    public:
        LiquidWindow();

        void setTheme(int newTheme);
        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        int theme = 0;
        float animationPhase = 0.0f;

        juce::Colour backgroundColour;
        juce::Colour liquidColour;
        juce::Colour highlightColour;
    };

    XLUREAudioProcessor& audioProcessor;

    LiquidWindow liquidWindow;

    // Four XLURE environments
    juce::TextButton oceanButton { "OCEAN" };
    juce::TextButton muddyButton { "MUDDY PUDDLE" };
    juce::TextButton riverButton { "RIVER" };
    juce::TextButton lilyButton { "LILY POND" };

    // Main controls
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label sampleLabel;
    juce::Label statusLabel;

    juce::TextButton loadButton { "LOAD WAV" };
    juce::TextButton playButton { "PLAY" };

    // Synthesis controls
    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;
    juce::Slider masterSlider;

    juce::Label cutoffLabel;
    juce::Label resonanceLabel;
    juce::Label masterLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void loadWavFile();
    void selectTheme(int theme);
    void styleButton(juce::TextButton& button);

    void setupSlider(
        juce::Slider& slider,
        juce::Label& label,
        const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        XLUREAudioProcessorEditor)
};
