#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float pi = 3.14159265358979323846f;
    constexpr double twoPi = 6.28318530717958647692;

    float clamp01(float value)
    {
        return juce::jlimit(0.0f, 1.0f, value);
    }

    float midiFrequency(int note)
    {
        return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    }
}

//============================================================
XLUREAudioProcessor::XLUREAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < 16; ++i)
        sampler.addVoice(new juce::SamplerVoice());
}

XLUREAudioProcessor::~XLUREAudioProcessor() = default;

//============================================================
void XLUREAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    currentSampleRate = juce::jmax(1.0, sampleRate);

    sampler.setCurrentPlaybackSampleRate(currentSampleRate);

    for (auto& voice : voices)
        resetVoice(voice);

    previewSamplesRemaining = 0;
}

void XLUREAudioProcessor::releaseResources()
{
    for (auto& voice : voices)
        resetVoice(voice);
}

bool XLUREAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

//============================================================
void XLUREAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int channel = getTotalNumInputChannels();
         channel < numChannels; ++channel)
    {
        buffer.clear(channel, 0, numSamples);
    }

    // Keep a copy of incoming MIDI for the WAV sampler.
    juce::MidiBuffer samplerMidi = midiMessages;

    // Preview the loaded WAV when requested.
    if (previewRequested.exchange(false))
    {
        samplerMidi.addEvent(
            juce::MidiMessage::noteOn(1, 60, 0.9f), 0);

        previewSamplesRemaining =
            static_cast<int>(currentSampleRate * 2.0);
    }

    if (previewSamplesRemaining > 0)
    {
        if (previewSamplesRemaining <= numSamples)
        {
            samplerMidi.addEvent(
                juce::MidiMessage::noteOff(1, 60),
                previewSamplesRemaining);
            previewSamplesRemaining = 0;
        }
        else
        {
            previewSamplesRemaining -= numSamples;
        }
    }

    // Render the independent WAV sampler.
    {
        const juce::ScopedLock lock(samplerLock);

        sampler.renderNextBlock(
            buffer, samplerMidi, 0, numSamples);
    }

    // Smoothly follow the XY pad controls.
    const float targetMorph = clamp01(morphX.load());
    const float targetBrightness = clamp01(morphY.load());

    const float targetCutoff = clamp01(filterCutoff.load());
    const float resonance = clamp01(filterResonance.load());
    const float modalControl = clamp01(modalAmount.load());
    const float master = clamp01(masterLevel.load());

    // Process MIDI events and render the hybrid synthesiser.
    juce::MidiBuffer::Iterator midiIterator(midiMessages);
    juce::MidiMessage midiMessage;
    int midiSamplePosition = 0;

    bool hasMidiEvent = midiIterator.getNextEvent(
        midiMessage, midiSamplePosition);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        while (hasMidiEvent && midiSamplePosition <= sample)
        {
            handleMidiMessage(midiMessage);

            hasMidiEvent = midiIterator.getNextEvent(
                midiMessage, midiSamplePosition);
        }

        smoothedMorph +=
            (targetMorph - smoothedMorph) * 0.0025f;

        smoothedBrightness +=
            (targetBrightness - smoothedBrightness) * 0.0025f;

        const float brightness = clamp01(
            0.15f + smoothedBrightness * 0.85f);

        const float effectiveCutoff = clamp01(
            targetCutoff * 0.35f + brightness * 0.65f);

        float synthOutput = 0.0f;

        for (auto& voice : voices)
        {
            if (voice.active)
                synthOutput += renderVoice(voice);
        }

        synthOutput *= master * 0.18f;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.addSample(
                channel, sample, synthOutput);
        }
    }

    midiMessages.clear();
}

//============================================================
void XLUREAudioProcessor::handleMidiMessage(
    const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        startVoice(
            message.getNoteNumber(),
            message.getFloatVelocity());
    }
    else if (message.isNoteOff())
    {
        stopVoice(message.getNoteNumber());
    }
    else if (message.isAllNotesOff()
             || message.isAllSoundOff())
    {
        for (auto& voice : voices)
            voice.releasing = true;
    }
}

int XLUREAudioProcessor::findFreeVoice() const
{
    for (int i = 0; i < maximumVoices; ++i)
    {
        if (!voices[i].active)
            return i;
    }

    // Steal the first voice if all voices are occupied.
    return 0;
}

void XLUREAudioProcessor::startVoice(
    int midiNote,
    float velocity)
{
    const int index = findFreeVoice();

    auto& voice = voices[index];

    resetVoice(voice);

    voice.active = true;
    voice.midiNote = midiNote;
    voice.velocity = juce::jlimit(0.0f, 1.0f, velocity);

    voice.envelope = 0.0f;
    voice.releasing = false;

    static constexpr float modalRatios[numberOfModes] =
    {
        1.0f, 2.01f, 2.72f, 3.91f,
        5.43f, 6.81f, 8.17f, 10.23f
    };

    for (int i = 0; i < numberOfModes; ++i)
    {
        voice.modalPhases[i] = 0.0;
        voice.modalLevels[i] =
            1.0f / std::pow(static_cast<float>(i + 1), 0.72f);
    }

    juce::ignoreUnused(modalRatios);
}

void XLUREAudioProcessor::stopVoice(int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice.active && voice.midiNote == midiNote)
            voice.releasing = true;
    }
}

//============================================================
float XLUREAudioProcessor::renderVoice(XLUREVoice& voice)
{
    if (!voice.active)
        return 0.0f;

    const float brightness = clamp01(smoothedBrightness);
    const float morph = clamp01(smoothedMorph);

    const float resonance = clamp01(filterResonance.load());

    const float subtractive =
        renderSubtractive(voice, brightness, resonance);

    const float modal =
        renderModal(voice, brightness);

    // Equal-power style blend between the two engines.
    const float subtractiveGain =
        std::cos(morph * pi * 0.5f);

    const float modalGain =
        std::sin(morph * pi * 0.5f);

    float output =
        subtractive * subtractiveGain
        + modal * modalGain;

    // Envelope attack and release.
    if (!voice.releasing)
    {
        const float attackRate =
            1.0f / static_cast<float>(currentSampleRate * 0.012);

        voice.envelope = juce::jmin(
            1.0f, voice.envelope + attackRate);
    }
    else
    {
        const float releaseRate =
            1.0f / static_cast<float>(currentSampleRate * 0.38);

        voice.envelope -= releaseRate;

        if (voice.envelope <= 0.0001f)
        {
            resetVoice(voice);
            return 0.0f;
        }
    }

    output *= voice.envelope * voice.velocity;

    return output;
}

//============================================================
float XLUREAudioProcessor::renderSubtractive(
    XLUREVoice& voice,
    float brightness,
    float resonance)
{
    const double baseFrequency =
        midiFrequency(voice.midiNote);

    const double incrementOne =
        baseFrequency / currentSampleRate;

    const double incrementTwo =
        (baseFrequency * 1.003) / currentSampleRate;

    voice.phaseOne += incrementOne;
    voice.phaseTwo += incrementTwo;

    if (voice.phaseOne >= 1.0)
        voice.phaseOne -= 1.0;

    if (voice.phaseTwo >= 1.0)
        voice.phaseTwo -= 1.0;

    const float sawOne =
        static_cast<float>(voice.phaseOne * 2.0 - 1.0);

    const float sawTwo =
        static_cast<float>(voice.phaseTwo * 2.0 - 1.0);

    const float pulse =
        voice.phaseOne < 0.5 ? 1.0f : -1.0f;

    const float sine =
        std::sin(static_cast<float>(voice.phaseOne * twoPi));

    const float oscillatorMix =
        sawOne * 0.42f
        + sawTwo * 0.28f
        + pulse * 0.18f
        + sine * 0.12f;

    const float cutoffControl =
        clamp01(filterCutoff.load()) * 0.35f
        + brightness * 0.65f;

    const float cutoffHz =
        100.0f + cutoffControl * 15000.0f;

    const float coefficient = juce::jlimit(
        0.001f,
        0.95f,
        1.0f - std::exp(
            -2.0f * pi * cutoffHz
            / static_cast<float>(currentSampleRate)));

    voice.filterState +=
        coefficient * (oscillatorMix - voice.filterState);

    const float resonanceBoost =
        1.0f + resonance * 1.4f;

    return voice.filterState * resonanceBoost * 0.55f;
}

//============================================================
float XLUREAudioProcessor::renderModal(
    XLUREVoice& voice,
    float brightness)
{
    static constexpr double ratios[numberOfModes] =
    {
        1.0, 2.01, 2.72, 3.91,
        5.43, 6.81, 8.17, 10.23
    };

    const double baseFrequency =
        midiFrequency(voice.midiNote);

    float output = 0.0f;

    const float modalBrightness =
        0.25f + brightness * 1.4f;

    const float damping =
        0.35f + brightness * 0.65f;

    for (int i = 0; i < numberOfModes; ++i)
    {
        const double frequency =
            baseFrequency * ratios[i];

        const double increment =
            frequency / currentSampleRate;

        voice.modalPhases[i] += increment;

        if (voice.modalPhases[i] >= 1.0)
            voice.modalPhases[i] -= 1.0;

        const float partial =
            std::sin(static_cast<float>(
                voice.modalPhases[i] * twoPi));

        const float modeBrightness =
            1.0f / std::pow(
                static_cast<float>(i + 1),
                modalBrightness);

        const float modeDamping =
            std::pow(damping, static_cast<float>(i) * 0.35f);

        output += partial
            * voice.modalLevels[i]
            * modeBrightness
            * modeDamping;
    }

    const float amount = clamp01(modalAmount.load());

    return output * (0.10f + amount * 0.30f);
}

//============================================================
void XLUREAudioProcessor::resetVoice(XLUREVoice& voice)
{
    voice.active = false;
    voice.releasing = false;

    voice.midiNote = 60;
    voice.velocity = 0.0f;

    voice.phaseOne = 0.0;
    voice.phaseTwo = 0.0;

    voice.envelope = 0.0f;
    voice.releaseRate = 0.0f;

    voice.filterState = 0.0f;

    voice.noiseState = 0.0f;

    for (int i = 0; i < numberOfModes; ++i)
    {
        voice.modalPhases[i] = 0.0;
        voice.modalLevels[i] = 0.0f;
    }
}

//============================================================
// XY MORPH PAD
//============================================================
void XLUREAudioProcessor::setMorphPosition(float x, float y)
{
    morphX.store(clamp01(x));
    morphY.store(clamp01(y));
}

float XLUREAudioProcessor::getMorphX() const
{
    return morphX.load();
}

float XLUREAudioProcessor::getMorphY() const
{
    return morphY.load();
}

//============================================================
// SYNTHESIS CONTROLS
//============================================================
void XLUREAudioProcessor::setFilterCutoff(float value)
{
    filterCutoff.store(clamp01(value));
}

void XLUREAudioProcessor::setFilterResonance(float value)
{
    filterResonance.store(clamp01(value));
}

void XLUREAudioProcessor::setModalAmount(float value)
{
    modalAmount.store(clamp01(value));
}

void XLUREAudioProcessor::setMasterLevel(float value)
{
    masterLevel.store(clamp01(value));
}

//============================================================
// WAV SAMPLE ENGINE
//============================================================
bool XLUREAudioProcessor::loadSampleFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
        return false;

    const double maximumLengthSeconds = 120.0;

    juce::BigInteger midiNotes;
    midiNotes.setRange(0, 128, true);

    const juce::ScopedLock lock(samplerLock);

    sampler.clearSounds();

    sampler.addSound(new juce::SamplerSound(
        file.getFileNameWithoutExtension(),
        *reader,
        midiNotes,
        60,
        0.01,
        0.8,
        maximumLengthSeconds));

    loadedSampleName = file.getFileName();

    return true;
}

void XLUREAudioProcessor::playPreview()
{
    previewRequested.store(true);
}

juce::String XLUREAudioProcessor::getLoadedSampleName() const
{
    return loadedSampleName;
}

//============================================================
// PLUGIN INFORMATION
//============================================================
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
    return 2.0;
}

int XLUREAudioProcessor::getNumPrograms()
{
    return 1;
}

int XLUREAudioProcessor::getCurrentProgram()
{
    return 0;
}

void XLUREAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String XLUREAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return "Default";
}

void XLUREAudioProcessor::changeProgramName(
    int index,
    const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//============================================================
// STATE
//============================================================
void XLUREAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(
        destData, false);

    stream.writeFloat(morphX.load());
    stream.writeFloat(morphY.load());
    stream.writeFloat(filterCutoff.load());
    stream.writeFloat(filterResonance.load());
    stream.writeFloat(modalAmount.load());
    stream.writeFloat(masterLevel.load());
}

void XLUREAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    if (data == nullptr || sizeInBytes < 6 * (int) sizeof(float))
        return;

    juce::MemoryInputStream stream(
        data,
        static_cast<size_t>(sizeInBytes),
        false);

    setMorphPosition(
        stream.readFloat(),
        stream.readFloat());

    setFilterCutoff(stream.readFloat());
    setFilterResonance(stream.readFloat());
    setModalAmount(stream.readFloat());
    setMasterLevel(stream.readFloat());
}

//============================================================
juce::AudioProcessorEditor*
XLUREAudioProcessor::createEditor()
{
    return new XLUREAudioProcessorEditor(*this);
}

bool XLUREAudioProcessor::hasEditor() const
{
    return true;
}

//============================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XLUREAudioProcessor();
}
