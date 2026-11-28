
#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class XLUREAudioProcessor : public juce::AudioProcessor
{
public:
    XLUREAudioProcessor();
    ~XLUREAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(
        juce::AudioBuffer<float>&,
        juce::MidiBuffer&) override;

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
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // WAV sample engine
    bool loadSampleFile(const juce::File& file);
    void playPreview();
    juce::String getLoadedSampleName() const;

    // XY Morph Pad controls
    void setMorphPosition(float x, float y);
    float getMorphX() const;
    float getMorphY() const;

    // Synthesis controls
    void setFilterCutoff(float value);
    void setFilterResonance(float value);
    void setModalAmount(float value);
    void setMasterLevel(float value);

private:

    // Individual subtractive/modal synthesiser voices
    struct XLUREVoice
    {
        bool active = false;
        bool releasing = false;

        int midiNote = 60;
        float velocity = 0.0f;

        double phaseOne = 0.0;
        double phaseTwo = 0.0;

        float envelope = 0.0f;
        float releaseRate = 0.0f;

        float filterState = 0.0f;

        std::array<double, 8> modalPhases {};
        std::array<float, 8> modalLevels {};

        float noiseState = 0.0f;
    };

    static constexpr int maximumVoices = 16;
    static constexpr int numberOfModes = 8;

    std::array<XLUREVoice, maximumVoices> voices;

    // WAV sample engine
    juce::AudioFormatManager formatManager;
    juce::Synthesiser sampler;
    juce::CriticalSection samplerLock;

    juce::String loadedSampleName;

    std::atomic<bool> previewRequested { false };
    int previewSamplesRemaining = 0;

    // Audio engine state
    double currentSampleRate = 44100.0;

    std::atomic<float> morphX { 0.5f };
    std::atomic<float> morphY { 0.5f };

    std::atomic<float> filterCutoff { 0.65f };
    std::atomic<float> filterResonance { 0.15f };
    std::atomic<float> modalAmount { 0.5f };
    std::atomic<float> masterLevel { 0.75f };

    float smoothedMorph = 0.5f;
    float smoothedBrightness = 0.5f;

    // MIDI and voice management
    void handleMidiMessage(const juce::MidiMessage& message);
    void startVoice(int midiNote, float velocity);
    void stopVoice(int midiNote);
    int findFreeVoice() const;

    // Sound generation
    float renderVoice(XLUREVoice& voice);
    float renderSubtractive(
        XLUREVoice& voice,
        float brightness,
        float resonance);

    float renderModal(
        XLUREVoice& voice,
        float brightness);

    void resetVoice(XLUREVoice& voice);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XLUREAudioProcessor)
};
