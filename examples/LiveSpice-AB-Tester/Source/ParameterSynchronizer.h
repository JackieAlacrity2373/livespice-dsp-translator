#pragma once

#include "IAudioProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

/**
 * ParameterSynchronizer - Manages parameter synchronization between processors
 * 
 * Responsibilities:
 * - Links parameters with matching IDs between processor A and B
 * - Propagates parameter changes bidirectionally
 * - Handles parameter mapping (e.g., different value ranges)
 * - Provides callbacks for UI updates
 */
class ParameterSynchronizer
{
public:
    using ParameterChangeCallback = std::function<void(const juce::String& parameterId, float value)>;

    ParameterSynchronizer();
    ~ParameterSynchronizer() = default;

    // Processor management
    void setProcessorA(IAudioProcessor* processor);
    void setProcessorB(IAudioProcessor* processor);
    
    IAudioProcessor* getProcessorA() { return processorA; }
    IAudioProcessor* getProcessorB() { return processorB; }

    // Synchronization control
    void setLinkingEnabled(bool enabled) { linkingEnabled = enabled; }
    bool isLinkingEnabled() const { return linkingEnabled; }
    
    // Update a parameter and sync to other processor
    void setParameter(const juce::String& parameterId, float value, bool notifyOther = true);
    
    // Get parameter from currently active processor
    float getParameter(const juce::String& parameterId) const;
    
    // Get all parameters from both processors (merged list)
    std::vector<IAudioProcessor::ParameterInfo> getMergedParameters() const;
    
    // Callbacks for UI updates
    void onParameterChanged(ParameterChangeCallback callback);

    // Copy all parameters from A to B or vice versa
    void copyParametersAtoB();
    void copyParametersBtoA();

private:
    // Helper functions for parameter filtering
    bool isValidEffectParameter(const IAudioProcessor::ParameterInfo& param) const;
    std::vector<IAudioProcessor::ParameterInfo> filterEffectParameters(
        const std::vector<IAudioProcessor::ParameterInfo>& params) const;

private:
    IAudioProcessor* processorA{nullptr};
    IAudioProcessor* processorB{nullptr};
    
    bool linkingEnabled{true};
    
    ParameterChangeCallback parameterCallback;
    
    // Helper to update linked parameter
    void updateLinkedParameter(IAudioProcessor* source, IAudioProcessor* target, 
                              const juce::String& parameterId, float value);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterSynchronizer)
};
