#include "JuceDSPGenerator.h"
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <cctype>

namespace LiveSpice {

    namespace {
        struct DiodeMemberSpec {
            std::string componentName;
            std::string memberName;
            std::string partNumber;
        };
        
        struct BJTMemberSpec {
            std::string componentName;
            std::string memberName;
            std::string partNumber;
        };
        
        struct FETMemberSpec {
            std::string componentName;
            std::string memberName;
            std::string partNumber;
        };

        std::string makeSafeIdentifier(const std::string& input) {
            std::string output;
            for (char c : input) {
                if (std::isalnum(static_cast<unsigned char>(c))) {
                    output += c;
                } else {
                    output += '_';
                }
            }

            if (output.empty()) {
                output = "component";
            }

            if (std::isdigit(static_cast<unsigned char>(output.front()))) {
                output = "_" + output;
            }

            return output;
        }

        std::vector<DiodeMemberSpec> collectDiodeMembers(const std::vector<CircuitStage>& stages) {
            std::vector<DiodeMemberSpec> members;
            std::set<std::string> usedNames;

            for (size_t stageIndex = 0; stageIndex < stages.size(); ++stageIndex) {
                const auto& stage = stages[stageIndex];
                for (size_t compIndex = 0; compIndex < stage.nonlinearComponents.size(); ++compIndex) {
                    const auto& nonlinear = stage.nonlinearComponents[compIndex];
                    if (!nonlinear.diodeChar.has_value()) {
                        continue;
                    }

                    std::string baseName = makeSafeIdentifier(nonlinear.name);
                    if (baseName == "component") {
                        baseName = "diode_" + std::to_string(stageIndex) + "_" + std::to_string(compIndex);
                    }

                    std::string memberName = baseName + "_clipper";
                    if (!usedNames.insert(memberName).second) {
                        continue;
                    }

                    std::string partNumber = nonlinear.partNumber.empty() ? "1N4148" : nonlinear.partNumber;
                    members.push_back({nonlinear.name, memberName, partNumber});
                }
            }

            return members;
        }
        
        std::vector<BJTMemberSpec> collectBJTMembers(const std::vector<CircuitStage>& stages) {
            std::vector<BJTMemberSpec> members;
            std::set<std::string> usedNames;

            for (size_t stageIndex = 0; stageIndex < stages.size(); ++stageIndex) {
                const auto& stage = stages[stageIndex];
                for (size_t compIndex = 0; compIndex < stage.nonlinearComponents.size(); ++compIndex) {
                    const auto& nonlinear = stage.nonlinearComponents[compIndex];
                    if (!nonlinear.bjtChar.has_value()) {
                        continue;
                    }

                    std::string baseName = makeSafeIdentifier(nonlinear.name);
                    if (baseName == "component") {
                        baseName = "bjt_" + std::to_string(stageIndex) + "_" + std::to_string(compIndex);
                    }

                    std::string memberName = baseName + "_amp";
                    if (!usedNames.insert(memberName).second) {
                        continue;
                    }

                    std::string partNumber = nonlinear.partNumber.empty() ? "2N3904" : nonlinear.partNumber;
                    members.push_back({nonlinear.name, memberName, partNumber});
                }
            }

            return members;
        }
        
        std::vector<FETMemberSpec> collectFETMembers(const std::vector<CircuitStage>& stages) {
            std::vector<FETMemberSpec> members;
            std::set<std::string> usedNames;

            for (size_t stageIndex = 0; stageIndex < stages.size(); ++stageIndex) {
                const auto& stage = stages[stageIndex];
                for (size_t compIndex = 0; compIndex < stage.nonlinearComponents.size(); ++compIndex) {
                    const auto& nonlinear = stage.nonlinearComponents[compIndex];
                    if (!nonlinear.fetChar.has_value()) {
                        continue;
                    }

                    std::string baseName = makeSafeIdentifier(nonlinear.name);
                    if (baseName == "component") {
                        baseName = "fet_" + std::to_string(stageIndex) + "_" + std::to_string(compIndex);
                    }

                    std::string memberName = baseName + "_amp";
                    if (!usedNames.insert(memberName).second) {
                        continue;
                    }

                    std::string partNumber = nonlinear.partNumber.empty() ? "2N7000" : nonlinear.partNumber;
                    members.push_back({nonlinear.name, memberName, partNumber});
                }
            }

            return members;
        }

        bool isLikelyToneStackStage(const CircuitStage& stage) {
            if (stage.type == StageType::ToneControl) {
                return true;
            }

            if (stage.type == StageType::InputBuffer
                || stage.type == StageType::OutputBuffer
                || stage.type == StageType::HighPassFilter
                || stage.type == StageType::LowPassFilter
                || stage.type == StageType::BandPassFilter) {
                return false;
            }

            bool hasPot = false;
            int capCount = 0;

            for (const auto& comp : stage.components) {
                if (!comp) {
                    continue;
                }

                switch (comp->getType()) {
                    case ComponentType::Potentiometer:
                    case ComponentType::VariableResistor:
                        hasPot = true;
                        break;
                    case ComponentType::Capacitor:
                        ++capCount;
                        break;
                    default:
                        break;
                }

                if (hasPot && capCount >= 2) {
                    break;
                }
            }

            if (stage.patternStrategy == "tone_stack") {
                return hasPot;
            }

            if (stage.name.find("Tone Control") != std::string::npos) {
                return true;
            }

            return hasPot && capCount >= 2;
        }
    }

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

            const bool isToneControl = isLikelyToneStackStage(stage);
            if (m_useBetaFeatures && isToneControl) {
                ss << "    // [BETA] Tone stack filters (low/mid/high)\n";
                ss << "    juce::dsp::IIR::Filter<float> stage" << i << "_toneLow;\n";
                ss << "    juce::dsp::IIR::Filter<float> stage" << i << "_toneMid;\n";
                ss << "    juce::dsp::IIR::Filter<float> stage" << i << "_toneHigh;\n\n";
                continue;
            }
            
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
            
            // Beta mode: Initialize optimized filters based on pattern
            const bool isToneControl = isLikelyToneStackStage(stage);
            if (m_useBetaFeatures && isToneControl) {
                ss << "    // [BETA] Tone stack filter setup\n";
                ss << "    *stage" << i << "_toneLow.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 120.0f, 0.707f, juce::Decibels::decibelsToGain(3.0f));\n";
                ss << "    *stage" << i << "_toneMid.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 1000.0f, 0.707f, juce::Decibels::decibelsToGain(-2.0f));\n";
                ss << "    *stage" << i << "_toneHigh.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 4500.0f, 0.707f, juce::Decibels::decibelsToGain(3.0f));\n";
                ss << "    stage" << i << "_toneLow.prepare(spec);\n";
                ss << "    stage" << i << "_toneMid.prepare(spec);\n";
                ss << "    stage" << i << "_toneHigh.prepare(spec);\n\n";
                continue;
            }

            if (m_useBetaFeatures && stage.patternStrategy == "cascaded_biquad" && stage.patternConfidence >= 0.8) {
                auto fcIt = stage.dspParams.find("cutoff_frequency");
                auto hpfcIt = stage.dspParams.find("highpass_frequency");
                
                if (stage.type == StageType::LowPassFilter && fcIt != stage.dspParams.end()) {
                    double fc = fcIt->second;
                    ss << "    // [BETA] Optimized low-pass biquad\n";
                    ss << "    *stage" << i << "_lpf.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, " 
                       << fc << "f);\n";
                    ss << "    stage" << i << "_lpf.prepare(spec);\n\n";
                    
                } else if ((stage.type == StageType::HighPassFilter || stage.type == StageType::InputBuffer) && hpfcIt != stage.dspParams.end()) {
                    double fc = hpfcIt->second;
                    ss << "    // [BETA] Optimized high-pass biquad\n";
                    ss << "    *stage" << i << "_hpf.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, " 
                       << fc << "f);\n";
                    ss << "    stage" << i << "_hpf.prepare(spec);\n\n";
                } else {
                    ss << "    // [BETA] No frequency params, skipping filter init\n\n";
                }
                continue; // Skip stable initialization
            }
            
            // Stable mode: Use LiveSPICE component initialization
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
            ss << "            // DSP Mapping: " << stage.dspDescription << "\n";

            const bool isToneControl = isLikelyToneStackStage(stage);
            if (m_useBetaFeatures && isToneControl) {
                ss << "            // [BETA] Tone stack (low/mid/high shelves)\n";
                ss << "            signal = stage" << i << "_toneLow.processSample(signal);\n";
                ss << "            signal = stage" << i << "_toneMid.processSample(signal);\n";
                ss << "            signal = stage" << i << "_toneHigh.processSample(signal);\n\n";
                continue;
            }

            if (m_useBetaFeatures && !stage.patternStrategy.empty() && stage.patternConfidence >= 0.8) {
                ss << "            // [BETA] Pattern: " << stage.patternName << " (confidence: " << stage.patternConfidence << ")\n";
                ss << generatePatternSpecificCode(stage, i);
            } else {
                if (m_useBetaFeatures && !stage.patternStrategy.empty() && stage.patternConfidence < 0.8) {
                    ss << "            // [BETA] Low confidence pattern match, using stable code\n";
                }
                ss << generateStableLegacyCode(stage, i);
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
            ss << "// DSP Mapping: " << stage.dspDescription << "\n";

            if (m_useBetaFeatures) {
                if (isLikelyToneStackStage(stage)) {
                    ss << "// [BETA] Tone stack (low/mid/high shelves)\n";
                } else if (stage.patternStrategy == "cascaded_biquad" && stage.patternConfidence >= 0.8) {
                    ss << "// [BETA] Optimized IIR filter for RC pattern\n";
                } else if (stage.patternStrategy == "nonlinear_clipper") {
                    ss << "// [BETA] Nonlinear clipper pattern\n";
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

// Nonlinear component models
#include "../../DiodeModels.h"
#include "../../TransistorModels.h"
#include "../../ComponentCharacteristicsDatabase.h"

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
            
            // Beta mode: Use optimized processors based on pattern
            const bool isToneControl = isLikelyToneStackStage(stage);
            if (m_useBetaFeatures && isToneControl) {
                ss << "    // [BETA] Tone stack filters (low/mid/high)\n";
                ss << "    juce::dsp::IIR::Filter<float> stage" << i << "_toneLow;\n";
                ss << "    juce::dsp::IIR::Filter<float> stage" << i << "_toneMid;\n";
                ss << "    juce::dsp::IIR::Filter<float> stage" << i << "_toneHigh;\n";
            } else if (m_useBetaFeatures && stage.patternStrategy == "cascaded_biquad" && stage.patternConfidence >= 0.8) {
                ss << "    // [BETA] Optimized IIR filter for RC pattern\n";
                if (stage.type == StageType::LowPassFilter) {
                    ss << "    juce::dsp::IIR::Filter<float> stage" << i << "_lpf;\n";
                } else if (stage.type == StageType::HighPassFilter || stage.type == StageType::InputBuffer) {
                    ss << "    juce::dsp::IIR::Filter<float> stage" << i << "_hpf;\n";
                }
            } else {
                // Stable mode: Use LiveSPICE component models
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
            }
            ss << "\n";
        }



        auto diodeMembers = collectDiodeMembers(stages);
        auto bjtMembers = collectBJTMembers(stages);
        auto fetMembers = collectFETMembers(stages);
        
        if (!diodeMembers.empty() || !bjtMembers.empty() || !fetMembers.empty()) {
            ss << "    // ========================================================================\n";
            ss << "    // Nonlinear Component Models\n";
            ss << "    // ========================================================================\n\n";

            if (!diodeMembers.empty()) {
                ss << "    // Diode clippers\n";
                for (const auto& member : diodeMembers) {
                    ss << "    Nonlinear::DiodeClippingStage " << member.memberName << ";\n";
                }
                ss << "\n";
            }
            
            if (!bjtMembers.empty()) {
                ss << "    // BJT amplifiers\n";
                for (const auto& member : bjtMembers) {
                    ss << "    Nonlinear::BJTModelEbersMoll " << member.memberName << ";\n";
                }
                ss << "\n";
            }
            
            if (!fetMembers.empty()) {
                ss << "    // FET amplifiers\n";
                for (const auto& member : fetMembers) {
                    ss << "    Nonlinear::FETModelQuadratic " << member.memberName << ";\n";
                }
                ss << "\n";
            }
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
        
        ss << paramGenerator.generateAPVTSConstructorParam();

        auto diodeMembers = collectDiodeMembers(stages);
        auto bjtMembers = collectBJTMembers(stages);
        auto fetMembers = collectFETMembers(stages);
        
        for (const auto& member : diodeMembers) {
            ss << ", " << member.memberName
               << "(Nonlinear::ComponentDB::getDiodeDB().getOrDefault(\""
               << member.partNumber << "\"), "
               << "Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f)";
        }
        
        for (const auto& member : bjtMembers) {
            ss << ", " << member.memberName
               << "(Nonlinear::ComponentDB::getBJTDB().getOrDefault(\""
               << member.partNumber << "\"))";
        }
        
        for (const auto& member : fetMembers) {
            ss << ", " << member.memberName
               << "(Nonlinear::ComponentDB::getFETDB().getOrDefault(\""
               << member.partNumber << "\"))";
        }

        ss << "\n";
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
        std::map<std::string, std::string> diodeMemberMap;
        std::map<std::string, std::string> bjtMemberMap;
        std::map<std::string, std::string> fetMemberMap;
        
        for (const auto& member : diodeMembers) {
            diodeMemberMap[member.componentName] = member.memberName;
        }
        for (const auto& member : bjtMembers) {
            bjtMemberMap[member.componentName] = member.memberName;
        }
        for (const auto& member : fetMembers) {
            fetMemberMap[member.componentName] = member.memberName;
        }

        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            
            ss << "            // Stage " << i << ": " << stage.name << "\n";

            const bool isToneControl = isLikelyToneStackStage(stage);
            if (m_useBetaFeatures && isToneControl) {
                ss << "            // [BETA] Tone stack (low/mid/high shelves)\n";
                ss << "            signal = stage" << i << "_toneLow.processSample(signal);\n";
                ss << "            signal = stage" << i << "_toneMid.processSample(signal);\n";
                ss << "            signal = stage" << i << "_toneHigh.processSample(signal);\n\n";
                continue;
            }
            
            // Use pattern-specific or legacy code generation based on mode
            if (m_useBetaFeatures && !stage.patternStrategy.empty() && stage.patternConfidence >= 0.8) {
                ss << "            // [BETA] Pattern: " << stage.patternName << " (confidence: " << stage.patternConfidence << ")\n";
                ss << generatePatternSpecificCode(stage, i);
            } else {
                if (m_useBetaFeatures && stage.patternConfidence < 0.8) {
                    ss << "            // [BETA] Low confidence pattern match, using stable code\n";
                }
                ss << generateStableLegacyCode(stage, i);
            }

            if (!stage.nonlinearComponents.empty()) {
                bool hasNonlinear = false;
                
                // Process diodes
                for (const auto& nonlinear : stage.nonlinearComponents) {
                    if (nonlinear.diodeChar.has_value()) {
                        if (!hasNonlinear) {
                            ss << "            // Nonlinear component processing\n";
                            hasNonlinear = true;
                        }
                        const auto it = diodeMemberMap.find(nonlinear.name);
                        if (it != diodeMemberMap.end()) {
                            ss << "            signal = " << it->second << ".processSample(signal);\n";
                        }
                    }
                }
                
                // Process BJTs
                for (const auto& nonlinear : stage.nonlinearComponents) {
                    if (nonlinear.bjtChar.has_value()) {
                        if (!hasNonlinear) {
                            ss << "            // Nonlinear component processing\n";
                            hasNonlinear = true;
                        }
                        const auto it = bjtMemberMap.find(nonlinear.name);
                        if (it != bjtMemberMap.end()) {
                            ss << "            signal = " << it->second << ".processSample(signal);\n";
                        }
                    }
                }
                
                // Process FETs
                for (const auto& nonlinear : stage.nonlinearComponents) {
                    if (nonlinear.fetChar.has_value()) {
                        if (!hasNonlinear) {
                            ss << "            // Nonlinear component processing\n";
                            hasNonlinear = true;
                        }
                        const auto it = fetMemberMap.find(nonlinear.name);
                        if (it != fetMemberMap.end()) {
                            ss << "            signal = " << it->second << ".processSample(signal);\n";
                        }
                    }
                }
                
                if (hasNonlinear) {
                    ss << "\n";
                }
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

    // ============================================================================
    // BETA: Pattern-Specific Code Generation
    // ============================================================================
    
    std::string JuceDSPGenerator::generatePatternSpecificCode(const CircuitStage& stage, size_t stageIndex) const {
        std::stringstream ss;
        
        // Generate optimized code based on pattern strategy
        const bool isToneControl = isLikelyToneStackStage(stage);
        if (isToneControl) {
            // Tone control network (simple 3-band EQ)
            ss << "            // [BETA] Tone stack (low/mid/high shelves)\n";
            ss << "            signal = stage" << stageIndex << "_toneLow.processSample(signal);\n";
            ss << "            signal = stage" << stageIndex << "_toneMid.processSample(signal);\n";
            ss << "            signal = stage" << stageIndex << "_toneHigh.processSample(signal);\n\n";

        } else if (stage.patternStrategy == "cascaded_biquad") {
            // Optimized biquad filter for RC patterns
            ss << "            // [BETA] Optimized biquad for RC filter pattern\n";
            
            // Get cutoff frequency from stage parameters
            auto fcIt = stage.dspParams.find("cutoff_frequency");
            auto hpfcIt = stage.dspParams.find("highpass_frequency");
            
            if (stage.type == StageType::LowPassFilter && fcIt != stage.dspParams.end()) {
                double fc = fcIt->second;
                ss << "            // Low-pass biquad: fc = " << fc << " Hz\n";
                ss << "            signal = stage" << stageIndex << "_lpf.processSample(signal);\n\n";
                
            } else if (stage.type == StageType::HighPassFilter && hpfcIt != stage.dspParams.end()) {
                double fc = hpfcIt->second;
                ss << "            // High-pass biquad: fc = " << fc << " Hz\n";
                ss << "            signal = stage" << stageIndex << "_hpf.processSample(signal);\n\n";
                
            } else if (stage.type == StageType::InputBuffer && hpfcIt != stage.dspParams.end()) {
                // Input buffer often has high-pass coupling
                double fc = hpfcIt->second;
                ss << "            // Input coupling high-pass: fc = " << fc << " Hz\n";
                ss << "            signal = stage" << stageIndex << "_hpf.processSample(signal);\n\n";
                
            } else {
                // No frequency parameters - fall back to stable
                ss << "            // No frequency parameters found, using stable implementation\n";
                ss << generateStableLegacyCode(stage, stageIndex);
            }
            
        } else if (stage.patternStrategy == "nonlinear_clipper") {
            // Already using nonlinear diode clipping - this is the optimized version
            ss << "            // [BETA] Nonlinear clipper (already optimized)\n";
            ss << "            // Using component-aware diode models with Newton-Raphson\n";
            // Nonlinear components are added separately, just note it here
            ss << "            // (Diode clipping applied after stage processing)\n\n";
            
        } else if (stage.patternStrategy == "op_amp_gain") {
            // Optimized op-amp gain stage
            ss << "            // [BETA] Optimized op-amp gain\n";
            
            auto gainIt = stage.dspParams.find("gain_linear");
            if (gainIt != stage.dspParams.end()) {
                double gain = gainIt->second;
                ss << "            // Simple gain multiplication: " << gain << "x\n";
                ss << "            signal *= " << std::fixed << std::setprecision(6) << gain << "f;\n\n";
            } else {
                ss << "            // No gain parameter, using stable implementation\n";
                ss << generateStableLegacyCode(stage, stageIndex);
            }
            
        } else {
            // Unknown pattern - use stable code
            ss << "            // Unknown pattern strategy: " << stage.patternStrategy << "\n";
            ss << generateStableLegacyCode(stage, stageIndex);
        }
        
        return ss.str();
    }
    
    // ============================================================================
    // STABLE: Legacy Code Generation (Proven)
    // ============================================================================
    
    std::string JuceDSPGenerator::generateStableLegacyCode(const CircuitStage& stage, size_t stageIndex) const {
        std::stringstream ss;
        
        switch (stage.type) {
            case StageType::HighPassFilter:
            case StageType::LowPassFilter:
            case StageType::InputBuffer:
                ss << "            // RC filter using LiveSPICE components\\n";
                ss << "            stage" << stageIndex << "_resistor.process(signal);\\n";
                ss << "            stage" << stageIndex << "_capacitor.process(signal, currentSampleRate);\\n";
                ss << "            signal = (float)stage" << stageIndex << "_capacitor.getVoltage();\\n\\n";
                break;
                
            case StageType::GainStage:
            case StageType::OutputBuffer:
                ss << "            // (Gain processed at block level after sample loop)\\n\\n";
                break;
                
            case StageType::OpAmpClipping:
            case StageType::DiodeClipper:
                ss << "            // Diode clipper with Shockley equation\\n";
                ss << "            stage" << stageIndex << "_diode1.process(signal);\\n";
                ss << "            stage" << stageIndex << "_diode2.process(-signal);\\n";
                ss << "            double clipped = stage" << stageIndex << "_diode1.getCurrent() - stage" << stageIndex << "_diode2.getCurrent();\\n";
                ss << "            stage" << stageIndex << "_opamp.process(0.0, clipped);\\n";
                ss << "            signal = (float)stage" << stageIndex << "_opamp.getOutputVoltage();\\n\\n";
                break;
                
            default:
                ss << "            // TODO: Process with LiveSPICE component\\n\\n";
                break;
        }
        
        return ss.str();
    }

} // namespace LiveSpice


