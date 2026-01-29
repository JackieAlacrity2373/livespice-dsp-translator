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
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

// LiveSPICE Component Library
#include "../third_party/livespice-components/ComponentModels.h"
#include "../third_party/livespice-components/DSPImplementations.h"

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

        // Drive (Variable Resistor)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"drive", 1},
            "Drive",
            juce::NormalisableRange<float>(0.0f, 1.0f),
            0.5f));

        // Level (Potentiometer)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"level", 1},
            "Level",
            juce::NormalisableRange<float>(0.0f, 1.0f),
            0.5f));

        // Tone (Potentiometer)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"tone", 1},
            "Tone",
            juce::NormalisableRange<float>(0.0f, 1.0f),
            1.0f));

        return layout;
    }

    // ========================================================================
    // LiveSPICE Component Processors - Real-time audio DSP
    // ========================================================================

    // Stage 0: Input Buffer
    // DSP Mapping: Capacitor: 18.000000nF
    LiveSpiceDSP::ResistorProcessor stage0_resistor;
    LiveSpiceDSP::CapacitorProcessor stage0_capacitor;

    // Stage 1: Op-Amp Clipping Stage
    // DSP Mapping: Op-Amp: Circuit.IdealOpAmp, Circuit, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null (Behavioral model)
    LiveSpiceDSP::DiodeProcessor stage1_diode1;
    LiveSpiceDSP::DiodeProcessor stage1_diode2;
    LiveSpiceDSP::OpAmpProcessor stage1_opamp;

    // Stage 2: RC Low-Pass Filter
    // DSP Mapping: Resistor: 100.000000kΩ
    LiveSpiceDSP::ResistorProcessor stage2_resistor;
    LiveSpiceDSP::CapacitorProcessor stage2_capacitor;

    // ========================================================================
    // APVTS - AudioProcessorValueTreeState for parameter management
    // ========================================================================
    juce::AudioProcessorValueTreeState apvts;

    // Parameter pointers for fast access
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* levelParam = nullptr;
    std::atomic<float>* toneParam = nullptr;

    // Sample rate for DSP processing
    double currentSampleRate = 44100.0;

    // Make APVTS accessible to editor
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircuitProcessor)
};
