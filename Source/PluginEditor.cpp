
#include "PluginEditor.h"
#include <cmath>

//==============================================================
// LIQUID DISPLAY
//==============================================================

PURELUREAudioProcessorEditor::LiquidWindow::LiquidWindow()
{
    setTheme(0);
    startTimerHz(30);
}

void PURELUREAudioProcessorEditor::LiquidWindow::setTheme(int newTheme)
{
    theme = juce::jlimit(0, 3, newTheme);

    switch (theme)
    {
        case 0:
            backgroundColour = juce::Colour::fromRGB(5, 17, 35);
            liquidColour = juce::Colour::fromRGB(0, 150, 220);
            highlightColour = juce::Colour::fromRGB(100, 235, 255);
            break;

        case 1:
            backgroundColour = juce::Colour::fromRGB(35, 23, 13);
            liquidColour = juce::Colour::fromRGB(175, 105, 40);
            highlightColour = juce::Colour::fromRGB(255, 195, 100);
            break;

        case 2:
            backgroundColour = juce::Colour::fromRGB(5, 28, 28);
            liquidColour = juce::Colour::fromRGB(0, 190, 135);
            highlightColour = juce::Colour::fromRGB(100, 255, 205);
            break;

        default:
            backgroundColour = juce::Colour::fromRGB(10, 31, 25);
            liquidColour = juce::Colour::fromRGB(75, 180, 115);
            highlightColour = juce::Colour::fromRGB(170, 255, 185);
            break;
    }

    repaint();
}

void PURELUREAudioProcessorEditor::LiquidWindow::timerCallback()
{
    animationPhase += 0.035f;

    if (animationPhase > juce::MathConstants<float>::twoPi)
        animationPhase -= juce::MathConstants<float>::twoPi;

    repaint();
}

void PURELUREAudioProcessorEditor::LiquidWindow::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    const float diameter = juce::jmin(
        bounds.getWidth(), bounds.getHeight()) - 12.0f;

    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();
    const float radius = diameter * 0.5f;

    juce::Rectangle<float> circle(
        centreX - radius,
        centreY - radius,
        diameter,
        diameter);

    g.setColour(liquidColour.withAlpha(0.12f));
    g.fillEllipse(circle.expanded(8.0f));

    g.setColour(backgroundColour);
    g.fillEllipse(circle);

    juce::Path clippingPath;
    clippingPath.addEllipse(circle);

    g.saveState();
    g.reduceClipRegion(clippingPath);

    g.setColour(liquidColour.withAlpha(0.25f));
    g.fillEllipse(circle);

    const float waveOne =
        std::sin(animationPhase) * radius * 0.10f;

    const float waveTwo =
        std::cos(animationPhase * 0.75f) * radius * 0.09f;

    const float waveThree =
        std::sin(animationPhase * 1.3f) * radius * 0.06f;

    juce::ColourGradient gradient(
        highlightColour.withAlpha(0.85f),
        centreX - radius * 0.45f,
        centreY - radius * 0.55f,
        liquidColour.withAlpha(0.55f),
        centreX + radius * 0.55f,
        centreY + radius * 0.65f,
        true);

    g.setGradientFill(gradient);

    juce::Path liquid;

    liquid.startNewSubPath(
        centreX - radius,
        centreY + waveOne);

    liquid.cubicTo(
        centreX - radius * 0.55f,
        centreY - radius * 0.65f + waveTwo,
        centreX + radius * 0.15f,
        centreY - radius * 0.75f,
        centreX + radius,
        centreY - radius * 0.20f + waveThree);

    liquid.lineTo(centreX + radius, centreY + radius);
    liquid.lineTo(centreX - radius, centreY + radius);
    liquid.closeSubPath();

    g.fillPath(liquid);

    g.setColour(highlightColour.withAlpha(0.22f));

    juce::Path highlight;

    highlight.startNewSubPath(
        centreX - radius,
        centreY + radius * 0.15f + waveTwo);

    highlight.cubicTo(
        centreX - radius * 0.4f,
        centreY - radius * 0.05f + waveThree,
        centreX + radius * 0.35f,
        centreY + radius * 0.30f + waveOne,
        centreX + radius,
        centreY + radius * 0.05f);

    g.strokePath(highlight, juce::PathStrokeType(3.0f));

    g.setColour(highlightColour.withAlpha(0.12f));

    g.fillEllipse(
        centreX - radius * 0.30f,
        centreY - radius * 0.30f,
        radius * 0.60f,
        radius * 0.60f);

    g.restoreState();

    g.setColour(highlightColour.withAlpha(0.65f));
    g.drawEllipse(circle, 2.5f);

    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawEllipse(circle.reduced(6.0f), 1.0f);
}

//==============================================================
// PURELURE EDITOR
//==============================================================

PURELUREAudioProcessorEditor::PURELUREAudioProcessorEditor(
    PURELUREAudioProcessor& processor)
    : juce::AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setSize(850, 760);
    setResizable(true, true);
    setResizeLimits(650, 600, 1400, 1100);

    addAndMakeVisible(titleLabel);
    titleLabel.setText("PURELURE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(36.0f, juce::Font::bold));
    titleLabel.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(100, 225, 255));

    addAndMakeVisible(subtitleLabel);
    subtitleLabel.setText(
        "HYBRID SOUND LAB  |  ENTER THE LIQUID",
        juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(12.0f));
    subtitleLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::lightgrey);

    addAndMakeVisible(liquidWindow);

    addAndMakeVisible(oceanButton);
    addAndMakeVisible(muddyButton);
    addAndMakeVisible(riverButton);
    addAndMakeVisible(lilyButton);

    styleButton(oceanButton);
    styleButton(muddyButton);
    styleButton(riverButton);
    styleButton(lilyButton);

    oceanButton.onClick = [this] { selectTheme(0); };
    muddyButton.onClick = [this] { selectTheme(1); };
    riverButton.onClick = [this] { selectTheme(2); };
    lilyButton.onClick = [this] { selectTheme(3); };

    selectTheme(0);

    addAndMakeVisible(loadButton);
    addAndMakeVisible(playButton);

    styleButton(loadButton);
    styleButton(playButton);

    loadButton.onClick = [this] { loadWavFile(); };
    playButton.onClick = [this] { audioProcessor.playPreview(); };

    addAndMakeVisible(sampleLabel);
    sampleLabel.setText(
        "NO WAV SAMPLE LOADED",
        juce::dontSendNotification);
    sampleLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::lightgrey);
    sampleLabel.setFont(juce::Font(13.0f));

    addAndMakeVisible(statusLabel);
    statusLabel.setText("READY", juce::dontSendNotification);
    statusLabel.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(100, 225, 190));
    statusLabel.setFont(juce::Font(12.0f));

    setupSlider(cutoffSlider, cutoffLabel, "FILTER");
    setupSlider(resonanceSlider, resonanceLabel, "RESONANCE");
    setupSlider(masterSlider, masterLabel, "MASTER");

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
}

PURELUREAudioProcessorEditor::~PURELUREAudioProcessorEditor() = default;

//==============================================================
// BACKGROUND
//==============================================================

void PURELUREAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(7, 11, 21));

    g.setColour(juce::Colour::fromRGB(20, 39, 57));
    g.drawHorizontalLine(82, 20.0f, getWidth() - 20.0f);

    g.setColour(
        juce::Colour::fromRGB(40, 85, 110).withAlpha(0.5f));

    g.drawRoundedRectangle(
        getLocalBounds().toFloat().reduced(8.0f),
        14.0f,
        1.0f);
}

//==============================================================
// LAYOUT
//==============================================================

void PURELUREAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);

    titleLabel.setBounds(area.removeFromTop(44));
    subtitleLabel.setBounds(area.removeFromTop(28));

    area.removeFromTop(12);

    auto themeArea = area.removeFromTop(54);
    const int halfWidth = themeArea.getWidth() / 2;

    auto topLeft = themeArea.removeFromTop(27);
    auto bottomLeft = themeArea.removeFromTop(27);

    oceanButton.setBounds(
        topLeft.removeFromLeft(halfWidth).reduced(3));

    muddyButton.setBounds(topLeft.reduced(3));

    riverButton.setBounds(
        bottomLeft.removeFromLeft(halfWidth).reduced(3));

    lilyButton.setBounds(bottomLeft.reduced(3));

    area.removeFromTop(12);

    auto displayArea = area.removeFromTop(
        juce::jmax(230, area.getHeight() - 205));

    liquidWindow.setBounds(displayArea);

    area.removeFromTop(10);

    auto sampleArea = area.removeFromTop(26);

    sampleLabel.setBounds(
        sampleArea.removeFromLeft(
            juce::jmax(150, sampleArea.getWidth() - 130)));

    statusLabel.setBounds(sampleArea);

    area.removeFromTop(8);

    auto controlsArea = area;

    auto buttonsArea = controlsArea.removeFromLeft(
        juce::jmin(190, controlsArea.getWidth() / 3));

    loadButton.setBounds(
        buttonsArea.removeFromTop(38).reduced(2));

    playButton.setBounds(
        buttonsArea.removeFromTop(38).reduced(2));

    controlsArea.removeFromLeft(10);

    const int controlWidth = controlsArea.getWidth() / 3;

    auto cutoffArea = controlsArea.removeFromLeft(controlWidth);
    auto resonanceArea = controlsArea.removeFromLeft(controlWidth);
    auto masterArea = controlsArea;

    cutoffLabel.setBounds(cutoffArea.removeFromTop(20));
    cutoffSlider.setBounds(cutoffArea);

    resonanceLabel.setBounds(resonanceArea.removeFromTop(20));
    resonanceSlider.setBounds(resonanceArea);

    masterLabel.setBounds(masterArea.removeFromTop(20));
    masterSlider.setBounds(masterArea);
}

//==============================================================
// BUTTON STYLING
//==============================================================

void PURELUREAudioProcessorEditor::styleButton(
    juce::TextButton& button)
{
    button.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour::fromRGB(18, 37, 54));

    button.setColour(
        juce::TextButton::buttonOnColourId,
        juce::Colour::fromRGB(0, 110, 145));

    button.setColour(
        juce::TextButton::textColourOffId,
        juce::Colour::fromRGB(190, 225, 240));

    button.setColour(
        juce::TextButton::textColourOnId,
        juce::Colours::white);
}

//==============================================================
// SLIDER SETUP
//==============================================================

void PURELUREAudioProcessorEditor::setupSlider(
    juce::Slider& slider,
    juce::Label& label,
    const juce::String& name)
{
    addAndMakeVisible(slider);
    addAndMakeVisible(label);

    label.setText(name, juce::dontSendNotification);
    label.setFont(juce::Font(11.0f, juce::Font::bold));
    label.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(115, 210, 235));

    label.setJustificationType(juce::Justification::centred);

    slider.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        55,
        18);

    slider.setRange(0.0, 1.0, 0.001);

    slider.setColour(
        juce::Slider::rotarySliderFillColourId,
        juce::Colour::fromRGB(0, 180, 220));

    slider.setColour(
        juce::Slider::rotarySliderOutlineColourId,
        juce::Colour::fromRGB(35, 65, 85));

    slider.setColour(
        juce::Slider::thumbColourId,
        juce::Colour::fromRGB(120, 235, 255));

    slider.setColour(
        juce::Slider::textBoxTextColourId,
        juce::Colours::white);

    slider.setColour(
        juce::Slider::textBoxOutlineColourId,
        juce::Colours::transparentBlack);
}

//==============================================================
// THEME SELECTION
//==============================================================

void PURELUREAudioProcessorEditor::selectTheme(int themeIndex)
{
    liquidWindow.setTheme(themeIndex);

    oceanButton.setToggleState(
        themeIndex == 0, juce::dontSendNotification);

    muddyButton.setToggleState(
        themeIndex == 1, juce::dontSendNotification);

    riverButton.setToggleState(
        themeIndex == 2, juce::dontSendNotification);

    lilyButton.setToggleState(
        themeIndex == 3, juce::dontSendNotification);
}

//==============================================================
// WAV LOADING
//==============================================================

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
                        "COULD NOT LOAD WAV",
                        juce::dontSendNotification);
                }
            }
        });
}
