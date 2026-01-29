#pragma once

#include "LiveSpiceParser.h"
#include "CircuitAnalyzer.h"
#include <string>
#include <sstream>
#include <vector>
#include <map>

namespace LiveSpice {

    /**
     * CircuitVisualizer - Generates visual representations of what the program extracted
     * 
     * This shows what the translation program understood from the LiveSpice file AFTER parsing
     * and analysis - not the original schematic, but the interpreted circuit structure.
     * 
     * Features:
     * - Parsed components and their extracted properties
     * - Analyzed node connectivity (what program determined connects to what)
     * - Identified circuit stages and their role
     * - DSP module mapping for each stage
     * - Potentiometer/control extraction and parameter mapping
     * - Signal flow based on program's topology analysis
     * - Troubleshooting guide (what was extracted vs what was expected)
     */
    class CircuitVisualizer {
    public:
        CircuitVisualizer(const Schematic& schematic, const CircuitAnalyzer& analyzer)
            : schematic(schematic), analyzer(analyzer) {}

        /**
         * Generate complete extracted circuit visualization with all analysis
         */
        std::string generateFullDiagram() const;

        /**
         * Generate parsed component list with extracted properties
         */
        std::string generateParsedComponents() const;

        /**
         * Generate analyzed connectivity (how program determined components connect)
         */
        std::string generateAnalyzedConnectivity() const;

        /**
         * Generate identified circuit stages and their DSP mappings
         */
        std::string generateIdentifiedStages() const;

        /**
         * Generate extracted potentiometer/control analysis
         */
        std::string generateExtractedControls() const;

        /**
         * Generate signal flow based on analyzed topology
         */
        std::string generateAnalyzedSignalFlow() const;

        /**
         * Generate troubleshooting guide (what was extracted, what might be missing)
         */
        std::string generateTroubleshootingGuide() const;

    private:
        const Schematic& schematic;
        const CircuitAnalyzer& analyzer;

        // Helper methods
        std::string componentTypeToString(ComponentType type) const;
        std::string stageTypeToString(StageType type) const;
        std::string componentTypeToSymbol(ComponentType type) const;
        std::string formatComponentValue(const std::string& value) const;
        std::string getExpectedDSPModule(StageType stage) const;
        
        std::string generateComponentDetailsTable() const;
        std::string generateNodeConnectivityTable() const;
        std::string generatePotentiometerDetailsTable() const;
        std::string generateStageBreakdown() const;
        std::string generateExtractionSummary() const;
    };

} // namespace LiveSpice
