#pragma once

#include "LiveSpiceParser.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace LiveSpice {

    struct CircuitPattern {
        std::string name;
        std::vector<ComponentType> signature;
        std::string dspStrategy;
        std::vector<std::string> parameters;
    };

    struct PatternMatch {
        CircuitPattern pattern;
        double confidence = 0.0;
    };

    class TopologyPatternLibrary {
    public:
        static const std::vector<CircuitPattern>& getPatterns();
    };

    class TopologyPatternMatcher {
    public:
        std::vector<PatternMatch> matchComponents(
            const std::vector<std::shared_ptr<Component>>& components) const;
    };

} // namespace LiveSpice
