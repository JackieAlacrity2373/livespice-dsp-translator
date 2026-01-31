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
#include "../../DiodeModels.h"
#include "../../TransistorModels.h"
#include "../../ComponentCharacteristicsDatabase.h"

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
    // DSP Mapping: Capacitor: 100.000000nF
    LiveSpiceDSP::ResistorProcessor stage0_resistor;
    LiveSpiceDSP::CapacitorProcessor stage0_capacitor;

    // Stage 1: RC Low-Pass Filter
    // DSP Mapping: Resistor: 100.000000kΩ
    LiveSpiceDSP::ResistorProcessor stage1_resistor;
    LiveSpiceDSP::CapacitorProcessor stage1_capacitor;

    // ========================================================================
    // APVTS - AudioProcessorValueTreeState for parameter management
    // ========================================================================
    juce::AudioProcessorValueTreeState apvts;

    // Parameter pointers for fast access
    std::atomic<float>* bypassParam = nullptr;

    // Sample rate for DSP processing
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircuitProcessor)
};
