#include "SchematicEditorComponent.h"

SchematicEditorComponent::SchematicEditorComponent(SchematicProcessor* processor)
    : processor(processor)
{
    setOpaque(true);
    
    if (processor)
    {
        createControls();
    }
}

SchematicEditorComponent::~SchematicEditorComponent()
{
    controls.clear();
}

void SchematicEditorComponent::createControls()
{
    controls.clear();
    
    auto params = processor->getParameters();
    
    for (const auto& paramInfo : params)
    {
        // Skip bypass parameters
        if (paramInfo.id.containsIgnoreCase("bypass"))
            continue;
        
        auto ctrl = std::make_unique<ParameterControl>();
        ctrl->parameterId = paramInfo.id;
        
        // Name label
        ctrl->nameLabel = std::make_unique<juce::Label>();
        ctrl->nameLabel->setText(paramInfo.name, juce::dontSendNotification);
        ctrl->nameLabel->setJustificationType(juce::Justification::centred);
        ctrl->nameLabel->setFont(juce::Font(12.0f, juce::Font::bold));
        addAndMakeVisible(*ctrl->nameLabel);
        
        // Slider
        ctrl->slider = std::make_unique<juce::Slider>();
        ctrl->slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        ctrl->slider->setRange(0.0, 1.0, 0.01);
        ctrl->slider->setValue(paramInfo.currentValue, juce::dontSendNotification);
        ctrl->slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 50, 20);
        
        // Store reference to parameterId for lambda capture
        juce::String paramId = paramInfo.id;
        
        // Slider callback
        ctrl->slider->onValueChange = [this, paramId]()
        {
            if (onParameterChanged && !controls.empty())
            {
                onParameterChanged(paramId, (float)controls[0]->slider->getValue());
            }
        };
        
        addAndMakeVisible(*ctrl->slider);
        
        // Value label
        ctrl->valueLabel = std::make_unique<juce::Label>();
        ctrl->valueLabel->setText(juce::String(paramInfo.currentValue, 2), juce::dontSendNotification);
        ctrl->valueLabel->setJustificationType(juce::Justification::centred);
        ctrl->valueLabel->setFont(juce::Font(11.0f));
        addAndMakeVisible(*ctrl->valueLabel);
        
        controls.push_back(std::move(ctrl));
    }
}

void SchematicEditorComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("Schematic Parameters", getLocalBounds().removeFromTop(30).reduced(5),
               juce::Justification::centredLeft);
    
    // Draw border
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 2);
}

void SchematicEditorComponent::resized()
{
    auto area = getLocalBounds();
    
    // Title area
    area.removeFromTop(40);
    area.reduce(10, 10);
    
    // Layout controls in a grid (2 columns for knobs)
    const int controlsPerRow = 2;
    const int controlSize = 100;
    const int spacing = 20;
    
    int row = 0;
    int col = 0;
    
    for (auto& ctrl : controls)
    {
        auto controlArea = area
            .withX(area.getX() + col * (controlSize + spacing))
            .withY(area.getY() + row * (controlSize + spacing + 60))
            .withWidth(controlSize)
            .withHeight(controlSize + 60);
        
        if (controlArea.getBottom() > getHeight() - 20)
            break;
        
        // Name label
        ctrl->nameLabel->setBounds(controlArea.removeFromTop(20));
        
        // Slider (knob)
        ctrl->slider->setBounds(controlArea.removeFromTop(controlSize));
        
        // Value label
        ctrl->valueLabel->setBounds(controlArea.removeFromTop(20));
        
        col++;
        if (col >= controlsPerRow)
        {
            col = 0;
            row++;
        }
    }
}
