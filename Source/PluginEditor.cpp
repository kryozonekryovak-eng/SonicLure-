
#include "PluginEditor.h"
#include <cmath>

//==============================================================
// XY MORPH PAD
//==============================================================

XLUREAudioProcessorEditor::XYMorphPad::XYMorphPad(
    XLUREAudioProcessor& processor)
    : audioProcessor(processor)
{
    setTheme(0);
    startTimerHz(30);
}

void XLUREAudioProcessorEditor::XYMorphPad::setTheme(int newTheme)
{
    theme = juce::jlimit(0, 3, newTheme);

    switch (theme)
    {
        case 0: // OCEAN
            backgroundColour = juce::Colour::fromRGB(5, 17, 35);
            liquidColour = juce::Colour::fromRGB(0, 180, 230);
            highlightColour = juce::Colour::fromRGB(100, 235, 255);
            break;

        case 1: // RIVER
            backgroundColour = juce::Colour::fromRGB(5, 28, 28);
            liquidColour = juce::Colour::fromRGB(0, 190, 135);
            highlightColour = juce::Colour::fromRGB(100, 255, 205);
            break;

        case 2: // MUDDY PUDDLE
            backgroundColour = juce::Colour::fromRGB(35, 23, 13);
            liquidColour = juce::Colour::fromRGB(175, 105, 40);
            highlightColour = juce::Colour::fromRGB(255, 195, 100);
            break;

        default: // LILY POND
            backgroundColour = juce::Colour::fromRGB(10, 31, 25);
            liquidColour = juce::Colour::fromRGB(75, 180, 115);
            highlightColour = juce::Colour::fromRGB(170, 255, 185);
            break;
    }

    repaint();
}

void XLUREAudioProcessorEditor::XYMorphPad::paint(
    juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    g.setColour(backgroundColour);
    g.fillRoundedRectangle(area, 22.0f);

    g.setColour(liquidColour.withAlpha(0.13f));
    g.drawRoundedRectangle(area.reduced(2.0f), 22.0f, 2.0f);

    // Subtle XY grid.
    g.setColour(highlightColour.withAlpha(0.10f));

    for (int i = 1; i < 4; ++i)
    {
        const float x = area.getX() + area.getWidth() * i / 4.0f;
        const float y = area.getY() + area.getHeight() * i / 4.0f;

        g.drawVerticalLine(
            juce::roundToInt(x), area.getY() + 15.0f,
            area.getBottom() - 15.0f);

        g.drawHorizontalLine(
            juce::roundToInt(y), area.getX() + 15.0f,
            area.getRight() - 15.0f);
    }

    // Animated liquid shapes.
    const float wobble = std::sin(animationPhase) * 8.0f;
    const float wobbleTwo = std::cos(animationPhase * 0.73f) * 7.0f;

    const float centreX =
        area.getX() + orbX * area.getWidth();

    const float centreY =
        area.getY() + (1.0f - orbY) * area.getHeight();

    const float orbSize = area.getWidth() * 0.24f;

    juce::ColourGradient liquidGradient(
        highlightColour.withAlpha(0.85f),
        centreX - orbSize * 0.35f,
        centreY - orbSize * 0.45f,
        liquidColour.withAlpha(0.12f),
        centreX + orbSize,
        centreY + orbSize,
        true);

    g.setGradientFill(liquidGradient);

    juce::Path liquid;
    liquid.startNewSubPath(
        centreX - orbSize * 0.75f + wobble,
        centreY);

    liquid.cubicTo(
        centreX - orbSize * 0.95f,
        centreY - orbSize * 0.75f + wobbleTwo,
        centreX + orbSize * 0.35f,
        centreY - orbSize * 1.1f,
        centreX + orbSize * 0.75f,
        centreY - orbSize * 0.25f);

    liquid.cubicTo(
        centreX + orbSize * 1.15f,
        centreY + orbSize * 0.5f,
        centreX + orbSize * 0.25f,
        centreY + orbSize * 1.0f + wobble,
        centreX - orbSize * 0.45f,
        centreY + orbSize * 0.65f);

    liquid.cubicTo(
        centreX - orbSize * 1.0f,
        centreY + orbSize * 0.4f,
        centreX - orbSize * 1.1f,
        centreY - orbSize * 0.3f,
        centreX - orbSize * 0.75f + wobble,
        centreY);

    liquid.closeSubPath();
    g.fillPath(liquid);

    // Glowing centre point.
    g.setColour(highlightColour.withAlpha(0.16f));
    g.fillEllipse(
        centreX - 19.0f, centreY - 19.0f,
        38.0f, 38.0f);

    g.setColour(highlightColour);
    g.fillEllipse(
        centreX - 6.0f, centreY - 6.0f,
        12.0f, 12.0f);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.drawEllipse(
        centreX - 12.0f, centreY - 12.0f,
        24.0f, 24.0f, 1.5f);

    // Axis labels.
    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.setFont(juce::Font(13.0f));

    g.drawText(
        "SUBTRACTIVE",
        16, 12, 120, 22,
        juce::Justification::left);

    g.drawText(
        "MODAL",
        getWidth() - 100, 12, 80, 22,
        juce::Justification::right);

    g.drawText(
        "DARK",
        16, getHeight() - 32, 70, 20,
        juce::Justification::left);

    g.drawText(
        "BRIGHT",
        getWidth() - 85, getHeight() - 32,
        70, 20,
        juce::Justification::right);
}

void XLUREAudioProcessorEditor::XYMorphPad::mouseDown(
    const juce::MouseEvent& event)
{
    updatePosition(event);
}

void XLUREAudioProcessorEditor::XYMorphPad::mouseDrag(
    const juce::MouseEvent& event)
{
    updatePosition(event);
}

void XLUREAudioProcessorEditor::XYMorphPad::updatePosition(
    const juce::MouseEvent& event)
{
    const float x = juce::jlimit(
        0.0f, 1.0f,
        static_cast<float>(event.x) /
        static_cast<float>(juce::jmax(1, getWidth())));

    const float y = juce::jlimit(
        0.0f, 1.0f,
        1.0f - static_cast<float>(event.y) /
        static_cast<float>(juce::jmax(1, getHeight())));

    audioProcessor.setMorphPosition(x, y);

    orbX = x;
    orbY = y;

    repaint();
}

void XLUREAudioProcessorEditor::XYMorphPad::timerCallback()
{
    animationPhase += 0.045f;

    if (animationPhase > juce::MathConstants<float>::twoPi)
        animationPhase -= juce::MathConstants<float>::twoPi;

    // Follow saved or externally changed XY positions.
    const float targetX = audioProcessor.getMorphX();
    const float targetY = audioProcessor.getMorphY();

    orbX += (targetX - orbX) * 0.12f;
    orbY += (targetY - orbY) * 0.12f;

    repaint();
}

//==============================================================
// MAIN XLURE EDITOR
//==============================================================

XLUREAudioProcessorEditor::XLUREAudioProcessorEditor(
    XLUREAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor),
      morphPad(processor)
{
    setSize(900, 720);
    setResizable(true, true);
    setResizeLimits(650, 560, 1400, 1100);

    addAndMakeVisible(titleLabel);
    titleLabel.setText("XLURE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(34.0f, juce::Font::bold));
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

    addAndMakeVisible(morphPad);

    addAndMakeVisible(oceanButton);
    addAndMakeVisible(riverButton);
    addAndMakeVisible(muddyButton);
    addAndMakeVisible(lilyButton);

    styleButton(oceanButton);
    styleButton(riverButton);
    styleButton(muddyButton);
    styleButton(lilyButton);

    oceanButton.onClick = [this] { selectTheme(0); };
    riverButton.onClick = [this] { selectTheme(1); };
    muddyButton.onClick = [this] { selectTheme(2); };
    lilyButton.onClick = [this] { selectTheme(3); };

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
    statusLabel.setText(
        "READY",
        juce::dontSendNotification);
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

XLUREAudioProcessorEditor::~XLUREAudioProcessorEditor() = default;

void XLUREAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(7, 11, 21));

    g.setColour(juce::Colour::fromRGB(20, 39, 57));
    g.drawHorizontalLine(76, 20.0f, getWidth() - 20.0f);

    g.setColour(juce::Colour::fromRGB(40, 85, 110).withAlpha(0.5f));
    g.drawRoundedRectangle(
        getLocalBounds().toFloat().reduced(8.0f),
        12.0f, 1.0f);
}

void XLUREAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);

    titleLabel.setBounds(area.removeFromTop(42));
    subtitleLabel.setBounds(area.removeFromTop(28));

    area.removeFromTop(12);

    auto themeArea = area.removeFromTop(38);

    const int themeWidth = themeArea.getWidth() / 4;

    oceanButton.setBounds(themeArea.removeFromLeft(themeWidth).reduced(3));
    riverButton.setBounds(themeArea.removeFromLeft(themeWidth).reduced(3));
    muddyButton.setBounds(themeArea.removeFromLeft(themeWidth).reduced(3));
    lilyButton.setBounds(themeArea.reduced(3));

    area.removeFromTop(12);

    const int bottomHeight = 155;
    auto padArea = area.removeFromTop(
        juce::jmax(180, area.getHeight() - bottomHeight - 10));

    morphPad.setBounds(padArea);

    area.removeFromTop(12);

    auto bottomArea = area;

    auto sampleArea = bottomArea.removeFromTop(28);
    sampleLabel.setBounds(sampleArea.removeFromLeft(
        juce::jmax(150, sampleArea.getWidth() - 220)));

    statusLabel.setBounds(sampleArea);

    bottomArea.removeFromTop(8);

    auto controlsArea = bottomArea;

    auto buttonsArea = controlsArea.removeFromLeft(
        juce::jmin(210, controlsArea.getWidth() / 3));

    loadButton.setBounds(buttonsArea.removeFromTop(38).reduced(2));
    playButton.setBounds(buttonsArea.removeFromTop(38).reduced(2));

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

void XLUREAudioProcessorEditor::styleButton(
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

void XLUREAudioProcessorEditor::setupSlider(
    juce::Slider& slider,
    juce::Label& label,
    const juce::String& labelText)
{
    addAndMakeVisible(slider);
    addAndMakeVisible(label);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(11.0f, juce::Font::bold));
    label.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(115, 210, 235));
    label.setJustificationType(juce::Justification::centred);

    slider.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false, 55, 18);

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

void XLUREAudioProcessorEditor::selectTheme(int themeIndex)
{
    morphPad.setTheme(themeIndex);
    repaint();
}

void XLUREAudioProcessorEditor::loadWavFile()
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
