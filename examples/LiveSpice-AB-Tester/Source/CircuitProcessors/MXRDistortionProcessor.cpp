#include "MXRDistortionProcessor.h"

MXRDistortionProcessor::MXRDistortionProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    driveParam = apvts.getRawParameterValue("drive");
    levelParam = apvts.getRawParameterValue("level");
    bypassParam = apvts.getRawParameterValue("bypass");
}

juce::AudioProcessorValueTreeState::ParameterLayout MXRDistortionProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drive", 1},
        "Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"level", 1},
        "Level",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"bypass", 1},
        "Bypass",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));

    return layout;
}

void MXRDistortionProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    // Input high-pass filter (removes DC offset, ~16 Hz cutoff)
    inputFilter.prepare(spec);
    *inputFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 16.0f);

    // Distortion clipper (soft clipping transfer function)
    clipper.prepare(spec);
    clipper.functionToUse = [](float x) 
    {
        // Soft clipping with tanh approximation
        return std::tanh(x * 2.0f) * 0.5f;
    };

    // Output low-pass filter (~16 kHz cutoff for smoothness)
    outputFilter.prepare(spec);
    *outputFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 16000.0f);

    // Output gain
    outputGain.prepare(spec);
}

void MXRDistortionProcessor::releaseResources()
{
}

void MXRDistortionProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get parameter values
    float bypass = bypassParam->load();
    
    // If bypassed, pass through without processing
    if (bypass > 0.5f)
    {
        return;  // Output stays as input
    }
    
    float drive = driveParam->load();
    float level = levelParam->load();

    // Create DSP context
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Input filter
    inputFilter.process(context);

    // Apply drive gain
    juce::dsp::Gain<float> driveGain;
    driveGain.setGainLinear(1.0f + drive * 9.0f);  // 1x to 10x gain
    driveGain.process(context);

    // Clipping stage
    clipper.process(context);

    // Output filter
    outputFilter.process(context);

    // Output level
    outputGain.setGainLinear(level);
    outputGain.process(context);
}

void MXRDistortionProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MXRDistortionProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}
