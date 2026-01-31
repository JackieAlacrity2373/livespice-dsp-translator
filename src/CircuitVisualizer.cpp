#include "CircuitVisualizer.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace LiveSpice {

    std::string CircuitVisualizer::componentTypeToString(ComponentType type) const {
        switch (type) {
            case ComponentType::Resistor: return "Resistor";
            case ComponentType::Capacitor: return "Capacitor";
            case ComponentType::Inductor: return "Inductor";
            case ComponentType::VariableResistor: return "Variable Resistor";
            case ComponentType::Potentiometer: return "Potentiometer";
            case ComponentType::Diode: return "Diode";
            case ComponentType::Transformer: return "Transformer";
            case ComponentType::OpAmp: return "Op-Amp";
            case ComponentType::Transistor: return "Transistor";
            case ComponentType::Speaker: return "Speaker";
            case ComponentType::Input: return "Input Jack";
            case ComponentType::Output: return "Output Jack";
            case ComponentType::Ground: return "Ground";
            case ComponentType::Rail: return "Power Rail";
            case ComponentType::Wire: return "Wire";
            case ComponentType::Label: return "Label";
            default: return "Unknown";
        }
    }

    std::string CircuitVisualizer::stageTypeToString(StageType type) const {
        switch (type) {
            case StageType::InputBuffer: return "Input Buffer";
            case StageType::GainStage: return "Gain Stage";
            case StageType::HighPassFilter: return "High-Pass Filter";
            case StageType::LowPassFilter: return "Low-Pass Filter";
            case StageType::BandPassFilter: return "Band-Pass Filter";
            case StageType::OpAmpClipping: return "Op-Amp Clipping";
            case StageType::DiodeClipper: return "Diode Clipper";
            case StageType::ToneControl: return "Tone Control";
            case StageType::OutputBuffer: return "Output Buffer";
            default: return "Unknown Stage";
        }
    }

    std::string CircuitVisualizer::componentTypeToSymbol(ComponentType type) const {
        switch (type) {
            case ComponentType::Resistor: return "[R]";
            case ComponentType::Capacitor: return "[C]";
            case ComponentType::Inductor: return "[L]";
            case ComponentType::VariableResistor: return "[Rv]";
            case ComponentType::Potentiometer: return "[Pot]";
            case ComponentType::Diode: return "[D]";
            case ComponentType::OpAmp: return "[+]";
            case ComponentType::Transistor: return "[Q]";
            case ComponentType::Speaker: return "[Spk]";
            case ComponentType::Input: return "[IN]";
            case ComponentType::Output: return "[OUT]";
            case ComponentType::Ground: return "[GND]";
            case ComponentType::Rail: return "[+V]";
            default: return "[?]";
        }
    }

    std::string CircuitVisualizer::formatComponentValue(const std::string& value) const {
        if (value.empty()) return "N/A";
        return value;
    }

    std::string CircuitVisualizer::getExpectedDSPModule(StageType stage) const {
        switch (stage) {
            case StageType::InputBuffer: return "juce::dsp::Gain (unity buffer)";
            case StageType::GainStage: return "juce::dsp::Gain";
            case StageType::HighPassFilter: return "juce::dsp::IIR::Filter (makeHighPass)";
            case StageType::LowPassFilter: return "juce::dsp::IIR::Filter (makeLowPass)";
            case StageType::BandPassFilter: return "juce::dsp::IIR::Filter (makeBandPass)";
            case StageType::OpAmpClipping: return "juce::dsp::WaveShaper (tanh/soft clip)";
            case StageType::DiodeClipper: return "juce::dsp::WaveShaper (asymmetric clip)";
            case StageType::ToneControl: return "juce::dsp::IIR::Filter (parametric)";
            case StageType::OutputBuffer: return "juce::dsp::Gain (output stage)";
            default: return "Unknown DSP Module";
        }
    }

    std::string CircuitVisualizer::generateComponentDetailsTable() const {
        const Netlist& netlist = schematic.getNetlist();
        std::ostringstream ss;

        ss << "+" << std::string(92, '-') << "+\n";
        ss << "| PARSED COMPONENTS FROM LIVESPICE FILE                                              |\n";
        ss << "+---------+----------------------+----+--------+---------------------------+--------+\n";
        ss << "| Ref     | Type                 | Val|ue      | Properties               | Pos    |\n";
        ss << "+---------+----------------------+----+--------+---------------------------+--------+\n";

        for (const auto& pair : netlist.getComponents()) {
            const auto& comp = pair.second;
            
            if (comp->getType() == ComponentType::Wire || comp->getType() == ComponentType::Label) {
                continue; // Skip non-components
            }

            std::string type = componentTypeToString(comp->getType());
            type = type.substr(0, 20); // Truncate if too long
            type.resize(20, ' ');

            std::string value = formatComponentValue(comp->getParamValue("Value"));
            value = value.substr(0, 12);
            value.resize(12, ' ');

            std::string props;
            if (comp->getType() == ComponentType::Potentiometer) {
                props = "Wipe: " + formatComponentValue(comp->getParamValue("Wipe"));
                std::string taper = comp->getParamValue("Taper");
                if (!taper.empty()) props += " | Taper: " + taper;
            }
            props = props.substr(0, 26);
            props.resize(26, ' ');

            int x, y;
            comp->getPosition(x, y);
            std::ostringstream pos;
            pos << "(" << x << "," << y << ")";
            std::string posStr = pos.str().substr(0, 8);
            posStr.resize(8, ' ');

            ss << "| " << std::setw(7) << std::left << comp->getName() << " | " 
               << type << " | " << value << " | " << props << " | " << posStr << " |\n";
        }

        ss << "+---------+----------------------+--+----------+---------------------------+--------+\n";
        ss << "Total Components Parsed: " << netlist.getComponentCount() << "\n";

        return ss.str();
    }

    std::string CircuitVisualizer::generateParsedComponents() const {
        return generateComponentDetailsTable();
    }

    std::string CircuitVisualizer::generateNodeConnectivityTable() const {
        const Netlist& netlist = schematic.getNetlist();
        std::ostringstream ss;

        ss << "\n" << std::string(110, '=') << "\n";
        ss << "ANALYZED NODE CONNECTIVITY (How Program Determined Components Connect)\n";
        ss << std::string(110, '=') << "\n\n";

        const auto& componentConnections = netlist.getConnectivityPool();
        
        // Group components by their position to show connections
        std::map<std::string, std::vector<std::string>> connectionMap;
        
        for (const auto& pair : netlist.getComponents()) {
            const auto& comp = pair.second;
            if (comp->getType() == ComponentType::Wire || comp->getType() == ComponentType::Label) {
                continue;
            }

            std::string compName = comp->getName();
            connectionMap[compName] = std::vector<std::string>();
        }

        // Analyze wire connections to determine connectivity
        for (const auto& wire : netlist.getWires()) {
            // Find components at wire endpoints
            std::vector<std::string> connectedAtNodeA;
            std::vector<std::string> connectedAtNodeB;

            for (const auto& pair : netlist.getComponents()) {
                const auto& comp = pair.second;
                int x, y;
                comp->getPosition(x, y);
                
                if (x == wire.nodeA_X && y == wire.nodeA_Y) {
                    connectedAtNodeA.push_back(comp->getName());
                }
                if (x == wire.nodeB_X && y == wire.nodeB_Y) {
                    connectedAtNodeB.push_back(comp->getName());
                }
            }

            // Cross-link the connections
            for (const auto& compA : connectedAtNodeA) {
                for (const auto& compB : connectedAtNodeB) {
                    if (compA != compB) {
                        connectionMap[compA].push_back(compB);
                    }
                }
            }
        }

        // Output the connectivity table
        for (const auto& pair : connectionMap) {
            std::string compName = pair.first;
            const auto& connections = pair.second;

            ss << "  " << std::setw(20) << std::left << (compName + ":") << " -> ";

            if (connections.empty()) {
                ss << "(No connections found)";
            } else {
                for (size_t i = 0; i < connections.size(); ++i) {
                    if (i > 0) ss << ", ";
                    ss << connections[i];
                }
            }
            ss << "\n";
        }

        ss << "\n";

        return ss.str();
    }

    std::string CircuitVisualizer::generateAnalyzedConnectivity() const {
        return generateNodeConnectivityTable();
    }

    std::string CircuitVisualizer::generateStageBreakdown() const {
        std::ostringstream ss;
        auto stages = const_cast<CircuitAnalyzer&>(analyzer).analyzeCircuit();

        ss << "\n" << std::string(110, '=') << "\n";
        ss << "IDENTIFIED CIRCUIT STAGES (Program's Topology Analysis)\n";
        ss << std::string(110, '=') << "\n\n";

        if (stages.empty()) {
            ss << "WARNING: No circuit stages identified. Check parsing and analysis.\n";
            return ss.str();
        }

        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];

            ss << "  STAGE " << (i + 1) << ": " << stageTypeToString(stage.type) << "\n";
            ss << "  " << std::string(72, '-') << "\n";
            ss << "  Expected DSP Module: " << getExpectedDSPModule(stage.type) << "\n\n";

            ss << "  Components in this stage:\n";
            for (const auto& comp : stage.components) {
                ss << "    * " << comp->getName() << " (" << componentTypeToString(comp->getType()) << ")\n";
                auto params = comp->getParams();
                if (!params.empty()) {
                    for (const auto& param : params) {
                        ss << "      - " << param.name << ": " << param.value;
                        if (!param.unit.empty()) ss << " " << param.unit;
                        ss << "\n";
                    }
                }
            }

            ss << "\n  DSP Parameters:\n";
            for (const auto& param : stage.dspParams) {
                ss << "    * " << param.first << ": " << std::fixed << std::setprecision(2) << param.second << "\n";
            }

            ss << "\n";
        }

        return ss.str();
    }

    std::string CircuitVisualizer::generateIdentifiedStages() const {
        return generateStageBreakdown();
    }

    std::string CircuitVisualizer::generatePotentiometerDetailsTable() const {
        const Netlist& netlist = schematic.getNetlist();
        std::ostringstream ss;

        // Find all potentiometers
        std::vector<std::shared_ptr<Component>> potentiometers;
        for (const auto& pair : netlist.getComponents()) {
            if (pair.second->getType() == ComponentType::Potentiometer ||
                pair.second->getType() == ComponentType::VariableResistor) {
                potentiometers.push_back(pair.second);
            }
        }

        ss << "\n" << std::string(110, '=') << "\n";
        ss << "EXTRACTED POTENTIOMETERS & CONTROLS (User Input Parameters)\n";
        ss << std::string(110, '=') << "\n\n";

        if (potentiometers.empty()) {
            ss << "  [none] - No potentiometers found\n";
        } else {
            for (const auto& pot : potentiometers) {
                std::string type = (pot->getType() == ComponentType::Potentiometer) ? "Potentiometer" : "Var Resistor";
                std::string value = formatComponentValue(pot->getParamValue("Value"));
                std::string wipe = formatComponentValue(pot->getParamValue("Wipe"));
                std::string taper = formatComponentValue(pot->getParamValue("Taper"));

                ss << "  " << std::setw(10) << std::left << pot->getName() 
                   << " | " << std::setw(14) << std::left << type
                   << " | " << std::setw(14) << std::left << value
                   << " | " << std::setw(14) << std::left << wipe
                   << " | " << std::setw(23) << std::left << taper << "\n";
            }
        }

        ss << "\n";

        if (!potentiometers.empty()) {
            ss << "Control Mapping Strategy:\n";
            ss << "  Each potentiometer will be converted to an AudioParameter in JUCE:\n";
            for (size_t i = 0; i < potentiometers.size(); ++i) {
                const auto& pot = potentiometers[i];
                std::string paramName = pot->getName();
                std::transform(paramName.begin(), paramName.end(), paramName.begin(), ::tolower);
                
                ss << "    " << (i + 1) << ". \"" << pot->getName() << "\" -> juce::AudioParameterFloat(\"" 
                   << paramName << "\", \"" << pot->getName() << "\", 0.0f, 1.0f, 0.5f)\n";
            }
        }

        return ss.str();
    }

    std::string CircuitVisualizer::generateExtractedControls() const {
        return generatePotentiometerDetailsTable();
    }

    std::string CircuitVisualizer::generateExtractionSummary() const {
        const Netlist& netlist = schematic.getNetlist();
        std::ostringstream ss;

        int componentCount = 0;
        int resistorCount = 0;
        int capacitorCount = 0;
        int potCount = 0;
        int opampCount = 0;
        int diodeCount = 0;
        int inputCount = 0;
        int outputCount = 0;

        for (const auto& pair : netlist.getComponents()) {
            const auto& comp = pair.second;
            if (comp->getType() == ComponentType::Wire || comp->getType() == ComponentType::Label) {
                continue;
            }
            componentCount++;

            switch (comp->getType()) {
                case ComponentType::Resistor: resistorCount++; break;
                case ComponentType::Capacitor: capacitorCount++; break;
                case ComponentType::Potentiometer:
                case ComponentType::VariableResistor: potCount++; break;
                case ComponentType::OpAmp: opampCount++; break;
                case ComponentType::Diode: diodeCount++; break;
                case ComponentType::Input: inputCount++; break;
                case ComponentType::Output: outputCount++; break;
                default: break;
            }
        }

        auto stages = const_cast<CircuitAnalyzer&>(analyzer).analyzeCircuit();

        ss << "\n" << std::string(110, '=') << "\n";
        ss << "EXTRACTION SUMMARY (What Your Program Understood)\n";
        ss << std::string(110, '=') << "\n\n";

        ss << "  Parsing Results:\n";
        ss << "    OK Total components extracted:  " << componentCount << "\n";
        ss << "    OK Resistors:                   " << resistorCount << "\n";
        ss << "    OK Capacitors:                  " << capacitorCount << "\n";
        ss << "    OK Potentiometers/Variables:    " << potCount << "\n";
        ss << "    OK Op-Amps:                     " << opampCount << "\n";
        ss << "    OK Diodes:                      " << diodeCount << "\n";
        ss << "    OK Input jacks:                 " << inputCount << "\n";
        ss << "    OK Output jacks:                " << outputCount << "\n";
        ss << "    OK Total wires/connections:     " << netlist.getWireCount() << "\n\n";

        ss << "  Circuit Analysis Results:\n";
        ss << "    OK Stages identified:           " << stages.size() << "\n";
        if (!stages.empty()) {
            for (size_t i = 0; i < stages.size(); ++i) {
                ss << "      " << (i + 1) << ". " << stageTypeToString(stages[i].type) 
                   << " (" << stages[i].components.size() << " components)\n";
            }
        }

        ss << "\n";

        return ss.str();
    }

    std::string CircuitVisualizer::generateAnalyzedSignalFlow() const {
        auto stages = const_cast<CircuitAnalyzer&>(analyzer).analyzeCircuit();
        std::ostringstream ss;

        ss << "\n" << std::string(110, '=') << "\n";
        ss << "ANALYZED SIGNAL FLOW (Program's Understanding of Audio Path)\n";
        ss << std::string(110, '=') << "\n\n";

        if (stages.empty()) {
            ss << "  WARNING: No signal flow determined. Circuit analysis may have failed.\n";
            return ss.str();
        }

        ss << "  INPUT\n";
        ss << "    |\n";
        ss << "    V\n";

        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            ss << "  +---------+\n";
            ss << "  | STAGE " << (i + 1) << ": " << stageTypeToString(stage.type) << "\n";
            ss << "  | DSP: " << getExpectedDSPModule(stage.type) << "\n";
            
            if (!stage.dspParams.empty()) {
                ss << "  |\n";
                ss << "  | Parameters:\n";
                for (const auto& param : stage.dspParams) {
                    ss << "  |   " << param.first << " = " << std::fixed << std::setprecision(2) << param.second << "\n";
                }
            }
            ss << "  +---------+\n";
            if (i < stages.size() - 1) {
                ss << "    |\n";
                ss << "    V\n";
            }
        }

        ss << "    |\n";
        ss << "    V\n";
        ss << "  OUTPUT\n\n";

        return ss.str();
    }

    std::string CircuitVisualizer::generateTroubleshootingGuide() const {
        const Netlist& netlist = schematic.getNetlist();
        auto stages = const_cast<CircuitAnalyzer&>(analyzer).analyzeCircuit();
        std::ostringstream ss;

        ss << "\n" << std::string(110, '=') << "\n";
        ss << "TROUBLESHOOTING GUIDE (Validating Extraction)\n";
        ss << std::string(110, '=') << "\n\n";

        // Check 1: Input/Output jacks
        ss << "  Check 1: Input/Output Configuration\n";
        bool hasInput = false, hasOutput = false;
        for (const auto& pair : netlist.getComponents()) {
            if (pair.second->getType() == ComponentType::Input) hasInput = true;
            if (pair.second->getType() == ComponentType::Output) hasOutput = true;
        }
        ss << "    * Input jacks found:  " << (hasInput ? "YES" : "NO") << "\n";
        ss << "    * Output jacks found: " << (hasOutput ? "YES" : "NO") << "\n";
        if (!hasInput || !hasOutput) {
            ss << "    WARNING: Missing input or output jacks. Circuit topology may be incomplete.\n";
        }
        ss << "\n";

        // Check 2: Active components
        ss << "  Check 2: Active Components (Op-Amps, Transistors)\n";
        int activeCount = 0;
        for (const auto& pair : netlist.getComponents()) {
            if (pair.second->getType() == ComponentType::OpAmp || 
                pair.second->getType() == ComponentType::Transistor) {
                activeCount++;
            }
        }
        ss << "    * Active components found: " << activeCount << "\n";
        if (activeCount == 0) {
            ss << "    WARNING: No active components (op-amps, transistors). Passive only circuit.\n";
        }
        ss << "\n";

        // Check 3: Power supply
        ss << "  Check 3: Power Supply\n";
        bool hasPower = false;
        for (const auto& pair : netlist.getComponents()) {
            if (pair.second->getType() == ComponentType::Rail) {
                hasPower = true;
                break;
            }
        }
        ss << "    * Power rails found: " << (hasPower ? "YES" : "NO") << "\n";
        if (!hasPower) {
            ss << "    WARNING: No power rail found. May need manual power supply connections.\n";
        }
        ss << "\n";

        // Check 4: Stage identification
        ss << "  Check 4: Circuit Stage Analysis\n";
        ss << "    * Stages identified: " << stages.size() << "\n";
        if (stages.empty()) {
            ss << "    ERROR: No stages identified. Parser may not have recognized component topology.\n";
            ss << "    -> Verify all components are correctly identified in the LiveSpice file.\n";
        } else {
            ss << "    OK Expected DSP signal flow created.\n";
        }
        ss << "\n";

        // Check 5: Potentiometers/Controls
        ss << "  Check 5: User Controls (Potentiometers)\n";
        int potCount = 0;
        for (const auto& pair : netlist.getComponents()) {
            if (pair.second->getType() == ComponentType::Potentiometer ||
                pair.second->getType() == ComponentType::VariableResistor) {
                potCount++;
            }
        }
        ss << "    * Potentiometers found: " << potCount << "\n";
        if (potCount == 0) {
            ss << "    WARNING: No potentiometers found. Circuit may be static (no knobs).\n";
        } else {
            ss << "    OK Each potentiometer will create an APVTS AudioParameter.\n";
        }
        ss << "\n";

        // Check 6: Connectivity
        ss << "  Check 6: Connectivity Analysis\n";
        ss << "    * Total wires in file: " << netlist.getWireCount() << "\n";
        if (netlist.getWireCount() == 0) {
            ss << "    ERROR: No wires detected. Components may not be connected.\n";
        }
        ss << "\n";

        ss << "  " << std::string(108, '=') << "\n";
        ss << "  NEXT STEPS:\n";
        ss << "    1. Review extracted components - do they match your schematic?\n";
        ss << "    2. Check identified stages - is the signal flow what you expect?\n";
        ss << "    3. If potentiometers missing: verify they're defined in LiveSpice file as Potentiometer type\n";
        ss << "    4. If connectivity looks wrong: check wire definitions in schematic\n";
        ss << "    5. If stages not identified: review CircuitAnalyzer stage detection logic\n";
        ss << "  " << std::string(108, '=') << "\n";

        return ss.str();
    }

    std::string CircuitVisualizer::generateFullDiagram() const {
        std::ostringstream ss;

        ss << "\n";
        ss << std::string(110, '=') << "\n";
        ss << "LIVESPICE DSP TRANSLATION - EXTRACTED CIRCUIT ANALYSIS\n";
        ss << "What Your Program Extracted & Analyzed from the LiveSpice File\n";
        ss << std::string(110, '=') << "\n";

        ss << generateExtractionSummary();
        ss << generateComponentDetailsTable();
        ss << generateNodeConnectivityTable();
        ss << generateStageBreakdown();
        ss << generatePotentiometerDetailsTable();
        ss << generateAnalyzedSignalFlow();
        ss << generateTroubleshootingGuide();

        ss << "\n";
        ss << std::string(110, '=') << "\n";
        ss << "END OF CIRCUIT ANALYSIS\n";
        ss << std::string(110, '=') << "\n";

        return ss.str();
    }

} // namespace LiveSpice
