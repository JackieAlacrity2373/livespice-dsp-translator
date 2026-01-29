/*
  ==============================================================================
    Auto-generated JUCE Audio Processor Implementation
    Phase 6: Real-time Parameter Controls
  ==============================================================================
*/

#include "CircuitProcessor.h"

CircuitProcessor::CircuitProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
, apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize parameter pointers
    bypassParam = apvts.getRawParameterValue("bypass");
}

CircuitProcessor::~CircuitProcessor()
{
}

const juce::String CircuitProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CircuitProcessor::acceptsMidi() const
{
    return false;
}

bool CircuitProcessor::producesMidi() const
{
    return false;
}

bool CircuitProcessor::isMidiEffect() const
{
    return false;
}

double CircuitProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CircuitProcessor::getNumPrograms()
{
    return 1;
}

int CircuitProcessor::getCurrentProgram()
{
    return 0;
}

void CircuitProcessor::setCurrentProgram (int)
{
}

const juce::String CircuitProcessor::getProgramName (int)
{
    return {};
}

void CircuitProcessor::changeProgramName (int, const juce::String&)
{
}

void CircuitProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 2;

    // ========================================================================
    // Initialize LiveSPICE Component Processors
    // ========================================================================

    // Stage 0: Input Buffer
    // RC High-Pass Filter: f = 15.9155 Hz
    stage0_resistor.prepare(100000);
    stage0_capacitor.prepare(1e-07, 0.1); // 1e-07 F with 0.1Ω ESR

    // Stage 1: RC Low-Pass Filter
    // RC Low-Pass Filter: fc = 15.9155 Hz
    stage1_resistor.prepare(10000);
    stage1_capacitor.prepare(1e-08, 0.1);

}

void CircuitProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Get current parameter values
    float bypassValue = bypassParam->load();

    // ========================================================================
    // LiveSPICE Component-Based DSP Processing
    // Sample-by-sample processing for accurate component modeling
    // ========================================================================

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float signal = channelData[sample];
            
            // Stage 0: Input Buffer
            // RC filter using LiveSPICE components
            stage0_resistor.process(signal);
            stage0_capacitor.process(signal, currentSampleRate);
            signal = (float)stage0_capacitor.getVoltage();

            // Stage 1: RC Low-Pass Filter
            // RC filter using LiveSPICE components
            stage1_resistor.process(signal);
            stage1_capacitor.process(signal, currentSampleRate);
            signal = (float)stage1_capacitor.getVoltage();

            channelData[sample] = signal;
        }
    }

    // Apply gain stages with parameter control
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

}

void CircuitProcessor::releaseResources()
{
    // Resources released automatically
}

juce::AudioProcessorEditor* CircuitProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

bool CircuitProcessor::hasEditor() const
{
    return true;
}

void CircuitProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CircuitProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CircuitProcessor();
}
