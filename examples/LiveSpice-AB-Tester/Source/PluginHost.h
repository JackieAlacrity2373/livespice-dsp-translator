/*
  ==============================================================================
    Plugin Host - Loads and manages VST/Standalone plugins
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

struct ParameterInfo
{
    juce::String id;
    juce::String name;
    float value;
    float minValue;
    float maxValue;
    float defaultValue;
};

class PluginHost
{
public:
    PluginHost();
    ~PluginHost();

    // Plugin management
    bool loadPlugin(const juce::String& pluginPath);
    void unloadPlugin();
    bool isLoaded() const { return plugin != nullptr; }
    
    // Get underlying processor for direct access
    juce::AudioProcessor* getProcessor() { return plugin.get(); }

    // Audio processing
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, 
                     juce::MidiBuffer& midiMessages,
                     int startSample, 
                     int numSamples);
    void releaseResources();

    // Parameter management
    std::vector<ParameterInfo> getParameterList() const;
    void setParameter(const juce::String& paramId, float value);
    void setParameters(const std::map<juce::String, float>& params);
    float getParameter(const juce::String& paramId) const;

    // Plugin info
    juce::String getPluginName() const;
    int getNumParameters() const;

    // Editor management
    juce::AudioProcessorEditor* getEditor() { return plugin ? plugin->getActiveEditor() : nullptr; }
    bool hasEditor() const { return plugin && plugin->hasEditor(); }
    juce::AudioProcessorEditor* createEditor();

private:
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::unique_ptr<juce::AudioProcessorGraph> graph;

    double currentSampleRate{44100.0};
    int currentBlockSize{512};

    bool loadVST3Plugin(const juce::String& path);
    bool loadVSTPlugin(const juce::String& path);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
};
