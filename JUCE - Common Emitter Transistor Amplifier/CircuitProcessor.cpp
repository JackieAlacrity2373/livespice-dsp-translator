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
, apvts(*this, nullptr, "Parameters", createParameterLayout()), Q1_amp(Nonlinear::ComponentDB::getBJTDB().getOrDefault("2N3904"))
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
    // [BETA] Optimized high-pass biquad
    *stage0_hpf.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 6366.2f);
    stage0_hpf.prepare(spec);

    // Stage 1: Transistor Gain Stage
    // [BETA] No frequency params, skipping filter init

    // Stage 2: RC Low-Pass Filter
    // [BETA] Optimized low-pass biquad
    *stage2_lpf.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 6366.2f);
    stage2_lpf.prepare(spec);

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
            // [BETA] Pattern: Passive RC High-Pass (confidence: 0.85)
            // [BETA] Optimized biquad for RC filter pattern
            // Input coupling high-pass: fc = 6366.2 Hz
            signal = stage0_hpf.processSample(signal);

            // Stage 1: Transistor Gain Stage
            // [BETA] Pattern: Passive RC Low-Pass (confidence: 1)
            // [BETA] Optimized biquad for RC filter pattern
            // No frequency parameters found, using stable implementation
            // (Gain processed at block level after sample loop)\n\n            // Nonlinear component processing
            signal = Q1_amp.processSample(signal);

            // Stage 2: RC Low-Pass Filter
            // [BETA] Pattern: Passive RC Low-Pass (confidence: 0.9)
            // [BETA] Optimized biquad for RC filter pattern
            // Low-pass biquad: fc = 6366.2 Hz
            signal = stage2_lpf.processSample(signal);

            channelData[sample] = signal;
        }
    }

    // Apply gain stages with parameter control
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    stage1_gain.process(context);
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
