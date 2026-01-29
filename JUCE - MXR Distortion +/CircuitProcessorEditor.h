/*
  ==============================================================================
    Custom GUI Editor for MXR Distortion+ Plugin
    With Debug Mode showing Potentiometer Resistance Values
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "CircuitProcessor.h"

class CircuitProcessorEditor : public juce::AudioProcessorEditor,
                                private juce::Timer
{
public:
    CircuitProcessorEditor (CircuitProcessor&, juce::AudioProcessorValueTreeState&);
    ~CircuitProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    
    // Calculate resistance from normalized 0-1 value
    float calculateDriveResistance(float normalizedValue);
    float calculateLevelResistance(float normalizedValue);

    CircuitProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& apvts;

    // Sliders and attachments
    juce::Slider driveSlider;
    juce::Slider levelSlider;
    juce::Label driveLabel;
    juce::Label levelLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment;

    // Debug mode
    juce::TextButton debugButton;
    bool debugMode = false;
    
    // Debug display labels
    juce::Label debugInfoLabel;
    juce::Label driveResistanceLabel;
    juce::Label levelResistanceLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircuitProcessorEditor)
};
