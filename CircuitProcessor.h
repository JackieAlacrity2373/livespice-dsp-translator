/*
  ==============================================================================
    Auto-generated JUCE Audio Processor
    Generated from LiveSpice circuit analysis
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

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
    // DSP State Variables
    // Circuit stages
    juce::dsp::Gain<float> gain0;
    juce::dsp::WaveShaper<float> clipper1;
    juce::dsp::IIR::Filter<float> filter2;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircuitProcessor)
};
