#pragma once

#include <JuceHeader.h>
#include <atomic>

class SonicLureAudioProcessor : public juce::AudioProcessor
{
public:
    SonicLureAudioProcessor();
    ~SonicLureAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(
        juce::AudioBuffer<float>& buffer,
        juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& name) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool loadSampleFile(const juce::File& file);
    void playPreview();

    juce::String getLoadedSampleName() const;

private:
    juce::AudioFormatManager formatManager;
    juce::Synthesiser synthesiser;

    juce::String loadedSampleName;

    std::atomic<bool> previewRequested { false };
    int previewSamplesRemaining = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        SonicLureAudioProcessor)
};
