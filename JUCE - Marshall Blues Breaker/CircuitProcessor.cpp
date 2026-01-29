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
, apvts(*this, nullptr, "Parameters", createParameterLayout()), D1_clipper(Nonlinear::ComponentDB::getDiodeDB().getOrDefault("1N4148"), Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f), D2_clipper(Nonlinear::ComponentDB::getDiodeDB().getOrDefault("1N4148"), Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f), D3_clipper(Nonlinear::ComponentDB::getDiodeDB().getOrDefault("1N4148"), Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f), D4_clipper(Nonlinear::ComponentDB::getDiodeDB().getOrDefault("1N4148"), Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f)
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
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 2;

    // ========================================================================
    // Initialize LiveSPICE Component Processors
    // ========================================================================

    // Stage 0: Input Buffer
    // RC High-Pass Filter: f = 15.9155 Hz
    stage0_resistor.prepare(1e+06);
    stage0_capacitor.prepare(1e-08, 0.1); // 1e-08 F with 0.1Ω ESR

    // Stage 1: Op-Amp Clipping Stage
    // Diode clipping with Shockley equation
    stage1_diode1.prepare("1N4148", 25.0); // Silicon diode, 25°C
    stage1_diode2.prepare("1N4148", 25.0);
    stage1_opamp.prepare("TL072", sampleRate); // Dual op-amp

    // Stage 2: Tone Control
    // [BETA] Tone stack filter setup
    *stage2_toneLow.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 120.0f, 0.707f, juce::Decibels::decibelsToGain(3.0f));
    *stage2_toneMid.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 1000.0f, 0.707f, juce::Decibels::decibelsToGain(-2.0f));
    *stage2_toneHigh.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 4500.0f, 0.707f, juce::Decibels::decibelsToGain(3.0f));
    stage2_toneLow.prepare(spec);
    stage2_toneMid.prepare(spec);
    stage2_toneHigh.prepare(spec);

    // Stage 3: RC Low-Pass Filter
    // [BETA] Optimized low-pass biquad
    *stage3_lpf.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 15.9155f);
    stage3_lpf.prepare(spec);

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
            // [BETA] Pattern: Tone Stack (confidence: 0.9)
            // Unknown pattern strategy: tone_stack
            // RC filter using LiveSPICE components\n            stage0_resistor.process(signal);\n            stage0_capacitor.process(signal, currentSampleRate);\n            signal = (float)stage0_capacitor.getVoltage();\n\n            // Stage 1: Op-Amp Clipping Stage
            // [BETA] Pattern: Op-Amp Diode Clipping (confidence: 0.95)
            // [BETA] Nonlinear clipper (already optimized)
            // Using component-aware diode models with Newton-Raphson
            // (Diode clipping applied after stage processing)

            // Nonlinear component processing
            signal = D1_clipper.processSample(signal);
            signal = D2_clipper.processSample(signal);
            signal = D3_clipper.processSample(signal);
            signal = D4_clipper.processSample(signal);

            // Stage 2: Tone Control
            // [BETA] Tone stack (low/mid/high shelves)
            signal = stage2_toneLow.processSample(signal);
            signal = stage2_toneMid.processSample(signal);
            signal = stage2_toneHigh.processSample(signal);

            // Stage 3: RC Low-Pass Filter
            // [BETA] Pattern: Passive RC Low-Pass (confidence: 0.9)
            // [BETA] Optimized biquad for RC filter pattern
            // Low-pass biquad: fc = 15.9155 Hz
            signal = stage3_lpf.processSample(signal);

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
