#include "JuceDSPGenerator.h"
#include <fstream>
#include <iomanip>

namespace LiveSpice {

    std::string JuceDSPGenerator::generateProcessorHeader() {
        std::stringstream ss;
        
        ss << R"(/*
  ==============================================================================
    Auto-generated JUCE Audio Processor
    Generated from LiveSpice circuit analysis
    Using LiveSPICE Component DSP Library
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

// LiveSPICE Component Library
#include "../../third_party/livespice-components/ComponentModels.h"
#include "../../third_party/livespice-components/DSPImplementations.h"

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
)";

        return ss.str();
    }

    std::string JuceDSPGenerator::generateStateVariables(const std::vector<CircuitStage>& stages) {
        std::stringstream ss;
        
        ss << "    // ========================================================================\n";
        ss << "    // LiveSPICE Component Processors - Real-time audio DSP\n";
        ss << "    // ========================================================================\n\n";
        
        // Track which processor types we need
        bool needsResistor = false;
        bool needsCapacitor = false;
        bool needsDiode = false;
        bool needsOpAmp = false;
        
        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            
            ss << "    // Stage " << i << ": " << stage.name << "\n";
            ss << "    // DSP Mapping: " << stage.dspDescription << "\n";
            
            // Determine which LiveSPICE processors to use based on stage type and DSP mapping
            switch (stage.type) {
                case StageType::HighPassFilter:
                case StageType::LowPassFilter:
                case StageType::InputBuffer: {
                    // RC filter using LiveSPICE components
                    ss << "    LiveSpiceDSP::ResistorProcessor stage" << i << "_resistor;\n";
                    ss << "    LiveSpiceDSP::CapacitorProcessor stage" << i << "_capacitor;\n";
                    needsResistor = true;
                    needsCapacitor = true;
                    break;
                }
                    
                case StageType::GainStage:
                    // Keep JUCE gain for simple amplification
                    ss << "    juce::dsp::Gain<float> stage" << i << "_gain;\n";
                    break;
                    
                case StageType::OpAmpClipping:
                case StageType::DiodeClipper: {
                    // Use LiveSPICE diode processors for clipping
                    ss << "    LiveSpiceDSP::DiodeProcessor stage" << i << "_diode1;\n";
                    ss << "    LiveSpiceDSP::DiodeProcessor stage" << i << "_diode2;\n";
                    ss << "    LiveSpiceDSP::OpAmpProcessor stage" << i << "_opamp;\n";
                    needsDiode = true;
                    needsOpAmp = true;
                    break;
                }
                    
                case StageType::OutputBuffer:
                    ss << "    juce::dsp::Gain<float> stage" << i << "_gain;\n";
                    break;
                    
                default:
                    ss << "    // TODO: Map to appropriate LiveSPICE processor\n";
                    break;
            }
            ss << "\n";
        }
        
        ss << "    // Sample rate for DSP processing\n";
        ss << "    double currentSampleRate = 44100.0;\n";
        ss << "\n    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircuitProcessor)\n";
        ss << "};\n";
        
        return ss.str();
    }

    std::string JuceDSPGenerator::generatePrepareToPlayCode(const std::vector<CircuitStage>& stages) {
        std::stringstream ss;
        
        ss << R"(void CircuitProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 2;

    // ========================================================================
    // Initialize LiveSPICE Component Processors
    // ========================================================================

)";

        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            
            ss << "    // Stage " << i << ": " << stage.name << "\n";
            
            switch (stage.type) {
                case StageType::HighPassFilter:
                case StageType::InputBuffer: {
                    auto itR = stage.dspParams.find("input_resistance");
                    auto itC = stage.dspParams.find("coupling_capacitance");
                    auto itF = stage.dspParams.find("highpass_frequency");
                    
                    double resistance = (itR != stage.dspParams.end()) ? itR->second : 100000.0;
                    double capacitance = (itC != stage.dspParams.end()) ? itC->second : 1e-8;
                    double frequency = (itF != stage.dspParams.end()) ? itF->second : 72.0;
                    
                    ss << "    // RC High-Pass Filter: f = " << frequency << " Hz\n";
                    ss << "    stage" << i << "_resistor.prepare(" << resistance << ");\n";
                    ss << "    stage" << i << "_capacitor.prepare(" << capacitance << ", 0.1); // " 
                       << capacitance << " F with 0.1Ω ESR\n\n";
                    break;
                }
                
                case StageType::LowPassFilter: {
                    auto itR = stage.dspParams.find("input_resistance");
                    auto itC = stage.dspParams.find("coupling_capacitance");
                    auto itF = stage.dspParams.find("cutoff_frequency");
                    
                    double resistance = (itR != stage.dspParams.end()) ? itR->second : 10000.0;
                    double capacitance = (itC != stage.dspParams.end()) ? itC->second : 1e-8;
                    double frequency = (itF != stage.dspParams.end()) ? itF->second : 15915.0;
                    
                    ss << "    // RC Low-Pass Filter: fc = " << frequency << " Hz\n";
                    ss << "    stage" << i << "_resistor.prepare(" << resistance << ");\n";
                    ss << "    stage" << i << "_capacitor.prepare(" << capacitance << ", 0.1);\n\n";
                    break;
                }
                
                case StageType::GainStage: {
                    auto it = stage.dspParams.find("gain_linear");
                    if (it != stage.dspParams.end()) {
                        ss << "    stage" << i << "_gain.setGainLinear(" << it->second << "f);\n";
                        ss << "    stage" << i << "_gain.prepare(spec);\n\n";
                    } else {
                        ss << "    stage" << i << "_gain.setGainLinear(1.0f);\n";
                        ss << "    stage" << i << "_gain.prepare(spec);\n\n";
                    }
                    break;
                }
                
                case StageType::OpAmpClipping:
                case StageType::DiodeClipper: {
                    ss << "    // Diode clipping with Shockley equation\n";
                    ss << "    stage" << i << "_diode1.prepare(\"1N4148\", 25.0); // Silicon diode, 25°C\n";
                    ss << "    stage" << i << "_diode2.prepare(\"1N4148\", 25.0);\n";
                    ss << "    stage" << i << "_opamp.prepare(\"TL072\", sampleRate); // Dual op-amp\n\n";
                    break;
                }
                
                case StageType::OutputBuffer: {
                    ss << "    stage" << i << "_gain.setGainLinear(0.5f); // 50% output level\n";
                    ss << "    stage" << i << "_gain.prepare(spec);\n\n";
                    break;
                }
                
                default:
                    ss << "    // TODO: Initialize LiveSPICE processor\n\n";
                    break;
            }
        }
        
        ss << "}\n\n";
        return ss.str();
    }

    std::string JuceDSPGenerator::generateProcessBlockCode(const std::vector<CircuitStage>& stages) {
        std::stringstream ss;
        
        ss << R"(void CircuitProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Check bypass parameter (bool)
    bool bypassed = bypassParam->load() >= 0.5f;
    if (bypassed)
    {
        // Bypass is ON - pass through without processing
        return;
    }

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
            
)";

        // Generate sample-by-sample processing for each stage
        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            
            ss << "            // Stage " << i << ": " << stage.name << "\n";
            
            switch (stage.type) {
                case StageType::HighPassFilter:
                case StageType::LowPassFilter:
                case StageType::InputBuffer: {
                    ss << "            // RC filter using LiveSPICE components\n";
                    ss << "            stage" << i << "_resistor.process(signal);\n";
                    ss << "            double current = stage" << i << "_resistor.getCurrent();\n";
                    ss << "            stage" << i << "_capacitor.process(signal, currentSampleRate);\n";
                    ss << "            signal = (float)stage" << i << "_capacitor.getVoltage();\n\n";
                    break;
                }
                    
                case StageType::GainStage:
                case StageType::OutputBuffer: {
                    // Use JUCE block processing for gain stages
                    ss << "            // (Gain processed at block level after sample loop)\n\n";
                    break;
                }
                    
                case StageType::OpAmpClipping:
                case StageType::DiodeClipper: {
                    ss << "            // Diode clipper with Shockley equation\n";
                    ss << "            stage" << i << "_diode1.process(signal);\n";
                    ss << "            stage" << i << "_diode2.process(-signal); // Opposite polarity\n";
                    ss << "            double clipped = stage" << i << "_diode1.getCurrent() - stage" << i << "_diode2.getCurrent();\n";
                    ss << "            stage" << i << "_opamp.process(0.0, clipped); // Op-amp recovery\n";
                    ss << "            signal = (float)stage" << i << "_opamp.getOutputVoltage();\n\n";
                    break;
                }
                    
                default:
                    ss << "            // TODO: Process with LiveSPICE component\n\n";
                    break;
            }
        }
        
        ss << R"(            channelData[sample] = signal;
        }
    }

    // Apply gain stages using JUCE block processing
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

)";

        // Apply gain stages
        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            
            if (stage.type == StageType::GainStage || stage.type == StageType::OutputBuffer) {
                ss << "    stage" << i << "_gain.process(context);\n";
            }
        }
        
        ss << "}\n\n";
        return ss.str();
    }

    std::string JuceDSPGenerator::generateProcessorImplementation() {
        std::stringstream ss;
        
        ss << R"(/*
  ==============================================================================
    Auto-generated JUCE Audio Processor Implementation
  ==============================================================================
*/

#include "CircuitProcessor.h"

CircuitProcessor::CircuitProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
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

)";

        return ss.str();
    }

    std::string JuceDSPGenerator::generateDSPStages(const std::vector<CircuitStage>& stages) {
        std::stringstream ss;
        
        ss << "// DSP Stages Generated from Circuit Analysis:\n";
        ss << "// Total stages: " << stages.size() << "\n\n";
        
        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            ss << "// Stage " << i << ": " << stage.name << "\n";
            
            if (!stage.dspParams.empty()) {
                ss << "// Parameters:\n";
                for (const auto& param : stage.dspParams) {
                    ss << "//   " << param.first << " = " << param.second << "\n";
                }
            }
            ss << "\n";
        }
        
        return ss.str();
    }

    void JuceDSPGenerator::generateJucePlugin(const std::string& outputDir, const std::string& pluginName) {
        // This will be implemented to write actual files
        // For now, we'll generate the code and print it
    }

    void JuceDSPGenerator::writePluginFiles(const std::string& pluginDir, const std::string& pluginName,
                                           const std::vector<CircuitStage>& stages, const Netlist& netlist) {
        // Generate processor header and implementation with parameter support
        std::string headerCode = generateProcessorHeaderWithParams(netlist, stages);
        std::string implCode = generateProcessorImplWithParams(netlist, stages);

        // Write header file
        std::ofstream headerFile(pluginDir + "/CircuitProcessor.h");
        if (headerFile.is_open()) {
            headerFile << headerCode;
            headerFile.close();
        }
        
        // Write implementation file
        std::ofstream implFile(pluginDir + "/CircuitProcessor.cpp");
        if (implFile.is_open()) {
            implFile << implCode;
            implFile.close();
        }
    }

    std::string JuceDSPGenerator::generateCMakeLists(const std::string& pluginName, 
                                                    const std::string& juceRelativePath) {
        std::stringstream ss;
        
        // Create a valid CMake project name (no spaces, special characters)
        std::string cmakeName = pluginName;
        for (auto& c : cmakeName) {
            if (!std::isalnum(c) && c != '_') c = '_';
        }
        
        ss << R"(cmake_minimum_required(VERSION 3.16)
project()" << cmakeName << R"( VERSION 1.0.0 LANGUAGES CXX)

# JUCE library
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/)" << juceRelativePath << R"(/JUCE")

# Add JUCE as subdirectory
add_subdirectory("${JUCE_PATH}" juce)

# Create plugin target
juce_add_plugin()" << cmakeName << R"(
    PRODUCT_NAME ")" << pluginName << R"("
    COMPANY_NAME "LiveSpice DSP"
    VERSION 1.0.0
    DESCRIPTION "Auto-generated from LiveSpice circuit schematic"
    PLUGIN_MANUFACTURER_CODE LSDP
    PLUGIN_CODE LSDP
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Add source files
target_sources()" << cmakeName << R"( PRIVATE
    CircuitProcessor.h
    CircuitProcessor.cpp)

# Link JUCE
target_link_libraries()" << cmakeName << R"( PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics)

# Set C++ standard
set_target_properties()" << cmakeName << R"( PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
)";
        
        return ss.str();
    }

    std::string JuceDSPGenerator::generateParameterDefinitions(const Netlist& netlist) {
        std::stringstream ss;
        
        ss << "// Parameter definitions from circuit components:\n\n";
        
        const auto& components = netlist.getComponents();
        
        // Generate parameters for potentiometers
        for (const auto& pair : components) {
            if (pair.second->getType() == ComponentType::Potentiometer) {
                std::string name = pair.second->getName();
                ss << "// Potentiometer: " << name << "\n";
                ss << "// This should be exposed as a plugin parameter\n\n";
            }
        }
        
        return ss.str();
    }

    // ============================================================================
    // Phase 6: Parameter-Aware Code Generation
    // ============================================================================

    std::string JuceDSPGenerator::generateProcessorHeaderWithParams(
        const Netlist& netlist, const std::vector<CircuitStage>& stages) 
    {
        std::stringstream ss;
        
        // Extract parameters from circuit
        auto parameters = paramGenerator.extractParametersFromCircuit(netlist);
        
        ss << R"(/*
  ==============================================================================
    Auto-generated JUCE Audio Processor with LiveSPICE Components
    Generated from LiveSpice circuit analysis
    Phase 6: Real-time Parameter Controls
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

// LiveSPICE Component Library
#include "../../third_party/livespice-components/ComponentModels.h"
#include "../../third_party/livespice-components/DSPImplementations.h"

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
    // ========================================================================
    // Parameter Layout Creation
    // ========================================================================
)";

        // Add parameter layout function
        ss << paramGenerator.generateParameterLayoutFunction(parameters);
        
        ss << "    // ========================================================================\n";
        ss << "    // LiveSPICE Component Processors - Real-time audio DSP\n";
        ss << "    // ========================================================================\n\n";
        
        // Generate component processors
        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            
            ss << "    // Stage " << i << ": " << stage.name << "\n";
            ss << "    // DSP Mapping: " << stage.dspDescription << "\n";
            
            switch (stage.type) {
                case StageType::HighPassFilter:
                case StageType::LowPassFilter:
                case StageType::InputBuffer:
                    ss << "    LiveSpiceDSP::ResistorProcessor stage" << i << "_resistor;\n";
                    ss << "    LiveSpiceDSP::CapacitorProcessor stage" << i << "_capacitor;\n";
                    break;
                    
                case StageType::GainStage:
                case StageType::OutputBuffer:
                    ss << "    juce::dsp::Gain<float> stage" << i << "_gain;\n";
                    break;
                    
                case StageType::OpAmpClipping:
                case StageType::DiodeClipper:
                    ss << "    LiveSpiceDSP::DiodeProcessor stage" << i << "_diode1;\n";
                    ss << "    LiveSpiceDSP::DiodeProcessor stage" << i << "_diode2;\n";
                    ss << "    LiveSpiceDSP::OpAmpProcessor stage" << i << "_opamp;\n";
                    break;
                    
                default:
                    ss << "    // TODO: Add processor for " << stage.name << "\n";
                    break;
            }
            ss << "\n";
        }
        
        ss << "    // ========================================================================\n";
        ss << "    // APVTS - AudioProcessorValueTreeState for parameter management\n";
        ss << "    // ========================================================================\n";
        ss << paramGenerator.generateAPVTSDeclaration();
        ss << "\n";
        ss << paramGenerator.generateParameterPointers(parameters);
        
        ss << "    // Sample rate for DSP processing\n";
        ss << "    double currentSampleRate = 44100.0;\n";
        ss << "\n    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircuitProcessor)\n";
        ss << "};\n";
        
        return ss.str();
    }

    std::string JuceDSPGenerator::generateProcessorImplWithParams(
        const Netlist& netlist, const std::vector<CircuitStage>& stages)
    {
        std::stringstream ss;
        
        auto parameters = paramGenerator.extractParametersFromCircuit(netlist);
        
        ss << R"(/*
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
)";
        
        ss << paramGenerator.generateAPVTSConstructorParam() << "\n";
        ss << "{\n";
        ss << paramGenerator.generateConstructorInit(parameters);
        ss << "}\n\n";
        
        ss << R"(CircuitProcessor::~CircuitProcessor()
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

)";

        // Generate prepareToPlay with processors
        ss << generatePrepareToPlayCode(stages);
        
        // Generate processBlock with parameter usage
        ss << "void CircuitProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)\n{\n";
        ss << "    juce::ScopedNoDenormals noDenormals;\n";
        ss << "    auto totalNumInputChannels  = getTotalNumInputChannels();\n";
        ss << "    auto totalNumOutputChannels = getTotalNumOutputChannels();\n\n";
        ss << "    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)\n";
        ss << "        buffer.clear (i, 0, buffer.getNumSamples());\n\n";
        
        // Add parameter value loading
        ss << paramGenerator.generateParameterUsageExample(parameters);
        
        ss << R"(    // ========================================================================
    // LiveSPICE Component-Based DSP Processing
    // Sample-by-sample processing for accurate component modeling
    // ========================================================================

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float signal = channelData[sample];
            
)";

        // Generate stage processing with parameter influence
        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            
            ss << "            // Stage " << i << ": " << stage.name << "\n";
            
            switch (stage.type) {
                case StageType::HighPassFilter:
                case StageType::LowPassFilter:
                case StageType::InputBuffer:
                    ss << "            // RC filter using LiveSPICE components\n";
                    ss << "            stage" << i << "_resistor.process(signal);\n";
                    ss << "            stage" << i << "_capacitor.process(signal, currentSampleRate);\n";
                    ss << "            signal = (float)stage" << i << "_capacitor.getVoltage();\n\n";
                    break;
                    
                case StageType::GainStage:
                case StageType::OutputBuffer:
                    ss << "            // (Gain processed at block level after sample loop)\n\n";
                    break;
                    
                case StageType::OpAmpClipping:
                case StageType::DiodeClipper:
                    ss << "            // Diode clipper with Shockley equation\n";
                    ss << "            stage" << i << "_diode1.process(signal);\n";
                    ss << "            stage" << i << "_diode2.process(-signal);\n";
                    ss << "            double clipped = stage" << i << "_diode1.getCurrent() - stage" << i << "_diode2.getCurrent();\n";
                    ss << "            stage" << i << "_opamp.process(0.0, clipped);\n";
                    ss << "            signal = (float)stage" << i << "_opamp.getOutputVoltage();\n\n";
                    break;
                    
                default:
                    ss << "            // TODO: Process with LiveSPICE component\n\n";
                    break;
            }
        }
        
        ss << R"(            channelData[sample] = signal;
        }
    }

    // Apply gain stages with parameter control
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

)";

        // Apply gains with parameters
        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            
            if (stage.type == StageType::GainStage || stage.type == StageType::OutputBuffer) {
                // Check if we have a parameter for this stage
                bool hasParam = false;
                for (const auto& param : parameters) {
                    if (param.id.find("drive") != std::string::npos || 
                        param.id.find("level") != std::string::npos) {
                        ss << "    // Apply " << param.name << " parameter\n";
                        ss << "    stage" << i << "_gain.setGainLinear(" << param.id << "Value);\n";
                        hasParam = true;
                        break;
                    }
                }
                if (!hasParam) {
                    ss << "    stage" << i << "_gain.process(context);\n";
                }
            }
        }
        
        ss << "}\n\n";
        
        ss << R"(void CircuitProcessor::releaseResources()
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
)";
        
        return ss.str();
    }

} // namespace LiveSpice


