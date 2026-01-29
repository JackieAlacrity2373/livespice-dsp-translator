#include "SchematicProcessor.h"
#include "../../../LiveSpiceParser.h"
#include "../../../CircuitAnalyzer.h"
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
        // Parse the schematic using existing parser
        auto schematicPath = schematicFile.getFullPathName().toStdString();
        schematic = std::make_unique<Schematic>(
            LiveSpice::SchematicParser::parseFile(schematicPath)
        );

        // Analyze the circuit
        LiveSpice::CircuitAnalyzer analyzer(*schematic);
        stages = analyzer.analyzeCircuit();

        // Extract parameters from components
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

    if (!schematic)
        return;

    const auto& netlist = schematic->getNetlist();

    // Extract potentiometers and variable resistors as parameters
    for (const auto& pair : netlist.getComponents())
    {
        auto comp = pair.second;
        
        if (comp->getType() == LiveSpice::ComponentType::Potentiometer ||
            comp->getType() == LiveSpice::ComponentType::VariableResistor)
        {
            ParameterInfo info;
            info.id = comp->getName();
            info.name = comp->getName();
            info.defaultValue = 0.5f;  // Middle position
            info.currentValue = 0.5f;
            info.label = "";

            parameterList.push_back(info);
            parameters[info.id] = info.currentValue;

            juce::Logger::writeToLog("  Parameter: " + info.name);
        }
    }

    // Always add bypass parameter
    ParameterInfo bypassParam;
    bypassParam.id = "bypass";
    bypassParam.name = "Bypass";
    bypassParam.defaultValue = 0.0f;  // Not bypassed
    bypassParam.currentValue = 0.0f;
    bypassParam.label = "";

    parameterList.push_back(bypassParam);
    parameters["bypass"] = bypassParam.currentValue;
    juce::Logger::writeToLog("  Parameter: Bypass");
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

    if (!schematicLoaded)
    {
        buffer.clear();
        return;
    }

    // Check bypass
    bool bypassed = parameters["bypass"] >= 0.5f;
    if (bypassed)
    {
        return;  // Pass through unchanged
    }

    // Process through DSP chain
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
    // TODO: Build actual DSP chain from stages
    // For now, this is a placeholder
}

void SchematicProcessor::processDSP(juce::AudioBuffer<float>& buffer)
{
    // TODO: Implement actual DSP processing
    // For now, this is pass-through
    juce::ignoreUnused(buffer);
}
