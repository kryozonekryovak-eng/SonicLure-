
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

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
              waveform(state.getRawParameterValue("WAVEFORM")),
              osc1Level(state.getRawParameterValue("OSC1_LEVEL")),
              osc2Level(state.getRawParameterValue("OSC2_LEVEL")),
              osc3Level(state.getRawParameterValue("OSC3_LEVEL")),
              osc2Detune(state.getRawParameterValue("OSC2_DETUNE")),
              osc3Detune(state.getRawParameterValue("OSC3_DETUNE")),
              subLevel(state.getRawParameterValue("SUB_LEVEL")),
              noiseLevel(state.getRawParameterValue("NOISE_LEVEL")),
              filterCutoff(state.getRawParameterValue("FILTER_CUTOFF"))
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

            phase1 = 0.0;
            phase2 = 0.0;
            phase3 = 0.0;
            subPhase = 0.0;
            driftPhase = 0.0;
            filterState = 0.0f;

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
            const int baseWave = juce::roundToInt(waveform->load());

            const float level1 = osc1Level->load();
            const float level2 = osc2Level->load();
            const float level3 = osc3Level->load();
            const float detune2 = osc2Detune->load();
            const float detune3 = osc3Detune->load();
            const float sub = subLevel->load();
            const float noise = noiseLevel->load();

            const float cutoff = juce::jlimit(
                200.0f, 18000.0f, filterCutoff->load());

            const float filterCoefficient =
                1.0f - std::exp(
                    -juce::MathConstants<float>::twoPi
                    * cutoff / static_cast<float>(sampleRate));

            for (int sample = 0; sample < numSamples; ++sample)
            {
                driftPhase += 0.0007 * movement;

                const double modulation =
                    std::sin(driftPhase) * movement * 0.015
                    + std::sin(driftPhase * 0.25)
                    * tideAmount * 0.02;

                const double baseFrequency =
                    frequency * (1.0 + modulation);

                phase1 += baseFrequency / sampleRate;

                phase2 += baseFrequency
                    * std::pow(2.0, detune2 / 1200.0)
                    / sampleRate;

                phase3 += baseFrequency
                    * std::pow(2.0, detune3 / 1200.0)
                    / sampleRate;

                subPhase += (baseFrequency * 0.5) / sampleRate;

                if (phase1 >= 1.0) phase1 -= 1.0;
                if (phase2 >= 1.0) phase2 -= 1.0;
                if (phase3 >= 1.0) phase3 -= 1.0;
                if (subPhase >= 1.0) subPhase -= 1.0;

                const int wave1 = baseWave;
                const int wave2 = (baseWave + 1) % 3;
                const int wave3 = (baseWave + 2) % 3;

                float signal = 0.0f;

                signal += oscillatorSample(phase1, wave1) * level1;
                signal += oscillatorSample(phase2, wave2) * level2;
                signal += oscillatorSample(phase3, wave3) * level3;

                signal += static_cast<float>(
                    std::sin(subPhase
                    * juce::MathConstants<double>::twoPi)) * sub;

                signal += (random.nextFloat() * 2.0f - 1.0f) * noise;

                const float envelope = adsr.getNextSample();

                signal *= envelope * level;

                // Smooth low-pass tone shaping
                filterState += filterCoefficient * (signal - filterState);

                // Brightness blend
                const float output =
                    filterState * (1.0f - tone * 0.35f)
                    + signal * tone * 0.35f;

                for (int channel = 0;
                     channel < buffer.getNumChannels(); ++channel)
                {
                    buffer.addSample(
                        channel, startSample + sample, output * 0.35f);
                }
            }

            if (!adsr.isActive())
                clearCurrentNote();
        }

    private:
        static float oscillatorSample(double phase, int wave)
        {
            if (wave == 1)
                return static_cast<float>(2.0 * phase - 1.0);

            if (wave == 2)
                return phase < 0.5 ? 1.0f : -1.0f;

            return static_cast<float>(
                std::sin(phase * juce::MathConstants<double>::twoPi));
        }

        std::atomic<float>* attack;
        std::atomic<float>* release;
        std::atomic<float>* brightness;
        std::atomic<float>* drift;
        std::atomic<float>* tide;
        std::atomic<float>* waveform;

        std::atomic<float>* osc1Level;
        std::atomic<float>* osc2Level;
        std::atomic<float>* osc3Level;
        std::atomic<float>* osc2Detune;
        std::atomic<float>* osc3Detune;
        std::atomic<float>* subLevel;
        std::atomic<float>* noiseLevel;
        std::atomic<float>* filterCutoff;

        juce::ADSR adsr;
        juce::Random random;

        double phase1 = 0.0;
        double phase2 = 0.0;
        double phase3 = 0.0;
        double subPhase = 0.0;
        double driftPhase = 0.0;
        double frequency = 440.0;

        float level = 0.0f;
        float filterState = 0.0f;
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

    // Three independent oscillator levels
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC1_LEVEL", "Oscillator 1", 0.0f, 1.0f, 0.60f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC2_LEVEL", "Oscillator 2", 0.0f, 1.0f, 0.28f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC3_LEVEL", "Oscillator 3", 0.0f, 1.0f, 0.18f));

    // Oscillator tuning in cents
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC2_DETUNE", "Oscillator 2 Detune",
        -24.0f, 24.0f, 7.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC3_DETUNE", "Oscillator 3 Detune",
        -24.0f, 24.0f, -12.0f));

    // Sub-bass and noise
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SUB_LEVEL", "Sub Bass", 0.0f, 1.0f, 0.30f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "NOISE_LEVEL", "Noise", 0.0f, 1.0f, 0.015f));

    // Tone shaping
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_CUTOFF", "Filter Cutoff",
        200.0f, 18000.0f, 12000.0f));

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
