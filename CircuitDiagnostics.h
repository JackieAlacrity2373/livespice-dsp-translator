#pragma once

#include "LiveSpiceParser.h"
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>

namespace LiveSpice {

    /**
     * CircuitDiagnostics - Deep analysis of circuit extraction issues
     * 
     * This tool generates detailed debug output showing:
     * - Raw wire data from schematic
     * - Component positions vs wire endpoints
     * - Position matching analysis
     * - Signal path tracing attempts
     * - Gap analysis (what connections are missing)
     */
    class CircuitDiagnostics {
    public:
        CircuitDiagnostics(const Schematic& schematic)
            : schematic(schematic) {}

        /**
         * Generate comprehensive diagnostics for troubleshooting extraction issues
         */
        std::string generateDiagnosticReport() const;

        /**
         * Trace specific signal paths (e.g., Input -> Drive -> Op-Amp -> Output)
         */
        std::string traceSignalPath(const std::string& startComponent, const std::string& endComponent) const;

        /**
         * Analyze why connectivity isn't being detected
         */
        std::string analyzeConnectivityFailures() const;

        /**
         * Generate wire-by-wire mapping for manual verification
         */
        std::string generateWireMapping() const;

    private:
        const Schematic& schematic;

        struct ComponentNode {
            std::string name;
            int x, y;
            std::string type;
        };

        struct WireData {
            int x1, y1;
            int x2, y2;
            std::vector<std::string> componentsAtStart;
            std::vector<std::string> componentsAtEnd;
        };

        std::vector<ComponentNode> extractComponentNodes() const;
        std::vector<WireData> analyzeWires() const;
        std::string positionToString(int x, int y) const;
    };

} // namespace LiveSpice
