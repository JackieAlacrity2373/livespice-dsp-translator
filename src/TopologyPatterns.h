#pragma once

#include "LiveSpiceParser.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <map>

namespace TopologyAnalysis {

// ============================================================================
// ENUMERATIONS AND CONSTANTS
// ============================================================================

enum class PatternCategory {
    PassiveFilter,
    ActiveFilter,
    AmplifierStage,
    ClippingStage,
    ToneControl,
    FeedbackNetwork,
    Coupling,
    Resonant,
    Unknown
};

// ============================================================================
// CORE PATTERN STRUCTURES
// ============================================================================

/**
 * CircuitPattern - Defines a recognizable circuit topology
 * 
 * Used to identify circuit structures and select optimal DSP generation
 * strategies. Each pattern encodes a component signature, suggested DSP
 * implementation method, and extractable parameters.
 */
struct CircuitPattern {
    // Identification
    std::string name;                          ///< Descriptive name (e.g., "Sallen-Key Low-Pass")
    PatternCategory category;                  ///< Circuit category
    std::string description;                   ///< Detailed description
    
    // Component Signature - the "fingerprint" of this pattern
    std::vector<LiveSpice::ComponentType> signature;
    
    // DSP Strategy Selection
    std::string dspStrategy;                   ///< Strategy for code generation
                                               ///< Options: "cascaded_biquad", "state_space", 
                                               ///< "diode_solver", "nonlinear_solver", "wdf_node", etc.
    
    // Parameter Extraction
    std::vector<std::string> parameters;       ///< Parameters to extract and expose
    
    // Matching Configuration
    float confidence_threshold = 0.80f;        ///< Minimum confidence to accept match
    int min_component_count = 1;               ///< Minimum components required
    int max_component_count = 20;              ///< Maximum components for this pattern
    
    // Metadata
    bool requiresNonlinearSolver = false;      ///< Does this pattern need feedback solver?
    bool hasResonance = false;                 ///< Does this pattern exhibit resonance?
    float typicalLatencyMs = 0.0f;             ///< Estimated DSP latency
};

/**
 * Component - Represents a single circuit component instance
 */
struct Component {
    std::string id;                            ///< Unique component identifier
    LiveSpice::ComponentType type;             ///< Component type (Resistor, Capacitor, etc.)
    std::string partNumber;                    ///< Part number if known (e.g., "1N4148")
    float value = 0.0f;                        ///< Component value in base units
    std::string unit;                          ///< Unit (Ω, F, H, etc.)
    std::map<std::string, float> parameters;   ///< Additional parameters
};

/**
 * Connection - Represents a connection between components
 */
struct Connection {
    std::string fromId;                        ///< Source component ID
    std::string toId;                          ///< Destination component ID
    int fromPin = 0;                           ///< Source pin number
    int toPin = 0;                             ///< Destination pin number
    bool isFeedback = false;                   ///< Is this a feedback connection?
};

/**
 * PatternMatch - Result of pattern matching operation
 */
struct PatternMatch {
    CircuitPattern* pattern = nullptr;         ///< Matched pattern (nullptr if no match)
    float confidence = 0.0f;                   ///< Confidence score (0.0 to 1.0)
    std::vector<Component> matchedComponents;  ///< Components that matched
    std::vector<Connection> matchedConnections;///< Connections that matched
    std::map<std::string, float> extractedParameters;  ///< Extracted parameter values
};

// ============================================================================
// PATTERN REGISTRY AND MATCHING
// ============================================================================

class PatternRegistry {
public:
    /**
     * Constructor - Initializes all built-in patterns
     */
    PatternRegistry();
    
    /**
     * matchPattern - Find best matching pattern for given components
     * 
     * @param circuitComponents - Vector of circuit components
     * @param connections - Vector of component connections
     * @return Best matching pattern (empty if no match above threshold)
     */
    PatternMatch matchPattern(const std::vector<Component>& circuitComponents,
                             const std::vector<Connection>& connections);
    
    /**
     * findAllPatterns - Find all matching patterns
     * 
     * Returns all patterns matching above their confidence threshold,
     * sorted by confidence (highest first).
     * 
     * @param circuitComponents - Vector of circuit components
     * @param connections - Vector of component connections
     * @return Sorted vector of all matches
     */
    std::vector<PatternMatch> findAllPatterns(const std::vector<Component>& circuitComponents,
                                             const std::vector<Connection>& connections);
    
    /**
     * getPattern - Retrieve pattern by name
     */
    CircuitPattern* getPattern(const std::string& name);
    
    /**
     * getPatternCount - Get total number of registered patterns
     */
    size_t getPatternCount() const { return patterns.size(); }
    
    /**
     * listPatterns - Get all registered patterns
     */
    const std::vector<CircuitPattern>& listPatterns() const { return patterns; }
    
private:
    std::vector<CircuitPattern> patterns;
    
    void initializeCorePatterns();
    
    float calculatePatternConfidence(const std::vector<Component>& circuitComponents,
                                    const std::vector<Connection>& connections,
                                    const CircuitPattern& pattern);
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Convert PatternCategory enum to human-readable string
 */
std::string getPatternCategoryName(PatternCategory category);

/**
 * Convert DSP strategy string to description
 */
std::string getDSPStrategyDescription(const std::string& strategy);

}  // namespace TopologyAnalysis
