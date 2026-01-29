#pragma once

#include "IAudioProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * AudioRouter - Manages audio signal routing between processors
 * 
 * Responsibilities:
 * - Routes input audio to selected processor (A or B)
 * - Handles smooth crossfading during A/B switches (optional)
 * - Manages buffer allocation and channel configuration
 * - Provides bypass/mute functionality
 */
class AudioRouter
{
public:
    enum class ProcessorSelection
    {
        A,
        B,
        Bypass  // Direct passthrough, no processing
    };

    AudioRouter();
    ~AudioRouter() = default;

    // Configuration
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources();

    // Routing control
    void setSelection(ProcessorSelection selection);
    ProcessorSelection getSelection() const { return currentSelection; }
    
    void setBypass(bool shouldBypass) { bypassed = shouldBypass; }
    bool isBypassed() const { return bypassed; }

    // Crossfade settings
    void setCrossfadeEnabled(bool enabled) { crossfadeEnabled = enabled; }
    void setCrossfadeDurationMs(float durationMs);
    bool isCrossfading() const { return crossfadeProgress < 1.0f; }

    // Audio processing
    void processBlock(
        juce::AudioBuffer<float>& buffer,
        juce::MidiBuffer& midiMessages,
        IAudioProcessor* processorA,
        IAudioProcessor* processorB
    );

private:
    ProcessorSelection currentSelection{ProcessorSelection::A};
    ProcessorSelection targetSelection{ProcessorSelection::A};
    
    bool bypassed{false};
    bool crossfadeEnabled{false};
    
    float crossfadeDurationMs{50.0f};
    float crossfadeSamples{0.0f};
    float crossfadeProgress{1.0f};  // 0.0 = fully A, 1.0 = fully B (or complete)
    
    double sampleRate{44100.0};
    int blockSize{512};
    
    // Temporary buffer for crossfading
    juce::AudioBuffer<float> tempBuffer;
    
    void updateCrossfade(int numSamples);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRouter)
};
