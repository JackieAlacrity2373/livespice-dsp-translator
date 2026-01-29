/*
  ==============================================================================
    Custom GUI Editor for Boss Super Overdrive SD-1 Plugin
    With Debug Mode showing Potentiometer Resistance Values
  ==============================================================================
*/

#include "CircuitProcessorEditor.h"

CircuitProcessorEditor::CircuitProcessorEditor (CircuitProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), apvts (vts)
{
    // Configure Drive Slider
    driveSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (driveSlider);
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "drive", driveSlider);

    driveLabel.setText ("Drive", juce::dontSendNotification);
    driveLabel.setJustificationType (juce::Justification::centred);
    driveLabel.attachToComponent (&driveSlider, false);
    addAndMakeVisible (driveLabel);

    // Configure Level Slider
    levelSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    levelSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (levelSlider);
    levelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "level", levelSlider);

    levelLabel.setText ("Level", juce::dontSendNotification);
    levelLabel.setJustificationType (juce::Justification::centred);
    levelLabel.attachToComponent (&levelSlider, false);
    addAndMakeVisible (levelLabel);

    // Configure Tone Slider
    toneSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    toneSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (toneSlider);
    toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tone", toneSlider);

    toneLabel.setText ("Tone", juce::dontSendNotification);
    toneLabel.setJustificationType (juce::Justification::centred);
    toneLabel.attachToComponent (&toneSlider, false);
    addAndMakeVisible (toneLabel);

    // Configure Debug Button
    debugButton.setButtonText ("Debug Mode");
    debugButton.setClickingTogglesState (true);
    debugButton.onClick = [this]() { 
        debugMode = debugButton.getToggleState(); 
        debugInfoLabel.setVisible(debugMode);
        driveResistanceLabel.setVisible(debugMode);
        levelResistanceLabel.setVisible(debugMode);
        toneResistanceLabel.setVisible(debugMode);
        resized();
    };
    addAndMakeVisible (debugButton);

    // Configure Debug Info Labels
    debugInfoLabel.setText ("=== Debug Info ===", juce::dontSendNotification);
    debugInfoLabel.setJustificationType (juce::Justification::centred);
    debugInfoLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    debugInfoLabel.setVisible (false);
    addAndMakeVisible (debugInfoLabel);

    driveResistanceLabel.setJustificationType (juce::Justification::centredLeft);
    driveResistanceLabel.setFont (juce::Font (14.0f, juce::Font::plain));
    driveResistanceLabel.setVisible (false);
    addAndMakeVisible (driveResistanceLabel);

    levelResistanceLabel.setJustificationType (juce::Justification::centredLeft);
    levelResistanceLabel.setFont (juce::Font (14.0f, juce::Font::plain));
    levelResistanceLabel.setVisible (false);
    addAndMakeVisible (levelResistanceLabel);

    toneResistanceLabel.setJustificationType (juce::Justification::centredLeft);
    toneResistanceLabel.setFont (juce::Font (14.0f, juce::Font::plain));
    toneResistanceLabel.setVisible (false);
    addAndMakeVisible (toneResistanceLabel);

    // Set window size
    setSize (500, debugMode ? 500 : 320);

    // Start timer for updating debug info (30 Hz refresh)
    startTimerHz (30);
}

CircuitProcessorEditor::~CircuitProcessorEditor()
{
    stopTimer();
}

void CircuitProcessorEditor::paint (juce::Graphics& g)
{
    // Background gradient
    g.fillAll (juce::Colours::darkgrey);
    
    auto bounds = getLocalBounds();
    g.setGradientFill (juce::ColourGradient (
        juce::Colour (0xff2a2a2a), bounds.getTopLeft().toFloat(),
        juce::Colour (0xff1a1a1a), bounds.getBottomRight().toFloat(),
        false));
    g.fillRect (bounds);

    // Title
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (24.0f, juce::Font::bold));
    g.drawText ("Boss Super Overdrive SD-1", getLocalBounds().removeFromTop(40), juce::Justification::centred);

    // Draw border around debug section
    if (debugMode)
    {
        g.setColour (juce::Colours::orange.withAlpha (0.3f));
        g.drawRect (20, 260, getWidth() - 40, 220, 2);
    }
}

void CircuitProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (50); // Skip title area

    // Control layout - 3 knobs in a row
    auto controlArea = bounds.removeFromTop (200);
    auto driveArea = controlArea.removeFromLeft (getWidth() / 3).reduced (20);
    auto levelArea = controlArea.removeFromLeft (getWidth() / 3).reduced (20);
    auto toneArea = controlArea.reduced (20);

    driveSlider.setBounds (driveArea);
    levelSlider.setBounds (levelArea);
    toneSlider.setBounds (toneArea);

    // Debug button
    debugButton.setBounds (bounds.removeFromTop(40).reduced(20, 5));

    if (debugMode)
    {
        bounds.removeFromTop (10);
        debugInfoLabel.setBounds (bounds.removeFromTop(30).reduced(20, 0));
        bounds.removeFromTop (10);
        
        driveResistanceLabel.setBounds (bounds.removeFromTop(25).reduced(30, 0));
        levelResistanceLabel.setBounds (bounds.removeFromTop(25).reduced(30, 0));
        toneResistanceLabel.setBounds (bounds.removeFromTop(25).reduced(30, 0));
        
        setSize (500, 500);
    }
    else
    {
        setSize (500, 320);
    }
}

void CircuitProcessorEditor::timerCallback()
{
    if (debugMode)
    {
        // Get current parameter values (0.0 to 1.0)
        float driveValue = apvts.getRawParameterValue("drive")->load();
        float levelValue = apvts.getRawParameterValue("level")->load();
        float toneValue = apvts.getRawParameterValue("tone")->load();

        // Calculate actual resistance values
        float driveResistance = calculateDriveResistance(driveValue);
        float levelResistance = calculateLevelResistance(levelValue);
        float toneResistance = calculateToneResistance(toneValue);

        // Update labels with resistance values
        driveResistanceLabel.setText (
            juce::String::formatted ("Drive: %.2f kΩ (%.1f%%)", 
                driveResistance / 1000.0f, 
                driveValue * 100.0f),
            juce::dontSendNotification);

        levelResistanceLabel.setText (
            juce::String::formatted ("Level: %.2f kΩ (%.1f%%)", 
                levelResistance / 1000.0f, 
                levelValue * 100.0f),
            juce::dontSendNotification);

        toneResistanceLabel.setText (
            juce::String::formatted ("Tone: %.2f kΩ (%.1f%%)", 
                toneResistance / 1000.0f, 
                toneValue * 100.0f),
            juce::dontSendNotification);
    }
}

float CircuitProcessorEditor::calculateDriveResistance(float normalizedValue)
{
    // Boss SD-1 Drive control: 1MΩ potentiometer
    // Linear taper: 0Ω to 1MΩ
    const float MAX_DRIVE_RESISTANCE = 1000000.0f; // 1MΩ
    return normalizedValue * MAX_DRIVE_RESISTANCE;
}

float CircuitProcessorEditor::calculateLevelResistance(float normalizedValue)
{
    // Boss SD-1 Level control: 10kΩ potentiometer
    // Linear taper: 0Ω to 10kΩ
    const float MAX_LEVEL_RESISTANCE = 10000.0f; // 10kΩ
    return normalizedValue * MAX_LEVEL_RESISTANCE;
}

float CircuitProcessorEditor::calculateToneResistance(float normalizedValue)
{
    // Boss SD-1 Tone control: 22kΩ potentiometer
    // Linear taper: 0Ω to 22kΩ
    const float MAX_TONE_RESISTANCE = 22000.0f; // 22kΩ
    return normalizedValue * MAX_TONE_RESISTANCE;
}
