/*
  ==============================================================================
    Control Panel - Unified parameter controls for both plugins
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginHost.h"

class ControlPanel : public juce::Component
{
public:
    ControlPanel();
    ~ControlPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Parameter management
    void setParameterList(const std::vector<ParameterInfo>& params);
    std::map<juce::String, float> getAllParameters() const;
    void clearParameters();

private:
    struct ParameterControl
    {
        juce::String id;
        juce::String name;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::Label> valueLabel;
        float currentValue{0.0f};
    };

    std::vector<std::unique_ptr<ParameterControl>> controls;
    juce::Viewport viewport;
    juce::Component controlsContainer;

    void createControlForParameter(const juce::String& id,
                                   const juce::String& name,
                                   float value,
                                   float minValue,
                                   float maxValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};
