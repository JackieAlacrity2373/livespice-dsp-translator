#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

namespace LiveSpice {

    // ============================================================================
    // Component Type Enumeration
    // ============================================================================
    enum class ComponentType {
        Resistor,
        Capacitor,
        Inductor,
        VariableResistor,
        Potentiometer,
        Diode,
        Transformer,
        OpAmp,
        Transistor,
        Speaker,
        Input,
        Output,
        Ground,
        Rail,
        Wire,
        Label,
        Unknown
    };

    // ============================================================================
    // Component Parameter Structure
    // ============================================================================
    struct ComponentParam {
        std::string name;
        std::string value;
        std::string unit;
    };

    // ============================================================================
    // Component Class - Represents any circuit element
    // ============================================================================
    class Component {
    public:
        Component(const std::string& id, ComponentType type, const std::string& name)
            : id(id), type(type), name(name), rotation(0), flip(false) {}

        std::string getId() const { return id; }
        ComponentType getType() const { return type; }
        std::string getName() const { return name; }
        std::vector<ComponentParam> getParams() const { return params; }

        void setPosition(int x, int y) { posX = x; posY = y; }
        void getPosition(int& x, int& y) const { x = posX; y = posY; }
        int getPosX() const { return posX; }
        int getPosY() const { return posY; }

        void setRotation(int rot) { rotation = rot; }
        int getRotation() const { return rotation; }

        void setFlip(bool f) { flip = f; }
        bool getFlipped() const { return flip; }

        void addParam(const std::string& paramName, const std::string& paramValue) {
            params.push_back({paramName, paramValue, ""});
        }

        void addParam(const std::string& paramName, const std::string& paramValue, const std::string& paramUnit) {
            params.push_back({paramName, paramValue, paramUnit});
        }

        std::string getParamValue(const std::string& paramName) const {
            for (const auto& param : params) {
                if (param.name == paramName) {
                    return param.value;
                }
            }
            return "";
        }

        double getParamValueAsDouble(const std::string& paramName) const {
            std::string value = getParamValue(paramName);
            if (value.empty()) return 0.0;
            return parseUnit(value);
        }

    private:
        std::string id;
        ComponentType type;
        std::string name;
        int posX = 0;
        int posY = 0;
        int rotation;
        bool flip;
        std::vector<ComponentParam> params;

        double parseUnit(const std::string& valueStr) const;
    };

    // ============================================================================
    // Node Connection Point - Identifies a connection location with coordinates
    // ============================================================================
    struct ConnectionNode {
        int x, y;
        std::vector<std::string> connectedComponents; // List of component names at this node
        
        bool operator<(const ConnectionNode& other) const {
            if (x != other.x) return x < other.x;
            return y < other.y;
        }
        
        bool operator==(const ConnectionNode& other) const {
            return x == other.x && y == other.y;
        }
    };

    // ============================================================================
    // Component Connection - Tracks a single component's connection
    // ============================================================================
    struct ComponentConnection {
        std::string componentName;
        std::string componentType;
        int posX, posY;
        std::vector<ConnectionNode> connectedNodes; // All nodes this component connects to
        std::vector<std::string> connectedComponents; // Other components this connects to
    };

    // ============================================================================
    // Wire Connection
    // ============================================================================
    struct Wire {
        int nodeA_X, nodeA_Y;
        int nodeB_X, nodeB_Y;
        std::string nodeAName;
        std::string nodeBName;
    };

    // ============================================================================
    // Netlist - Represents the circuit connectivity
    // ============================================================================
    class Netlist {
    public:
        void addComponent(std::shared_ptr<Component> comp) {
            components[comp->getName()] = comp;
        }

        void addWire(const Wire& wire) {
            wires.push_back(wire);
        }

        const std::map<std::string, std::shared_ptr<Component>>& getComponents() const {
            return components;
        }

        const std::vector<Wire>& getWires() const {
            return wires;
        }

        std::shared_ptr<Component> getComponent(const std::string& name) const {
            auto it = components.find(name);
            if (it != components.end()) {
                return it->second;
            }
            return nullptr;
        }

        size_t getComponentCount() const { return components.size(); }
        size_t getWireCount() const { return wires.size(); }

        // Build and retrieve connectivity pool
        void buildConnectivityPool();
        const std::map<ConnectionNode, std::vector<std::string>>& getConnectivityPool() const {
            return connectivityPool;
        }
        const std::map<std::string, ComponentConnection>& getComponentConnections() const {
            return componentConnections;
        }

    private:
        std::map<std::string, std::shared_ptr<Component>> components;
        std::vector<Wire> wires;
        std::map<ConnectionNode, std::vector<std::string>> connectivityPool; // Node -> Components
        std::map<std::string, ComponentConnection> componentConnections; // Component -> Connections
    };

    // ============================================================================
    // Schematic - Top-level representation
    // ============================================================================
    class Schematic {
    public:
        Schematic(const std::string& name = "", const std::string& desc = "")
            : name(name), description(desc) {}

        std::string getName() const { return name; }
        std::string getDescription() const { return description; }

        Netlist& getNetlist() { return netlist; }
        const Netlist& getNetlist() const { return netlist; }

    private:
        std::string name;
        std::string description;
        Netlist netlist;
    };

    // ============================================================================
    // LiveSpice XML Parser
    // ============================================================================
    class SchematicParser {
    public:
        static Schematic parseFile(const std::string& filePath);
        static Schematic parseString(const std::string& xmlContent);

    private:
        static ComponentType getComponentType(const std::string& typeStr);
        static std::string extractAttributeValue(const std::string& line, const std::string& attrName);
        static std::string extractElementType(const std::string& line);
        static std::string extractComponentName(const std::string& componentLine);
    };

} // namespace LiveSpice
