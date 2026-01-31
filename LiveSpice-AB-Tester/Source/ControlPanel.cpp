/*
  ==============================================================================
    Control Panel Implementation
  ==============================================================================
*/

#include "ControlPanel.h"

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
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("Unified Parameter Controls", 
        getLocalBounds().removeFromTop(30).reduced(5), 
        juce::Justification::centredLeft);

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

    for (const auto& param : params)
    {
        createControlForParameter(param.id, param.name, 
            param.value, param.minValue, param.maxValue);
    }

    resized();
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

void ControlPanel::createControlForParameter(const juce::String& id,
                                             const juce::String& name,
                                             float value,
                                             float minValue,
                                             float maxValue)
{
    auto control = std::make_unique<ParameterControl>();
    control->id = id;
    control->name = name;
    control->currentValue = value;

    // Create label
    control->label = std::make_unique<juce::Label>();
    control->label->setText(name, juce::dontSendNotification);
    control->label->setFont(juce::Font(14.0f, juce::Font::bold));
    control->label->setColour(juce::Label::textColourId, juce::Colours::white);
    controlsContainer.addAndMakeVisible(*control->label);

    // Create slider
    control->slider = std::make_unique<juce::Slider>();
    control->slider->setRange(minValue, maxValue, 0.001);
    control->slider->setValue(value, juce::dontSendNotification);
    control->slider->setSliderStyle(juce::Slider::LinearHorizontal);
    control->slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    controlsContainer.addAndMakeVisible(*control->slider);

    // Create value label
    control->valueLabel = std::make_unique<juce::Label>();
    control->valueLabel->setText(juce::String(value, 3), juce::dontSendNotification);
    control->valueLabel->setJustificationType(juce::Justification::centred);
    control->valueLabel->setColour(juce::Label::textColourId, juce::Colours::lightblue);
    controlsContainer.addAndMakeVisible(*control->valueLabel);

    // Update value label when slider changes
    control->slider->onValueChange = [this, ctrl = control.get()]() {
        float newValue = static_cast<float>(ctrl->slider->getValue());
        ctrl->currentValue = newValue;
        ctrl->valueLabel->setText(juce::String(newValue, 3), juce::dontSendNotification);
    };

    controls.push_back(std::move(control));
}
