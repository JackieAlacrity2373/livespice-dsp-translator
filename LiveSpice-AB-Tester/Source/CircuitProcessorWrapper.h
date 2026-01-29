#pragma once

#include "IAudioProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

/**
 * CircuitProcessorWrapper<T> - Template wrapper for native JUCE AudioProcessor DSP
 * 
 * This template adapts any JUCE AudioProcessor-based CircuitProcessor to the
 * IAudioProcessor interface, enabling native DSP circuits to work alongside VST3 plugins.
 * 
 * Template parameter T must be a JUCE AudioProcessor subclass with:
 * - Default constructor
 * - getAPVTS() method returning AudioProcessorValueTreeState&
 * 
 * Example usage:
 *   auto mxr = std::make_unique<CircuitProcessorWrapper<MXRDistortionProcessor>>();
 *   auto boss = std::make_unique<CircuitProcessorWrapper<BossSD1Processor>>();
 */
template<typename T>
class CircuitProcessorWrapper : public IAudioProcessor
{
public:
    CircuitProcessorWrapper()
        : processor(std::make_unique<T>())
    {
        static_assert(std::is_base_of<juce::AudioProcessor, T>::value,
                      "T must inherit from juce::AudioProcessor");
        
        // Initialize processor
        processor->setRateAndBufferSizeDetails(44100.0, 512);
    }

    ~CircuitProcessorWrapper() override = default;

    // IAudioProcessor implementation
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        processor->setRateAndBufferSizeDetails(sampleRate, maximumExpectedSamplesPerBlock);
        processor->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
    }

    void releaseResources() override
    {
        processor->releaseResources();
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override
    {
        processor->processBlock(buffer, midiMessages);
    }

    std::vector<ParameterInfo> getParameters() const override
    {
        std::vector<ParameterInfo> params;
        
        // Access APVTS through const_cast (JUCE's API limitation)
        auto& apvts = const_cast<CircuitProcessorWrapper*>(this)->processor->getAPVTS();
        
        for (auto* param : apvts.processor.getParameters())
        {
            if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            {
                ParameterInfo info;
                info.id = paramWithID->getParameterID();
                info.name = param->getName(100);
                info.defaultValue = param->getDefaultValue();
                info.currentValue = param->getValue();
                info.label = param->getLabel();
                
                params.push_back(info);
            }
        }
        
        return params;
    }

    bool setParameter(const juce::String& parameterId, float value) override
    {
        auto& apvts = processor->getAPVTS();
        
        // Try to find parameter by ID
        if (auto* param = apvts.getParameter(parameterId))
        {
            param->setValueNotifyingHost(value);
            return true;
        }
        
        // Try case-insensitive match
        for (auto* param : apvts.processor.getParameters())
        {
            if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            {
                if (paramWithID->getParameterID().equalsIgnoreCase(parameterId))
                {
                    param->setValueNotifyingHost(value);
                    return true;
                }
            }
        }
        
        return false;
    }

    float getParameter(const juce::String& parameterId) const override
    {
        auto& apvts = const_cast<CircuitProcessorWrapper*>(this)->processor->getAPVTS();
        
        if (auto* param = apvts.getParameter(parameterId))
        {
            return param->getValue();
        }
        
        // Try case-insensitive match
        for (auto* param : apvts.processor.getParameters())
        {
            if (auto* paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            {
                if (paramWithID->getParameterID().equalsIgnoreCase(parameterId))
                {
                    return param->getValue();
                }
            }
        }
        
        return 0.0f;
    }

    juce::String getName() const override
    {
        return processor->getName();
    }

    juce::String getType() const override
    {
        return "Native DSP";
    }

    bool isLoaded() const override
    {
        return processor != nullptr;
    }

    void reset() override
    {
        processor->reset();
        
        // Reset all parameters to defaults
        auto& apvts = processor->getAPVTS();
        for (auto* param : apvts.processor.getParameters())
        {
            param->setValueNotifyingHost(param->getDefaultValue());
        }
    }

    // Access to underlying JUCE AudioProcessor (for advanced use)
    T* getProcessor() { return processor.get(); }
    const T* getProcessor() const { return processor.get(); }

private:
    std::unique_ptr<T> processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircuitProcessorWrapper)
};
