
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    float safeFloat(float value)
    {
        return std::isfinite(value) ? value : 0.0f;
    }
}

//==============================================================================

XLUREAudioProcessor::XLUREAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();
}

XLUREAudioProcessor::~XLUREAudioProcessor() = default;

//==============================================================================

void XLUREAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    for (auto& voice : voices)
        resetVoice(voice);

    for (auto& voice : sampleVoices)
        resetSampleVoice(voice);
}

void XLUREAudioProcessor::releaseResources()
{
}

bool XLUREAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

//==============================================================================

void XLUREAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    for (const auto metadata : midiMessages)
        handleMidiMessage(metadata.getMessage());

    if (previewRequested.exchange(false))
        startSampleVoice(60, 1.0f, true);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    juce::ScopedLock lock(samplerLock);

    const float x = morphX.load();
    const float y = morphY.load();
    const float cutoff = filterCutoff.load();
    const float resonance = filterResonance.load();
    const float modal = modalAmount.load();
    const float master = masterLevel.load();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float synthOutput = 0.0f;

        for (auto& voice : voices)
        {
            if (voice.active)
                synthOutput += renderVoice(voice);
        }

        synthOutput *= master;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            float output = synthOutput;

            for (auto& sampleVoice : sampleVoices)
            {
                if (sampleVoice.active)
                    output += renderSampleVoice(sampleVoice, channel) * master;
            }

            buffer.addSample(channel, sample, output);
        }
    }
}

//==============================================================================

void XLUREAudioProcessor::handleMidiMessage(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        startVoice(message.getNoteNumber(), message.getFloatVelocity());
        startSampleVoice(message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isNoteOff())
    {
        stopVoice(message.getNoteNumber());

        // Sample playback is one-shot.
        // MIDI note-off does not interrupt the WAV.
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        for (auto& voice : voices)
            resetVoice(voice);

        for (auto& voice : sampleVoices)
            resetSampleVoice(voice);
    }
}

//==============================================================================

void XLUREAudioProcessor::startVoice(int midiNote, float velocity)
{
    const int index = findFreeVoice();

    if (index < 0)
        return;

    auto& voice = voices[static_cast<size_t>(index)];

    resetVoice(voice);

    voice.active = true;
    voice.midiNote = midiNote;
    voice.velocity = juce::jlimit(0.0f, 1.0f, velocity);
    voice.envelope = 0.0f;
}

void XLUREAudioProcessor::stopVoice(int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice.active && voice.midiNote == midiNote)
        {
            voice.releasing = true;
            voice.releaseRate = static_cast<float>(
                1.0 / (0.3 * currentSampleRate));
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

    return 0;
}

//==============================================================================
// WAV SAMPLE ENGINE

bool XLUREAudioProcessor::loadSampleFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
        return false;

    if (reader->lengthInSamples <= 0
        || reader->lengthInSamples > static_cast<juce::int64>(reader->sampleRate * 120.0))
        return false;

    const int channels = juce::jlimit(1, 2, static_cast<int>(reader->numChannels));
    const int samples = static_cast<int>(reader->lengthInSamples);

    juce::AudioBuffer<float> newBuffer(channels, samples);

    if (!reader->read(&newBuffer, 0, samples, 0, true, true))
        return false;

    {
        juce::ScopedLock lock(samplerLock);

        sampleBuffer = std::move(newBuffer);
        loadedSampleRate = reader->sampleRate;
        loadedSampleName = file.getFileNameWithoutExtension();

        for (auto& voice : sampleVoices)
            resetSampleVoice(voice);
    }

    return true;
}

void XLUREAudioProcessor::playPreview()
{
    previewRequested.store(true);
}

juce::String XLUREAudioProcessor::getLoadedSampleName() const
{
    const juce::ScopedLock lock(samplerLock);
    return loadedSampleName;
}

void XLUREAudioProcessor::setSampleStart(float value)
{
    value = juce::jlimit(0.0f, 0.999f, safeFloat(value));

    sampleStart.store(juce::jmin(value, sampleEnd.load() - 0.001f));
}

void XLUREAudioProcessor::setSampleEnd(float value)
{
    value = juce::jlimit(0.001f, 1.0f, safeFloat(value));

    sampleEnd.store(juce::jmax(value, sampleStart.load() + 0.001f));
}

float XLUREAudioProcessor::getSampleStart() const
{
    return sampleStart.load();
}

float XLUREAudioProcessor::getSampleEnd() const
{
    return sampleEnd.load();
}

void XLUREAudioProcessor::resetSampleRange()
{
    sampleStart.store(0.0f);
    sampleEnd.store(1.0f);
}

void XLUREAudioProcessor::startSampleVoice(
    int midiNote,
    float velocity,
    bool preview)
{
    if (sampleBuffer.getNumSamples() <= 0)
        return;

    int index = -1;

    for (int i = 0; i < maximumVoices; ++i)
    {
        if (!sampleVoices[static_cast<size_t>(i)].active)
        {
            index = i;
            break;
        }
    }

    if (index < 0)
        index = 0;

    auto& voice = sampleVoices[static_cast<size_t>(index)];

    resetSampleVoice(voice);

    const double startPosition =
        static_cast<double>(sampleStart.load()) * sampleBuffer.getNumSamples();

    const double pitch =
        std::pow(2.0, static_cast<double>(midiNote - 60) / 12.0);

    voice.active = true;
    voice.preview = preview;
    voice.midiNote = midiNote;
    voice.velocity = juce::jlimit(0.0f, 1.0f, velocity);
    voice.position = startPosition;
    voice.increment = (loadedSampleRate / currentSampleRate) * pitch;
}

void XLUREAudioProcessor::stopSampleVoice(int midiNote)
{
    for (auto& voice : sampleVoices)
    {
        if (voice.active && voice.midiNote == midiNote)
            resetSampleVoice(voice);
    }
}

float XLUREAudioProcessor::renderSampleVoice(
    SampleVoice& voice,
    int channel)
{
    if (!voice.active || sampleBuffer.getNumSamples() <= 0)
        return 0.0f;

    const int totalSamples = sampleBuffer.getNumSamples();

    const double endPosition =
        static_cast<double>(sampleEnd.load()) * totalSamples;

    if (voice.position >= endPosition || voice.position >= totalSamples)
    {
        resetSampleVoice(voice);
        return 0.0f;
    }

    const int sourceChannel =
        juce::jlimit(0, sampleBuffer.getNumChannels() - 1, channel);

    const int index = static_cast<int>(voice.position);
    const int nextIndex = juce::jmin(index + 1, totalSamples - 1);

    const float fraction =
        static_cast<float>(voice.position - index);

    const float first =
        sampleBuffer.getSample(sourceChannel, index);

    const float second =
        sampleBuffer.getSample(sourceChannel, nextIndex);

    const float output =
        (first + (second - first) * fraction) * voice.velocity;

    // Advance once per output frame, after the final channel.
    if (channel == getTotalNumOutputChannels() - 1)
    {
        voice.position += voice.increment;

        if (voice.position >= endPosition)
            resetSampleVoice(voice);
    }

    return output;
}

void XLUREAudioProcessor::resetSampleVoice(SampleVoice& voice)
{
    voice.active = false;
    voice.preview = false;
    voice.midiNote = 60;
    voice.velocity = 0.0f;
    voice.position = 0.0;
    voice.increment = 1.0;
    voice.previewSamplesRemaining = 0;
}

//==============================================================================
// SYNTH ENGINE

float XLUREAudioProcessor::renderVoice(XLUREVoice& voice)
{
    if (!voice.active)
        return 0.0f;

    const float attackRate =
        static_cast<float>(1.0 / (0.012 * currentSampleRate));

    if (!voice.releasing)
    {
        voice.envelope = juce::jmin(
            1.0f, voice.envelope + attackRate);
    }
    else
    {
        voice.envelope -= voice.releaseRate;

        if (voice.envelope <= 0.0f)
        {
            resetVoice(voice);
            return 0.0f;
        }
    }

    const float brightness = juce::jlimit(
        0.0f, 1.0f,
        0.5f + (morphY.load() - 0.5f) * 1.5f);

    const float resonance = filterResonance.load();

    const float subtractive =
        renderSubtractive(voice, brightness, resonance);

    const float modal =
        renderModal(voice, brightness);

    const float blend =
        juce::jlimit(0.0f, 1.0f, modalAmount.load());

    const float morphBlend =
        juce::jlimit(0.0f, 1.0f, morphX.load());

    const float finalBlend =
        juce::jlimit(0.0f, 1.0f, (blend + morphBlend) * 0.5f);

    return (subtractive * (1.0f - finalBlend)
        + modal * finalBlend)
        * voice.envelope
        * voice.velocity
        * 0.25f;
}

float XLUREAudioProcessor::renderSubtractive(
    XLUREVoice& voice,
    float brightness,
    float resonance)
{
    const double frequency =
        440.0 * std::pow(2.0, (voice.midiNote - 69) / 12.0);

    const double increment =
        twoPi * frequency / currentSampleRate;

    voice.phaseOne += increment;
    voice.phaseTwo += increment * 1.003;

    if (voice.phaseOne >= twoPi)
        voice.phaseOne -= twoPi;

    if (voice.phaseTwo >= twoPi)
        voice.phaseTwo -= twoPi;

    const float sawOne =
        static_cast<float>(2.0 * (voice.phaseOne / twoPi) - 1.0);

    const float sawTwo =
        static_cast<float>(2.0 * (voice.phaseTwo / twoPi) - 1.0);

    const float pulse =
        std::sin(static_cast<float>(voice.phaseOne));

    const float oscillator =
        (sawOne * 0.4f + sawTwo * 0.4f + pulse * 0.2f);

    const float cutoff =
        80.0f * std::pow(200.0f, filterCutoff.load());

    const float coefficient =
        juce::jlimit(0.001f, 0.95f,
            1.0f - std::exp(
                -twoPi * cutoff / static_cast<float>(currentSampleRate)));

    const float feedback =
        voice.filterState * resonance * 0.85f;

    const float input =
        oscillator * (0.3f + brightness * 0.7f) - feedback;

    voice.filterState += coefficient * (input - voice.filterState);

    return voice.filterState;
}

float XLUREAudioProcessor::renderModal(
    XLUREVoice& voice,
    float brightness)
{
    const double baseFrequency =
        440.0 * std::pow(2.0, (voice.midiNote - 69) / 12.0);

    const std::array<double, 8> ratios {
        1.0, 2.01, 3.9, 5.43, 7.2, 8.8, 11.1, 13.7
    };

    float output = 0.0f;

    for (int i = 0; i < numberOfModes; ++i)
    {
        const auto index = static_cast<size_t>(i);

        const double frequency =
            baseFrequency * ratios[index];

        voice.modalPhases[index] +=
            twoPi * frequency / currentSampleRate;

        if (voice.modalPhases[index] >= twoPi)
            voice.modalPhases[index] -= twoPi;

        const float level =
            std::pow(0.55f, static_cast<float>(i))
            * (0.25f + brightness * 0.75f);

        output += static_cast<float>(
            std::sin(voice.modalPhases[index])) * level;
    }

    return output / 3.0f;
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

//==============================================================================
// CONTROLS

void XLUREAudioProcessor::setMorphPosition(float x, float y)
{
    morphX.store(juce::jlimit(0.0f, 1.0f, safeFloat(x)));
    morphY.store(juce::jlimit(0.0f, 1.0f, safeFloat(y)));
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
    filterCutoff.store(juce::jlimit(0.0f, 1.0f, safeFloat(value)));
}

void XLUREAudioProcessor::setFilterResonance(float value)
{
    filterResonance.store(juce::jlimit(0.0f, 1.0f, safeFloat(value)));
}

void XLUREAudioProcessor::setModalAmount(float value)
{
    modalAmount.store(juce::jlimit(0.0f, 1.0f, safeFloat(value)));
}

void XLUREAudioProcessor::setMasterLevel(float value)
{
    masterLevel.store(juce::jlimit(0.0f, 1.0f, safeFloat(value)));
}

//==============================================================================
// PLUGIN INFORMATION

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
    return 0.0;
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

void XLUREAudioProcessor::changeProgramName(int, const juce::String&)
{
}

//==============================================================================
// STATE

void XLUREAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    juce::XmlElement xml("XLURE_STATE");

    xml.setAttribute("morphX", morphX.load());
    xml.setAttribute("morphY", morphY.load());
    xml.setAttribute("cutoff", filterCutoff.load());
    xml.setAttribute("resonance", filterResonance.load());
    xml.setAttribute("modal", modalAmount.load());
    xml.setAttribute("master", masterLevel.load());
    xml.setAttribute("sampleStart", sampleStart.load());
    xml.setAttribute("sampleEnd", sampleEnd.load());

    copyXmlToBinary(xml, destData);
}

void XLUREAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));

    if (xml == nullptr || !xml->hasTagName("XLURE_STATE"))
        return;

    setMorphPosition(
        static_cast<float>(xml->getDoubleAttribute("morphX", 0.5)),
        static_cast<float>(xml->getDoubleAttribute("morphY", 0.5)));

    setFilterCutoff(
        static_cast<float>(xml->getDoubleAttribute("cutoff", 0.65)));

    setFilterResonance(
        static_cast<float>(xml->getDoubleAttribute("resonance", 0.15)));

    setModalAmount(
        static_cast<float>(xml->getDoubleAttribute("modal", 0.5)));

    setMasterLevel(
        static_cast<float>(xml->getDoubleAttribute("master", 0.75)));

    setSampleStart(
        static_cast<float>(xml->getDoubleAttribute("sampleStart", 0.0)));

    setSampleEnd(
        static_cast<float>(xml->getDoubleAttribute("sampleEnd", 1.0)));
}

//==============================================================================

juce::AudioProcessorEditor* XLUREAudioProcessor::createEditor()
{
    return new XLUREAudioProcessorEditor(*this);
}

bool XLUREAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XLUREAudioProcessor();
}
