/*
  ==============================================================================
    A/B Switch - Toggle between Plugin A and Plugin B
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ABSwitch : public juce::Component
{
public:
    ABSwitch();
    ~ABSwitch() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    std::function<void(bool)> onSwitch;

    bool isUsingA() const { return usePluginA; }

private:
    bool usePluginA{true};
    
    juce::Rectangle<int> switchArea;
    juce::Rectangle<int> labelAArea;
    juce::Rectangle<int> labelBArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ABSwitch)
};
