#include "LiveSpiceConnectionMapper.h"
#include <cmath>
#include <sstream>
#include <algorithm>

LiveSpiceConnectionMapper::LiveSpiceConnectionMapper(const Schematic& schem)
    : schematic(&schem) {
    // Extract wires and components from schematic's netlist
    const Netlist& netlist = schematic->getNetlist();
    wires = netlist.getWires();
    
    // Extract components
    const auto& comps = netlist.getComponents();
    for (const auto& comp : comps) {
        components.push_back(comp.second.get());
    }
    
    // Analyze junctions
    extractAllJunctions();
    linkComponentsToJunctions();
}

std::vector<Junction> LiveSpiceConnectionMapper::mapJunctions() {
    return junctions;
}

void LiveSpiceConnectionMapper::extractAllJunctions() {
    std::set<std::pair<double, double>> uniquePoints;
    
    for (size_t i = 0; i < wires.size(); ++i) {
        uniquePoints.insert({wires[i].nodeA_X, wires[i].nodeA_Y});
        uniquePoints.insert({wires[i].nodeB_X, wires[i].nodeB_Y});
    }
    
    for (const auto& point : uniquePoints) {
        Junction j(point.first, point.second);
        
        for (size_t i = 0; i < wires.size(); ++i) {
            if (pointNear(point.first, point.second, wires[i].nodeA_X, wires[i].nodeA_Y) ||
                pointNear(point.first, point.second, wires[i].nodeB_X, wires[i].nodeB_Y)) {
                j.connectedWires.push_back(std::to_string(i));
            }
        }
        
        junctions.push_back(j);
    }
}

void LiveSpiceConnectionMapper::linkComponentsToJunctions() {
    for (const auto& comp : components) {
        for (auto& junction : junctions) {
            if (pointNear(comp->getPosX(), comp->getPosY(), junction.x, junction.y, 0.1)) {
                junction.connectedComponents.push_back(comp->getName());
            }
        }
    }
}

std::vector<int> LiveSpiceConnectionMapper::findWiresAtPoint(double x, double y, double tolerance) {
    std::vector<int> wiresAtPoint;
    
    for (size_t i = 0; i < wires.size(); ++i) {
        if (pointNear(x, y, wires[i].nodeA_X, wires[i].nodeA_Y, tolerance) ||
            pointNear(x, y, wires[i].nodeB_X, wires[i].nodeB_Y, tolerance)) {
            wiresAtPoint.push_back(static_cast<int>(i));
        }
    }
    
    return wiresAtPoint;
}

bool LiveSpiceConnectionMapper::pointNear(double x1, double y1, double x2, double y2, double tolerance) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    double distance = std::sqrt(dx * dx + dy * dy);
    return distance <= tolerance;
}

std::vector<std::pair<std::string, double>> LiveSpiceConnectionMapper::findNearbyComponents(double x, double y, double searchRadius) {
    std::vector<std::pair<std::string, double>> nearby;
    
    for (const auto& comp : components) {
        double dx = comp->getPosX() - x;
        double dy = comp->getPosY() - y;
        double distance = std::sqrt(dx * dx + dy * dy);
        
        if (distance <= searchRadius) {
            nearby.push_back({comp->getName(), distance});
        }
    }
    
    std::sort(nearby.begin(), nearby.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    return nearby;
}

ConnectionPath LiveSpiceConnectionMapper::traceSignalPath(const std::string& fromComponent, const std::string& toComponent) {
    ConnectionPath path;
    path.fromComponent = fromComponent;
    path.toComponent = toComponent;
    path.totalDistance = 0.0;
    path.isSignalPath = true;
    
    auto fromIt = std::find_if(components.begin(), components.end(),
        [&](const auto& c) { return c->getName() == fromComponent; });
    auto toIt = std::find_if(components.begin(), components.end(),
        [&](const auto& c) { return c->getName() == toComponent; });
    
    if (fromIt == components.end() || toIt == components.end()) {
        return path;
    }
    
    for (const auto& junc : junctions) {
        if (pointNear(junc.x, junc.y, (*fromIt)->getPosX(), (*fromIt)->getPosY(), 1.0)) {
            path.junctions.push_back(junc);
            break;
        }
    }
    
    return path;
}

std::map<std::string, std::vector<std::string>> LiveSpiceConnectionMapper::generateConnectivityMap() {
    std::map<std::string, std::vector<std::string>> connectivity;
    
    for (const auto& junction : junctions) {
        if (junction.connectedComponents.size() >= 2) {
            for (const auto& comp : junction.connectedComponents) {
                for (const auto& otherComp : junction.connectedComponents) {
                    if (comp != otherComp) {
                        connectivity[comp].push_back(otherComp);
                    }
                }
            }
        } else if (junction.connectedComponents.size() == 1 && junction.connectedWires.size() >= 2) {
            auto nearby = findNearbyComponents(junction.x, junction.y, 100.0);
            for (const auto& nb : nearby) {
                if (nb.first != junction.connectedComponents[0]) {
                    connectivity[junction.connectedComponents[0]].push_back(nb.first);
                }
            }
        }
    }
    
    return connectivity;
}

bool LiveSpiceConnectionMapper::isSignalWire(int wireIndex) {
    if (wireIndex < 0 || wireIndex >= static_cast<int>(wires.size())) {
        return false;
    }
    
    const auto& wire = wires[wireIndex];
    auto endpointA = findNearbyComponents(wire.nodeA_X, wire.nodeA_Y, 2.0);
    auto endpointB = findNearbyComponents(wire.nodeB_X, wire.nodeB_Y, 2.0);
    
    auto isPowerGround = [](const std::string& name) {
        return name.find("GND") != std::string::npos ||
               name.find("V1") != std::string::npos ||
               name.find("V2") != std::string::npos ||
               name.find("V3") != std::string::npos;
    };
    
    if (!endpointA.empty() && !endpointB.empty()) {
        if (isPowerGround(endpointA[0].first) && isPowerGround(endpointB[0].first)) {
            return false;
        }
    }
    
    return true;
}

Junction* LiveSpiceConnectionMapper::findOrCreateJunction(double x, double y) {
    for (auto& junction : junctions) {
        if (pointNear(x, y, junction.x, junction.y)) {
            return &junction;
        }
    }
    
    junctions.emplace_back(x, y);
    return &junctions.back();
}

std::vector<Junction> LiveSpiceConnectionMapper::traceWireSegments(int startWireIndex, double targetX, double targetY) {
    std::vector<Junction> path;
    
    if (startWireIndex < 0 || startWireIndex >= static_cast<int>(wires.size())) {
        return path;
    }
    
    const auto& startWire = wires[startWireIndex];
    
    for (const auto& junction : junctions) {
        if ((pointNear(junction.x, junction.y, startWire.nodeA_X, startWire.nodeA_Y) ||
             pointNear(junction.x, junction.y, startWire.nodeB_X, startWire.nodeB_Y)) ||
            (pointNear(junction.x, junction.y, targetX, targetY))) {
            path.push_back(junction);
        }
    }
    
    return path;
}

std::string LiveSpiceConnectionMapper::generateConnectivityReport() {
    std::stringstream ss;
    
    ss << "=== JUNCTION-BASED CONNECTIVITY ANALYSIS ===\n\n";
    ss << "Total Junctions Found: " << junctions.size() << "\n";
    ss << "Total Wires: " << wires.size() << "\n";
    ss << "Total Components: " << components.size() << "\n\n";
    
    ss << "KEY JUNCTIONS (connecting multiple components):\n";
    ss << "| Junction Point | Connected Components | Wire Count |\n";
    ss << "|---|---|---|\n";
    
    int multiComponentJunctions = 0;
    for (const auto& junction : junctions) {
        if (junction.connectedComponents.size() >= 2) {
            multiComponentJunctions++;
            ss << "| (" << junction.x << ", " << junction.y << ") | ";
            for (const auto& comp : junction.connectedComponents) {
                ss << comp << " ";
            }
            ss << "| " << junction.connectedWires.size() << " |\n";
        }
    }
    
    ss << "\nMulti-component junctions: " << multiComponentJunctions << "\n\n";
    ss << "ROUTING POINTS (junctions connecting components through wires):\n";
    ss << "| Point | Wires Connected | Nearby Components |\n";
    ss << "|---|---|---|\n";
    
    int routingPoints = 0;
    for (const auto& junction : junctions) {
        if (junction.connectedWires.size() >= 2 && junction.connectedComponents.size() <= 1) {
            routingPoints++;
            auto nearby = findNearbyComponents(junction.x, junction.y, 30.0);
            ss << "| (" << junction.x << ", " << junction.y << ") | "
               << junction.connectedWires.size() << " | ";
            for (const auto& nb : nearby) {
                ss << nb.first << " ";
            }
            ss << "|\n";
        }
    }
    
    ss << "\nRouting points: " << routingPoints << "\n\n";
    
    auto connMap = generateConnectivityMap();
    
    ss << "CONNECTIVITY MAP (from junction analysis):\n";
    for (const auto& [comp, connections] : connMap) {
        if (!connections.empty()) {
            ss << comp << " connects to: ";
            for (const auto& conn : connections) {
                ss << conn << " ";
            }
            ss << "\n";
        }
    }
    
    return ss.str();
}
