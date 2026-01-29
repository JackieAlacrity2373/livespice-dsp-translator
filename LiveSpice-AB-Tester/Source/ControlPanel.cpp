/*
  ==============================================================================
    Control Panel Implementation with MIDI CC Support
  ==============================================================================
*/

#include "MainComponent.h"
#include "ControlPanel.h"  // For logDebug function

ControlPanel::ControlPanel()
{
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&controlsContainer, false);
    viewport.setScrollBarsShown(true, false);
}

ControlPanel::~ControlPanel()
{
}

void ControlPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    static int paintCounter = 0;
    if (paintCounter++ % 10 == 0)
        logDebug("ControlPanel::paint - Rendering with " + juce::String(controls.size()) + " controls visible");
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("Unified Parameter Controls (MIDI CC)", 
        getLocalBounds().removeFromTop(30).reduced(5), 
        juce::Justification::centredLeft);

    // Show message if no controls
    if (controls.empty())
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(12.0f));
        g.drawText("[Waiting for plugin parameters...]", 
            getLocalBounds().reduced(10),
            juce::Justification::centred);
    }

    // Draw border
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 2);
}

void ControlPanel::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(35); // Header space
    viewport.setBounds(area.reduced(5));

    // Layout controls in grid
    if (!controls.empty())
    {
        const int controlHeight = 80;
        const int numControls = static_cast<int>(controls.size());
        const int totalHeight = numControls * controlHeight;

        controlsContainer.setBounds(0, 0, viewport.getWidth() - 20, totalHeight);
        DBG("ControlPanel: Resizing " + juce::String(numControls) + " controls, container height: " + juce::String(totalHeight));

        int yPos = 0;
        for (auto& control : controls)
        {
            auto controlArea = juce::Rectangle<int>(10, yPos, 
                controlsContainer.getWidth() - 20, controlHeight);

            control->label->setBounds(controlArea.removeFromTop(20));
            
            auto sliderArea = controlArea.removeFromLeft(
                controlsContainer.getWidth() - 100);
            control->slider->setBounds(sliderArea.reduced(5));
            
            control->valueLabel->setBounds(controlArea.reduced(5));

            yPos += controlHeight;
        }
    }
}

void ControlPanel::setParameterList(const std::vector<ParameterInfo>& params)
{
    clearParameters();

    DBG("ControlPanel: Setting up " + juce::String(params.size()) + " parameters");

    int midiCC = 0;  // Start with CC 0
    for (const auto& param : params)
    {
        DBG("  Creating control for: " + param.name);
        createControlForParameter(param.id, param.name, 
            0.5f,  // Default to 0.5
            param.minValue, param.maxValue, midiCC);
        
        midiCC++;
        if (midiCC > 119) midiCC = 0;  // Wrap around MIDI CC range
    }

    DBG("ControlPanel: Created " + juce::String(controls.size()) + " controls");
    resized();
    repaint();
}

std::map<juce::String, float> ControlPanel::getAllParameters() const
{
    std::map<juce::String, float> params;

    for (const auto& control : controls)
    {
        params[control->id] = control->currentValue;
    }

    return params;
}

void ControlPanel::clearParameters()
{
    controls.clear();
    controlsContainer.removeAllChildren();
}

void ControlPanel::sliderValueChanged(juce::Slider* slider)
{
    // Find which control's slider was changed
    for (auto& control : controls)
    {
        if (control->slider.get() == slider)
        {
            float newValue = static_cast<float>(slider->getValue());
            control->currentValue = newValue;
            control->valueLabel->setText(juce::String(newValue, 3), juce::dontSendNotification);
            
            // Notify listener (for sending to plugins)
            if (onParameterChanged)
            {
                onParameterChanged(control->id, newValue);
            }
            break;
        }
    }
}

void ControlPanel::createControlForParameter(const juce::String& id,
                                             const juce::String& name,
                                             float value,
                                             float minValue,
                                             float maxValue,
                                             int midiCC)
{
    auto control = std::make_unique<ParameterControl>();
    control->id = id;
    control->name = name;
    control->currentValue = value;
    control->midiCC = midiCC;

    // Create label with MIDI CC info
    control->label = std::make_unique<juce::Label>();
    control->label->setText(name + " (CC" + juce::String(midiCC) + ")", 
        juce::dontSendNotification);
    control->label->setFont(juce::Font(14.0f, juce::Font::bold));
    control->label->setColour(juce::Label::textColourId, juce::Colours::white);
    controlsContainer.addAndMakeVisible(*control->label);

    // Create slider
    control->slider = std::make_unique<juce::Slider>();
    control->slider->setRange(0.0, 1.0, 0.001);  // Normalized 0-1 range for MIDI CC
    control->slider->setValue(value, juce::dontSendNotification);
    control->slider->setSliderStyle(juce::Slider::LinearHorizontal);
    control->slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    control->slider->addListener(this);  // Add this as listener
    controlsContainer.addAndMakeVisible(*control->slider);

    // Create value label
    control->valueLabel = std::make_unique<juce::Label>();
    control->valueLabel->setText(juce::String(value, 3), juce::dontSendNotification);
    control->valueLabel->setJustificationType(juce::Justification::centred);
    control->valueLabel->setColour(juce::Label::textColourId, juce::Colours::lightblue);
    controlsContainer.addAndMakeVisible(*control->valueLabel);

    controls.push_back(std::move(control));
}
