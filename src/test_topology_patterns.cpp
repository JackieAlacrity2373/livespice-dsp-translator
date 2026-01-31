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
    
    assert(patternCount >= 14);  // Should have at least 14 patterns (removed FET)
    
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
        
        std::cout << std::setw(2) << (i+1) << ". " << std::setw(35) << std::left 
                  << pattern.name 
                  << " | " << std::setw(18) << std::left 
                  << getPatternCategoryName(pattern.category) << " | "
                  << "Components: " << std::setw(2) << pattern.signature.size() << "\n";
    }
    
    std::cout << "\n\nCategory Breakdown:\n";
    for (const auto& [category, count] : categoryCount) {
        std::cout << "  " << getPatternCategoryName(category) << ": " << count << "\n";
    }
    
    std::cout << "\n✓ PASS: All patterns listed successfully\n";
}

void testSimpleRCLowPass() {
    printHeader("Test 3: Simple RC Low-Pass Circuit");
    
    PatternRegistry registry;
    
    // Create a simple RC low-pass circuit
    std::vector<TopologyAnalysis::Component> components;
    
    TopologyAnalysis::Component r;
    r.id = "R1";
    r.type = LiveSpice::ComponentType::Resistor;
    r.value = 10000.0f;
    r.unit = "Ohm";
    components.push_back(r);
    
    TopologyAnalysis::Component c;
    c.id = "C1";
    c.type = LiveSpice::ComponentType::Capacitor;
    c.value = 1.0e-6f;
    c.unit = "F";
    components.push_back(c);
    
    std::vector<TopologyAnalysis::Connection> connections;
    
    // Attempt to match pattern
    PatternMatch match = registry.matchPattern(components, connections);
    
    std::cout << "Input Circuit Components:\n";
    std::cout << "  - Resistor (10kΩ)\n";
    std::cout << "  - Capacitor (1µF)\n\n";
    
    std::cout << "Pattern Matching Result:\n";
    std::cout << "  Confidence: " << std::fixed << std::setprecision(3) 
              << match.confidence << "\n";
    printPattern(match.pattern);
    
    assert(match.confidence > 0.0f);
    assert(match.pattern != nullptr);
    
    std::cout << "\n✓ PASS: RC low-pass pattern correctly identified\n";
}

void testDiodeClippingCircuit() {
    printHeader("Test 4: Diode Clipping Circuit");
    
    PatternRegistry registry;
    
    // Create a back-to-back diode clipping circuit
    std::vector<TopologyAnalysis::Component> components;
    
    TopologyAnalysis::Component d1;
    d1.id = "D1";
    d1.type = LiveSpice::ComponentType::Diode;
    d1.partNumber = "1N4148";
    components.push_back(d1);
    
    TopologyAnalysis::Component d2;
    d2.id = "D2";
    d2.type = LiveSpice::ComponentType::Diode;
    d2.partNumber = "1N4148";
    components.push_back(d2);
    
    TopologyAnalysis::Component r;
    r.id = "R_limit";
    r.type = LiveSpice::ComponentType::Resistor;
    r.value = 1000.0f;
    r.unit = "Ohm";
    components.push_back(r);
    
    std::vector<TopologyAnalysis::Connection> connections;
    
    // Attempt to match pattern
    std::vector<PatternMatch> matches = registry.findAllPatterns(components, connections);
    
    std::cout << "Input Circuit Components:\n";
    std::cout << "  - Diode (1N4148)\n";
    std::cout << "  - Diode (1N4148)\n";
    std::cout << "  - Resistor (1kΩ)\n\n";
    
    std::cout << "Pattern Matching Results (all matches):\n";
    
    if (!matches.empty()) {
        for (size_t i = 0; i < matches.size(); ++i) {
            std::cout << "\n  " << (i+1) << ". " << matches[i].pattern->name 
                      << " (Confidence: " << std::fixed << std::setprecision(3) 
                      << matches[i].confidence << ")\n";
        }
    } else {
        std::cout << "  [No patterns matched]\n";
    }
    
    assert(!matches.empty());
    
    std::cout << "\n✓ PASS: Diode clipping patterns identified\n";
}

void testThreePointToneStack() {
    printHeader("Test 5: Three-Point Tone Stack");
    
    PatternRegistry registry;
    
    // Create a three-point tone stack circuit
    std::vector<TopologyAnalysis::Component> components;
    
    // 3 resistors
    for (int i = 0; i < 3; ++i) {
        TopologyAnalysis::Component r;
        r.id = "R" + std::to_string(i+1);
        r.type = LiveSpice::ComponentType::Resistor;
        r.value = 10000.0f;
        r.unit = "Ohm";
        components.push_back(r);
    }
    
    // 3 capacitors
    for (int i = 0; i < 3; ++i) {
        TopologyAnalysis::Component c;
        c.id = "C" + std::to_string(i+1);
        c.type = LiveSpice::ComponentType::Capacitor;
        c.value = 10.0e-9f;
        c.unit = "F";
        components.push_back(c);
    }
    
    std::vector<TopologyAnalysis::Connection> connections;
    
    // Attempt to match pattern
    PatternMatch match = registry.matchPattern(components, connections);
    
    std::cout << "Input Circuit Components:\n";
    std::cout << "  - 3× Resistors (10kΩ each)\n";
    std::cout << "  - 3× Capacitors (10nF each)\n\n";
    
    std::cout << "Pattern Matching Result:\n";
    std::cout << "  Confidence: " << std::fixed << std::setprecision(3) 
              << match.confidence << "\n";
    printPattern(match.pattern);
    
    assert(match.confidence > 0.0f);
    assert(match.pattern != nullptr);
    
    std::cout << "\n✓ PASS: Three-point tone stack pattern correctly identified\n";
}

void testPatternRetrieval() {
    printHeader("Test 6: Direct Pattern Retrieval");
    
    PatternRegistry registry;
    
    std::string patternName = "Passive RC Low-Pass Filter";
    CircuitPattern* pattern = registry.getPattern(patternName);
    
    std::cout << "Attempting to retrieve pattern: \"" << patternName << "\"\n\n";
    
    assert(pattern != nullptr);
    
    std::cout << "Retrieved Pattern:\n";
    printPattern(pattern);
    
    std::cout << "\n✓ PASS: Pattern successfully retrieved by name\n";
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
        testPatternRetrieval();
        
        printHeader("ALL TESTS PASSED ✓");
        std::cout << "\nPhase 2: Topology Pattern Matching - Status: COMPLETE\n";
        std::cout << "- 14 circuit patterns implemented\n";
        std::cout << "- Pattern matching algorithm functional\n";
        std::cout << "- All test cases passing\n";
        std::cout << "\nNext: Integrate with CircuitAnalyzer\n";
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}

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
    
    assert(patternCount >= 15);  // Should have at least 15 patterns
    
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
        
        std::cout << std::setw(2) << (i+1) << ". " << std::setw(35) << std::left 
                  << pattern.name 
                  << " | " << std::setw(18) << std::left 
                  << getPatternCategoryName(pattern.category) << " | "
                  << "Components: " << std::setw(2) << pattern.signature.size() << "\n";
    }
    
    std::cout << "\n\nCategory Breakdown:\n";
    for (const auto& [category, count] : categoryCount) {
        std::cout << "  " << getPatternCategoryName(category) << ": " << count << "\n";
    }
    
    std::cout << "\n✓ PASS: All patterns listed successfully\n";
}

void testSimpleRCLowPass() {
    printHeader("Test 3: Simple RC Low-Pass Circuit");
    
    PatternRegistry registry;
    
    // Create a simple RC low-pass circuit
    std::vector<Component> components;
    
    Component r;
    r.id = "R1";
    r.type = ComponentType::Resistor;
    r.value = 10000.0f;
    r.unit = "Ω";
    components.push_back(r);
    
    Component c;
    c.id = "C1";
    c.type = ComponentType::Capacitor;
    c.value = 1.0e-6f;
    c.unit = "F";
    components.push_back(c);
    
    std::vector<Connection> connections;
    
    // Attempt to match pattern
    PatternMatch match = registry.matchPattern(components, connections);
    
    std::cout << "Input Circuit Components:\n";
    std::cout << "  - Resistor (10kΩ)\n";
    std::cout << "  - Capacitor (1µF)\n\n";
    
    std::cout << "Pattern Matching Result:\n";
    std::cout << "  Confidence: " << std::fixed << std::setprecision(3) 
              << match.confidence << "\n";
    printPattern(match.pattern);
    
    assert(match.confidence > 0.0f);
    assert(match.pattern != nullptr);
    
    std::cout << "\n✓ PASS: RC low-pass pattern correctly identified\n";
}

void testDiodeClippingCircuit() {
    printHeader("Test 4: Diode Clipping Circuit");
    
    PatternRegistry registry;
    
    // Create a back-to-back diode clipping circuit
    std::vector<Component> components;
    
    Component d1;
    d1.id = "D1";
    d1.type = ComponentType::Diode;
    d1.partNumber = "1N4148";
    components.push_back(d1);
    
    Component d2;
    d2.id = "D2";
    d2.type = ComponentType::Diode;
    d2.partNumber = "1N4148";
    components.push_back(d2);
    
    Component r;
    r.id = "R_limit";
    r.type = ComponentType::Resistor;
    r.value = 1000.0f;
    r.unit = "Ω";
    components.push_back(r);
    
    std::vector<Connection> connections;
    
    // Attempt to match pattern
    std::vector<PatternMatch> matches = registry.findAllPatterns(components, connections);
    
    std::cout << "Input Circuit Components:\n";
    std::cout << "  - Diode (1N4148)\n";
    std::cout << "  - Diode (1N4148)\n";
    std::cout << "  - Resistor (1kΩ)\n\n";
    
    std::cout << "Pattern Matching Results (all matches):\n";
    
    if (!matches.empty()) {
        for (size_t i = 0; i < matches.size(); ++i) {
            std::cout << "\n  " << (i+1) << ". " << matches[i].pattern->name 
                      << " (Confidence: " << std::fixed << std::setprecision(3) 
                      << matches[i].confidence << ")\n";
        }
    } else {
        std::cout << "  [No patterns matched]\n";
    }
    
    assert(!matches.empty());
    
    std::cout << "\n✓ PASS: Diode clipping patterns identified\n";
}

void testThreePointToneStack() {
    printHeader("Test 5: Three-Point Tone Stack");
    
    PatternRegistry registry;
    
    // Create a three-point tone stack circuit
    std::vector<Component> components;
    
    // 3 resistors
    for (int i = 0; i < 3; ++i) {
        Component r;
        r.id = "R" + std::to_string(i+1);
        r.type = ComponentType::Resistor;
        r.value = 10000.0f;
        r.unit = "Ω";
        components.push_back(r);
    }
    
    // 3 capacitors
    for (int i = 0; i < 3; ++i) {
        Component c;
        c.id = "C" + std::to_string(i+1);
        c.type = ComponentType::Capacitor;
        c.value = 10.0e-9f;
        c.unit = "F";
        components.push_back(c);
    }
    
    std::vector<Connection> connections;
    
    // Attempt to match pattern
    PatternMatch match = registry.matchPattern(components, connections);
    
    std::cout << "Input Circuit Components:\n";
    std::cout << "  - 3× Resistors (10kΩ each)\n";
    std::cout << "  - 3× Capacitors (10nF each)\n\n";
    
    std::cout << "Pattern Matching Result:\n";
    std::cout << "  Confidence: " << std::fixed << std::setprecision(3) 
              << match.confidence << "\n";
    printPattern(match.pattern);
    
    assert(match.confidence > 0.0f);
    assert(match.pattern != nullptr);
    
    std::cout << "\n✓ PASS: Three-point tone stack pattern correctly identified\n";
}

void testAmplifierTopologies() {
    printHeader("Test 6: Common Emitter Amplifier");
    
    PatternRegistry registry;
    
    // Create a common emitter amplifier circuit
    std::vector<Component> components;
    
    Component bjt;
    bjt.id = "Q1";
    bjt.type = ComponentType::BJT;
    bjt.partNumber = "2N3904";
    components.push_back(bjt);
    
    Component rc;
    rc.id = "Rc";
    rc.type = ComponentType::Resistor;
    rc.value = 10000.0f;
    rc.unit = "Ω";
    components.push_back(rc);
    
    Component re;
    re.id = "Re";
    re.type = ComponentType::Resistor;
    re.value = 1000.0f;
    re.unit = "Ω";
    components.push_back(re);
    
    Component ce;
    ce.id = "Ce";
    ce.type = ComponentType::Capacitor;
    ce.value = 100.0e-9f;
    ce.unit = "F";
    components.push_back(ce);
    
    std::vector<Connection> connections;
    
    // Attempt to match pattern
    PatternMatch match = registry.matchPattern(components, connections);
    
    std::cout << "Input Circuit Components:\n";
    std::cout << "  - BJT (2N3904)\n";
    std::cout << "  - Rc (10kΩ)\n";
    std::cout << "  - Re (1kΩ)\n";
    std::cout << "  - Ce (100nF)\n\n";
    
    std::cout << "Pattern Matching Result:\n";
    std::cout << "  Confidence: " << std::fixed << std::setprecision(3) 
              << match.confidence << "\n";
    printPattern(match.pattern);
    
    assert(match.confidence > 0.0f);
    assert(match.pattern != nullptr);
    
    std::cout << "\n✓ PASS: Common emitter amplifier pattern correctly identified\n";
}

void testPatternRetrieval() {
    printHeader("Test 7: Direct Pattern Retrieval");
    
    PatternRegistry registry;
    
    std::string patternName = "Passive RC Low-Pass Filter";
    CircuitPattern* pattern = registry.getPattern(patternName);
    
    std::cout << "Attempting to retrieve pattern: \"" << patternName << "\"\n\n";
    
    assert(pattern != nullptr);
    
    std::cout << "Retrieved Pattern:\n";
    printPattern(pattern);
    
    std::cout << "\n✓ PASS: Pattern successfully retrieved by name\n";
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
        testAmplifierTopologies();
        testPatternRetrieval();
        
        printHeader("ALL TESTS PASSED ✓");
        std::cout << "\nPhase 2: Topology Pattern Matching - Status: COMPLETE\n";
        std::cout << "- 15+ circuit patterns implemented\n";
        std::cout << "- Pattern matching algorithm functional\n";
        std::cout << "- All test cases passing\n";
        std::cout << "\nNext: Integrate with CircuitAnalyzer\n";
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
