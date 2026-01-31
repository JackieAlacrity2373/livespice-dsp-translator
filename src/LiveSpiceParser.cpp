#include "LiveSpiceParser.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <cmath>
#include <algorithm>

namespace LiveSpice {

    // ============================================================================
    // Unit Parsing - Convert values with units to doubles
    // ============================================================================
    double Component::parseUnit(const std::string& valueStr) const {
        if (valueStr.empty()) return 0.0;

        // Trim whitespace
        std::string trimmed = valueStr;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
        trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

        // Find where the number ends and unit begins
        size_t unitStart = 0;
        for (size_t i = 0; i < trimmed.length(); i++) {
            if (isalpha(trimmed[i])) {
                unitStart = i;
                break;
            }
        }

        std::string numStr = trimmed.substr(0, unitStart);
        std::string unitStr = trimmed.substr(unitStart);

        // Parse number
        double value = 0.0;
        try {
            value = std::stod(numStr);
        } catch (...) {
            return 0.0;
        }

        // Apply unit multiplier
        if (unitStr.find("p") != std::string::npos || unitStr.find("pF") != std::string::npos) {
            return value * 1e-12;
        } else if (unitStr.find("n") != std::string::npos || unitStr.find("nF") != std::string::npos) {
            return value * 1e-9;
        } else if (unitStr.find("u") != std::string::npos || unitStr.find("μ") != std::string::npos || unitStr.find("uF") != std::string::npos || unitStr.find("μF") != std::string::npos) {
            return value * 1e-6;
        } else if (unitStr.find("m") != std::string::npos && unitStr.find("mF") != std::string::npos) {
            return value * 1e-3;
        } else if (unitStr.find("k") != std::string::npos || unitStr.find("kΩ") != std::string::npos) {
            return value * 1e3;
        } else if (unitStr.find("M") != std::string::npos || unitStr.find("MΩ") != std::string::npos) {
            return value * 1e6;
        } else if (unitStr.find("G") != std::string::npos) {
            return value * 1e9;
        } else if (unitStr.find("fA") != std::string::npos) {
            return value * 1e-15;
        } else if (unitStr.find("pA") != std::string::npos) {
            return value * 1e-12;
        } else if (unitStr.find("nA") != std::string::npos) {
            return value * 1e-9;
        } else if (unitStr.find("uA") != std::string::npos || unitStr.find("μA") != std::string::npos) {
            return value * 1e-6;
        } else if (unitStr.find("mA") != std::string::npos) {
            return value * 1e-3;
        } else if (unitStr.find("V") != std::string::npos) {
            return value; // Volts are base unit
        } else if (unitStr.find("Ω") != std::string::npos) {
            return value; // Ohms are base unit
        }

        // No unit recognized, return raw value
        return value;
    }

    // ============================================================================
    // Component Type Recognition
    // ============================================================================
    ComponentType SchematicParser::getComponentType(const std::string& typeStr) {
        // Check for component type in the _Type attribute
        if (typeStr.find("Resistor") != std::string::npos) {
            if (typeStr.find("Variable") != std::string::npos) {
                return ComponentType::VariableResistor;
            }
            return ComponentType::Resistor;
        } else if (typeStr.find("Capacitor") != std::string::npos) {
            return ComponentType::Capacitor;
        } else if (typeStr.find("Inductor") != std::string::npos) {
            return ComponentType::Inductor;
        } else if (typeStr.find("Potentiometer") != std::string::npos) {
            return ComponentType::Potentiometer;
        } else if (typeStr.find("Diode") != std::string::npos) {
            return ComponentType::Diode;
        } else if (typeStr.find("BipolarJunctionTransistor") != std::string::npos || 
                   typeStr.find("BJT") != std::string::npos ||
                   typeStr.find("Transistor") != std::string::npos) {
            return ComponentType::Transistor;
        } else if (typeStr.find("Transformer") != std::string::npos) {
            return ComponentType::Transformer;
        } else if (typeStr.find("OpAmp") != std::string::npos || typeStr.find("IdealOpAmp") != std::string::npos) {
            return ComponentType::OpAmp;
        } else if (typeStr.find("Speaker") != std::string::npos) {
            return ComponentType::Speaker;
        } else if (typeStr.find("Input") != std::string::npos) {
            return ComponentType::Input;
        } else if (typeStr.find("Output") != std::string::npos) {
            return ComponentType::Output;
        } else if (typeStr.find("Ground") != std::string::npos) {
            return ComponentType::Ground;
        } else if (typeStr.find("Rail") != std::string::npos) {
            return ComponentType::Rail;
        } else if (typeStr.find("Wire") != std::string::npos) {
            return ComponentType::Wire;
        } else if (typeStr.find("Label") != std::string::npos) {
            return ComponentType::Label;
        }
        return ComponentType::Unknown;
    }

    // ============================================================================
    // XML Attribute Extraction
    // ============================================================================
    std::string SchematicParser::extractAttributeValue(const std::string& line, const std::string& attrName) {
        std::string searchStr = attrName + "=\"";
        size_t start = line.find(searchStr);
        if (start == std::string::npos) return "";

        start += searchStr.length();
        size_t end = line.find("\"", start);
        if (end == std::string::npos) return "";

        return line.substr(start, end - start);
    }

    // ============================================================================
    // Main Parser Implementation
    // ============================================================================
    Schematic SchematicParser::parseFile(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filePath);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return parseString(buffer.str());
    }

    Schematic SchematicParser::parseString(const std::string& xmlContent) {
        Schematic schematic;
        std::istringstream ss(xmlContent);
        std::string line;
        int lineNum = 0;

        while (std::getline(ss, line)) {
            lineNum++;

            // Skip empty lines and XML declaration
            if (line.empty() || line.find("<?xml") != std::string::npos) continue;

            // Parse Schematic element
            if (line.find("<Schematic") != std::string::npos) {
                std::string name = extractAttributeValue(line, "Name");
                std::string desc = extractAttributeValue(line, "Description");
                // Schematic attributes set (could store name/desc if needed)
                continue;
            }

            // Parse Element (component or wire)
            if (line.find("<Element") != std::string::npos) {
                // Get Element Type
                std::string elementType = extractAttributeValue(line, "Type");
                ComponentType compType = getComponentType(elementType);

                // Get Position
                int x = 0, y = 0;
                std::string posStr = extractAttributeValue(line, "Position");
                if (!posStr.empty()) {
                    size_t comma = posStr.find(",");
                    if (comma != std::string::npos) {
                        try {
                            x = std::stoi(posStr.substr(0, comma));
                            y = std::stoi(posStr.substr(comma + 1));
                        } catch (...) {}
                    }
                }

                // Get Rotation and Flip
                int rotation = 0;
                bool flip = false;
                std::string rotStr = extractAttributeValue(line, "Rotation");
                std::string flipStr = extractAttributeValue(line, "Flip");
                try {
                    rotation = std::stoi(rotStr);
                } catch (...) {}
                flip = (flipStr == "true");

                // Handle Wire elements specifically
                if (elementType.find("Wire") != std::string::npos) {
                    Wire wire;
                    std::string aStr = extractAttributeValue(line, "A");
                    std::string bStr = extractAttributeValue(line, "B");

                    if (!aStr.empty() && !bStr.empty()) {
                        size_t comma_a = aStr.find(",");
                        size_t comma_b = bStr.find(",");
                        if (comma_a != std::string::npos && comma_b != std::string::npos) {
                            try {
                                wire.nodeA_X = std::stoi(aStr.substr(0, comma_a));
                                wire.nodeA_Y = std::stoi(aStr.substr(comma_a + 1));
                                wire.nodeB_X = std::stoi(bStr.substr(0, comma_b));
                                wire.nodeB_Y = std::stoi(bStr.substr(comma_b + 1));
                                schematic.getNetlist().addWire(wire);
                            } catch (...) {}
                        }
                    }
                } else {
                    // For all non-wire elements (symbols), look for the Component sub-element
                    std::string componentLine;
                    while (std::getline(ss, componentLine)) {
                        lineNum++;
                        if (componentLine.find("</Element>") != std::string::npos) break;

                        if (componentLine.find("<Component") != std::string::npos) {
                            // Extract component name from Name attribute
                            std::string componentName = extractAttributeValue(componentLine, "Name");
                            if (componentName.empty()) componentName = "Unnamed_" + std::to_string(lineNum);

                            // Get the actual component type from the _Type attribute on the Component element
                            std::string componentTypeStr = extractAttributeValue(componentLine, "_Type");
                            if (!componentTypeStr.empty()) {
                                compType = getComponentType(componentTypeStr);
                            }
                            if (componentTypeStr.empty() || compType == ComponentType::Unknown) {
                                // Fallback: Use Type attribute (often contains BipolarJunctionTransistor, etc.)
                                std::string componentTypeFallback = extractAttributeValue(componentLine, "Type");
                                if (!componentTypeFallback.empty()) {
                                    compType = getComponentType(componentTypeFallback);
                                }
                            }

                            // Create component
                            auto comp = std::make_shared<Component>(componentName, compType, componentName);
                            comp->setPosition(x, y);
                            comp->setRotation(rotation);
                            comp->setFlip(flip);

                            // Extract all component parameters (attributes on Component element)
                            std::string attrStr = componentLine.substr(componentLine.find("<Component"));

                            // Look for attributes with values
                            std::vector<std::string> attrNames = {
                                "Resistance", "Capacitance", "Inductance", "Voltage",
                                "Impedance", "Turns", "Wipe", "IS", "n", "PartNumber",
                                "Type", "Sweep"
                            };

                            for (const auto& attrName : attrNames) {
                                std::string value = extractAttributeValue(componentLine, attrName);
                                if (!value.empty()) {
                                    comp->addParam(attrName, value);
                                }
                            }

                            schematic.getNetlist().addComponent(comp);
                            break;
                        }
                    }
                }
            }
        }

        return schematic;
    }

    // ============================================================================
    // Netlist Connectivity Pool Building
    // ============================================================================
    void Netlist::buildConnectivityPool() {
        // Clear existing pools
        connectivityPool.clear();
        componentConnections.clear();

        // First pass: Build connectivity pool from components and wires
        // Group components by their position (connection nodes)
        std::map<std::pair<int, int>, std::vector<std::string>> positionToComponents;
        
        for (const auto& pair : components) {
            int x, y;
            pair.second->getPosition(x, y);
            positionToComponents[{x, y}].push_back(pair.first);
        }

        // Add wires to create additional connection points
        for (const auto& wire : wires) {
            ConnectionNode nodeA = {wire.nodeA_X, wire.nodeA_Y};
            ConnectionNode nodeB = {wire.nodeB_X, wire.nodeB_Y};
            
            // Add the wire endpoints to the connectivity pool
            if (connectivityPool.find(nodeA) == connectivityPool.end()) {
                connectivityPool[nodeA] = std::vector<std::string>();
            }
            if (connectivityPool.find(nodeB) == connectivityPool.end()) {
                connectivityPool[nodeB] = std::vector<std::string>();
            }
        }

        // Second pass: Build component connection information
        for (const auto& pair : components) {
            ComponentConnection compConn;
            compConn.componentName = pair.first;
            
            // Get component type string
            ComponentType type = pair.second->getType();
            switch (type) {
                case ComponentType::Resistor: compConn.componentType = "Resistor"; break;
                case ComponentType::Capacitor: compConn.componentType = "Capacitor"; break;
                case ComponentType::Inductor: compConn.componentType = "Inductor"; break;
                case ComponentType::VariableResistor: compConn.componentType = "VariableResistor"; break;
                case ComponentType::Potentiometer: compConn.componentType = "Potentiometer"; break;
                case ComponentType::Diode: compConn.componentType = "Diode"; break;
                case ComponentType::OpAmp: compConn.componentType = "OpAmp"; break;
                case ComponentType::Transformer: compConn.componentType = "Transformer"; break;
                case ComponentType::Transistor: compConn.componentType = "Transistor"; break;
                case ComponentType::Speaker: compConn.componentType = "Speaker"; break;
                case ComponentType::Input: compConn.componentType = "Input"; break;
                case ComponentType::Output: compConn.componentType = "Output"; break;
                case ComponentType::Ground: compConn.componentType = "Ground"; break;
                case ComponentType::Rail: compConn.componentType = "Rail"; break;
                case ComponentType::Wire: compConn.componentType = "Wire"; break;
                case ComponentType::Label: compConn.componentType = "Label"; break;
                case ComponentType::Unknown: compConn.componentType = "Unknown"; break;
            }
            
            pair.second->getPosition(compConn.posX, compConn.posY);
            
            // Find all connection nodes within proximity (components can have multi-pin connections)
            ConnectionNode mainNode = {compConn.posX, compConn.posY};
            compConn.connectedNodes.push_back(mainNode);
            
            // Find wire connections to this component
            for (const auto& wire : wires) {
                if ((wire.nodeA_X == compConn.posX && wire.nodeA_Y == compConn.posY) ||
                    (wire.nodeB_X == compConn.posX && wire.nodeB_Y == compConn.posY)) {
                    
                    // Add the connected node
                    if (wire.nodeA_X == compConn.posX && wire.nodeA_Y == compConn.posY) {
                        ConnectionNode connNode = {wire.nodeB_X, wire.nodeB_Y};
                        if (std::find(compConn.connectedNodes.begin(), compConn.connectedNodes.end(), connNode) 
                            == compConn.connectedNodes.end()) {
                            compConn.connectedNodes.push_back(connNode);
                        }
                    } else {
                        ConnectionNode connNode = {wire.nodeA_X, wire.nodeA_Y};
                        if (std::find(compConn.connectedNodes.begin(), compConn.connectedNodes.end(), connNode) 
                            == compConn.connectedNodes.end()) {
                            compConn.connectedNodes.push_back(connNode);
                        }
                    }
                }
            }
            
            // Find other components at connected nodes
            for (const auto& node : compConn.connectedNodes) {
                auto it = positionToComponents.find({node.x, node.y});
                if (it != positionToComponents.end()) {
                    for (const auto& compName : it->second) {
                        if (compName != pair.first) {
                            if (std::find(compConn.connectedComponents.begin(), 
                                        compConn.connectedComponents.end(), compName) 
                                == compConn.connectedComponents.end()) {
                                compConn.connectedComponents.push_back(compName);
                            }
                        }
                    }
                }
            }
            
            componentConnections[pair.first] = compConn;
        }

        // Build the connectivity pool (Node -> Components at that node)
        for (const auto& posComp : positionToComponents) {
            ConnectionNode node = {posComp.first.first, posComp.first.second};
            connectivityPool[node] = posComp.second;
        }
    }

} // namespace LiveSpice
