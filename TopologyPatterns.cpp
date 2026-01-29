#include "TopologyPatterns.h"
#include <map>

namespace LiveSpice {

    const std::vector<CircuitPattern>& TopologyPatternLibrary::getPatterns() {
        static const std::vector<CircuitPattern> patterns = {
            {
                "Passive RC Low-Pass",
                {ComponentType::Resistor, ComponentType::Capacitor},
                "cascaded_biquad",
                {"cutoff_frequency"}
            },
            {
                "Passive RC High-Pass",
                {ComponentType::Capacitor, ComponentType::Resistor},
                "cascaded_biquad",
                {"cutoff_frequency"}
            },
            {
                "Diode Clipping Stage",
                {ComponentType::Diode},
                "nonlinear_clipper",
                {"diode_part_number"}
            },
            {
                "Op-Amp Gain Stage",
                {ComponentType::OpAmp, ComponentType::Resistor},
                "linear_gain",
                {"gain_linear"}
            },
            {
                "Tone Stack",
                {ComponentType::Potentiometer, ComponentType::Resistor, ComponentType::Capacitor},
                "tone_stack",
                {"tone"}
            },
            {
                "Tone Stack",
                {ComponentType::VariableResistor, ComponentType::Resistor, ComponentType::Capacitor},
                "tone_stack",
                {"tone"}
            }
        };

        return patterns;
    }

    std::vector<PatternMatch> TopologyPatternMatcher::matchComponents(
        const std::vector<std::shared_ptr<Component>>& components) const {
        std::map<ComponentType, int> counts;
        for (const auto& component : components) {
            if (!component) {
                continue;
            }
            counts[component->getType()]++;
        }

        std::vector<PatternMatch> matches;
        for (const auto& pattern : TopologyPatternLibrary::getPatterns()) {
            double score = 0.0;
            for (const auto& type : pattern.signature) {
                if (counts[type] > 0) {
                    score += 1.0;
                }
            }

            if (!pattern.signature.empty()) {
                score /= static_cast<double>(pattern.signature.size());
            }

            if (score > 0.0) {
                matches.push_back({pattern, score});
            }
        }

        return matches;
    }

} // namespace LiveSpice
