#pragma once

#include "CircuitAnalyzer.h"
#include "ComponentDSPMapper.h"
#include "ParameterGenerator.h"
#include <string>
#include <sstream>
#include <vector>

namespace LiveSpice {

    // ============================================================================
    // JUCE DSP Code Generator - Converts circuit analysis to JUCE DSP code
    // ============================================================================
    class JuceDSPGenerator {
    public:
        JuceDSPGenerator() {}

        // Generate complete JUCE plugin processor code
        std::string generateProcessorHeader();
        std::string generateProcessorImplementation();
        
        // Generate DSP processing blocks
        std::string generateDSPStages(const std::vector<CircuitStage>& stages);
        
        // Generate parameter definitions
        std::string generateParameterDefinitions(const Netlist& netlist);
        
        // Generate the complete plugin files
        void generateJucePlugin(const std::string& outputDir, const std::string& pluginName);
        
        // Write generated files to disk
        void writePluginFiles(const std::string& pluginDir, const std::string& pluginName, 
                            const std::vector<CircuitStage>& stages, const Netlist& netlist);
        
        // Generate CMakeLists.txt for JUCE compilation
        std::string generateCMakeLists(const std::string& pluginName, const std::string& juceRelativePath);
        
        // Helper methods for code generation
        std::string generateFilterCode(const CircuitStage& stage);
        std::string generateGainCode(const CircuitStage& stage);
        std::string generateClippingCode(const CircuitStage& stage);
        std::string generateToneControlCode(const CircuitStage& stage);
        
        // Generate JUCE-specific code components
        std::string generateProcessBlockCode(const std::vector<CircuitStage>& stages);
        std::string generateStateVariables(const std::vector<CircuitStage>& stages);
        std::string generatePrepareToPlayCode(const std::vector<CircuitStage>& stages);
        
        // Phase 6: Parameter generation with APVTS
        std::string generateProcessorHeaderWithParams(const Netlist& netlist, const std::vector<CircuitStage>& stages);
        std::string generateProcessorImplWithParams(const Netlist& netlist, const std::vector<CircuitStage>& stages);

    private:
        ParameterGenerator paramGenerator;
    };

} // namespace LiveSpice
