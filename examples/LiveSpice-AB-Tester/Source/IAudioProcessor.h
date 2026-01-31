#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <string>

/**
 * IAudioProcessor - Pure abstract interface for audio processing
 * 
 * This interface provides a unified API for both:
 * - External VST3 plugins (wrapped via PluginHostWrapper)
 * - Native DSP CircuitProcessor implementations (wrapped via CircuitProcessorWrapper)
 * 
 * Benefits:
 * - Eliminates tight coupling to JUCE's AudioProcessor
 * - Enables seamless A/B switching between VST3 and native DSP
 * - Simplifies MainComponent logic (no type checking)
 * - Facilitates testing and mocking
 */
class IAudioProcessor
{
public:
    virtual ~IAudioProcessor() = default;

    // Audio Processing
    virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) = 0;
    virtual void releaseResources() = 0;
    virtual void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) = 0;

    // Parameter Management
    struct ParameterInfo
    {
        juce::String id;           // Unique identifier (e.g., "drive", "level", "tone")
        juce::String name;         // Display name (e.g., "Drive", "Level", "Tone")
        float defaultValue;        // Default value [0.0, 1.0]
        float currentValue;        // Current value [0.0, 1.0]
        juce::String label;        // Unit label (e.g., "%", "dB", "Hz")
    };

    virtual std::vector<ParameterInfo> getParameters() const = 0;
    virtual bool setParameter(const juce::String& parameterId, float value) = 0;
    virtual float getParameter(const juce::String& parameterId) const = 0;

    // Metadata
    virtual juce::String getName() const = 0;
    virtual juce::String getType() const = 0;  // Returns "VST3" or "Native DSP"
    virtual bool isLoaded() const = 0;

    // State Management
    virtual void reset() = 0;  // Reset to initial state (clear buffers, etc.)
};
