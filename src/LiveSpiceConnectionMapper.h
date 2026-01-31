#ifndef LIVESPICE_CONNECTION_MAPPER_H
#define LIVESPICE_CONNECTION_MAPPER_H

#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include "LiveSpiceParser.h"
#include "CircuitAnalyzer.h"

using namespace LiveSpice;

// Enhanced wire tracing through junctions
struct Junction {
    double x, y;
    std::vector<std::string> connectedWires;  // Wire indices connected at this junction
    std::vector<std::string> connectedComponents;  // Components at this junction
    
    Junction(double x_, double y_) : x(x_), y(y_) {}
    
    bool operator==(const Junction& other) const {
        return (x == other.x && y == other.y);
    }
};

struct ConnectionPath {
    std::string fromComponent;
    std::string toComponent;
    std::vector<Junction> junctions;  // Path through junction points
    double totalDistance;
    bool isSignalPath;  // true if carrying signal, false if power/ground
};

class LiveSpiceConnectionMapper {
public:
    LiveSpiceConnectionMapper(const Schematic& schematic);
    
    // Map all junctions in the circuit
    std::vector<Junction> mapJunctions();
    
    // Find all wires that meet at a point
    std::vector<int> findWiresAtPoint(double x, double y, double tolerance = 0.1);
    
    // Trace a signal path from one component to another through junctions
    ConnectionPath traceSignalPath(const std::string& fromComponent, const std::string& toComponent);
    
    // Find nearest components to a junction
    std::vector<std::pair<std::string, double>> findNearbyComponents(double x, double y, double searchRadius = 50.0);
    
    // Generate complete connectivity map
    std::map<std::string, std::vector<std::string>> generateConnectivityMap();
    
    // Generate detailed connectivity report
    std::string generateConnectivityReport();
    
private:
    const Schematic* schematic;
    
    std::vector<Junction> junctions;
    std::vector<Component*> components;
    std::vector<Wire> wires;
    std::map<std::string, std::vector<std::string>> componentConnections;
    
    // Find or create junction at coordinates
    Junction* findOrCreateJunction(double x, double y);
    
    // Check if point is near another point (within tolerance)
    bool pointNear(double x1, double y1, double x2, double y2, double tolerance = 0.1);
    
    // Extract junction points from all wires
    void extractAllJunctions();
    
    // Link components to nearby junctions
    void linkComponentsToJunctions();
    
    // Determine if a wire is a signal path or power/ground
    bool isSignalWire(int wireIndex);
    
    // Trace connected wires through junctions
    std::vector<Junction> traceWireSegments(int startWireIndex, double targetX, double targetY);
};

#endif  // LIVESPICE_CONNECTION_MAPPER_H
