#include "TopologyPatterns.h"
#include <algorithm>
#include <iostream>

namespace TopologyAnalysis {

// ============================================================================
// Pattern Registry - Initialize all known patterns
// ============================================================================

PatternRegistry::PatternRegistry() {
    initializeCorePatterns();
}

void PatternRegistry::initializeCorePatterns() {
    // ========================================================================
    // 1. PASSIVE FILTERS
    // ========================================================================
    
    CircuitPattern rcLowPass;
    rcLowPass.name = "Passive RC Low-Pass Filter";
    rcLowPass.category = PatternCategory::PassiveFilter;
    rcLowPass.signature = {LiveSpice::ComponentType::Resistor, LiveSpice::ComponentType::Capacitor};
    rcLowPass.dspStrategy = "cascaded_biquad";
    rcLowPass.parameters = {"Resistance", "Capacitance", "CutoffFrequency"};
    rcLowPass.description = "Simple RC network for low-pass filtering";
    rcLowPass.confidence_threshold = 0.85f;
    patterns.push_back(rcLowPass);
    
    CircuitPattern rcHighPass;
    rcHighPass.name = "Passive RC High-Pass Filter";
    rcHighPass.category = PatternCategory::PassiveFilter;
    rcHighPass.signature = {LiveSpice::ComponentType::Capacitor, LiveSpice::ComponentType::Resistor};
    rcHighPass.dspStrategy = "cascaded_biquad";
    rcHighPass.parameters = {"Resistance", "Capacitance", "CutoffFrequency"};
    rcHighPass.description = "Simple RC network for high-pass filtering";
    rcHighPass.confidence_threshold = 0.85f;
    patterns.push_back(rcHighPass);
    
    CircuitPattern lcBandPass;
    lcBandPass.name = "Passive LC Band-Pass Filter";
    lcBandPass.category = PatternCategory::PassiveFilter;
    lcBandPass.signature = {LiveSpice::ComponentType::Inductor, LiveSpice::ComponentType::Capacitor};
    lcBandPass.dspStrategy = "cascaded_biquad";
    lcBandPass.parameters = {"Inductance", "Capacitance", "ResonantFrequency", "Q"};
    lcBandPass.description = "LC resonant tank for narrow band-pass";
    lcBandPass.confidence_threshold = 0.90f;
    patterns.push_back(lcBandPass);
    
    // ========================================================================
    // 2. ACTIVE FILTERS (Op-Amp based)
    // ========================================================================
    
    CircuitPattern sallenKey;
    sallenKey.name = "Sallen-Key Low-Pass Filter";
    sallenKey.category = PatternCategory::ActiveFilter;
    sallenKey.signature = {LiveSpice::ComponentType::OpAmp, LiveSpice::ComponentType::Resistor, 
                           LiveSpice::ComponentType::Resistor, LiveSpice::ComponentType::Capacitor, 
                           LiveSpice::ComponentType::Capacitor};
    sallenKey.dspStrategy = "state_space";
    sallenKey.parameters = {"R1", "R2", "C1", "C2", "GainFactor"};
    sallenKey.description = "Unity-gain Sallen-Key topology for smooth low-pass";
    sallenKey.confidence_threshold = 0.88f;
    patterns.push_back(sallenKey);
    
    CircuitPattern multipleFeedback;
    multipleFeedback.name = "Multiple Feedback Filter";
    multipleFeedback.category = PatternCategory::ActiveFilter;
    multipleFeedback.signature = {LiveSpice::ComponentType::OpAmp, LiveSpice::ComponentType::Resistor, 
                                  LiveSpice::ComponentType::Resistor, LiveSpice::ComponentType::Resistor,
                                  LiveSpice::ComponentType::Capacitor, LiveSpice::ComponentType::Capacitor};
    multipleFeedback.dspStrategy = "state_space";
    multipleFeedback.parameters = {"R1", "R2", "Rf", "C1", "C2", "Gain", "Q"};
    multipleFeedback.description = "High-Q active filter with multiple feedback paths";
    multipleFeedback.confidence_threshold = 0.85f;
    patterns.push_back(multipleFeedback);
    
    CircuitPattern stateVariable;
    stateVariable.name = "State-Variable Filter";
    stateVariable.category = PatternCategory::ActiveFilter;
    stateVariable.signature = {LiveSpice::ComponentType::OpAmp, LiveSpice::ComponentType::OpAmp, 
                               LiveSpice::ComponentType::OpAmp, LiveSpice::ComponentType::Resistor,
                               LiveSpice::ComponentType::Capacitor};
    stateVariable.dspStrategy = "state_space";
    stateVariable.parameters = {"LowpassOutput", "BandpassOutput", "HighpassOutput"};
    stateVariable.description = "Universal filter with simultaneous LP/BP/HP outputs";
    stateVariable.confidence_threshold = 0.90f;
    patterns.push_back(stateVariable);
    
    // ========================================================================
    // 3. AMPLIFIER TOPOLOGIES
    // ========================================================================
    
    CircuitPattern commonEmitter;
    commonEmitter.name = "Common Emitter Amplifier";
    commonEmitter.category = PatternCategory::AmplifierStage;
    commonEmitter.signature = {LiveSpice::ComponentType::Transistor, LiveSpice::ComponentType::Resistor, 
                               LiveSpice::ComponentType::Resistor, LiveSpice::ComponentType::Capacitor};
    commonEmitter.dspStrategy = "nonlinear_solver";
    commonEmitter.parameters = {"Gain", "InputImpedance", "OutputImpedance"};
    commonEmitter.description = "Transistor voltage amplifier with inverting phase shift";
    commonEmitter.confidence_threshold = 0.87f;
    patterns.push_back(commonEmitter);
    
    CircuitPattern cascode;
    cascode.name = "Cascode Amplifier";
    cascode.category = PatternCategory::AmplifierStage;
    cascode.signature = {LiveSpice::ComponentType::Transistor, LiveSpice::ComponentType::Transistor, LiveSpice::ComponentType::Resistor,
                        LiveSpice::ComponentType::Resistor, LiveSpice::ComponentType::Resistor};
    cascode.dspStrategy = "nonlinear_solver";
    cascode.parameters = {"Gain", "Bandwidth", "InputImpedance"};
    cascode.description = "High-gain low-noise amplifier topology";
    cascode.confidence_threshold = 0.88f;
    patterns.push_back(cascode);
    
    // ========================================================================
    // 4. CLIPPING / DISTORTION STAGES
    // ========================================================================
    
    CircuitPattern seriesDiodeClip;
    seriesDiodeClip.name = "Series Diode Clipping";
    seriesDiodeClip.category = PatternCategory::ClippingStage;
    seriesDiodeClip.signature = {LiveSpice::ComponentType::Diode, LiveSpice::ComponentType::Resistor};
    seriesDiodeClip.dspStrategy = "diode_solver";
    seriesDiodeClip.parameters = {"DiodeType", "ClippingThreshold", "Symmetry"};
    seriesDiodeClip.description = "Soft clipping via forward-biased diode";
    seriesDiodeClip.confidence_threshold = 0.80f;
    patterns.push_back(seriesDiodeClip);
    
    CircuitPattern parallelDiodeClip;
    parallelDiodeClip.name = "Parallel Diode Clipping";
    parallelDiodeClip.category = PatternCategory::ClippingStage;
    parallelDiodeClip.signature = {LiveSpice::ComponentType::Diode, LiveSpice::ComponentType::Resistor};
    parallelDiodeClip.dspStrategy = "diode_solver";
    parallelDiodeClip.parameters = {"DiodeType", "LoadResistance"};
    parallelDiodeClip.description = "Current-limiting diode clipping";
    parallelDiodeClip.confidence_threshold = 0.80f;
    patterns.push_back(parallelDiodeClip);
    
    CircuitPattern backToBackDiode;
    backToBackDiode.name = "Back-to-Back Diode Clipping";
    backToBackDiode.category = PatternCategory::ClippingStage;
    backToBackDiode.signature = {LiveSpice::ComponentType::Diode, LiveSpice::ComponentType::Diode, 
                                 LiveSpice::ComponentType::Resistor};
    backToBackDiode.dspStrategy = "diode_solver";
    backToBackDiode.parameters = {"DiodeType", "SymmetricClipping"};
    backToBackDiode.description = "Symmetric clipping on positive and negative peaks";
    backToBackDiode.confidence_threshold = 0.85f;
    patterns.push_back(backToBackDiode);
    
    // ========================================================================
    // 5. TONE CONTROL TOPOLOGIES
    // ========================================================================
    
    CircuitPattern shelvingEQ;
    shelvingEQ.name = "Shelving Tone Control";
    shelvingEQ.category = PatternCategory::ToneControl;
    shelvingEQ.signature = {LiveSpice::ComponentType::Resistor, LiveSpice::ComponentType::Resistor,
                            LiveSpice::ComponentType::Capacitor};
    shelvingEQ.dspStrategy = "cascaded_biquad";
    shelvingEQ.parameters = {"CornerFrequency", "Gain", "Q"};
    shelvingEQ.description = "Passive high or low shelving filter";
    shelvingEQ.confidence_threshold = 0.82f;
    patterns.push_back(shelvingEQ);
    
    CircuitPattern threePointTone;
    threePointTone.name = "Three-Point Tone Stack";
    threePointTone.category = PatternCategory::ToneControl;
    threePointTone.signature = {LiveSpice::ComponentType::Resistor, LiveSpice::ComponentType::Resistor,
                                LiveSpice::ComponentType::Resistor, LiveSpice::ComponentType::Capacitor,
                                LiveSpice::ComponentType::Capacitor, LiveSpice::ComponentType::Capacitor};
    threePointTone.dspStrategy = "state_space";
    threePointTone.parameters = {"BassFreq", "MidFreq", "TrebleFreq", "Load"};
    threePointTone.description = "Classic Marshall/Fender tone stack topology";
    threePointTone.confidence_threshold = 0.90f;
    patterns.push_back(threePointTone);
    
    // ========================================================================
    // 6. FEEDBACK NETWORKS
    // ========================================================================
    
    CircuitPattern negativeFeedback;
    negativeFeedback.name = "Negative Feedback Network";
    negativeFeedback.category = PatternCategory::FeedbackNetwork;
    negativeFeedback.signature = {LiveSpice::ComponentType::Resistor, LiveSpice::ComponentType::Resistor};
    negativeFeedback.dspStrategy = "implicit_solver";
    negativeFeedback.parameters = {"GainFactor", "Frequency", "Stability"};
    negativeFeedback.description = "Closed-loop stabilization via negative feedback";
    negativeFeedback.confidence_threshold = 0.75f;
    patterns.push_back(negativeFeedback);
    
    CircuitPattern integrator;
    integrator.name = "Integrator (Feedback Capacitor)";
    integrator.category = PatternCategory::FeedbackNetwork;
    integrator.signature = {LiveSpice::ComponentType::Resistor, LiveSpice::ComponentType::Capacitor};
    integrator.dspStrategy = "state_space";
    integrator.parameters = {"IntegrationConstant", "BandwidthLimit"};
    integrator.description = "Active integrator for low-frequency roll-off";
    integrator.confidence_threshold = 0.80f;
    patterns.push_back(integrator);
    
    // ========================================================================
    // 7. COUPLING AND BYPASS
    // ========================================================================
    
    CircuitPattern acCoupling;
    acCoupling.name = "AC Coupling Capacitor";
    acCoupling.category = PatternCategory::Coupling;
    acCoupling.signature = {LiveSpice::ComponentType::Capacitor, LiveSpice::ComponentType::Resistor};
    acCoupling.dspStrategy = "cascaded_biquad";
    acCoupling.parameters = {"CouplingFrequency", "InputImpedance"};
    acCoupling.description = "High-pass filter for DC blocking";
    acCoupling.confidence_threshold = 0.75f;
    patterns.push_back(acCoupling);
    
    CircuitPattern bypassCapacitor;
    bypassCapacitor.name = "Bypass Capacitor";
    bypassCapacitor.category = PatternCategory::Coupling;
    bypassCapacitor.signature = {LiveSpice::ComponentType::Capacitor};
    bypassCapacitor.dspStrategy = "none";
    bypassCapacitor.parameters = {"BypassFrequency"};
    bypassCapacitor.description = "Power supply or emitter bypass";
    bypassCapacitor.confidence_threshold = 0.70f;
    patterns.push_back(bypassCapacitor);
    
    std::cout << "[TopologyPatterns] Initialized " << patterns.size() 
              << " circuit patterns\n";
}

// ============================================================================
// Pattern Matching Algorithm
// ============================================================================

PatternMatch PatternRegistry::matchPattern(const std::vector<Component>& circuitComponents,
                                          const std::vector<Connection>& connections) {
    PatternMatch bestMatch;
    bestMatch.confidence = 0.0f;
    bestMatch.pattern = nullptr;
    
    if (patterns.empty() || circuitComponents.empty()) {
        return bestMatch;
    }
    
    // Try to match each pattern in the registry
    for (auto& pattern : patterns) {
        float confidence = calculatePatternConfidence(circuitComponents, connections, pattern);
        
        if (confidence > bestMatch.confidence && 
            confidence >= pattern.confidence_threshold) {
            bestMatch.confidence = confidence;
            bestMatch.pattern = &pattern;
            bestMatch.matchedComponents = circuitComponents;
            bestMatch.matchedConnections = connections;
        }
    }
    
    return bestMatch;
}

std::vector<PatternMatch> PatternRegistry::findAllPatterns(
    const std::vector<Component>& circuitComponents,
    const std::vector<Connection>& connections) {
    
    std::vector<PatternMatch> matches;
    
    for (auto& pattern : patterns) {
        float confidence = calculatePatternConfidence(circuitComponents, connections, pattern);
        
        if (confidence >= pattern.confidence_threshold) {
            PatternMatch match;
            match.confidence = confidence;
            match.pattern = &pattern;
            match.matchedComponents = circuitComponents;
            match.matchedConnections = connections;
            matches.push_back(match);
        }
    }
    
    // Sort by confidence (highest first)
    std::sort(matches.begin(), matches.end(),
        [](const PatternMatch& a, const PatternMatch& b) {
            return a.confidence > b.confidence;
        });
    
    return matches;
}

float PatternRegistry::calculatePatternConfidence(
    const std::vector<Component>& circuitComponents,
    const std::vector<Connection>& connections,
    const CircuitPattern& pattern) {
    
    if (pattern.signature.empty()) {
        return 0.0f;
    }
    
    // Count component type matches
    std::map<LiveSpice::ComponentType, int> requiredCounts;
    for (const auto& type : pattern.signature) {
        requiredCounts[type]++;
    }
    
    std::map<LiveSpice::ComponentType, int> actualCounts;
    for (const auto& comp : circuitComponents) {
        actualCounts[comp.type]++;
    }
    
    // Calculate match percentage
    int totalMatches = 0;
    int totalRequired = pattern.signature.size();
    
    for (const auto& [type, count] : requiredCounts) {
        if (actualCounts.count(type)) {
            totalMatches += std::min(count, actualCounts[type]);
        }
    }
    
    float componentConfidence = static_cast<float>(totalMatches) / totalRequired;
    
    // Bonus for connectivity analysis (future enhancement)
    // For now, return basic component-count-based confidence
    return componentConfidence * 0.95f;  // 95% weight on component matching
}

CircuitPattern* PatternRegistry::getPattern(const std::string& name) {
    for (auto& pattern : patterns) {
        if (pattern.name == name) {
            return &pattern;
        }
    }
    return nullptr;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string getPatternCategoryName(PatternCategory category) {
    switch (category) {
        case PatternCategory::PassiveFilter:   return "Passive Filter";
        case PatternCategory::ActiveFilter:    return "Active Filter";
        case PatternCategory::AmplifierStage:  return "Amplifier Stage";
        case PatternCategory::ClippingStage:   return "Clipping Stage";
        case PatternCategory::ToneControl:     return "Tone Control";
        case PatternCategory::FeedbackNetwork: return "Feedback Network";
        case PatternCategory::Coupling:        return "Coupling/Bypass";
        case PatternCategory::Resonant:        return "Resonant Circuit";
        case PatternCategory::Unknown:
        default:                                return "Unknown";
    }
}

std::string getDSPStrategyDescription(const std::string& strategy) {
    if (strategy == "cascaded_biquad")     return "Cascade of 2nd-order IIR filters";
    if (strategy == "state_space")         return "Linear state-space representation";
    if (strategy == "diode_solver")        return "Iterative diode equation solver";
    if (strategy == "nonlinear_solver")    return "Nonlinear Newton-Raphson solver";
    if (strategy == "implicit_solver")     return "Implicit feedback loop solver";
    if (strategy == "wdf_node")            return "Wave digital filter topology";
    if (strategy == "none")                return "Passive/no processing needed";
    return "Custom/Unknown";
}

}  // namespace TopologyAnalysis
