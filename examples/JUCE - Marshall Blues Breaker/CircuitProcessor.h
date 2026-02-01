/*
  ==============================================================================
    Auto-generated JUCE Audio Processor with LiveSPICE Components
    Generated from LiveSpice circuit analysis
    Phase 6: Real-time Parameter Controls
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

// Nonlinear component models
#include "../../src/DiodeModels.h"
#include "../../src/TransistorModels.h"
#include "../../src/ComponentCharacteristicsDatabase.h"

// LiveSPICE Component Library
#include "../../third_party/livespice-components/ComponentModels.h"
#include "../../third_party/livespice-components/DSPImplementations.h"

class CircuitProcessor : public juce::AudioProcessor
{
public:
    CircuitProcessor();
    ~CircuitProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    // ========================================================================
    // Parameter Layout Creation
    // ========================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // Drive (Potentiometer)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"drive", 1},
            "Drive",
            juce::NormalisableRange<float>(
                0.0f,
                1.0f),
            0.6f));

        // Level (Potentiometer)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"level", 1},
            "Level",
            juce::NormalisableRange<float>(
                0.0f,
                1.0f),
            0.5f));

        // Tone (Potentiometer)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"tone", 1},
            "Tone",
            juce::NormalisableRange<float>(
                0.0f,
                1.0f),
            0.8f));

        // Bypass Switch (Potentiometer)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"bypass", 1},
            "Bypass",
            false));

        return layout;
    }

    // ========================================================================
    // LiveSPICE Component Processors - Real-time audio DSP
    // ========================================================================

    // Stage 0: Input Buffer
    // DSP Mapping: Capacitor: 10.000000nF
    LiveSpiceDSP::ResistorProcessor stage0_resistor;
    LiveSpiceDSP::CapacitorProcessor stage0_capacitor;

    // Stage 1: Op-Amp Clipping Stage
    // DSP Mapping: Op-Amp: Circuit.IdealOpAmp, Circuit, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null (Behavioral model)
    LiveSpiceDSP::DiodeProcessor stage1_diode1;
    LiveSpiceDSP::DiodeProcessor stage1_diode2;
    LiveSpiceDSP::OpAmpProcessor stage1_opamp;

    // Stage 2: Tone Control
    // DSP Mapping: Resistor: 1.000000MΩ
    // TODO: Add processor for Tone Control

    // Stage 3: RC Low-Pass Filter
    // DSP Mapping: Resistor: 1.000000MΩ
    LiveSpiceDSP::ResistorProcessor stage3_resistor;
    LiveSpiceDSP::CapacitorProcessor stage3_capacitor;

    // ========================================================================
    // Nonlinear Component Models
    // ========================================================================

    // Diode clippers
    Nonlinear::DiodeClippingStage D1_clipper;
    Nonlinear::DiodeClippingStage D2_clipper;
    Nonlinear::DiodeClippingStage D3_clipper;
    Nonlinear::DiodeClippingStage D4_clipper;

    // ========================================================================
    // APVTS - AudioProcessorValueTreeState for parameter management
    // ========================================================================
    juce::AudioProcessorValueTreeState apvts;

    // Parameter pointers for fast access
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* levelParam = nullptr;
    std::atomic<float>* toneParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    // Sample rate for DSP processing
    double currentSampleRate = 44100.0;

    // ========================================================================
    // DEBUG MODE - Audio Diagnostics
    // ========================================================================
    bool enableDebugLogging = false;  // Set to true to enable audio diagnostics
    int debugSampleCounter = 0;
    
    // Audio statistics for polling/timing analysis
    struct AudioStats {
        float peakLevel = 0.0f;
        float minLevel = 0.0f;
        float maxLevel = 0.0f;
        int zeroCrossingCount = 0;
        int crackleSuspectSamples = 0;  // Samples with sudden amplitude jumps
        float averageLevelInBypass = 0.0f;
    } audioStats;
    
    // Helper function for detecting polling artifacts (sudden jumps > threshold)
    inline bool isCrackleArtifact(float prevSample, float currSample, float threshold = 0.5f)
    {
        return std::abs(currSample - prevSample) > threshold;
    }

    // ========================================================================
    // NOISE REDUCTION - Anti-denormalization, DC removal, parameter smoothing
    // ========================================================================
    
    // Tone control: Proper low-pass filter with variable cutoff frequency
    std::array<float, 100> toneCoefficients;  // Lookup table for filter coefficients
    float toneFilterState = 0.0f;
    
    // Parameter smoothing to prevent clicks/pops from UI changes
    float prevParameterSmooth_drive = 1.0f;
    float prevParameterSmooth_level = 0.3f;
    float prevParameterSmooth_tone = 0.5f;  // Tone smoothing (0-1)
    
    // DC offset removal accumulator (high-pass filter)
    float dcOffsetAccumulator = 0.0f;
    
    // Helper function to pre-calculate tone control filter coefficients
    void updateToneControlCoefficients(double sampleRate)
    {
        // Pre-calculate coefficients for single-pole low-pass filter
        // Cutoff range: 500 Hz (index 0) to 20 kHz (index 99)
        const float minCutoff = 500.0f;
        const float maxCutoff = 20000.0f;
        
        for (int i = 0; i < 100; ++i)
        {
            float normalizedIndex = static_cast<float>(i) / 99.0f;
            // Logarithmic interpolation for more natural tone sweep
            float cutoff = minCutoff * std::pow(maxCutoff / minCutoff, normalizedIndex);
            
            // Single-pole low-pass filter coefficient: fc / sampleRate
            toneCoefficients[i] = (2.0f * juce::MathConstants<float>::pi * cutoff) / static_cast<float>(sampleRate);
            toneCoefficients[i] = std::min(toneCoefficients[i], 0.99f);  // Clamp to valid range
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircuitProcessor)
};
