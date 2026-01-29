/*
  ==============================================================================
    Plugin Host Implementation
  ==============================================================================
*/

#include "MainComponent.h"
#include "PluginHost.h"  // For logDebug function

PluginHost::PluginHost()
{
    // Create audio processor graph for hosting
    graph = std::make_unique<juce::AudioProcessorGraph>();
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

    // Try loading as VST3 or VST
    auto extension = pluginFile.getFileExtension().toLowerCase();

    if (extension == ".vst3")
        return loadVST3Plugin(pluginPath);
    else if (extension == ".dll")
        return loadVSTPlugin(pluginPath);

    DBG("Unsupported plugin format: " + extension);
    return false;
}

bool PluginHost::loadVST3Plugin(const juce::String& path)
{
    juce::VST3PluginFormat vst3Format;
    juce::OwnedArray<juce::PluginDescription> typesFound;

    vst3Format.findAllTypesForFile(typesFound, path);

    if (typesFound.isEmpty())
    {
        DBG("No VST3 plugins found in file");
        return false;
    }

    juce::String errorMessage;
    plugin = vst3Format.createInstanceFromDescription(*typesFound[0],
                                                       currentSampleRate,
                                                       currentBlockSize,
                                                       errorMessage);

    if (plugin == nullptr)
    {
        DBG("Failed to create VST3 instance: " + errorMessage);
        return false;
    }

    plugin->prepareToPlay(currentSampleRate, currentBlockSize);
    plugin->setNonRealtime(false);
    
    DBG("Successfully loaded VST3: " + plugin->getName());
    return true;
}

bool PluginHost::loadVSTPlugin(const juce::String& path)
{
    #if JUCE_PLUGINHOST_VST
    juce::VSTPluginFormat vstFormat;
    juce::OwnedArray<juce::PluginDescription> typesFound;

    vstFormat.findAllTypesForFile(typesFound, path);

    if (typesFound.isEmpty())
    {
        DBG("No VST plugins found in file");
        return false;
    }

    juce::String errorMessage;
    plugin = vstFormat.createInstanceFromDescription(*typesFound[0],
                                                      currentSampleRate,
                                                      currentBlockSize,
                                                      errorMessage);

    if (plugin == nullptr)
    {
        DBG("Failed to create VST instance: " + errorMessage);
        return false;
    }

    plugin->prepareToPlay(currentSampleRate, currentBlockSize);
    plugin->setNonRealtime(false);
    
    DBG("Successfully loaded VST: " + plugin->getName());
    return true;
    #else
    DBG("VST2 support not enabled");
    return false;
    #endif
}

juce::AudioProcessorEditor* PluginHost::createEditor()
{
    if (!plugin || !plugin->hasEditor())
        return nullptr;

    auto editor = plugin->createEditor();
    if (editor)
    {
        DBG("Created editor for: " + plugin->getName());
    }
    return editor;
}

void PluginHost::unloadPlugin()
{
    if (plugin)
    {
        plugin->releaseResources();
        plugin.reset();
    }

    if (graph)
    {
        graph->clear();
    }
}

void PluginHost::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    logDebug("PluginHost::prepareToPlay - SR: " + juce::String(sampleRate) + ", Block: " + juce::String(samplesPerBlock));
    
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    if (graph)
        graph->prepareToPlay(sampleRate, samplesPerBlock);

    if (plugin)
    {
        plugin->setPlayConfigDetails(2, 2, sampleRate, samplesPerBlock);
        plugin->prepareToPlay(sampleRate, samplesPerBlock);
        plugin->setNonRealtime(false);
        plugin->enableAllBuses();
        logDebug("  Plugin prepared and buses enabled");
    }
}

void PluginHost::processBlock(juce::AudioBuffer<float>& buffer,
                              juce::MidiBuffer& midiMessages,
                              int startSample,
                              int numSamples)
{
    if (!plugin)
    {
        logDebug("PluginHost::processBlock - Plugin is null!");
        return;
    }

    static int processCounter = 0;
    if (processCounter++ % 1000 == 0)
    {
        logDebug("PluginHost::processBlock - Processing " + juce::String(numSamples) + " samples");
    }

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
    if (graph)
        graph->releaseResources();
        
    if (plugin)
        plugin->releaseResources();
}

std::vector<ParameterInfo> PluginHost::getParameterList() const
{
    std::vector<ParameterInfo> params;

    if (!plugin)
    {
        logDebug("PluginHost::getParameterList - Plugin is null!");
        return params;
    }

    // First try the legacy getParameters() API
    auto& pluginParams = plugin->getParameters();
    logDebug("PluginHost::getParameterList - Found " + juce::String(pluginParams.size()) + " legacy parameters");
    
    for (int i = 0; i < pluginParams.size(); ++i)
    {
        auto* param = pluginParams[i];
        
        if (!param)
        {
            logDebug("  Parameter " + juce::String(i) + " is null!");
            continue;
        }
        
        ParameterInfo info;
        info.id = param->getName(100);
        info.name = param->getName(100);
        info.value = param->getValue();
        info.minValue = 0.0f;
        info.maxValue = 1.0f;
        info.defaultValue = param->getDefaultValue();

        logDebug("  Legacy Param " + juce::String(i) + ": " + info.name + " = " + juce::String(info.value, 3));
        params.push_back(info);
    }
    
    // If no parameters found via legacy API, try accessing APVTS via the plugin's state
    if (params.empty())
    {
        logDebug("PluginHost::getParameterList - Trying to access APVTS parameters...");
        
        // Get the plugin's parameter layout by examining the plugin processor
        // The plugin stores parameters in its internal state which we can access via ValueTree
        juce::MemoryBlock state;
        plugin->getStateInformation(state);
        
        if (state.getSize() > 0)
        {
            logDebug("  Plugin state size: " + juce::String(state.getSize()) + " bytes - has state data");
            
            // Try to deserialize and extract parameter info from XML
            std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(
                juce::String::fromUTF8(static_cast<const char*>(state.getData()), static_cast<int>(state.getSize()))));
            if (xml)
            {
                logDebug("  Successfully parsed plugin state XML");
                
                // Look for parameter nodes
                forEachXmlChildElement(*xml, elem)
                {
                    if (elem->hasAttribute("id"))
                    {
                        ParameterInfo info;
                        info.id = elem->getStringAttribute("id");
                        info.name = elem->getStringAttribute("name", info.id);
                        info.value = juce::jlimit(0.0f, 1.0f, 
                            static_cast<float>(elem->getDoubleAttribute("value", 0.5)));
                        info.minValue = 0.0f;
                        info.maxValue = 1.0f;
                        info.defaultValue = 0.5f;
                        
                        logDebug("  APVTS Param: " + info.name + " = " + juce::String(info.value, 3));
                        params.push_back(info);
                    }
                }
            }
        }
        
        // If still no parameters, we'll show a message in the UI
        if (params.empty())
        {
            logDebug("PluginHost::getParameterList - No parameters found in plugin state");
            // Don't create fake parameters - let the user know
        }
    }
    
    logDebug("PluginHost::getParameterList - Returning " + juce::String(params.size()) + " parameters");
    return params;
}

void PluginHost::setParameter(const juce::String& paramId, float value)
{
    if (!plugin)
    {
        logDebug("PluginHost::setParameter - Plugin is null!");
        return;
    }

    logDebug("PluginHost::setParameter - Setting " + paramId + " to " + juce::String(value, 3));

    // First try the legacy getParameters() API
    auto& pluginParams = plugin->getParameters();
    
    for (int i = 0; i < pluginParams.size(); ++i)
    {
        auto* param = pluginParams[i];
        if (param)
        {
            auto paramName = param->getName(100);
            
            // Try exact match first
            if (paramName == paramId)
            {
                param->setValue(value);
                logDebug("  Set via exact name match at index " + juce::String(i));
                return;
            }
            
            // Try case-insensitive match
            if (paramName.toLowerCase() == paramId.toLowerCase())
            {
                param->setValue(value);
                logDebug("  Set via case-insensitive match at index " + juce::String(i));
                return;
            }
        }
    }
    
    logDebug("  WARNING: Parameter not found: " + paramId + " (checked " + juce::String(pluginParams.size()) + " params)");
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
