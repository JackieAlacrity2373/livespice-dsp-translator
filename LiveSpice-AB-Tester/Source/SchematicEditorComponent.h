#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SchematicProcessor.h"

/**
 * SchematicEditorComponent - Visual UI for schematic parameters
 * 
 * Displays interactive parameter controls (sliders) for the loaded schematic
 * Mirrors the control panel but with visual feedback specific to the schematic
 */
class SchematicEditorComponent : public juce::Component
{
public:
    explicit SchematicEditorComponent(SchematicProcessor* processor);
    ~SchematicEditorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Callback for parameter changes
    std::function<void(const juce::String& parameterId, float value)> onParameterChanged;

private:
    SchematicProcessor* processor;
    
    struct ParameterControl
    {
        juce::String parameterId;
        std::unique_ptr<juce::Label> nameLabel;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> valueLabel;
    };
    
    std::vector<std::unique_ptr<ParameterControl>> controls;
    
    void createControls();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SchematicEditorComponent)
};
