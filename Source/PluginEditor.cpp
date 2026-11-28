#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
}

PURELUREAudioProcessorEditor::LiquidWindow::LiquidWindow()
{
    setOpaque(true);
    startTimerHz(30);
    setTheme(0);
}

void PURELUREAudioProcessorEditor::LiquidWindow::setTheme(int newTheme)
{
    theme = juce::jlimit(0, 3, newTheme);

    switch (theme)
    {
        case 0:
            backgroundColour = juce::Colour(0xff071c2b);
            liquidColour = juce::Colour(0xff087eaa);
            highlightColour = juce::Colour(0xff54e6ff);
            break;

        case 1:
            backgroundColour = juce::Colour(0xff211b16);
            liquidColour = juce::Colour(0xff76603b);
            highlightColour = juce::Colour(0xffd5bd82);
            break;

        case 2:
            backgroundColour = juce::Colour(0xff071e1b);
            liquidColour = juce::Colour(0xff087c65);
            highlightColour = juce::Colour(0xff61f5c2);
            break;

        default:
            backgroundColour = juce::Colour(0xff14201d);
            liquidColour = juce::Colour(0xff416b54);
            highlightColour = juce::Colour(0xffb4f4b0);
            break;
    }

    repaint();
}

void PURELUREAudioProcessorEditor::LiquidWindow::timerCallback()
{
    animationPhase += 0.025f;

    if (animationPhase > twoPi)
        animationPhase -= twoPi;

    repaint();
}

void PURELUREAudioProcessorEditor::LiquidWindow::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll(backgroundColour);

    juce::ColourGradient background(
        backgroundColour.brighter(0.12f),
        bounds.getTopLeft(),
        backgroundColour.darker(0.5f),
        bounds.getBottomRight(),
        false);

    g.setGradientFill(background);
    g.fillRect(bounds);

    juce::Path liquid;

    const float baseY = bounds.getHeight() * 0.78f;
    const float width = bounds.getWidth();

    liquid.startNewSubPath(0.0f, baseY);

    for (int x = 0; x <= static_cast<int>(width); x += 4)
    {
        const float xf = static_cast<float>(x);

        const float wave =
            std::sin(xf * 0.012f + animationPhase) * 12.0f
            + std::sin(xf * 0.021f - animationPhase * 0.7f) * 7.0f;

        liquid.lineTo(xf, baseY + wave);
    }

    liquid.lineTo(bounds.getRight(), bounds.getBottom());
    liquid.lineTo(bounds.getLeft(), bounds.getBottom());
    liquid.closeSubPath();

    g.setColour(liquidColour.withAlpha(0.22f));
    g.fillPath(liquid);

    g.setColour(highlightColour.withAlpha(0.45f));
    g.strokePath(liquid, juce::PathStrokeType(1.5f));
}

PURELUREAudioProcessorEditor::PURELUREAudioProcessorEditor(
    PURELUREAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setSize(900, 620);

    addAndMakeVisible(liquidWindow);
    liquidWindow.toBack();

    titleLabel.setText("PURELURE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(34.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText(
        "LIQUID SOUND DESIGN SYNTHESIS",
        juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(12.0f));
    subtitleLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::lightblue);
    subtitleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(subtitleLabel);

    sampleLabel.setText("NO SAMPLE LOADED", juce::dontSendNotification);
    sampleLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::white);
    sampleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(sampleLabel);

    statusLabel.setText("READY", juce::dontSendNotification);
    statusLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::lightgreen);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    styleButton(oceanButton);
    styleButton(muddyButton);
    styleButton(riverButton);
    styleButton(lilyButton);
    styleButton(loadButton);
    styleButton(playButton);

    addAndMakeVisible(oceanButton);
    addAndMakeVisible(muddyButton);
    addAndMakeVisible(riverButton);
    addAndMakeVisible(lilyButton);
    addAndMakeVisible(loadButton);
    addAndMakeVisible(playButton);

    oceanButton.onClick = [this] { selectTheme(0); };
    muddyButton.onClick = [this] { selectTheme(1); };
    riverButton.onClick = [this] { selectTheme(2); };
    lilyButton.onClick = [this] { selectTheme(3); };

    loadButton.onClick = [this] { loadWavFile(); };

    playButton.onClick = [this]
    {
        audioProcessor.playPreview();
        statusLabel.setText("PREVIEW PLAYING", juce::dontSendNotification);
    };

    setupSlider(cutoffSlider, cutoffLabel, "FILTER CUTOFF");
    setupSlider(resonanceSlider, resonanceLabel, "RESONANCE");
    setupSlider(masterSlider, masterLabel, "MASTER LEVEL");

    cutoffSlider.setValue(0.65);
    resonanceSlider.setValue(0.15);
    masterSlider.setValue(0.75);

    cutoffSlider.onValueChange = [this]
    {
        audioProcessor.setFilterCutoff(
            static_cast<float>(cutoffSlider.getValue()));
    };

    resonanceSlider.onValueChange = [this]
    {
        audioProcessor.setFilterResonance(
            static_cast<float>(resonanceSlider.getValue()));
    };

    masterSlider.onValueChange = [this]
    {
        audioProcessor.setMasterLevel(
            static_cast<float>(masterSlider.getValue()));
    };

    selectTheme(0);
}

PURELUREAudioProcessorEditor::~PURELUREAudioProcessorEditor() = default;

void PURELUREAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(
        getLocalBounds().toFloat().reduced(8.0f),
        16.0f,
        1.5f);
}

void PURELUREAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);

    liquidWindow.setBounds(getLocalBounds());

    titleLabel.setBounds(area.removeFromTop(48));
    subtitleLabel.setBounds(area.removeFromTop(26));

    area.removeFromTop(18);

    auto themeArea = area.removeFromTop(42);
    const int themeWidth = themeArea.getWidth() / 4;

    oceanButton.setBounds(themeArea.removeFromLeft(themeWidth).reduced(4));
    muddyButton.setBounds(themeArea.removeFromLeft(themeWidth).reduced(4));
    riverButton.setBounds(themeArea.removeFromLeft(themeWidth).reduced(4));
    lilyButton.setBounds(themeArea.reduced(4));

    area.removeFromTop(28);

    sampleLabel.setBounds(area.removeFromTop(30));
    statusLabel.setBounds(area.removeFromTop(24));

    area.removeFromTop(10);

    auto sampleButtons = area.removeFromTop(44);

    loadButton.setBounds(
        sampleButtons.removeFromLeft(sampleButtons.getWidth() / 2).reduced(5));
    playButton.setBounds(sampleButtons.reduced(5));

    area.removeFromTop(30);

    auto controls = area.removeFromTop(210);
    const int controlWidth = controls.getWidth() / 3;

    cutoffLabel.setBounds(
        controls.getX(), controls.getY(), controlWidth, 24);

    cutoffSlider.setBounds(
        controls.getX(), controls.getY() + 24, controlWidth, 170);

    resonanceLabel.setBounds(
        controls.getX() + controlWidth, controls.getY(), controlWidth, 24);

    resonanceSlider.setBounds(
        controls.getX() + controlWidth, controls.getY() + 24,
        controlWidth, 170);

    masterLabel.setBounds(
        controls.getX() + controlWidth * 2, controls.getY(), controlWidth, 24);

    masterSlider.setBounds(
        controls.getX() + controlWidth * 2, controls.getY() + 24,
        controlWidth, 170);
}

void PURELUREAudioProcessorEditor::styleButton(juce::TextButton& button)
{
    button.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour(0xff123c50));

    button.setColour(
        juce::TextButton::buttonOnColourId,
        juce::Colour(0xff087eaa));

    button.setColour(
        juce::TextButton::textColourOffId,
        juce::Colours::white);

    button.setColour(
        juce::TextButton::textColourOnId,
        juce::Colours::white);

    button.setConnectedEdges(0);
}

void PURELUREAudioProcessorEditor::setupSlider(
    juce::Slider& slider,
    juce::Label& label,
    const juce::String& name)
{
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setFont(juce::Font(12.0f, juce::Font::bold));
    addAndMakeVisible(label);

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 22);
    slider.setRange(0.0, 1.0, 0.001);

    slider.setColour(
        juce::Slider::rotarySliderFillColourId,
        juce::Colour(0xff22c8df));

    slider.setColour(
        juce::Slider::rotarySliderOutlineColourId,
        juce::Colour(0xff31566b));

    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    slider.setColour(
        juce::Slider::textBoxTextColourId,
        juce::Colours::white);

    slider.setColour(
        juce::Slider::textBoxBackgroundColourId,
        juce::Colour(0xff102b3b));

    addAndMakeVisible(slider);
}

void PURELUREAudioProcessorEditor::selectTheme(int themeIndex)
{
    liquidWindow.setTheme(themeIndex);

    oceanButton.setToggleState(themeIndex == 0, juce::dontSendNotification);
    muddyButton.setToggleState(themeIndex == 1, juce::dontSendNotification);
    riverButton.setToggleState(themeIndex == 2, juce::dontSendNotification);
    lilyButton.setToggleState(themeIndex == 3, juce::dontSendNotification);

    repaint();
}

void PURELUREAudioProcessorEditor::loadWavFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose a WAV sample",
        juce::File{},
        "*.wav");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();

            if (file.existsAsFile())
            {
                if (audioProcessor.loadSampleFile(file))
                {
                    sampleLabel.setText(
                        audioProcessor.getLoadedSampleName(),
                        juce::dontSendNotification);

                    statusLabel.setText(
                        "SAMPLE LOADED",
                        juce::dontSendNotification);
                }
                else
                {
                    statusLabel.setText(
                        "SAMPLE LOAD FAILED",
                        juce::dontSendNotification);
                }
            }
        });
}
