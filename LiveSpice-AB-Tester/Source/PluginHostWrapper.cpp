#include "PluginHostWrapper.h"
#include "Logging.h"

PluginHostWrapper::PluginHostWrapper()
    : pluginHost(std::make_unique<PluginHost>())
{
}

PluginHostWrapper::PluginHostWrapper(std::unique_ptr<PluginHost> host)
    : pluginHost(std::move(host))
{
}

bool PluginHostWrapper::loadPlugin(const juce::String& pluginPath)
{
    parametersCached = false;
    cachedParameters.clear();
    
    bool success = pluginHost->loadPlugin(pluginPath);
    
    if (success)
    {
        logDebug("PluginHostWrapper: Loaded VST3 plugin - " + getName(), LogLevel::INFO);
        updateParameterCache();
    }
    else
    {
        logDebug("PluginHostWrapper: Failed to load plugin from " + pluginPath, LogLevel::ERROR);
    }
    
    return success;
}

void PluginHostWrapper::unloadPlugin()
{
    pluginHost->unloadPlugin();
    parametersCached = false;
    cachedParameters.clear();
    logDebug("PluginHostWrapper: Plugin unloaded", LogLevel::INFO);
}

void PluginHostWrapper::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    if (pluginHost && isLoaded())
    {
        pluginHost->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
        logDebug("PluginHostWrapper: Prepared to play - " + 
                 juce::String(sampleRate) + " Hz, " + 
                 juce::String(maximumExpectedSamplesPerBlock) + " samples", LogLevel::DEBUG);
    }
}

void PluginHostWrapper::releaseResources()
{
    if (pluginHost && isLoaded())
    {
        pluginHost->releaseResources();
        logDebug("PluginHostWrapper: Resources released", LogLevel::DEBUG);
    }
}

void PluginHostWrapper::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (pluginHost && isLoaded())
    {
        // Process entire buffer through plugin
        pluginHost->processBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    }
    else
    {
        // No plugin loaded - pass through silence or input
        buffer.clear();
    }
}

std::vector<IAudioProcessor::ParameterInfo> PluginHostWrapper::getParameters() const
{
    if (!parametersCached)
    {
        updateParameterCache();
    }
    return cachedParameters;
}

bool PluginHostWrapper::setParameter(const juce::String& parameterId, float value)
{
    if (!isLoaded())
    {
        logDebug("PluginHostWrapper: Cannot set parameter - no plugin loaded", LogLevel::WARNING);
        return false;
    }

    pluginHost->setParameter(parameterId, value);
    
    // Update cache
    if (parametersCached)
    {
        for (auto& param : cachedParameters)
        {
            if (param.id.equalsIgnoreCase(parameterId))
            {
                param.currentValue = value;
                break;
            }
        }
    }
    
    logDebug("PluginHostWrapper: Set parameter " + parameterId + " = " + juce::String(value), LogLevel::DEBUG);
    return true;
}

float PluginHostWrapper::getParameter(const juce::String& parameterId) const
{
    if (!isLoaded())
        return 0.0f;

    return pluginHost->getParameter(parameterId);
}

juce::String PluginHostWrapper::getName() const
{
    if (!isLoaded())
        return "No Plugin Loaded";
    
    return pluginHost->getPluginName();
}

bool PluginHostWrapper::isLoaded() const
{
    return pluginHost && pluginHost->isLoaded();
}

void PluginHostWrapper::reset()
{
    if (isLoaded())
    {
        // Reset by unloading/reloading would lose state
        // Instead, reset parameters to defaults
        auto params = getParameters();
        for (const auto& param : params)
        {
            setParameter(param.id, param.defaultValue);
        }
        logDebug("PluginHostWrapper: Reset to defaults", LogLevel::INFO);
    }
}

void PluginHostWrapper::updateParameterCache() const
{
    cachedParameters.clear();
    
    if (!isLoaded())
    {
        parametersCached = true;
        return;
    }

    // Get parameters from PluginHost
    auto hostParams = pluginHost->getParameterList();
    
    // Convert PluginHost::ParameterInfo to IAudioProcessor::ParameterInfo
    for (const auto& hostParam : hostParams)
    {
        IAudioProcessor::ParameterInfo param;
        param.id = hostParam.id;
        param.name = hostParam.name;
        param.defaultValue = hostParam.defaultValue;
        param.currentValue = hostParam.value;
        param.label = "";  // PluginHost doesn't provide labels
        
        cachedParameters.push_back(param);
    }
    
    parametersCached = true;
    logDebug("PluginHostWrapper: Cached " + juce::String(cachedParameters.size()) + " parameters", LogLevel::DEBUG);
}
