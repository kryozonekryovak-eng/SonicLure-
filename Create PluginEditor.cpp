
#include "PluginEditor.h"

SonicLureAudioProcessorEditor::SonicLureAudioProcessorEditor(
    SonicLureAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setSize(760, 540);

    title.setText("SONICLURE", juce::dontSendNotification);
    title.setFont(juce::Font(32.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId,
                    juce::Colour(0xff69e6e0));
    title.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(title);

    subtitle.setText("KRYOZONE AUDIO  |  SOUND FROM THE DEPTHS",
                     juce::dontSendNotification);
    subtitle.setFont(juce::Font(12.0f));
    subtitle.setColour(juce::Label::textColourId,
                       juce::Colour(0xffa6c5d1));
    subtitle.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(subtitle);

    setupSlider(attack, attackLabel, "ATTACK");
    setupSlider(release, releaseLabel, "RELEASE");
    setupSlider(brightness, brightnessLabel, "BRIGHTNESS");
    setupSlider(drift, driftLabel, "DRIFT");
    setupSlider(tide, tideLabel, "TIDE");
    setupSlider(output, outputLabel, "OUTPUT");

    waveformLabel.setText("WAVEFORM", juce::dontSendNotification);
    waveformLabel.setColour(juce::Label::textColourId,
                            juce::Colour(0xff69e6e0));
    waveformLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(waveformLabel);

    waveform.addItem("Sine", 1);
    waveform.addItem("Saw", 2);
    waveform.addItem("Square", 3);
    waveform.setColour(juce::ComboBox::backgroundColourId,
                       juce::Colour(0xff142b3b));
    waveform.setColour(juce::ComboBox::textColourId,
                       juce::Colours::white);
    waveform.setColour(juce::ComboBox::outlineColourId,
                       juce::Colour(0xff69e6e0));
    addAndMakeVisible(waveform);

    auto& parameters = audioProcessor.parameters;

    attackAttachment = std::make_unique<SliderAttachment>(
        parameters, "ATTACK", attack);
    releaseAttachment = std::make_unique<SliderAttachment>(
        parameters, "RELEASE", release);
    brightnessAttachment = std::make_unique<SliderAttachment>(
        parameters, "BRIGHTNESS", brightness);
    driftAttachment = std::make_unique<SliderAttachment>(
        parameters, "DRIFT", drift);
    tideAttachment = std::make_unique<SliderAttachment>(
        parameters, "TIDE", tide);
    outputAttachment = std::make_unique<SliderAttachment>(
        parameters, "OUTPUT", output);

    waveformAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            parameters, "WAVEFORM", waveform);
}

SonicLureAudioProcessorEditor::~SonicLureAudioProcessorEditor() {}

void SonicLureAudioProcessorEditor::setupSlider(
    juce::Slider& slider,
    juce::Label& label,
    const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 75, 22);

    slider.setColour(juce::Slider::rotarySliderFillColourId,
                     juce::Colour(0xff32c9c2));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId,
                     juce::Colour(0xff29485a));
    slider.setColour(juce::Slider::thumbColourId,
                     juce::Colour(0xffe3ffff));
    slider.setColour(juce::Slider::textBoxTextColourId,
                     juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId,
                     juce::Colour(0xff142b3b));

    label.setText(name, juce::dontSendNotification);
    label.setFont(juce::Font(13.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId,
                    juce::Colour(0xff69e6e0));
    label.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

void SonicLureAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff071521));

    juce::ColourGradient ocean(
        juce::Colour(0xff102e43), 0.0f, 0.0f,
        juce::Colour(0xff071521), 0.0f,
        static_cast<float>(getHeight()), false);

    g.setGradientFill(ocean);
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colour(0xff1b5364));
    g.drawRoundedRectangle(
        getLocalBounds().toFloat().reduced(12.0f),
        14.0f, 1.5f);

    g.setColour(juce::Colour(0x2269e6e0));

    for (int i = 0; i < 8; ++i)
    {
        const float y = 150.0f + i * 48.0f;
        g.drawHorizontalLine(
            static_cast<int>(y), 24.0f,
            static_cast<float>(getWidth() - 24));
    }

    g.setColour(juce::Colour(0xff69e6e0));
    g.setFont(11.0f);
    g.drawFittedText(
        "EBB  <----  TIDE  ---->  FLOW",
        20, getHeight() - 36, getWidth() - 40, 20,
        juce::Justification::centred, 1);
}

void SonicLureAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);

    title.setBounds(area.removeFromTop(48));
    subtitle.setBounds(area.removeFromTop(26));
    area.removeFromTop(20);

    const int columnWidth = area.getWidth() / 3;
    const int rowHeight = 155;

    juce::Slider* sliders[] = {
        &attack, &release, &brightness,
        &drift, &tide, &output
    };

    juce::Label* labels[] = {
        &attackLabel, &releaseLabel, &brightnessLabel,
        &driftLabel, &tideLabel, &outputLabel
    };

    for (int i = 0; i < 6; ++i)
    {
        const int column = i % 3;
        const int row = i / 3;

        const int x = area.getX() + column * columnWidth;
        const int y = area.getY() + row * rowHeight;

        labels[i]->setBounds(x, y, columnWidth, 24);
        sliders[i]->setBounds(
            x + 12, y + 25, columnWidth - 24, 125);
    }

    const int bottomY = area.getY() + rowHeight * 2 + 4;

    waveformLabel.setBounds(
        area.getX(), bottomY, columnWidth, 24);

    waveform.setBounds(
        area.getX() + columnWidth, bottomY,
        columnWidth, 30);
}
