
#include "PluginProcessor.h"
#include "PluginEditor.h"

XLUREAudioProcessor::XLUREAudioProcessor()
    : AudioProcessor(
        BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < 16; ++i)
        sampler.addVoice(new juce::SamplerVoice());
}

XLUREAudioProcessor::~XLUREAudioProcessor()
{
}

const juce::String XLUREAudioProcessor::getName() const
{
    return "XLURE";
}

bool XLUREAudioProcessor::acceptsMidi() const
{
    return true;
}

bool XLUREAudioProcessor::producesMidi() const
{
    return false;
}

bool XLUREAudioProcessor::isMidiEffect() const
{
    return false;
}

double XLUREAudioProcessor::getTailLengthSeconds() const
{
    return 1.0;
}

int XLUREAudioProcessor::getNumPrograms()
{
    return 1;
}

int XLUREAudioProcessor::getCurrentProgram()
{
    return 0;
}

void XLUREAudioProcessor::setCurrentProgram(int)
{
}

const juce::String XLUREAudioProcessor::getProgramName(int)
{
    return {};
}

void XLUREAudioProcessor::changeProgramName(
    int,
    const juce::String&)
{
}

void XLUREAudioProcessor::prepareToPlay(
    double sampleRate,
    int)
{
    currentSampleRate = sampleRate;
    sampler.setCurrentPlaybackSampleRate(sampleRate);
}

void XLUREAudioProcessor::releaseResources()
{
    juce::ScopedLock lock(samplerLock);
    sampler.allNotesOff(0, false);
}

bool XLUREAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

void XLUREAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    for (int channel = getTotalNumInputChannels();
         channel < getTotalNumOutputChannels();
         ++channel)
    {
        buffer.clear(channel, 0, numSamples);
    }

    juce::ScopedLock lock(samplerLock);

    if (previewRequested.exchange(false))
    {
        sampler.noteOn(1, 60, 0.8f);
        previewSamplesRemaining =
            static_cast<int>(currentSampleRate * 0.8);
    }

    juce::MidiBuffer playbackMidi;
    playbackMidi.addEvents(
        midiMessages,
        0,
        numSamples,
        0);

    if (previewSamplesRemaining > 0)
    {
        if (previewSamplesRemaining <= numSamples)
        {
            juce::MidiMessage noteOff =
                juce::MidiMessage::noteOff(1, 60);

            playbackMidi.addEvent(
                noteOff,
                previewSamplesRemaining);

            previewSamplesRemaining = 0;
        }
        else
        {
            previewSamplesRemaining -= numSamples;
        }
    }

    sampler.renderNextBlock(
        buffer,
        playbackMidi,
        0,
        numSamples);
}

bool XLUREAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor*
XLUREAudioProcessor::createEditor()
{
    return new XLUREAudioProcessorEditor(*this);
}

bool XLUREAudioProcessor::loadSampleFile(
    const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
        return false;

    if (reader->lengthInSamples <= 0
        || reader->lengthInSamples >
            static_cast<juce::int64>(reader->sampleRate * 120.0))
    {
        return false;
    }

    juce::ScopedLock lock(samplerLock);

    sampler.allNotesOff(0, false);
    sampler.clearSounds();

    sampler.addSound(
        new juce::SamplerSound(
            file.getFileNameWithoutExtension(),
            *reader,
            juce::BigInteger(0, 128),
            60,
            0.01,
            0.8,
            120.0));

    loadedSampleName = file.getFileName();

    return true;
}

void XLUREAudioProcessor::playPreview()
{
    previewRequested = true;
}

juce::String XLUREAudioProcessor::getLoadedSampleName() const
{
    return loadedSampleName;
}

void XLUREAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void XLUREAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XLUREAudioProcessor();
}
