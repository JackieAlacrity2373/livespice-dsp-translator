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
    driveParam = apvts.getRawParameterValue("drive");
    levelParam = apvts.getRawParameterValue("level");
    toneParam = apvts.getRawParameterValue("tone");
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
    // TODO: Initialize DSP processors when implemented
}

void CircuitProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Get current parameter values
    float driveValue = driveParam->load();
    float levelValue = levelParam->load();
    float toneValue = toneParam->load();
    bool bypassed = bypassParam->load() >= 0.5f;

    // Check bypass
    if (bypassed)
    {
        // Bypass is ON - pass through without processing
        return;
    }

    // TODO: Implement actual DSP processing using parameters
    // For now, the audio passes through unchanged
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
