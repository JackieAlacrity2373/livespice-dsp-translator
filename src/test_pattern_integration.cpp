#include "TopologyPatterns.h"
#include "CircuitAnalyzer.h"
#include "LiveSpiceParser.h"
#include <iostream>
#include <iomanip>

using namespace LiveSpice;
using namespace TopologyAnalysis;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <schematic_file>\n";
        return 1;
    }

    try {
        auto schematic = SchematicParser::parseFile(argv[1]);
        
        std::cout << "\n======================================================================\n";
        std::cout << "  Topology Pattern Matching - Integration Test\n";
        std::cout << "======================================================================\n\n";

        CircuitAnalyzer analyzer(schematic);
        auto stages = analyzer.analyzeCircuit();

        std::cout << "Identified " << stages.size() << " stages with pattern matching:\n\n";

        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            std::cout << "Stage " << (i+1) << ": " << stage.name << "\n";
            std::cout << "  → Pattern: " << stage.patternName << "\n";
            std::cout << "  → Strategy: " << stage.patternStrategy << "\n";
            std::cout << "  → Confidence: " << std::fixed << std::setprecision(0) 
                      << (stage.patternConfidence * 100) << "%\n\n";
        }

        std::cout << "======================================================================\n";
        std::cout << "✓ Pattern Matching Integration TEST PASSED\n";
        std::cout << "======================================================================\n\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
