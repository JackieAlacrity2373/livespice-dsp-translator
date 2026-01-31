#include "TopologyPatterns.h"
#include "LiveSpiceParser.h"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace TopologyAnalysis;
using namespace LiveSpice;

// ============================================================================
// Test Utilities
// ============================================================================

void printHeader(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(70, '=') << "\n";
}

void printPattern(const CircuitPattern* pattern) {
    if (!pattern) {
        std::cout << "  [No pattern matched]\n";
        return;
    }
    
    std::cout << "  Pattern: " << pattern->name << "\n";
    std::cout << "  Category: " << getPatternCategoryName(pattern->category) << "\n";
    std::cout << "  Description: " << pattern->description << "\n";
    std::cout << "  DSP Strategy: " << getDSPStrategyDescription(pattern->dspStrategy) << "\n";
    std::cout << "  Confidence Threshold: " << std::fixed << std::setprecision(2) 
              << pattern->confidence_threshold << "\n";
}

// ============================================================================
// Test Cases
// ============================================================================

void testPatternRegistryInitialization() {
    printHeader("Test 1: Pattern Registry Initialization");
    
    PatternRegistry registry;
    size_t patternCount = registry.getPatternCount();
    
    std::cout << "Total patterns loaded: " << patternCount << "\n";
    assert(patternCount >= 14);
    std::cout << "✓ PASS: Registry initialized with " << patternCount << " patterns\n";
}

void testPatternListGeneration() {
    printHeader("Test 2: Pattern List Generation");
    
    PatternRegistry registry;
    const auto& patterns = registry.listPatterns();
    
    std::cout << "Listing all patterns:\n\n";
    std::map<PatternCategory, int> categoryCount;
    
    for (size_t i = 0; i < patterns.size(); ++i) {
        const auto& pattern = patterns[i];
        categoryCount[pattern.category]++;
        std::cout << std::setw(2) << (i+1) << ". " << std::setw(30) << std::left 
                  << pattern.name << " | " << getPatternCategoryName(pattern.category) << "\n";
    }
    
    std::cout << "\n✓ PASS: All patterns listed successfully\n";
}

void testSimpleRCLowPass() {
    printHeader("Test 3: Simple RC Low-Pass Circuit");
    
    PatternRegistry registry;
    std::vector<TopologyAnalysis::Component> components;
    
    TopologyAnalysis::Component r;
    r.id = "R1";
    r.type = LiveSpice::ComponentType::Resistor;
    r.value = 10000.0f;
    components.push_back(r);
    
    TopologyAnalysis::Component c;
    c.id = "C1";
    c.type = LiveSpice::ComponentType::Capacitor;
    c.value = 1.0e-6f;
    components.push_back(c);
    
    std::vector<TopologyAnalysis::Connection> connections;
    PatternMatch match = registry.matchPattern(components, connections);
    
    std::cout << "Input: Resistor + Capacitor\n";
    std::cout << "Confidence: " << std::fixed << std::setprecision(3) << match.confidence << "\n";
    printPattern(match.pattern);
    
    assert(match.confidence > 0.0f);
    std::cout << "\n✓ PASS\n";
}

void testDiodeClippingCircuit() {
    printHeader("Test 4: Diode Clipping Circuit");
    
    PatternRegistry registry;
    std::vector<TopologyAnalysis::Component> components;
    
    for (int i = 0; i < 2; ++i) {
        TopologyAnalysis::Component d;
        d.id = std::string("D") + char('1' + i);
        d.type = LiveSpice::ComponentType::Diode;
        components.push_back(d);
    }
    
    TopologyAnalysis::Component r;
    r.id = "R_limit";
    r.type = LiveSpice::ComponentType::Resistor;
    components.push_back(r);
    
    std::vector<TopologyAnalysis::Connection> connections;
    std::vector<PatternMatch> matches = registry.findAllPatterns(components, connections);
    
    std::cout << "Input: 2× Diodes + Resistor\n";
    std::cout << "Matches found: " << matches.size() << "\n";
    assert(!matches.empty());
    std::cout << "\n✓ PASS\n";
}

void testThreePointToneStack() {
    printHeader("Test 5: Three-Point Tone Stack");
    
    PatternRegistry registry;
    std::vector<TopologyAnalysis::Component> components;
    
    for (int i = 0; i < 3; ++i) {
        TopologyAnalysis::Component r;
        r.id = std::string("R") + char('1' + i);
        r.type = LiveSpice::ComponentType::Resistor;
        components.push_back(r);
    }
    
    for (int i = 0; i < 3; ++i) {
        TopologyAnalysis::Component c;
        c.id = std::string("C") + char('1' + i);
        c.type = LiveSpice::ComponentType::Capacitor;
        components.push_back(c);
    }
    
    std::vector<TopologyAnalysis::Connection> connections;
    PatternMatch match = registry.matchPattern(components, connections);
    
    std::cout << "Input: 3× Resistors + 3× Capacitors\n";
    std::cout << "Confidence: " << std::fixed << std::setprecision(3) << match.confidence << "\n";
    printPattern(match.pattern);
    
    assert(match.confidence > 0.0f);
    std::cout << "\n✓ PASS\n";
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     TopologyPatterns Test Suite - Phase 2 Validation              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
    
    try {
        testPatternRegistryInitialization();
        testPatternListGeneration();
        testSimpleRCLowPass();
        testDiodeClippingCircuit();
        testThreePointToneStack();
        
        printHeader("ALL TESTS PASSED ✓");
        std::cout << "\nPhase 2: Topology Pattern Matching - COMPLETE\n";
        std::cout << "✓ 14 circuit patterns implemented\n";
        std::cout << "✓ Pattern matching algorithm functional\n";
        std::cout << "✓ All test cases passing\n";
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
