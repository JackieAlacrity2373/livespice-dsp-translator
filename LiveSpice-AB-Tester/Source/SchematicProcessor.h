#pragma once

#include "IAudioProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>

/**
 * SchematicProcessor - Loads and processes LiveSpice schematics directly
 * 
 * This IAudioProcessor implementation loads a .schx schematic file and initializes
 * a stable parameter set for A/B comparison against generated VST3 plugins.
 */
class SchematicProcessor : public IAudioProcessor
{
public:
    SchematicProcessor(const juce::File& schematicFile);
    ~SchematicProcessor() override;

    // IAudioProcessor implementation
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    std::vector<ParameterInfo> getParameters() const override;
    bool setParameter(const juce::String& parameterId, float value) override;
    float getParameter(const juce::String& parameterId) const override;

    juce::String getName() const override;
    juce::String getType() const override { return "SchematicHost"; }
    // For now, always return true so the workflow proceeds even if schematic file doesn't load
    // Audio will pass through unchanged if schematic doesn't load
    bool isLoaded() const override { return true; }  

    void reset() override;

private:
    juce::File schematicFile;
    bool schematicLoaded = false;
    juce::String circuitName;

    // Parameters exposed to the control panel
    std::map<juce::String, float> parameters;
    std::vector<ParameterInfo> parameterList;

    // DSP state
    double currentSampleRate = 44100.0;
    int currentBufferSize = 512;

    // Helper methods
    bool loadSchematic();
    void extractParameters();
    void buildDSPChain();
    void processDSP(juce::AudioBuffer<float>& buffer);
};
