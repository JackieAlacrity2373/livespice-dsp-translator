/*
  ==============================================================================
    Plugin Host Implementation
  ==============================================================================
*/

#include "PluginHost.h"

PluginHost::PluginHost()
{
    // Note: Plugin hosting requires proper plugin format setup
    // For now, we'll scan for plugins on-demand during loading
}

PluginHost::~PluginHost()
{
    unloadPlugin();
}

bool PluginHost::loadPlugin(const juce::String& pluginPath)
{
    unloadPlugin();

    juce::File pluginFile(pluginPath);
    
    if (!pluginFile.exists())
    {
        DBG("Plugin file does not exist: " + pluginPath);
        return false;
    }

    // Try different loading methods based on file extension
    auto extension = pluginFile.getFileExtension().toLowerCase();

    if (extension == ".vst3")
        return loadVST3Plugin(pluginPath);
    else if (extension == ".dll")
        return loadVSTPlugin(pluginPath);
    else if (extension == ".exe")
        return loadStandalonePlugin(pluginPath);

    DBG("Unsupported plugin format: " + extension);
    return false;
}

bool PluginHost::loadVST3Plugin(const juce::String& path)
{
    // Note: Full VST hosting requires more complex setup
    // For initial version, user should compare via DAW
    DBG("Plugin loading from file: " + path);
    DBG("VST hosting will be implemented in future version");
    DBG("Current workaround: Load both plugins in your DAW for comparison");
    return false;
}

bool PluginHost::loadVSTPlugin(const juce::String& path)
{
    // Simplified plugin loading
    DBG("VST loading temporarily simplified - full implementation pending");
    return false;
}

bool PluginHost::loadStandalonePlugin(const juce::String& path)
{
    // For standalone .exe files, we can't directly host them
    // This is a placeholder - in practice, you'd need to wrap them or use IPC
    DBG("Standalone .exe hosting not yet implemented");
    DBG("Suggested: Convert standalone to VST3 format for testing");
    return false;
}

void PluginHost::unloadPlugin()
{
    if (plugin)
    {
        plugin->releaseResources();
        plugin.reset();
    }
}

void PluginHost::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    if (plugin)
        plugin->prepareToPlay(sampleRate, samplesPerBlock);
}

void PluginHost::processBlock(juce::AudioBuffer<float>& buffer,
                              juce::MidiBuffer& midiMessages,
                              int startSample,
                              int numSamples)
{
    if (!plugin)
        return;

    // Create a temporary buffer for the plugin
    juce::AudioBuffer<float> tempBuffer(buffer.getNumChannels(), numSamples);
    
    // Copy input data
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        tempBuffer.copyFrom(ch, 0, buffer.getReadPointer(ch, startSample), numSamples);
    }

    // Process through plugin
    plugin->processBlock(tempBuffer, midiMessages);

    // Copy processed data back
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        buffer.copyFrom(ch, startSample, tempBuffer.getReadPointer(ch), numSamples);
    }
}

void PluginHost::releaseResources()
{
    if (plugin)
        plugin->releaseResources();
}

std::vector<ParameterInfo> PluginHost::getParameterList() const
{
    std::vector<ParameterInfo> params;

    if (!plugin)
        return params;

    auto& pluginParams = plugin->getParameters();
    
    for (int i = 0; i < pluginParams.size(); ++i)
    {
        auto* param = pluginParams[i];
        
        ParameterInfo info;
        info.id = param->getName(100);
        info.name = param->getName(100);
        info.value = param->getValue();
        info.minValue = 0.0f;
        info.maxValue = 1.0f;
        info.defaultValue = param->getDefaultValue();

        params.push_back(info);
    }

    return params;
}

void PluginHost::setParameter(const juce::String& paramId, float value)
{
    if (!plugin)
        return;

    auto& pluginParams = plugin->getParameters();
    
    for (auto* param : pluginParams)
    {
        if (param->getName(100) == paramId)
        {
            param->setValue(value);
            return;
        }
    }
}

void PluginHost::setParameters(const std::map<juce::String, float>& params)
{
    for (const auto& [id, value] : params)
    {
        setParameter(id, value);
    }
}

float PluginHost::getParameter(const juce::String& paramId) const
{
    if (!plugin)
        return 0.0f;

    auto& pluginParams = plugin->getParameters();
    
    for (auto* param : pluginParams)
    {
        if (param->getName(100) == paramId)
            return param->getValue();
    }

    return 0.0f;
}

juce::String PluginHost::getPluginName() const
{
    return plugin ? plugin->getName() : "No Plugin";
}

int PluginHost::getNumParameters() const
{
    return plugin ? plugin->getParameters().size() : 0;
}
