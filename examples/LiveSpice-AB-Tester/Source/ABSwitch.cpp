/*
  ==============================================================================
    A/B Switch Implementation
  ==============================================================================
*/

#include "ABSwitch.h"

ABSwitch::ABSwitch()
{
}

ABSwitch::~ABSwitch()
{
}

void ABSwitch::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    g.setColour(juce::Colours::grey);
    g.drawRect(bounds, 2);

    // Calculate areas
    auto area = bounds.reduced(10);
    auto labelHeight = 25;
    
    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    auto titleArea = area.removeFromTop(labelHeight);
    g.drawText("A/B Switch", titleArea, juce::Justification::centred);

    area.removeFromTop(5);

    // Switch area (toggle button)
    switchArea = area.removeFromTop(30).reduced(area.getWidth() / 3, 0);
    
    // Draw switch background
    g.setColour(juce::Colour(0xff333333));
    g.fillRoundedRectangle(switchArea.toFloat(), 15.0f);

    // Draw active side
    auto activeArea = usePluginA ? 
        switchArea.removeFromLeft(switchArea.getWidth() / 2) :
        switchArea.removeFromRight(switchArea.getWidth() / 2);
    
    g.setColour(usePluginA ? juce::Colours::lightblue : juce::Colours::lightcoral);
    g.fillRoundedRectangle(activeArea.toFloat(), 15.0f);

    // Draw labels on switch
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    
    auto leftLabel = switchArea.removeFromLeft(switchArea.getWidth() / 2);
    auto rightLabel = switchArea;
    
    g.drawText("A", leftLabel, juce::Justification::centred);
    g.drawText("B", rightLabel, juce::Justification::centred);

    // Plugin labels below
    area.removeFromTop(10);
    auto labelsArea = area.removeFromTop(labelHeight);
    
    labelAArea = labelsArea.removeFromLeft(labelsArea.getWidth() / 2);
    labelBArea = labelsArea;

    g.setFont(juce::Font(12.0f));
    g.setColour(usePluginA ? juce::Colours::lightblue : juce::Colours::grey);
    g.drawText("LiveSpice VST", labelAArea, juce::Justification::centred);
    
    g.setColour(!usePluginA ? juce::Colours::lightcoral : juce::Colours::grey);
    g.drawText("Generated Plugin", labelBArea, juce::Justification::centred);
}

void ABSwitch::resized()
{
    repaint();
}

void ABSwitch::mouseDown(const juce::MouseEvent& event)
{
    if (switchArea.contains(event.getPosition()))
    {
        usePluginA = !usePluginA;
        repaint();

        if (onSwitch)
            onSwitch(usePluginA);
    }
}
