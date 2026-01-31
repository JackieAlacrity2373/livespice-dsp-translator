#include "ProcessorFactory.h"
#include "CircuitProcessors/MXRDistortionProcessor.h"
#include "SchematicProcessor.h"
#include "Logging.h"

ProcessorFactory::ProcessorType ProcessorFactory::detectType(const juce::String& identifier)
{
    // Check if it's a file path
    juce::File file(identifier);
    if (file.existsAsFile())
    {
        if (identifier.endsWithIgnoreCase(".vst3"))
            return ProcessorType::VST3Plugin;
        if (identifier.endsWithIgnoreCase(".schx"))
            return ProcessorType::NativeDSP;  // Schematic files are treated as native DSP
    }
    
    // Check if it's a native DSP name
    auto availableCircuits = getAvailableNativeDSP();
    if (availableCircuits.contains(identifier, true))
        return ProcessorType::NativeDSP;
    
    return ProcessorType::Unknown;
}

std::unique_ptr<IAudioProcessor> ProcessorFactory::create(const juce::String& identifier)
{
    auto type = detectType(identifier);
    
    switch (type)
    {
        case ProcessorType::VST3Plugin:
            return createVST3(identifier);
            
        case ProcessorType::NativeDSP:
            return createNativeDSP(identifier);
            
        default:
            logDebug("ProcessorFactory: Unknown processor type for " + identifier, LogLevel::ERROR);
            return nullptr;
    }
}

std::unique_ptr<IAudioProcessor> ProcessorFactory::createVST3(const juce::String& pluginPath)
{
    auto wrapper = std::make_unique<PluginHostWrapper>();
    
    if (wrapper->loadPlugin(pluginPath))
    {
        logDebug("ProcessorFactory: Created VST3 processor - " + wrapper->getName(), LogLevel::INFO);
        return wrapper;
    }
    
    logDebug("ProcessorFactory: Failed to load VST3 from " + pluginPath, LogLevel::ERROR);
    return nullptr;
}

std::unique_ptr<IAudioProcessor> ProcessorFactory::createNativeDSP(const juce::String& circuitName)
{
    // Check if it's a schematic file path
    juce::File schematicFile(circuitName);
    if (schematicFile.existsAsFile() && circuitName.endsWithIgnoreCase(".schx"))
    {
        auto processor = std::make_unique<SchematicProcessor>(schematicFile);
        if (processor->isLoaded())
        {
            logDebug("ProcessorFactory: Created SchematicProcessor - " + schematicFile.getFileNameWithoutExtension(), LogLevel::INFO);
            return processor;
        }
        logDebug("ProcessorFactory: Failed to load schematic from " + circuitName, LogLevel::ERROR);
        return nullptr;
    }

    // Check for named circuits
    if (circuitName.equalsIgnoreCase("MXR Distortion+") || 
        circuitName.equalsIgnoreCase("MXR") ||
        circuitName.equalsIgnoreCase("MXRDistortion"))
    {
        auto processor = std::make_unique<CircuitProcessorWrapper<MXRDistortionProcessor>>();
        logDebug("ProcessorFactory: Created native DSP - MXR Distortion+", LogLevel::INFO);
        return processor;
    }
    
    // Add more circuits here as they're integrated
    /*
    else if (circuitName.equalsIgnoreCase("Boss SD-1"))
    {
        return std::make_unique<CircuitProcessorWrapper<BossSD1Processor>>();
    }
    */
    
    logDebug("ProcessorFactory: Unknown native DSP circuit - " + circuitName, LogLevel::ERROR);
    return nullptr;
}

juce::StringArray ProcessorFactory::getAvailableNativeDSP()
{
    juce::StringArray circuits;
    circuits.add("MXR Distortion+");
    // Add more as integrated:
    // circuits.add("Boss SD-1");
    // circuits.add("Marshall Blues Breaker");
    // etc.
    return circuits;
}
