
#include "PluginProcessor.h"
#include <cmath>

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    float safeFloat(float value)
    {
        return std::isfinite(value) ? value : 0.0f;
    }
}

XLUREAudioProcessor::XLUREAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < 16; ++i)
        sampler.addVoice(new juce::SamplerVoice());
}

XLUREAudioProcessor::~XLUREAudioProcessor() = default;

void XLUREAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = juce::jmax(22050.0, sampleRate);
    sampler.setCurrentPlaybackSampleRate(currentSampleRate);

    for (auto& voice : voices)
        resetVoice(voice);
}

void XLUREAudioProcessor::releaseResources()
{
    for (auto& voice : voices)
        resetVoice(voice);
}

bool XLUREAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

    const int numberOfSamples = buffer.getNumSamples();
    buffer.clear();

    // Build a separate MIDI stream for the WAV sampler.
    juce::MidiBuffer samplerMidi;
    samplerMidi.addEvents(midiMessages, 0, numberOfSamples, 0);

    // The preview button plays a short middle-C sample.
    if (previewRequested.exchange(false))
        previewSamplesRemaining = static_cast<int>(currentSampleRate * 0.5);

    if (previewSamplesRemaining > 0 && numberOfSamples > 0)
    {
        samplerMidi.addEvent(
            juce::MidiMessage::noteOn(1, 60, 0.85f), 0);

        if (previewSamplesRemaining <= numberOfSamples)
        {
            samplerMidi.addEvent(
                juce::MidiMessage::noteOff(1, 60),
                previewSamplesRemaining);
            previewSamplesRemaining = 0;
        }
        else
        {
            previewSamplesRemaining -= numberOfSamples;
        }
    }

    // Render the independent WAV sampler.
    {
        const juce::ScopedLock lock(samplerLock);
        sampler.renderNextBlock(
            buffer, samplerMidi, 0, numberOfSamples);
    }

    // Render the subtractive/modal synth from incoming MIDI.
    juce::MidiBuffer::Iterator midiIterator(midiMessages);
    juce::MidiMessage midiMessage;
    int midiPosition = 0;

    bool hasMidiEvent = midiIterator.getNextEvent(
        midiMessage, midiPosition);

    for (int sample = 0; sample < numberOfSamples; ++sample)
    {
        while (hasMidiEvent && midiPosition <= sample)
        {
            handleMidiMessage(midiMessage);

            hasMidiEvent = midiIterator.getNextEvent(
                midiMessage, midiPosition);
        }

        const float targetMorph = morphX.load();
        const float targetBrightness = morphY.load();

        smoothedMorph += (targetMorph - smoothedMorph) * 0.0025f;
        smoothedBrightness +=
            (targetBrightness - smoothedBrightness) * 0.0025f;

        float synthOutput = 0.0f;

        for (auto& voice : voices)
        {
            if (voice.active)
                synthOutput += renderVoice(voice);
        }

        synthOutput *= masterLevel.load();
        synthOutput = safeFloat(synthOutput);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.addSample(channel, sample, synthOutput);
    }
}

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
    else if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        for (auto& voice : voices)
        {
            if (voice.active)
                stopVoice(voice.midiNote);
        }
    }
}

void XLUREAudioProcessor::startVoice(int midiNote, float velocity)
{
    const int voiceIndex = findFreeVoice();

    if (voiceIndex < 0)
        return;

    auto& voice = voices[static_cast<size_t>(voiceIndex)];

    resetVoice(voice);

    voice.active = true;
    voice.releasing = false;
    voice.midiNote = midiNote;
    voice.velocity = juce::jlimit(0.0f, 1.0f, velocity);

    const std::array<float, numberOfModes> levels
    {
        1.0f, 0.62f, 0.42f, 0.30f,
        0.22f, 0.16f, 0.12f, 0.08f
    };

    voice.modalLevels = levels;
}

void XLUREAudioProcessor::stopVoice(int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice.active
            && voice.midiNote == midiNote
            && !voice.releasing)
        {
            voice.releasing = true;

            const float releaseSeconds = 0.30f;
            voice.releaseRate = juce::jmax(
                0.000001f,
                voice.envelope /
                static_cast<float>(currentSampleRate * releaseSeconds));
        }
    }
}

int XLUREAudioProcessor::findFreeVoice() const
{
    for (int i = 0; i < maximumVoices; ++i)
    {
        if (!voices[static_cast<size_t>(i)].active)
            return i;
    }

    // Steal the first voice if all voices are occupied.
    return 0;
}

float XLUREAudioProcessor::renderVoice(XLUREVoice& voice)
{
    if (!voice.active)
        return 0.0f;

    if (!voice.releasing)
    {
        const float attackRate =
            1.0f / static_cast<float>(currentSampleRate * 0.012);

        voice.envelope = juce::jmin(
            1.0f, voice.envelope + attackRate);
    }
    else
    {
        voice.envelope -= voice.releaseRate;

        if (voice.envelope <= 0.0001f)
        {
            resetVoice(voice);
            return 0.0f;
        }
    }

    const float brightness = juce::jlimit(
        0.0f, 1.0f, smoothedBrightness);

    const float resonance = filterResonance.load();

    const float subtractive = renderSubtractive(
        voice, brightness, resonance);

    const float modal = renderModal(
        voice, brightness);

    const float morph = juce::jlimit(
        0.0f, 1.0f, smoothedMorph);

    // Equal-power crossfade between the two sound engines.
    const float subtractiveGain =
        std::cos(morph * juce::MathConstants<float>::halfPi);

    const float modalGain =
        std::sin(morph * juce::MathConstants<float>::halfPi);

    const float output =
        subtractive * subtractiveGain
        + modal * modalGain;

    return safeFloat(output * voice.envelope * voice.velocity * 0.22f);
}

float XLUREAudioProcessor::renderSubtractive(
    XLUREVoice& voice,
    float brightness,
    float resonance)
{
    const double frequency =
        juce::MidiMessage::getMidiNoteInHertz(voice.midiNote);

    const double incrementOne = frequency / currentSampleRate;
    const double incrementTwo =
        (frequency * 1.003) / currentSampleRate;

    voice.phaseOne += incrementOne;
    voice.phaseTwo += incrementTwo;

    if (voice.phaseOne >= 1.0)
        voice.phaseOne -= std::floor(voice.phaseOne);

    if (voice.phaseTwo >= 1.0)
        voice.phaseTwo -= std::floor(voice.phaseTwo);

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
        filterCutoff.load();

    const float cutoffHz =
        120.0f + cutoffControl * (16000.0f - 120.0f);

    const float brightnessMultiplier =
        0.18f + brightness * 0.82f;

    const float effectiveCutoff =
        juce::jlimit(80.0f, 18000.0f,
            cutoffHz * brightnessMultiplier);

    const float coefficient =
        1.0f - std::exp(
            -static_cast<float>(twoPi)
            * effectiveCutoff
            / static_cast<float>(currentSampleRate));

    const float feedback =
        juce::jlimit(0.0f, 0.35f, resonance * 0.35f);

    const float input =
        oscillatorMix + voice.filterState * feedback;

    voice.filterState +=
        coefficient * (input - voice.filterState);

    return safeFloat(voice.filterState);
}

float XLUREAudioProcessor::renderModal(
    XLUREVoice& voice,
    float brightness)
{
    const double fundamental =
        juce::MidiMessage::getMidiNoteInHertz(voice.midiNote);

    const std::array<double, numberOfModes> ratios
    {
        1.0, 2.01, 3.90, 5.40,
        7.20, 9.10, 11.30, 13.70
    };

    float output = 0.0f;
    float totalLevel = 0.0f;

    const float brightnessScale =
        0.25f + brightness * 0.75f;

    for (int i = 0; i < numberOfModes; ++i)
    {
        const size_t index = static_cast<size_t>(i);

        const double frequency =
            fundamental * ratios[index];

        const double increment =
            frequency / currentSampleRate;

        voice.modalPhases[index] += increment;

        if (voice.modalPhases[index] >= 1.0)
            voice.modalPhases[index] -=
                std::floor(voice.modalPhases[index]);

        const float partial =
            std::sin(static_cast<float>(
                voice.modalPhases[index] * twoPi));

        const float highModeScale =
            1.0f - (static_cast<float>(i) / 10.0f)
            * (1.0f - brightnessScale);

        const float level =
            voice.modalLevels[index] * highModeScale;

        output += partial * level;
        totalLevel += level;
    }

    if (totalLevel > 0.0f)
        output /= totalLevel;

    return safeFloat(output);
}

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

    voice.modalPhases.fill(0.0);
    voice.modalLevels.fill(0.0f);

    voice.noiseState = 0.0f;
}

void XLUREAudioProcessor::setMorphPosition(float x, float y)
{
    morphX.store(juce::jlimit(0.0f, 1.0f, x));
    morphY.store(juce::jlimit(0.0f, 1.0f, y));
}

float XLUREAudioProcessor::getMorphX() const
{
    return morphX.load();
}

float XLUREAudioProcessor::getMorphY() const
{
    return morphY.load();
}

void XLUREAudioProcessor::setFilterCutoff(float value)
{
    filterCutoff.store(juce::jlimit(0.0f, 1.0f, value));
}

void XLUREAudioProcessor::setFilterResonance(float value)
{
    filterResonance.store(juce::jlimit(0.0f, 1.0f, value));
}

void XLUREAudioProcessor::setModalAmount(float value)
{
    modalAmount.store(juce::jlimit(0.0f, 1.0f, value));
}

void XLUREAudioProcessor::setMasterLevel(float value)
{
    masterLevel.store(juce::jlimit(0.0f, 1.0f, value));
}

bool XLUREAudioProcessor::loadSampleFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
        return false;

    if (reader->lengthInSamples <= 0
        || reader->sampleRate <= 0.0
        || reader->lengthInSamples / reader->sampleRate > 120.0)
        return false;

    juce::BigInteger allNotes;
    allNotes.setRange(0, 128, true);

    auto* newSound = new juce::SamplerSound(
        file.getFileNameWithoutExtension(),
        *reader,
        allNotes,
        60,
        0.01,
        0.8,
        120.0);

    {
        const juce::ScopedLock lock(samplerLock);
        sampler.clearSounds();
        sampler.addSound(newSound);
    }

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

juce::AudioProcessorEditor* XLUREAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

bool XLUREAudioProcessor::hasEditor() const
{
    return true;
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

void XLUREAudioProcessor::setCurrentProgram(int)
{
}

const juce::String XLUREAudioProcessor::getProgramName(int)
{
    return "XLURE Default";
}

void XLUREAudioProcessor::changeProgramName(
    int, const juce::String&)
{
}

void XLUREAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    juce::ValueTree state("XLURESTATE");

    state.setProperty("morphX", morphX.load(), nullptr);
    state.setProperty("morphY", morphY.load(), nullptr);
    state.setProperty("filterCutoff", filterCutoff.load(), nullptr);
    state.setProperty("filterResonance", filterResonance.load(), nullptr);
    state.setProperty("modalAmount", modalAmount.load(), nullptr);
    state.setProperty("masterLevel", masterLevel.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void XLUREAudioProcessor::setStateInformation(
    const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));

    if (xml == nullptr || !xml->hasTagName("XLURESTATE"))
        return;

    const juce::ValueTree state =
        juce::ValueTree::fromXml(*xml);

    setMorphPosition(
        static_cast<float>(state.getProperty("morphX", 0.5f)),
        static_cast<float>(state.getProperty("morphY", 0.5f)));

    setFilterCutoff(
        static_cast<float>(state.getProperty("filterCutoff", 0.65f)));

    setFilterResonance(
        static_cast<float>(state.getProperty("filterResonance", 0.15f)));

    setModalAmount(
        static_cast<float>(state.getProperty("modalAmount", 0.5f)));

    setMasterLevel(
        static_cast<float>(state.getProperty("masterLevel", 0.75f)));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XLUREAudioProcessor();
}
