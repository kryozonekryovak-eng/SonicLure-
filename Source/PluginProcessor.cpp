
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    class SonicLureSound : public juce::SynthesiserSound
    {
    public:
        bool appliesToNote(int) override { return true; }
        bool appliesToChannel(int) override { return true; }
    };

    class SonicLureVoice : public juce::SynthesiserVoice
    {
    public:
        SonicLureVoice(juce::AudioProcessorValueTreeState& state)
            : attack(state.getRawParameterValue("ATTACK")),
              release(state.getRawParameterValue("RELEASE")),
              brightness(state.getRawParameterValue("BRIGHTNESS")),
              drift(state.getRawParameterValue("DRIFT")),
              tide(state.getRawParameterValue("TIDE")),
              waveform(state.getRawParameterValue("WAVEFORM"))
        {
        }

        bool canPlaySound(juce::SynthesiserSound* sound) override
        {
            return dynamic_cast<SonicLureSound*>(sound) != nullptr;
        }

        void startNote(int midiNote, float velocity,
                       juce::SynthesiserSound*, int) override
        {
            frequency = juce::MidiMessage::getMidiNoteInHertz(midiNote);
            level = velocity;
            phase = 0.0;
            driftPhase = 0.0;

            juce::ADSR::Parameters envelope;
            envelope.attack = attack->load();
            envelope.decay = 0.2f;
            envelope.sustain = 0.8f;
            envelope.release = release->load();

            adsr.setParameters(envelope);
            adsr.noteOn();
        }

        void stopNote(float, bool allowTailOff) override
        {
            if (allowTailOff)
                adsr.noteOff();
            else
            {
                adsr.reset();
                clearCurrentNote();
            }
        }

        void pitchWheelMoved(int) override {}
        void controllerMoved(int, int) override {}

        void renderNextBlock(juce::AudioBuffer<float>& buffer,
                             int startSample, int numSamples) override
        {
            if (!isVoiceActive())
                return;

            const double sampleRate = getSampleRate();
            if (sampleRate <= 0.0)
                return;

            const float tone = brightness->load();
            const float movement = drift->load();
            const float tideAmount = tide->load();
            const int wave = juce::roundToInt(waveform->load());

            for (int sample = 0; sample < numSamples; ++sample)
            {
                driftPhase += 0.0007 * movement;
                const double modulation =
                    std::sin(driftPhase) * movement * 0.015;

                const double currentFrequency =
                    frequency * (1.0 + modulation
                    + std::sin(driftPhase * 0.25) * tideAmount * 0.02);

                const double increment = currentFrequency / sampleRate;
                phase += increment;
                if (phase >= 1.0)
                    phase -= 1.0;

                float signal = 0.0f;

                if (wave == 1)
                    signal = static_cast<float>(2.0 * phase - 1.0);
                else if (wave == 2)
                    signal = phase < 0.5 ? 1.0f : -1.0f;
                else
                    signal = static_cast<float>(
                        std::sin(phase * juce::MathConstants<double>::twoPi));

                const float envelope = adsr.getNextSample();
                const float output = signal * envelope * level
                                   * (0.25f + tone * 0.5f);

                for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                    buffer.addSample(channel, startSample + sample, output);
            }

            if (!adsr.isActive())
                clearCurrentNote();
        }

    private:
        std::atomic<float>* attack;
        std::atomic<float>* release;
        std::atomic<float>* brightness;
        std::atomic<float>* drift;
        std::atomic<float>* tide;
        std::atomic<float>* waveform;

        juce::ADSR adsr;
        double phase = 0.0;
        double driftPhase = 0.0;
        double frequency = 440.0;
        float level = 0.0f;
    };
}

SonicLureAudioProcessor::SonicLureAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "SONICLURE", createParameterLayout())
{
    for (int i = 0; i < 16; ++i)
        synthesiser.addVoice(new SonicLureVoice(parameters));

    synthesiser.addSound(new SonicLureSound());
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

    return layout;
}

void SonicLureAudioProcessor::prepareToPlay(double sampleRate, int)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
}

void SonicLureAudioProcessor::releaseResources() {}

void SonicLureAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();
    synthesiser.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    const float gain = parameters.getRawParameterValue("OUTPUT")->load();

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
    return "SonicLure";
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
    const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);

    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SonicLureAudioProcessor();
}
