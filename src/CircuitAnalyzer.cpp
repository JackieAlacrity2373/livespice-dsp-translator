#define M_PI 3.14159265358979323846
#include "CircuitAnalyzer.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

namespace LiveSpice {

    // ============================================================================
    // Helper function to sanitize Unicode characters to ASCII
    // ============================================================================
    static std::string sanitizeUnicode(const std::string& input) {
        std::string result;
        for (size_t i = 0; i < input.length(); ) {
            unsigned char c = input[i];
            
            // Handle multi-byte UTF-8 sequences
            if (c == 0xE2 && i + 2 < input.length()) {
                // Check for Ohm symbol (Ω = E2 84 A9)
                if ((unsigned char)input[i+1] == 0x84 && (unsigned char)input[i+2] == 0xA9) {
                    result += "Ohm";
                    i += 3;
                    continue;
                }
            } else if (c == 0xCE && i + 1 < input.length()) {
                // Check for Ohm symbol in different encoding (Ω = CE A9)
                if ((unsigned char)input[i+1] == 0xA9) {
                    result += "Ohm";
                    i += 2;
                    continue;
                }
            } else if (c == 0xC2 && i + 1 < input.length()) {
                // Check for micro symbol (μ = C2 B5)
                if ((unsigned char)input[i+1] == 0xB5) {
                    result += "u";
                    i += 2;
                    continue;
                }
            } else if (c == 0xCF && i + 1 < input.length()) {
                // Check for Greek micro (μ = CF 80 for pi, or other variants)
                if ((unsigned char)input[i+1] == 0x80) {
                    result += "u";
                    i += 2;
                    continue;
                }
            } else if (c == 0xE2 && i + 2 < input.length()) {
                // Check for infinity symbol (∞ = E2 88 9E)
                if ((unsigned char)input[i+1] == 0x88 && (unsigned char)input[i+2] == 0x9E) {
                    result += "inf";
                    i += 3;
                    continue;
                }
            }
            
            // Regular ASCII character
            if ((unsigned char)c < 128) {
                result += c;
                i++;
            } else {
                // Skip other UTF-8 continuation bytes
                i++;
            }
        }
        return result;
    }

    static std::string trimString(const std::string& input) {
        size_t start = 0;
        while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
            ++start;
        }

        size_t end = input.size();
        while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
            --end;
        }

        return input.substr(start, end - start);
    }

    static std::string toUpperString(std::string input) {
        for (auto& ch : input) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        return input;
    }

    static std::string normalizePartNumber(const std::string& raw) {
        return toUpperString(trimString(sanitizeUnicode(raw)));
    }

    static bool isLikelyPNP(const std::string& partNumber, const std::string& typeParam) {
        std::string partUpper = toUpperString(partNumber);
        std::string typeUpper = toUpperString(typeParam);

        if (typeUpper == "PNP") return true;
        if (partUpper.find("PNP") != std::string::npos) return true;
        if (partUpper == "2N3906") return true;

        return false;
    }

    // ============================================================================
    // CircuitGraph Implementation
    // ============================================================================
    CircuitGraph::CircuitGraph(const Netlist& netlist) : wires(netlist.getWires()) {
        // Build node map from wires
        for (const auto& wire : wires) {
            std::pair<int, int> nodeA = {wire.nodeA_X, wire.nodeA_Y};
            std::pair<int, int> nodeB = {wire.nodeB_X, wire.nodeB_Y};

            if (nodes.find(nodeA) == nodes.end()) {
                nodes[nodeA] = Node(wire.nodeA_X, wire.nodeA_Y);
            }
            if (nodes.find(nodeB) == nodes.end()) {
                nodes[nodeB] = Node(wire.nodeB_X, wire.nodeB_Y);
            }
        }

        // Store all components for later analysis
        for (const auto& pair : netlist.getComponents()) {
            allComponents.push_back(pair.second);
        }
    }

    std::vector<std::shared_ptr<Component>> CircuitGraph::findComponentsByType(ComponentType type) const {
        std::vector<std::shared_ptr<Component>> result;
        for (const auto& comp : allComponents) {
            if (comp->getType() == type) {
                result.push_back(comp);
            }
        }
        return result;
    }

    std::vector<Node> CircuitGraph::getConnectedNodes(std::shared_ptr<Component> comp) const {
        std::vector<Node> connectedNodes;
        int x, y;
        comp->getPosition(x, y);

        // Find nodes that connect to this component's position
        for (const auto& pair : nodes) {
            // Simple proximity check - nodes at same or nearby positions
            if (std::abs(pair.first.first - x) <= 10 && std::abs(pair.first.second - y) <= 10) {
                connectedNodes.push_back(pair.second);
            }
        }
        return connectedNodes;
    }

    bool CircuitGraph::isGroundNode(const Node& node) const {
        for (const auto& comp : allComponents) {
            if (comp->getType() == ComponentType::Ground) {
                int x, y;
                comp->getPosition(x, y);
                if (x == node.getX() && y == node.getY()) {
                    return true;
                }
            }
        }
        return false;
    }

    // ============================================================================
    // CircuitAnalyzer Implementation
    // ============================================================================
    CircuitAnalyzer::CircuitAnalyzer(const Schematic& schematic)
        : schematic(schematic), circuitGraph(schematic.getNetlist()) {
    }

    std::vector<CircuitStage> CircuitAnalyzer::analyzeCircuit() {
        identifiedStages.clear();

        // Identify common circuit stages
        auto inputComps = findComponentsByType(ComponentType::Input);
        auto outputComps = findComponentsByType(ComponentType::Output);
        auto opamps = findComponentsByType(ComponentType::OpAmp);
        auto diodes = findComponentsByType(ComponentType::Diode);
        auto transistors = findComponentsByType(ComponentType::Transistor);
        auto resistors = findComponentsByType(ComponentType::Resistor);
        auto capacitors = findComponentsByType(ComponentType::Capacitor);
        auto potentiometers = findComponentsByType(ComponentType::Potentiometer);
        auto variableResistors = findComponentsByType(ComponentType::VariableResistor);

        // 1. Input stage (Input -> coupling capacitor -> resistor -> buffer)
        if (!inputComps.empty()) {
            identifiedStages.push_back(identifyInputStage());
        }

        // 2. Op-amp based stages (gain, clipping, filtering)
        if (!opamps.empty()) {
            // Try to identify what type of op-amp stage this is
            if (!diodes.empty()) {
                identifiedStages.push_back(identifyClippingStage());
            } else {
                identifiedStages.push_back(identifyOpAmpStage());
            }
        }

        // 2b. Transistor-based gain stages (BJT/FET)
        if (!transistors.empty()) {
            identifiedStages.push_back(identifyTransistorStage());
        }

        // 3. Tone control stages
        if (!potentiometers.empty() || !variableResistors.empty()) {
            identifiedStages.push_back(identifyToneControlStage());
        }

        // 4. Passive filter stages (RC networks)
        if (!resistors.empty() && !capacitors.empty()) {
            identifiedStages.push_back(identifyFilterStage());
        }

        // 5. Output stage
        if (!outputComps.empty()) {
            identifiedStages.push_back(identifyOutputStage());

            // 6. Apply topology pattern matching to refine identifications
            applyPatternMatching();
        }

        return identifiedStages;
    }

    CircuitStage CircuitAnalyzer::identifyInputStage() {
        CircuitStage stage;
        stage.type = StageType::InputBuffer;
        stage.name = "Input Buffer";

        auto inputs = findComponentsByType(ComponentType::Input);
        for (const auto& input : inputs) {
            stage.components.push_back(input);
        }

        // Look for coupling capacitor and input resistor
        auto capacitors = findComponentsByType(ComponentType::Capacitor);
        auto resistors = findComponentsByType(ComponentType::Resistor);

        if (!capacitors.empty()) {
            stage.components.push_back(capacitors[0]);
            double cap = capacitors[0]->getParamValueAsDouble("Capacitance");
            stage.dspParams["coupling_capacitance"] = cap;

            if (!resistors.empty()) {
                stage.components.push_back(resistors[0]);
                double res = resistors[0]->getParamValueAsDouble("Resistance");
                stage.dspParams["input_resistance"] = res;

                // Calculate high-pass filter frequency: f = 1/(2*pi*R*C)
                double freq = 1.0 / (2.0 * M_PI * res * cap);
                stage.dspParams["highpass_frequency"] = freq;
            }
        }

        populateDSPMapping(stage);
        populateNonlinearComponents(stage);
        populatePatternInfo(stage);
        stage.patternName = "Tone Stack";
        stage.patternStrategy = "tone_stack";
        stage.patternConfidence = 0.9;
        return stage;
    }

    CircuitStage CircuitAnalyzer::identifyOutputStage() {
        CircuitStage stage;
        stage.type = StageType::OutputBuffer;
        stage.name = "Output Buffer";

        auto outputs = findComponentsByType(ComponentType::Output);
        for (const auto& output : outputs) {
            stage.components.push_back(output);
        }

        populateDSPMapping(stage);
        populateNonlinearComponents(stage);
        populatePatternInfo(stage);
        stage.patternName = "Tone Stack";
        stage.patternStrategy = "tone_stack";
        stage.patternConfidence = 0.9;
        return stage;
    }

    CircuitStage CircuitAnalyzer::identifyOpAmpStage() {
        CircuitStage stage;
        stage.type = StageType::GainStage;
        stage.name = "Op-Amp Gain Stage";

        auto opamps = findComponentsByType(ComponentType::OpAmp);
        if (!opamps.empty()) {
            stage.components.push_back(opamps[0]);
        }

        // Try to calculate gain from feedback network
        auto resistors = findComponentsByType(ComponentType::Resistor);
        if (resistors.size() >= 2) {
            double r1 = resistors[0]->getParamValueAsDouble("Resistance");
            double r2 = resistors[1]->getParamValueAsDouble("Resistance");
            if (r1 > 0) {
                double gain = 1.0 + (r2 / r1);
                stage.dspParams["gain_linear"] = gain;
                stage.dspParams["gain_db"] = 20.0 * log10(gain);
            }
        }

        populateDSPMapping(stage);
        populateNonlinearComponents(stage);
        populatePatternInfo(stage);
        return stage;
    }

    CircuitStage CircuitAnalyzer::identifyTransistorStage() {
        CircuitStage stage;
        stage.type = StageType::GainStage;
        stage.name = "Transistor Gain Stage";

        auto transistors = findComponentsByType(ComponentType::Transistor);
        for (const auto& transistor : transistors) {
            stage.components.push_back(transistor);
        }

        // Add nearby resistors/capacitors as context if present
        auto resistors = findComponentsByType(ComponentType::Resistor);
        auto capacitors = findComponentsByType(ComponentType::Capacitor);
        if (!resistors.empty()) {
            stage.components.push_back(resistors[0]);
        }
        if (!capacitors.empty()) {
            stage.components.push_back(capacitors[0]);
        }

        populateDSPMapping(stage);
        populateNonlinearComponents(stage);
        populatePatternInfo(stage);
        return stage;
    }

    CircuitStage CircuitAnalyzer::identifyFilterStage() {
        CircuitStage stage;
        stage.type = StageType::LowPassFilter;
        stage.name = "RC Low-Pass Filter";

        auto resistors = findComponentsByType(ComponentType::Resistor);
        auto capacitors = findComponentsByType(ComponentType::Capacitor);

        if (!resistors.empty() && !capacitors.empty()) {
            stage.components.push_back(resistors[0]);
            stage.components.push_back(capacitors[0]);

            double res = resistors[0]->getParamValueAsDouble("Resistance");
            double cap = capacitors[0]->getParamValueAsDouble("Capacitance");

            double cutoff = calculateFilterFrequency(res, cap);
            stage.dspParams["cutoff_frequency"] = cutoff;
        }

        populateDSPMapping(stage);
        populateNonlinearComponents(stage);
        populatePatternInfo(stage);
        return stage;
    }

    CircuitStage CircuitAnalyzer::identifyToneControlStage() {
        CircuitStage stage;
        stage.type = StageType::ToneControl;
        stage.name = "Tone Control";

        auto potentiometers = findComponentsByType(ComponentType::Potentiometer);
        auto resistors = findComponentsByType(ComponentType::Resistor);
        auto capacitors = findComponentsByType(ComponentType::Capacitor);

        if (!potentiometers.empty()) {
            stage.components.push_back(potentiometers[0]);
        }
        if (!resistors.empty()) {
            stage.components.push_back(resistors[0]);
        }
        if (!capacitors.empty()) {
            stage.components.push_back(capacitors[0]);
        }

        populateDSPMapping(stage);
        populateNonlinearComponents(stage);
        populatePatternInfo(stage);
        return stage;
    }

    CircuitStage CircuitAnalyzer::identifyClippingStage() {
        CircuitStage stage;
        stage.type = StageType::OpAmpClipping;
        stage.name = "Op-Amp Clipping Stage";

        auto opamps = findComponentsByType(ComponentType::OpAmp);
        auto diodes = findComponentsByType(ComponentType::Diode);

        for (const auto& opamp : opamps) {
            stage.components.push_back(opamp);
        }

        for (const auto& diode : diodes) {
            stage.components.push_back(diode);
            // Diode parameters
            double is = diode->getParamValueAsDouble("IS");
            double n = diode->getParamValueAsDouble("n");
            stage.dspParams["diode_IS"] = is;
            stage.dspParams["diode_n"] = n;
        }

        populateDSPMapping(stage);
        populateNonlinearComponents(stage);
        populatePatternInfo(stage);
        return stage;
    }

    CircuitStage CircuitAnalyzer::identifyToneControl() {
        CircuitStage stage;
        stage.type = StageType::ToneControl;
        stage.name = "Tone Control";

        populateDSPMapping(stage);
        populateNonlinearComponents(stage);
        populatePatternInfo(stage);
        return stage;
    }

    std::shared_ptr<Component> CircuitAnalyzer::findComponentByName(const std::string& name) const {
        return schematic.getNetlist().getComponent(name);
    }

    std::vector<std::shared_ptr<Component>> CircuitAnalyzer::findComponentsByType(ComponentType type) const {
        return circuitGraph.findComponentsByType(type);
    }

    double CircuitAnalyzer::calculateFilterFrequency(double resistance, double capacitance) {
        if (resistance <= 0 || capacitance <= 0) return 0.0;
        return 1.0 / (2.0 * M_PI * resistance * capacitance);
    }

    double CircuitAnalyzer::calculateGain(const CircuitStage& stage) {
        auto it = stage.dspParams.find("gain_linear");
        if (it != stage.dspParams.end()) {
            return it->second;
        }
        return 1.0;
    }

    std::string CircuitAnalyzer::generateReport() const {
        std::stringstream ss;
        ss << "=== Circuit Analysis Report ===\n\n";

        auto comps = schematic.getNetlist().getComponents();
        ss << "Total Components: " << comps.size() << "\n";

        // Component type count
        std::map<ComponentType, int> typeCounts;
        for (const auto& pair : comps) {
            typeCounts[pair.second->getType()]++;
        }

        ss << "\nComponent Breakdown:\n";
        ss << "  Resistors: " << typeCounts[ComponentType::Resistor] << "\n";
        ss << "  Capacitors: " << typeCounts[ComponentType::Capacitor] << "\n";
        ss << "  Op-Amps: " << typeCounts[ComponentType::OpAmp] << "\n";
        ss << "  Diodes: " << typeCounts[ComponentType::Diode] << "\n";
        ss << "  Potentiometers: " << typeCounts[ComponentType::Potentiometer] << "\n";
        ss << "  Variable Resistors: " << typeCounts[ComponentType::VariableResistor] << "\n";

        // Detailed component listing with values
        ss << "\n=== Detailed Component List ===\n";
        
        // Resistors
        if (typeCounts[ComponentType::Resistor] > 0) {
            ss << "\nResistors:\n";
            for (const auto& pair : comps) {
                if (pair.second->getType() == ComponentType::Resistor) {
                    std::string resistance = sanitizeUnicode(pair.second->getParamValue("Resistance"));
                    ss << "  " << pair.second->getName() << ": " << resistance << "\n";
                }
            }
        }

        // Capacitors
        if (typeCounts[ComponentType::Capacitor] > 0) {
            ss << "\nCapacitors:\n";
            for (const auto& pair : comps) {
                if (pair.second->getType() == ComponentType::Capacitor) {
                    std::string capacitance = sanitizeUnicode(pair.second->getParamValue("Capacitance"));
                    ss << "  " << pair.second->getName() << ": " << capacitance << "\n";
                }
            }
        }

        // Inductors
        if (typeCounts[ComponentType::Inductor] > 0) {
            ss << "\nInductors:\n";
            for (const auto& pair : comps) {
                if (pair.second->getType() == ComponentType::Inductor) {
                    std::string inductance = sanitizeUnicode(pair.second->getParamValue("Inductance"));
                    ss << "  " << pair.second->getName() << ": " << inductance << "\n";
                }
            }
        }

        // Variable Resistors
        if (typeCounts[ComponentType::VariableResistor] > 0) {
            ss << "\nVariable Resistors:\n";
            for (const auto& pair : comps) {
                if (pair.second->getType() == ComponentType::VariableResistor) {
                    std::string resistance = sanitizeUnicode(pair.second->getParamValue("Resistance"));
                    std::string wipe = pair.second->getParamValue("Wipe");
                    ss << "  " << pair.second->getName() << ": " << resistance;
                    if (!wipe.empty()) ss << " (Wipe: " << wipe << ")";
                    ss << "\n";
                }
            }
        }

        // Potentiometers
        if (typeCounts[ComponentType::Potentiometer] > 0) {
            ss << "\nPotentiometers:\n";
            for (const auto& pair : comps) {
                if (pair.second->getType() == ComponentType::Potentiometer) {
                    std::string resistance = sanitizeUnicode(pair.second->getParamValue("Resistance"));
                    std::string wipe = pair.second->getParamValue("Wipe");
                    ss << "  " << pair.second->getName() << ": " << resistance;
                    if (!wipe.empty()) ss << " (Wipe: " << wipe << ")";
                    ss << "\n";
                }
            }
        }

        // Diodes
        if (typeCounts[ComponentType::Diode] > 0) {
            ss << "\nDiodes:\n";
            for (const auto& pair : comps) {
                if (pair.second->getType() == ComponentType::Diode) {
                    std::string partNumber = pair.second->getParamValue("PartNumber");
                    std::string isParam = sanitizeUnicode(pair.second->getParamValue("IS"));
                    ss << "  " << pair.second->getName() << ": " << partNumber;
                    if (!isParam.empty()) ss << " (IS: " << isParam << ")";
                    ss << "\n";
                }
            }
        }

        // Op-Amps
        if (typeCounts[ComponentType::OpAmp] > 0) {
            ss << "\nOp-Amps:\n";
            for (const auto& pair : comps) {
                if (pair.second->getType() == ComponentType::OpAmp) {
                    std::string partNumber = pair.second->getParamValue("PartNumber");
                    std::string gbp = pair.second->getParamValue("GBP");
                    ss << "  " << pair.second->getName() << ": " << partNumber;
                    if (!gbp.empty()) ss << " (GBP: " << gbp << ")";
                    ss << "\n";
                }
            }
        }

        // Input sources
        if (typeCounts[ComponentType::Input] > 0) {
            ss << "\nInput Sources:\n";
            for (const auto& pair : comps) {
                if (pair.second->getType() == ComponentType::Input) {
                    std::string voltage = sanitizeUnicode(pair.second->getParamValue("V0dBFS"));
                    ss << "  " << pair.second->getName();
                    if (!voltage.empty()) ss << ": " << voltage;
                    ss << "\n";
                }
            }
        }

        // Power Rails
        if (typeCounts[ComponentType::Rail] > 0) {
            ss << "\nPower Rails:\n";
            for (const auto& pair : comps) {
                if (pair.second->getType() == ComponentType::Rail) {
                    std::string voltage = sanitizeUnicode(pair.second->getParamValue("Voltage"));
                    ss << "  " << pair.second->getName();
                    if (!voltage.empty()) ss << ": " << voltage;
                    ss << "\n";
                }
            }
        }

        ss << "\nIdentified Stages: " << identifiedStages.size() << "\n";
        for (const auto& stage : identifiedStages) {
            ss << "\n  Stage: " << stage.name << "\n";
            ss << "  Components: " << stage.components.size() << "\n";
            if (!stage.nonlinearComponents.empty()) {
                ss << "  Nonlinear Components: " << stage.nonlinearComponents.size() << "\n";
                for (const auto& nonlinear : stage.nonlinearComponents) {
                    ss << "    - " << nonlinear.name << " (" << nonlinear.typeString() << ")";
                    if (!nonlinear.partNumber.empty()) {
                        ss << " [" << nonlinear.partNumber << "]";
                    }
                    ss << "\n";
                }
            }
            if (!stage.patternName.empty()) {
                ss << "  Pattern Match: " << stage.patternName;
                ss << " (" << stage.patternStrategy << ", confidence "
                   << std::fixed << std::setprecision(2) << stage.patternConfidence << ")\n";
            }
            
            // Add DSP mapping information
            ss << "  LiveSPICE DSP Mapping: " << stage.dspDescription << "\n";
            
            if (!stage.dspParams.empty()) {
                ss << "  DSP Parameters:\n";
                for (const auto& param : stage.dspParams) {
                    ss << "    " << param.first << " = " << param.second << "\n";
                }
            }
        }

        ss << "\nWires: " << schematic.getNetlist().getWireCount() << "\n";

        return ss.str();
    }

    std::string CircuitAnalyzer::generateConnectivityReport() const {
        // Build connectivity pool if not already done
        const Netlist& netlist = schematic.getNetlist();
        const_cast<Netlist&>(netlist).buildConnectivityPool();

        std::stringstream ss;
        ss << "\n=== Circuit Connectivity Map ===\n\n";

        // Get connectivity information
        const auto& connectivityPool = netlist.getConnectivityPool();
        const auto& componentConnections = netlist.getComponentConnections();

        ss << "Total Connection Nodes: " << connectivityPool.size() << "\n";
        ss << "Total Component Connections: " << componentConnections.size() << "\n";

        // List all connection nodes and their components
        ss << "\n=== Connection Nodes ===\n";
        int nodeId = 0;
        for (const auto& nodeEntry : connectivityPool) {
            const auto& node = nodeEntry.first;
            const auto& components = nodeEntry.second;
            
            if (!components.empty()) {
                ss << "\nNode " << nodeId << " (Position: " << node.x << ", " << node.y << ")\n";
                ss << "  Connected Components:\n";
                for (const auto& compName : components) {
                    auto comp = netlist.getComponent(compName);
                    if (comp) {
                        std::string typeStr;
                        switch (comp->getType()) {
                            case ComponentType::Resistor: typeStr = "Resistor"; break;
                            case ComponentType::Capacitor: typeStr = "Capacitor"; break;
                            case ComponentType::Inductor: typeStr = "Inductor"; break;
                            case ComponentType::VariableResistor: typeStr = "VariableResistor"; break;
                            case ComponentType::Potentiometer: typeStr = "Potentiometer"; break;
                            case ComponentType::Diode: typeStr = "Diode"; break;
                            case ComponentType::OpAmp: typeStr = "OpAmp"; break;
                            case ComponentType::Speaker: typeStr = "Speaker"; break;
                            case ComponentType::Input: typeStr = "Input"; break;
                            case ComponentType::Output: typeStr = "Output"; break;
                            case ComponentType::Ground: typeStr = "Ground"; break;
                            case ComponentType::Rail: typeStr = "Rail"; break;
                            default: typeStr = "Unknown"; break;
                        }
                        ss << "    - " << compName << " (" << typeStr << ")\n";
                    }
                }
                nodeId++;
            }
        }

        // List component connections
        ss << "\n=== Component Connection Details ===\n";
        for (const auto& compEntry : componentConnections) {
            const auto& compConn = compEntry.second;
            
            if (compConn.connectedComponents.empty() && compConn.connectedNodes.size() <= 1) {
                continue; // Skip isolated components
            }
            
            ss << "\n" << compConn.componentName << " (" << compConn.componentType << ")\n";
            ss << "  Position: (" << compConn.posX << ", " << compConn.posY << ")\n";
            ss << "  Connected To (" << compConn.connectedComponents.size() << " components):\n";
            
            if (compConn.connectedComponents.empty()) {
                ss << "    (No other components directly connected)\n";
            } else {
                for (const auto& connectedComp : compConn.connectedComponents) {
                    auto comp = netlist.getComponent(connectedComp);
                    if (comp) {
                        std::string typeStr;
                        switch (comp->getType()) {
                            case ComponentType::Resistor: typeStr = "Resistor"; break;
                            case ComponentType::Capacitor: typeStr = "Capacitor"; break;
                            case ComponentType::Inductor: typeStr = "Inductor"; break;
                            case ComponentType::VariableResistor: typeStr = "VariableResistor"; break;
                            case ComponentType::Potentiometer: typeStr = "Potentiometer"; break;
                            case ComponentType::Diode: typeStr = "Diode"; break;
                            case ComponentType::OpAmp: typeStr = "OpAmp"; break;
                            case ComponentType::Speaker: typeStr = "Speaker"; break;
                            case ComponentType::Input: typeStr = "Input"; break;
                            case ComponentType::Output: typeStr = "Output"; break;
                            case ComponentType::Ground: typeStr = "Ground"; break;
                            case ComponentType::Rail: typeStr = "Rail"; break;
                            default: typeStr = "Unknown"; break;
                        }
                        ss << "    - " << connectedComp << " (" << typeStr << ")\n";
                    }
                }
            }
            
            if (!compConn.connectedNodes.empty()) {
                ss << "  Connection Nodes (" << compConn.connectedNodes.size() << "):\n";
                for (const auto& node : compConn.connectedNodes) {
                    ss << "    (" << node.x << ", " << node.y << ")\n";
                }
            }
        }

        return ss.str();
    }

    // ============================================================================
    // LiveSPICE DSP Mapping - Populate stage with component DSP information
    // ============================================================================
    void CircuitAnalyzer::populateDSPMapping(CircuitStage& stage) {
        // Find the primary active component in the stage
        std::shared_ptr<Component> primaryComponent = nullptr;
        
        for (const auto& comp : stage.components) {
            ComponentType type = comp->getType();
            
            // Prioritize active components
            if (type == ComponentType::OpAmp || 
                type == ComponentType::Diode || 
                type == ComponentType::Transistor) {
                primaryComponent = comp;
                break;
            }
        }
        
        // If no active component, use the first passive component
        if (!primaryComponent && !stage.components.empty()) {
            for (const auto& comp : stage.components) {
                ComponentType type = comp->getType();
                if (type == ComponentType::Resistor || 
                    type == ComponentType::Capacitor || 
                    type == ComponentType::Inductor) {
                    primaryComponent = comp;
                    break;
                }
            }
        }
        
        // Map to DSP processor type
        if (primaryComponent) {
            stage.primaryProcessorType = dspMapper.mapComponentToProcessor(primaryComponent);
            stage.dspDescription = dspMapper.getComponentDSPDescription(primaryComponent);
        } else {
            stage.primaryProcessorType = ComponentDSPMapper::DSPProcessorType::Unknown;
            stage.dspDescription = "No DSP mapping available";
        }
    }

    void CircuitAnalyzer::populatePatternInfo(CircuitStage& stage) {
        // Pattern matching is now handled in applyPatternMatching()
        // This method is preserved for backward compatibility with stage initialization
        stage.patternName = "Unknown";
        stage.patternStrategy = "unknown";
        stage.patternConfidence = 0.0;
    }

    void CircuitAnalyzer::populateNonlinearComponents(CircuitStage& stage) {
        stage.nonlinearComponents.clear();

        for (const auto& comp : stage.components) {
            if (!comp) continue;

            ComponentType type = comp->getType();
            bool isTransistorFallback = false;
            if (type != ComponentType::Diode && type != ComponentType::Transistor) {
                const std::string compName = comp->getName();
                if (!compName.empty() && (compName[0] == 'Q' || compName[0] == 'M')) {
                    isTransistorFallback = true;
                } else {
                    continue;
                }
            }

            std::string partNumber = normalizePartNumber(comp->getParamValue("PartNumber"));
            if (partNumber.empty()) {
                partNumber = normalizePartNumber(comp->getParamValue("Model"));
            }
            if (partNumber.empty()) {
                partNumber = normalizePartNumber(comp->getParamValue("Value"));
            }

            const std::string componentName = comp->getName();

            if (type == ComponentType::Diode) {
                if (partNumber.empty()) {
                    partNumber = "1N4148";
                }
                stage.nonlinearComponents.push_back(
                    Nonlinear::ComponentDB::NonlinearComponentInfo::fromDiode(partNumber, componentName)
                );
                continue;
            }

            // Treat fallback as transistor
            if (isTransistorFallback && type != ComponentType::Transistor) {
                type = ComponentType::Transistor;
            }

            const std::string typeParam = comp->getParamValue("Type");
            const bool isPNP = isLikelyPNP(partNumber, typeParam);

            auto bjtMatch = Nonlinear::ComponentDB::getBJTDB().lookup(partNumber);
            auto fetMatch = Nonlinear::ComponentDB::getFETDB().lookup(partNumber);

            if (bjtMatch.has_value()) {
                stage.nonlinearComponents.push_back(
                    Nonlinear::ComponentDB::NonlinearComponentInfo::fromBJT(partNumber, componentName, isPNP)
                );
                continue;
            }

            if (fetMatch.has_value()) {
                stage.nonlinearComponents.push_back(
                    Nonlinear::ComponentDB::NonlinearComponentInfo::fromFET(partNumber, componentName, isPNP)
                );
                continue;
            }

            if (!componentName.empty() && componentName[0] == 'M') {
                stage.nonlinearComponents.push_back(
                    Nonlinear::ComponentDB::NonlinearComponentInfo::fromFET(
                        partNumber.empty() ? "2N7000" : partNumber,
                        componentName,
                        isPNP
                    )
                );
                continue;
            }

            stage.nonlinearComponents.push_back(
                Nonlinear::ComponentDB::NonlinearComponentInfo::fromBJT(
                    partNumber.empty() ? "2N3904" : partNumber,
                    componentName,
                    isPNP
                )
            );
        }
    }

    void CircuitAnalyzer::applyPatternMatching() {
        for (auto& stage : identifiedStages) {
            if (stage.components.empty()) continue;

            // Convert stage components to TopologyPatterns format
            std::vector<TopologyAnalysis::Component> patternComponents;
            for (size_t i = 0; i < stage.components.size(); ++i) {
                const auto& comp = stage.components[i];
                if (!comp) continue;
                
                TopologyAnalysis::Component pcomp;
                pcomp.id = comp->getName();
                pcomp.type = comp->getType();
                pcomp.partNumber = comp->getParamValue("PartNumber");
                pcomp.value = static_cast<float>(comp->getParamValueAsDouble("Value"));
                patternComponents.push_back(pcomp);
            }

            // Empty connections for now (simplified matching)
            std::vector<TopologyAnalysis::Connection> connections;
            
            // Try to match against all patterns
            auto bestMatch = patternRegistry.matchPattern(patternComponents, connections);
            if (bestMatch.pattern && bestMatch.confidence > 0.0f) {
                stage.patternName = bestMatch.pattern->name;
                
                // Map pattern category to strategy string
                switch (bestMatch.pattern->category) {
                    case TopologyAnalysis::PatternCategory::PassiveFilter:
                        stage.patternStrategy = "passive_filter"; break;
                    case TopologyAnalysis::PatternCategory::ActiveFilter:
                        stage.patternStrategy = "active_filter"; break;
                    case TopologyAnalysis::PatternCategory::AmplifierStage:
                        stage.patternStrategy = "amplifier"; break;
                    case TopologyAnalysis::PatternCategory::ClippingStage:
                        stage.patternStrategy = "clipping"; break;
                    case TopologyAnalysis::PatternCategory::ToneControl:
                        stage.patternStrategy = "tone_control"; break;
                    case TopologyAnalysis::PatternCategory::FeedbackNetwork:
                        stage.patternStrategy = "feedback"; break;
                    case TopologyAnalysis::PatternCategory::Coupling:
                        stage.patternStrategy = "coupling"; break;
                    case TopologyAnalysis::PatternCategory::Resonant:
                        stage.patternStrategy = "resonant"; break;
                    default:
                        stage.patternStrategy = "unknown";
                }
                
                stage.patternConfidence = bestMatch.confidence;
                stage.dspDescription = bestMatch.pattern->description;
                if (!bestMatch.pattern->dspStrategy.empty()) {
                    stage.dspDescription += " -> " + bestMatch.pattern->dspStrategy;
                }
            }
        }
    }

} // namespace LiveSpice
