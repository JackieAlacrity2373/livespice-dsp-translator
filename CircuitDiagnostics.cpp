#include "CircuitDiagnostics.h"
#include <algorithm>
#include <map>

namespace LiveSpice {

    std::vector<CircuitDiagnostics::ComponentNode> CircuitDiagnostics::extractComponentNodes() const {
        std::vector<ComponentNode> nodes;
        const Netlist& netlist = schematic.getNetlist();

        for (const auto& pair : netlist.getComponents()) {
            const auto& comp = pair.second;
            int x, y;
            comp->getPosition(x, y);

            std::string type = "Unknown";
            switch (comp->getType()) {
                case ComponentType::Resistor: type = "R"; break;
                case ComponentType::Capacitor: type = "C"; break;
                case ComponentType::Inductor: type = "L"; break;
                case ComponentType::OpAmp: type = "IC"; break;
                case ComponentType::Diode: type = "D"; break;
                case ComponentType::Potentiometer: type = "POT"; break;
                case ComponentType::VariableResistor: type = "RVAR"; break;
                case ComponentType::Input: type = "IN"; break;
                case ComponentType::Output: type = "OUT"; break;
                case ComponentType::Ground: type = "GND"; break;
                case ComponentType::Rail: type = "PWR"; break;
                default: continue; // Skip wires, labels, etc.
            }

            nodes.push_back({comp->getName(), x, y, type});
        }

        return nodes;
    }

    std::string CircuitDiagnostics::positionToString(int x, int y) const {
        std::ostringstream ss;
        ss << "(" << std::setw(4) << std::right << x << "," << std::setw(4) << std::right << y << ")";
        return ss.str();
    }

    std::vector<CircuitDiagnostics::WireData> CircuitDiagnostics::analyzeWires() const {
        const Netlist& netlist = schematic.getNetlist();
        auto nodes = extractComponentNodes();
        std::vector<WireData> wires;

        for (const auto& wire : netlist.getWires()) {
            WireData wd;
            wd.x1 = wire.nodeA_X;
            wd.y1 = wire.nodeA_Y;
            wd.x2 = wire.nodeB_X;
            wd.y2 = wire.nodeB_Y;

            // Find components at wire endpoints
            for (const auto& node : nodes) {
                if (node.x == wd.x1 && node.y == wd.y1) {
                    wd.componentsAtStart.push_back(node.name);
                }
                if (node.x == wd.x2 && node.y == wd.y2) {
                    wd.componentsAtEnd.push_back(node.name);
                }
            }

            wires.push_back(wd);
        }

        return wires;
    }

    std::string CircuitDiagnostics::generateWireMapping() const {
        auto wires = analyzeWires();
        auto nodes = extractComponentNodes();
        std::ostringstream ss;

        ss << "\n" << std::string(130, '=') << "\n";
        ss << "WIRE-BY-WIRE MAPPING (Raw connection data from schematic file)\n";
        ss << std::string(130, '=') << "\n\n";

        ss << "Total wires in file: " << wires.size() << "\n";
        ss << "Total unique positions: " << nodes.size() << "\n\n";

        ss << std::setw(15) << "Wire #" << " | " 
           << std::setw(16) << "Start Pos" << " | "
           << std::setw(16) << "End Pos" << " | "
           << std::setw(30) << "Components at Start" << " | "
           << std::setw(30) << "Components at End" << "\n";
        ss << std::string(130, '-') << "\n";

        for (size_t i = 0; i < wires.size(); ++i) {
            const auto& w = wires[i];
            
            std::string startComps = "[";
            if (w.componentsAtStart.empty()) {
                startComps += "EMPTY";
            } else {
                for (size_t j = 0; j < w.componentsAtStart.size(); ++j) {
                    if (j > 0) startComps += ",";
                    startComps += w.componentsAtStart[j];
                }
            }
            startComps += "]";

            std::string endComps = "[";
            if (w.componentsAtEnd.empty()) {
                endComps += "EMPTY";
            } else {
                for (size_t j = 0; j < w.componentsAtEnd.size(); ++j) {
                    if (j > 0) endComps += ",";
                    endComps += w.componentsAtEnd[j];
                }
            }
            endComps += "]";

            ss << std::setw(15) << (i + 1) << " | "
               << std::setw(16) << positionToString(w.x1, w.y1) << " | "
               << std::setw(16) << positionToString(w.x2, w.y2) << " | "
               << std::setw(30) << startComps.substr(0, 30) << " | "
               << std::setw(30) << endComps.substr(0, 30) << "\n";
        }

        ss << "\n" << std::string(130, '=') << "\n";
        ss << "COMPONENT POSITION REFERENCE\n";
        ss << std::string(130, '=') << "\n\n";

        std::map<std::string, int> typeCount;
        for (const auto& node : nodes) {
            ss << "  " << std::setw(10) << std::left << node.name 
               << " [" << std::setw(4) << std::left << node.type << "]  at "
               << positionToString(node.x, node.y) << "\n";
            typeCount[node.type]++;
        }

        ss << "\nComponent type summary:\n";
        for (const auto& pair : typeCount) {
            ss << "  " << std::setw(10) << std::left << pair.first << ": " << pair.second << "\n";
        }

        return ss.str();
    }

    std::string CircuitDiagnostics::analyzeConnectivityFailures() const {
        auto wires = analyzeWires();
        auto nodes = extractComponentNodes();
        std::ostringstream ss;

        ss << "\n" << std::string(130, '=') << "\n";
        ss << "CONNECTIVITY FAILURE ANALYSIS (Why connections aren't detected)\n";
        ss << std::string(130, '=') << "\n\n";

        // Count wire categories
        int wiresWithBothEnds = 0;
        int wiresWithOneEnd = 0;
        int wiresWithNoEnds = 0;
        int wiresLikelyRouting = 0;

        for (const auto& w : wires) {
            if (!w.componentsAtStart.empty() && !w.componentsAtEnd.empty()) {
                wiresWithBothEnds++;
            } else if (!w.componentsAtStart.empty() || !w.componentsAtEnd.empty()) {
                wiresWithOneEnd++;
            } else {
                wiresWithNoEnds++;
            }

            // Detect likely routing wires (connected to ground or power at one end)
            if ((w.componentsAtStart.empty() || w.componentsAtEnd.empty()) &&
                ((!w.componentsAtStart.empty() && (w.componentsAtStart[0].find("GND") != std::string::npos ||
                                                    w.componentsAtStart[0].find("V") != std::string::npos)) ||
                 (!w.componentsAtEnd.empty() && (w.componentsAtEnd[0].find("GND") != std::string::npos ||
                                                 w.componentsAtEnd[0].find("V") != std::string::npos)))) {
                wiresLikelyRouting++;
            }
        }

        ss << "Wire connectivity breakdown:\n";
        ss << "  Both endpoints have components:    " << wiresWithBothEnds << " wires\n";
        ss << "  Only one endpoint has component:   " << wiresWithOneEnd << " wires (ROUTING/DANGLING)\n";
        ss << "  No components at either endpoint:  " << wiresWithNoEnds << " wires (UNCONNECTED SEGMENTS)\n";
        ss << "  Likely routing wires (to GND/PWR): " << wiresLikelyRouting << " wires\n\n";

        // Analyze gaps
        ss << "POTENTIAL ISSUES:\n\n";

        if (wiresWithNoEnds > 0) {
            ss << "  ISSUE 1: " << wiresWithNoEnds << " wires have no components at their endpoints\n";
            ss << "    - Wire positions may not match component positions\n";
            ss << "    - Components may have no position data\n";
            ss << "    - Possible coordinate system mismatch\n\n";
        }

        if (wiresWithOneEnd > wiresWithBothEnds) {
            ss << "  ISSUE 2: More wires with one endpoint than both\n";
            ss << "    - Suggests routing/junction wires in the schematic\n";
            ss << "    - Parser may not handle multi-segment connections\n";
            ss << "    - May need to trace through intermediate junctions\n\n";
        }

        ss << "MISSING SIGNAL PATHS (compared to schematic):\n\n";
        
        // Check for key signal paths that should exist
        std::vector<std::pair<std::string, std::string>> expectedPaths = {
            {"V1", "Drive"},      // Input to drive control
            {"Drive", "R3"},      // Drive to resistor network
            {"R3", "X1"},         // Resistor to op-amp
            {"X1", "D1"},         // Op-amp to feedback diodes
            {"X1", "D2"},         // Op-amp to feedback diodes
            {"D1", "R2"},         // Diode feedback to op-amp
            {"X1", "C4"},         // Op-amp to output cap
            {"C4", "Level"},      // Output cap to level control
            {"Level", "S1"}       // Level to output
        };

        for (const auto& path : expectedPaths) {
            bool found = false;

            // Check if both components exist
            auto startIt = std::find_if(nodes.begin(), nodes.end(),
                [&](const ComponentNode& n) { return n.name == path.first; });
            auto endIt = std::find_if(nodes.begin(), nodes.end(),
                [&](const ComponentNode& n) { return n.name == path.second; });

            if (startIt != nodes.end() && endIt != nodes.end()) {
                // Check if there's a wire connecting them (directly or through routing)
                for (const auto& w : wires) {
                    // Direct connection check
                    bool startAtBegin = std::find(w.componentsAtStart.begin(), w.componentsAtStart.end(), 
                                                  path.first) != w.componentsAtStart.end();
                    bool startAtEnd = std::find(w.componentsAtEnd.begin(), w.componentsAtEnd.end(),
                                               path.first) != w.componentsAtEnd.end();
                    bool endAtBegin = std::find(w.componentsAtStart.begin(), w.componentsAtStart.end(),
                                               path.second) != w.componentsAtStart.end();
                    bool endAtEnd = std::find(w.componentsAtEnd.begin(), w.componentsAtEnd.end(),
                                             path.second) != w.componentsAtEnd.end();

                    if ((startAtBegin || startAtEnd) && (endAtBegin || endAtEnd)) {
                        found = true;
                        break;
                    }
                }
            }

            ss << "  " << std::setw(10) << std::left << path.first 
               << " -> " << std::setw(10) << std::left << path.second 
               << ": " << (found ? "FOUND" : "MISSING") << "\n";
        }

        ss << "\n" << std::string(130, '=') << "\n";
        ss << "DIAGNOSIS:\n";
        ss << "  The extracted circuit shows 'No connections found' for all components.\n";
        ss << "  This indicates a fundamental issue in the connectivity detection algorithm.\n\n";
        ss << "LIKELY CAUSES:\n";
        ss << "  1. Wire endpoint positions don't match component positions\n";
        ss << "  2. Multiple wires form junctions that aren't being handled\n";
        ss << "  3. Components positioned at junction points\n";
        ss << "  4. Routing wires between signal points\n";
        ss << "  5. Parser position data incomplete or inaccurate\n\n";
        ss << "RECOMMENDED ACTION:\n";
        ss << "  Review the wire mapping above. For each wire:\n";
        ss << "  - Check if start/end positions correspond to component pins\n";
        ss << "  - Look for wires with no components (these are routing segments)\n";
        ss << "  - Identify junction points where multiple wires meet\n\n";

        return ss.str();
    }

    std::string CircuitDiagnostics::generateDiagnosticReport() const {
        std::ostringstream ss;

        ss << "\n" << std::string(130, '=') << "\n";
        ss << "LIVESPICE CIRCUIT EXTRACTION DIAGNOSTICS\n";
        ss << "Deep analysis for troubleshooting extraction failures\n";
        ss << std::string(130, '=') << "\n";

        ss << generateWireMapping();
        ss << analyzeConnectivityFailures();

        return ss.str();
    }

    std::string CircuitDiagnostics::traceSignalPath(const std::string& startComponent, 
                                                    const std::string& endComponent) const {
        auto wires = analyzeWires();
        auto nodes = extractComponentNodes();
        std::ostringstream ss;

        ss << "\nTracing signal path from " << startComponent << " to " << endComponent << "...\n\n";

        // Simple BFS to find path
        std::map<std::string, std::string> parent;
        std::map<std::string, bool> visited;
        std::vector<std::string> queue;

        queue.push_back(startComponent);
        visited[startComponent] = true;

        while (!queue.empty()) {
            std::string current = queue.front();
            queue.erase(queue.begin());

            if (current == endComponent) {
                ss << "Path found:\n";
                std::vector<std::string> path;
                std::string node = endComponent;
                while (!node.empty()) {
                    path.insert(path.begin(), node);
                    node = parent[node];
                }
                for (size_t i = 0; i < path.size(); ++i) {
                    ss << "  " << i << ". " << path[i] << "\n";
                }
                return ss.str();
            }

            // Find connected components
            for (const auto& w : wires) {
                std::string next;
                bool currentAtStart = std::find(w.componentsAtStart.begin(), w.componentsAtStart.end(),
                                               current) != w.componentsAtStart.end();
                bool currentAtEnd = std::find(w.componentsAtEnd.begin(), w.componentsAtEnd.end(),
                                             current) != w.componentsAtEnd.end();

                if (currentAtStart && !w.componentsAtEnd.empty()) {
                    next = w.componentsAtEnd[0];
                } else if (currentAtEnd && !w.componentsAtStart.empty()) {
                    next = w.componentsAtStart[0];
                }

                if (!next.empty() && !visited[next]) {
                    visited[next] = true;
                    parent[next] = current;
                    queue.push_back(next);
                }
            }
        }

        ss << "No path found between " << startComponent << " and " << endComponent << "\n";
        return ss.str();
    }

} // namespace LiveSpice
