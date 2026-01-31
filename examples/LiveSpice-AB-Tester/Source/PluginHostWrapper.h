#pragma once

#include "IAudioProcessor.h"
#include "PluginHost.h"
#include <memory>

/**
 * PluginHostWrapper - Adapts existing PluginHost to IAudioProcessor interface
 * 
 * This wrapper enables external VST3 plugins to work through the unified
 * IAudioProcessor abstraction, allowing seamless integration with native DSP.
 */
class PluginHostWrapper : public IAudioProcessor
{
public:
    PluginHostWrapper();
    explicit PluginHostWrapper(std::unique_ptr<PluginHost> host);
    ~PluginHostWrapper() override = default;

    // Load VST3 from file path
    bool loadPlugin(const juce::String& pluginPath);
    void unloadPlugin();

    // IAudioProcessor implementation
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    std::vector<ParameterInfo> getParameters() const override;
    bool setParameter(const juce::String& parameterId, float value) override;
    float getParameter(const juce::String& parameterId) const override;

    juce::String getName() const override;
    juce::String getType() const override { return "VST3"; }
    bool isLoaded() const override;

    void reset() override;

    // Access to underlying PluginHost (for migration compatibility)
    PluginHost* getPluginHost() { return pluginHost.get(); }
    const PluginHost* getPluginHost() const { return pluginHost.get(); }

private:
    std::unique_ptr<PluginHost> pluginHost;

    // Cache parameters to avoid repeated parsing
    mutable std::vector<ParameterInfo> cachedParameters;
    mutable bool parametersCached{false};

    void updateParameterCache() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHostWrapper)
};
