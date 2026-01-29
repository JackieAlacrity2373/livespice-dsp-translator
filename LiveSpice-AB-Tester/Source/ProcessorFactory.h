#pragma once

#include "IAudioProcessor.h"
#include "PluginHostWrapper.h"
#include "CircuitProcessorWrapper.h"
#include <juce_core/juce_core.h>
#include <memory>

/**
 * ProcessorFactory - Creates IAudioProcessor instances from various sources
 * 
 * Supports:
 * - VST3 plugins from file path
 * - Native CircuitProcessor implementations by name
 * - Future: VST2, AU, presets, etc.
 */
class ProcessorFactory
{
public:
    enum class ProcessorType
    {
        VST3Plugin,
        NativeDSP,
        Unknown
    };

    // Detect processor type from path or identifier
    static ProcessorType detectType(const juce::String& identifier);

    // Create processor from file path or identifier
    static std::unique_ptr<IAudioProcessor> create(const juce::String& identifier);

    // Create VST3 plugin wrapper
    static std::unique_ptr<IAudioProcessor> createVST3(const juce::String& pluginPath);

    // Create native DSP processor by name
    static std::unique_ptr<IAudioProcessor> createNativeDSP(const juce::String& circuitName);

    // Get list of available native DSP circuits
    static juce::StringArray getAvailableNativeDSP();

private:
    ProcessorFactory() = delete;  // Static class
};
