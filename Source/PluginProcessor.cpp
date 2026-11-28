
#include "PluginProcessor.h"
#include "PluginEditor.h"

SonicLureAudioProcessor::SonicLureAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "XLURE", createParameterLayout())
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < 16; ++i)
        synthesiser.addVoice(new juce::SamplerVoice());
}

SonicLureAudioProcessor::~SonicLureAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
SonicLureAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ATTACK", "Attack", 0.01f, 5.0f, 0.05f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "RELEASE", "Release", 0.05f, 8.0f, 0.8f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "BRIGHTNESS", "Brightness", 0.0f, 1.0f, 0.7f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "DRIFT", "Drift", 0.0f, 1.0f, 0.2f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "TIDE", "Tide", 0.0f, 1.0f, 0.3f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OUTPUT", "Output", 0.0f, 1.0f, 0.7f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "WAVEFORM", "Waveform",
        juce::StringArray { "Sine", "Saw", "Square" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC1_LEVEL", "Oscillator 1", 0.0f, 1.0f, 0.8f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC2_LEVEL", "Oscillator 2", 0.0f, 1.0f, 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC3_LEVEL", "Oscillator 3", 0.0f, 1.0f, 0.3f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC2_DETUNE", "Oscillator 2 Detune", -50.0f, 50.0f, 7.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC3_DETUNE", "Oscillator 3 Detune", -50.0f, 50.0f, -7.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SUB_LEVEL", "Sub Bass", 0.0f, 1.0f, 0.2f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "NOISE_LEVEL", "Noise", 0.0f, 0.5f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_CUTOFF", "Filter Cutoff", 40.0f, 18000.0f, 12000.0f));

    return layout;
}

void SonicLureAudioProcessor::loadSampleBank(const juce::File& folder)
{
    clearSampleBank();

    if (!folder.isDirectory())
        return;

    juce::Array<juce::File> sampleFiles;

    folder.findChildFiles(
        sampleFiles,
        juce::File::findFiles,
        true,
        formatManager.getWildcardForAllFormats());

    if (sampleFiles.isEmpty())
        return;

    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        formatManager.createReaderFor(sampleFiles[0]));

    if (reader == nullptr)
        return;

    juce::BigInteger allNotes;
    allNotes.setRange(0, 128, true);

    const float attack =
        parameters.getRawParameterValue("ATTACK")->load();

    const float release =
        parameters.getRawParameterValue("RELEASE")->load();

    synthesiser.addSound(new juce::SamplerSound(
        sampleFiles[0].getFileNameWithoutExtension(),
        *reader,
        allNotes,
        60,
        attack,
        release,
        120.0));
}

void SonicLureAudioProcessor::clearSampleBank()
{
    synthesiser.clearSounds();
}

void SonicLureAudioProcessor::prepareToPlay(double sampleRate, int)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
}

void SonicLureAudioProcessor::releaseResources() {}

void SonicLureAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    synthesiser.renderNextBlock(
        buffer,
        midi,
        0,
        buffer.getNumSamples());

    const float gain =
        parameters.getRawParameterValue("OUTPUT")->load();

    buffer.applyGain(gain);
}

juce::AudioProcessorEditor*
SonicLureAudioProcessor::createEditor()
{
    return new SonicLureAudioProcessorEditor(*this);
}

bool SonicLureAudioProcessor::hasEditor() const { return true; }

const juce::String SonicLureAudioProcessor::getName() const
{
    return "XLURE";
}

bool SonicLureAudioProcessor::acceptsMidi() const { return true; }
bool SonicLureAudioProcessor::producesMidi() const { return false; }
bool SonicLureAudioProcessor::isMidiEffect() const { return false; }

double SonicLureAudioProcessor::getTailLengthSeconds() const
{
    return 8.0;
}

int SonicLureAudioProcessor::getNumPrograms() { return 1; }
int SonicLureAudioProcessor::getCurrentProgram() { return 0; }
void SonicLureAudioProcessor::setCurrentProgram(int) {}
const juce::String SonicLureAudioProcessor::getProgramName(int) { return {}; }
void SonicLureAudioProcessor::changeProgramName(int, const juce::String&) {}

void SonicLureAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    auto xml = state.createXml();

    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void SonicLureAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);

    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SonicLureAudioProcessor();
}
