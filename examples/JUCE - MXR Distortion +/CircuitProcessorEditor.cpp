/*
  ==============================================================================
    Custom GUI Editor for MXR Distortion+ Plugin
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

    // Configure Debug Button
    debugButton.setButtonText ("Debug Mode");
    debugButton.setClickingTogglesState (true);
    debugButton.onClick = [this]() { 
        debugMode = debugButton.getToggleState(); 
        debugInfoLabel.setVisible(debugMode);
        driveResistanceLabel.setVisible(debugMode);
        levelResistanceLabel.setVisible(debugMode);
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

    // Set window size
    setSize (400, debugMode ? 450 : 300);

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
    g.drawText ("MXR Distortion+", getLocalBounds().removeFromTop(40), juce::Justification::centred);

    // Draw border around debug section
    if (debugMode)
    {
        g.setColour (juce::Colours::orange.withAlpha (0.3f));
        g.drawRect (20, 220, getWidth() - 40, 200, 2);
    }
}

void CircuitProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (50); // Skip title area

    // Control layout
    auto controlArea = bounds.removeFromTop (180);
    auto driveArea = controlArea.removeFromLeft (getWidth() / 2).reduced (20);
    auto levelArea = controlArea.reduced (20);

    driveSlider.setBounds (driveArea);
    levelSlider.setBounds (levelArea);

    // Debug button
    debugButton.setBounds (bounds.removeFromTop(40).reduced(20, 5));

    if (debugMode)
    {
        bounds.removeFromTop (10);
        debugInfoLabel.setBounds (bounds.removeFromTop(30).reduced(20, 0));
        bounds.removeFromTop (10);
        
        driveResistanceLabel.setBounds (bounds.removeFromTop(30).reduced(30, 0));
        levelResistanceLabel.setBounds (bounds.removeFromTop(30).reduced(30, 0));
        
        setSize (400, 450);
    }
    else
    {
        setSize (400, 300);
    }
}

void CircuitProcessorEditor::timerCallback()
{
    if (debugMode)
    {
        // Get current parameter values (0.0 to 1.0)
        float driveValue = apvts.getRawParameterValue("drive")->load();
        float levelValue = apvts.getRawParameterValue("level")->load();

        // Calculate actual resistance values
        float driveResistance = calculateDriveResistance(driveValue);
        float levelResistance = calculateLevelResistance(levelValue);

        // Update labels with resistance values
        driveResistanceLabel.setText (
            juce::String::formatted ("Drive Pot: %.2f kΩ (%.1f%%)", 
                driveResistance / 1000.0f, 
                driveValue * 100.0f),
            juce::dontSendNotification);

        levelResistanceLabel.setText (
            juce::String::formatted ("Level Pot: %.2f kΩ (%.1f%%)", 
                levelResistance / 1000.0f, 
                levelValue * 100.0f),
            juce::dontSendNotification);
    }
}

float CircuitProcessorEditor::calculateDriveResistance(float normalizedValue)
{
    // MXR Distortion+ Drive control: 500kΩ potentiometer
    // Linear taper: 0Ω to 500kΩ
    const float MAX_DRIVE_RESISTANCE = 500000.0f; // 500kΩ
    return normalizedValue * MAX_DRIVE_RESISTANCE;
}

float CircuitProcessorEditor::calculateLevelResistance(float normalizedValue)
{
    // MXR Distortion+ Level control: 100kΩ potentiometer
    // Linear taper: 0Ω to 100kΩ
    const float MAX_LEVEL_RESISTANCE = 100000.0f; // 100kΩ
    return normalizedValue * MAX_LEVEL_RESISTANCE;
}
