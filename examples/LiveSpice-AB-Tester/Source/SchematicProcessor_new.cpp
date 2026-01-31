#include "SchematicProcessor.h"
#include <iostream>

SchematicProcessor::SchematicProcessor(const juce::File& schematicFile)
    : schematicFile(schematicFile)
{
    circuitName = schematicFile.getFileNameWithoutExtension();
    schematicLoaded = loadSchematic();
}

SchematicProcessor::~SchematicProcessor()
{
    releaseResources();
}

bool SchematicProcessor::loadSchematic()
{
    if (!schematicFile.existsAsFile())
    {
        juce::Logger::writeToLog("SchematicProcessor: File not found: " + schematicFile.getFullPathName());
        return false;
    }

    try
    {
        // For now, just verify the file exists and extract basic parameters
        // Full circuit analysis will be implemented in phase 2
        extractParameters();

        juce::Logger::writeToLog("SchematicProcessor: Loaded schematic '" + circuitName + 
                                "' with " + juce::String(parameterList.size()) + " parameters");
        return true;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("SchematicProcessor: Error loading schematic: " + juce::String(e.what()));
        return false;
    }
}

void SchematicProcessor::extractParameters()
{
    parameterList.clear();
    parameters.clear();

    // Standard guitar pedal parameters for comparison
    
    // Drive parameter
    {
        ParameterInfo info;
        info.id = "drive";
        info.name = "Drive";
        info.defaultValue = 0.5f;
        info.currentValue = 0.5f;
        info.label = "";
        parameterList.push_back(info);
        parameters[info.id] = info.currentValue;
        juce::Logger::writeToLog("  Parameter: Drive");
    }
    
    // Level parameter
    {
        ParameterInfo info;
        info.id = "level";
        info.name = "Level";
        info.defaultValue = 0.5f;
        info.currentValue = 0.5f;
        info.label = "";
        parameterList.push_back(info);
        parameters[info.id] = info.currentValue;
        juce::Logger::writeToLog("  Parameter: Level");
    }
    
    // Tone parameter
    {
        ParameterInfo info;
        info.id = "tone";
        info.name = "Tone";
        info.defaultValue = 0.8f;
        info.currentValue = 0.8f;
        info.label = "";
        parameterList.push_back(info);
        parameters[info.id] = info.currentValue;
        juce::Logger::writeToLog("  Parameter: Tone");
    }

    // Bypass parameter
    {
        ParameterInfo info;
        info.id = "bypass";
        info.name = "Bypass";
        info.defaultValue = 0.0f;
        info.currentValue = 0.0f;
        info.label = "";
        parameterList.push_back(info);
        parameters[info.id] = info.currentValue;
        juce::Logger::writeToLog("  Parameter: Bypass");
    }
}

void SchematicProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBufferSize = maximumExpectedSamplesPerBlock;
    buildDSPChain();
}

void SchematicProcessor::releaseResources()
{
    // TODO: Clean up DSP resources
}

void SchematicProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    // DIAGNOSTIC: Log load state occasionally
    static int processCounter2 = 0;
    if (processCounter2++ % 500 == 0)
    {
        juce::Logger::writeToLog("SchematicProcessor::processBlock - schematicLoaded=" + 
                                juce::String((int)schematicLoaded) + 
                                ", circuitName=" + circuitName);
    }

    // If schematic not loaded, pass through with NO processing (don't clear!)
    // This allows the audio to flow through even if schematic loading failed
    if (!schematicLoaded)
    {
        return;  // Pass through unchanged - audio will come through untouched
    }

    // Check bypass
    bool bypassed = parameters["bypass"] >= 0.5f;
    if (bypassed)
    {
        return;  // Pass through unchanged
    }

    // Process through DSP chain (placeholder)
    processDSP(buffer);
}

std::vector<IAudioProcessor::ParameterInfo> SchematicProcessor::getParameters() const
{
    return parameterList;
}

bool SchematicProcessor::setParameter(const juce::String& parameterId, float value)
{
    if (parameters.find(parameterId) != parameters.end())
    {
        parameters[parameterId] = juce::jlimit(0.0f, 1.0f, value);
        
        // Update parameter list with current value
        for (auto& param : const_cast<SchematicProcessor*>(this)->parameterList)
        {
            if (param.id == parameterId)
            {
                param.currentValue = parameters[parameterId];
                break;
            }
        }
        
        return true;
    }
    return false;
}

float SchematicProcessor::getParameter(const juce::String& parameterId) const
{
    auto it = parameters.find(parameterId);
    if (it != parameters.end())
        return it->second;
    return 0.0f;
}

juce::String SchematicProcessor::getName() const
{
    return circuitName;
}

void SchematicProcessor::reset()
{
    // TODO: Reset DSP state
}

void SchematicProcessor::buildDSPChain()
{
    // Initialize DSP state - this is placeholder that will do basic distortion
    juce::Logger::writeToLog("SchematicProcessor: Building DSP chain");
}

void SchematicProcessor::processDSP(juce::AudioBuffer<float>& buffer)
{
    // Implement basic distortion processing to verify signal flow
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    
    float drive = parameters["drive"];        // 0-1, controls gain before distortion
    float level = parameters["level"];        // 0-1, output level
    float tone = parameters["tone"];          // 0-1, simple low-pass filter
    
    // Log audio processing periodically
    static int processCounter = 0;
    if (processCounter++ % 100 == 0)  // MORE FREQUENT
    {
        float inputLevel = buffer.getMagnitude(0, 0, numSamples);
        juce::Logger::writeToLog("SchematicProcessor::processDSP - PROCESSING CALLED! Channels: " + juce::String(numChannels) + 
                                ", Samples: " + juce::String(numSamples) + 
                                ", InputLevel: " + juce::String(inputLevel, 4) + 
                                ", Drive: " + juce::String(drive, 2) + 
                                ", Level: " + juce::String(level, 2) + 
                                ", GainMultiplier: " + juce::String(1.0f + drive * 4.0f, 2));
    }
    
    // Apply processing
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        
        for (int i = 0; i < numSamples; ++i)
        {
            // Apply drive (gain before distortion)
            float sample = samples[i] * (1.0f + drive * 4.0f);  // 1-5x gain
            
            // Soft clipping using tanh approximation for smoother saturation
            float x = juce::jlimit(-3.0f, 3.0f, sample);
            sample = x - (x * x * x) * 0.3333f;
            
            // Apply output level
            sample *= level;
            
            samples[i] = sample;
        }
    }
}
