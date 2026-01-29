#include "ParameterSynchronizer.h"
#include "Logging.h"

ParameterSynchronizer::ParameterSynchronizer()
{
}

void ParameterSynchronizer::setProcessorA(IAudioProcessor* processor)
{
    processorA = processor;
    logDebug("ParameterSynchronizer: Processor A set - " + 
             (processor ? processor->getName() : "nullptr"), LogLevel::DEBUG);
}

void ParameterSynchronizer::setProcessorB(IAudioProcessor* processor)
{
    processorB = processor;
    logDebug("ParameterSynchronizer: Processor B set - " + 
             (processor ? processor->getName() : "nullptr"), LogLevel::DEBUG);
}

void ParameterSynchronizer::setParameter(const juce::String& parameterId, float value, bool notifyOther)
{
    bool foundInA = false;
    bool foundInB = false;
    
    // Update processor A
    if (processorA && processorA->isLoaded())
    {
        foundInA = processorA->setParameter(parameterId, value);
    }
    
    // Update processor B (only if linking enabled and parameter exists)
    if (processorB && processorB->isLoaded() && linkingEnabled && notifyOther)
    {
        foundInB = processorB->setParameter(parameterId, value);
        
        if (!foundInB)
        {
            logDebug("ParameterSynchronizer: Parameter '" + parameterId + 
                     "' not found in processor B - skipping sync", LogLevel::DEBUG);
        }
    }
    
    // Notify callback
    if (parameterCallback)
    {
        parameterCallback(parameterId, value);
    }
    
    if (foundInA || foundInB)
    {
        logDebug("ParameterSynchronizer: Set " + parameterId + " = " + juce::String(value) + 
                 " (A:" + juce::String(foundInA ? "yes" : "no") + 
                 ", B:" + juce::String(foundInB ? "yes" : "no") + ")", LogLevel::DEBUG);
    }
}

float ParameterSynchronizer::getParameter(const juce::String& parameterId) const
{
    // Try processor A first
    if (processorA && processorA->isLoaded())
    {
        return processorA->getParameter(parameterId);
    }
    
    // Fall back to processor B
    if (processorB && processorB->isLoaded())
    {
        return processorB->getParameter(parameterId);
    }
    
    return 0.0f;
}

std::vector<IAudioProcessor::ParameterInfo> ParameterSynchronizer::getMergedParameters() const
{
    std::vector<IAudioProcessor::ParameterInfo> commonParams;
    
    // If either processor not loaded, return empty
    if (!processorA || !processorA->isLoaded() || !processorB || !processorB->isLoaded())
    {
        // If only A is loaded, return its parameters
        if (processorA && processorA->isLoaded())
        {
            auto params = processorA->getParameters();
            return filterEffectParameters(params);
        }
        
        // If only B is loaded, return its parameters
        if (processorB && processorB->isLoaded())
        {
            auto params = processorB->getParameters();
            return filterEffectParameters(params);
        }
            
        return commonParams;
    }
    
    // Both loaded - find COMMON parameters only
    auto paramsA = processorA->getParameters();
    auto paramsB = processorB->getParameters();
    
    // Find parameters that exist in both A and B (by ID)
    for (const auto& paramA : paramsA)
    {
        // Skip bypass and non-potentiometer parameters
        if (!isValidEffectParameter(paramA))
            continue;
            
        // Look for matching parameter in B
        auto it = std::find_if(paramsB.begin(), paramsB.end(),
            [&paramA](const IAudioProcessor::ParameterInfo& paramB)
            {
                return paramB.id.equalsIgnoreCase(paramA.id);
            });
        
        if (it != paramsB.end())
        {
            // Found in both - use A's info as base
            commonParams.push_back(paramA);
        }
    }
    
    logDebug("getMergedParameters: Found " + juce::String(commonParams.size()) + 
             " common parameters (A:" + juce::String(paramsA.size()) + 
             ", B:" + juce::String(paramsB.size()) + ")", LogLevel::DEBUG);
    
    return commonParams;
}

bool ParameterSynchronizer::isValidEffectParameter(const IAudioProcessor::ParameterInfo& param) const
{
    // Filter out bypass and non-potentiometer parameters
    juce::String idLower = param.id.toLowerCase();
    juce::String nameLower = param.name.toLowerCase();
    
    // Exclude bypass switches
    if (idLower.contains("bypass") || nameLower.contains("bypass"))
        return false;
    
    // Exclude certain non-potentiometer controls
    if (idLower.contains("mode") || nameLower.contains("mode"))
        return false;
    if (idLower.contains("type") || nameLower.contains("type"))
        return false;
    if (idLower.contains("mix") && (idLower.contains("mode") || idLower.contains("select")))
        return false;
    
    // Keep potentiometers and effect-related parameters
    return true;
}

std::vector<IAudioProcessor::ParameterInfo> ParameterSynchronizer::filterEffectParameters(
    const std::vector<IAudioProcessor::ParameterInfo>& params) const
{
    std::vector<IAudioProcessor::ParameterInfo> filtered;
    for (const auto& param : params)
    {
        if (isValidEffectParameter(param))
            filtered.push_back(param);
    }
    return filtered;
}

void ParameterSynchronizer::onParameterChanged(ParameterChangeCallback callback)
{
    parameterCallback = callback;
}

void ParameterSynchronizer::copyParametersAtoB()
{
    if (!processorA || !processorA->isLoaded() || !processorB || !processorB->isLoaded())
    {
        logDebug("ParameterSynchronizer: Cannot copy A to B - processor not loaded", LogLevel::WARNING);
        return;
    }
    
    auto paramsA = processorA->getParameters();
    
    for (const auto& param : paramsA)
    {
        processorB->setParameter(param.id, param.currentValue);
    }
    
    logDebug("ParameterSynchronizer: Copied " + juce::String(paramsA.size()) + 
             " parameters from A to B", LogLevel::INFO);
}

void ParameterSynchronizer::copyParametersBtoA()
{
    if (!processorA || !processorA->isLoaded() || !processorB || !processorB->isLoaded())
    {
        logDebug("ParameterSynchronizer: Cannot copy B to A - processor not loaded", LogLevel::WARNING);
        return;
    }
    
    auto paramsB = processorB->getParameters();
    
    for (const auto& param : paramsB)
    {
        processorA->setParameter(param.id, param.currentValue);
    }
    
    logDebug("ParameterSynchronizer: Copied " + juce::String(paramsB.size()) + 
             " parameters from B to A", LogLevel::INFO);
}

void ParameterSynchronizer::updateLinkedParameter(IAudioProcessor* source, IAudioProcessor* target, 
                                                  const juce::String& parameterId, float value)
{
    if (!source || !target || !linkingEnabled)
        return;
    
    if (!source->isLoaded() || !target->isLoaded())
        return;
    
    target->setParameter(parameterId, value);
}
