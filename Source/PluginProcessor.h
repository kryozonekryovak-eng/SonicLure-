#include "PluginProcessor.h"
#include "PluginEditor.h"

SonicLureAudioProcessor::SonicLureAudioProcessor()
    : AudioProcessor(
        BusesProperties()
            .withOutput(
                "Output",
                juce::AudioChannelSet::stereo(),
                true))
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < 16; ++i)
        synthesiser.addVoice(new juce::SamplerVoice());
}

void SonicLureAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
}

void SonicLureAudioProcessor::releaseResources()
{
    const juce::ScopedLock lock(getCallbackLock());
    synthesiser.allNotesOff(0, false);
}

void SonicLureAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    if (previewRequested.exchange(false))
    {
        midiMessages.addEvent(
            juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

        previewSamplesRemaining =
            static_cast<int>(getSampleRate() * 0.8);
    }

    if (previewSamplesRemaining > 0)
    {
        const int blockSize = buffer.getNumSamples();

        if (previewSamplesRemaining <= blockSize)
        {
            midiMessages.addEvent(
                juce::MidiMessage::noteOff(1, 60),
                previewSamplesRemaining);

            previewSamplesRemaining = 0;
        }
        else
        {
            previewSamplesRemaining -= blockSize;
        }
    }

    const juce::ScopedLock lock(getCallbackLock());

    synthesiser.renderNextBlock(
        buffer,
        midiMessages,
        0,
        buffer.getNumSamples());
}

bool SonicLureAudioProcessor::loadSampleFile(
    const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
        return false;

    juce::BigInteger midiNotes;
    midiNotes.setRange(0, 128, true);

    auto newSound = std::make_unique<juce::SamplerSound>(
        file.getFileNameWithoutExtension(),
        *reader,
        midiNotes,
        60,
        0.01,
        0.8,
        120.0);

    {
        const juce::ScopedLock lock(getCallbackLock());

        synthesiser.clearSounds();
        synthesiser.addSound(newSound.release());

        loadedSampleName = file.getFileName();
    }

    return true;
}

void SonicLureAudioProcessor::playPreview()
{
    previewRequested.store(true);
}

juce::String SonicLureAudioProcessor::getLoadedSampleName() const
{
    return loadedSampleName;
}

const juce::String SonicLureAudioProcessor::getName() const
{
    return "XLURE";
}

bool SonicLureAudioProcessor::acceptsMidi() const
{
    return true;
}

bool SonicLureAudioProcessor::producesMidi() const
{
    return false;
}

bool SonicLureAudioProcessor::isMidiEffect() const
{
    return false;
}

double SonicLureAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SonicLureAudioProcessor::getNumPrograms()
{
    return 1;
}

int SonicLureAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SonicLureAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String SonicLureAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return "Default Sample";
}

void SonicLureAudioProcessor::changeProgramName(
    int index,
    const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void SonicLureAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void SonicLureAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

bool SonicLureAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor*
SonicLureAudioProcessor::createEditor()
{
    return new SonicLureAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SonicLureAudioProcessor();
}
