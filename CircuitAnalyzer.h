#pragma once

#include "LiveSpiceParser.h"
#include "ComponentDSPMapper.h"
#include <map>
#include <set>
#include <memory>

namespace LiveSpice {

    // ============================================================================
    // Circuit Stage - Represents a distinct processing stage in the circuit
    // ============================================================================
    enum class StageType {
        InputBuffer,
        GainStage,
        HighPassFilter,
        LowPassFilter,
        BandPassFilter,
        OpAmpClipping,
        DiodeClipper,
        ToneControl,
        OutputBuffer,
        Unknown
    };

    struct CircuitStage {
        StageType type;
        std::string name;
        std::vector<std::shared_ptr<Component>> components;
        std::map<std::string, double> dspParams; // Parameters for DSP implementation
        
        // LiveSPICE component DSP mapping
        ComponentDSPMapper::DSPProcessorType primaryProcessorType;
        std::string dspDescription;
    };

    // ============================================================================
    // Node Class - Represents electrical nodes in the circuit
    // ============================================================================
    class Node {
    public:
        Node() : x(0), y(0) {}
        Node(int x, int y) : x(x), y(y) {}

        int getX() const { return x; }
        int getY() const { return y; }

        void addComponent(std::shared_ptr<Component> comp, const std::string& terminal) {
            connectedComponents[comp->getName()] = terminal;
        }

        const std::map<std::string, std::string>& getConnectedComponents() const {
            return connectedComponents;
        }

        bool operator==(const Node& other) const {
            return x == other.x && y == other.y;
        }

    private:
        int x, y;
        std::map<std::string, std::string> connectedComponents; // component name -> terminal
    };

    // ============================================================================
    // Circuit Graph - Represents the circuit topology
    // ============================================================================
    class CircuitGraph {
    public:
        CircuitGraph(const Netlist& netlist);

        const std::map<std::pair<int, int>, Node>& getNodes() const { return nodes; }
        const std::vector<Wire>& getWires() const { return wires; }

        // Find all components of a specific type
        std::vector<std::shared_ptr<Component>> findComponentsByType(ComponentType type) const;

        // Find nodes connected to a component
        std::vector<Node> getConnectedNodes(std::shared_ptr<Component> comp) const;

        // Check if node is ground
        bool isGroundNode(const Node& node) const;

    private:
        std::map<std::pair<int, int>, Node> nodes;
        std::vector<Wire> wires;
        std::vector<std::shared_ptr<Component>> allComponents;
    };

    // ============================================================================
    // Circuit Analyzer - Analyzes circuit topology and identifies stages
    // ============================================================================
    class CircuitAnalyzer {
    public:
        CircuitAnalyzer(const Schematic& schematic);

        std::vector<CircuitStage> analyzeCircuit();
        std::string generateReport() const;
        std::string generateConnectivityReport() const;

    private:
        const Schematic& schematic;
        CircuitGraph circuitGraph;
        std::vector<CircuitStage> identifiedStages;
        ComponentDSPMapper dspMapper;  // LiveSPICE component to DSP mapper

        // Stage identification methods
        CircuitStage identifyInputStage();
        CircuitStage identifyOutputStage();
        CircuitStage identifyOpAmpStage();
        CircuitStage identifyFilterStage();
        CircuitStage identifyClippingStage();
        CircuitStage identifyToneControl();

        // Helper methods
        std::shared_ptr<Component> findComponentByName(const std::string& name) const;
        std::vector<std::shared_ptr<Component>> findComponentsByType(ComponentType type) const;

        // DSP parameter calculation
        double calculateFilterFrequency(double resistance, double capacitance);
        double calculateGain(const CircuitStage& stage);
        
        // LiveSPICE DSP mapping
        void populateDSPMapping(CircuitStage& stage);
    };

} // namespace LiveSpice
