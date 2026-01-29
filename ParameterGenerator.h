#pragma once

#include "LiveSpiceParser.h"
#include "ComponentDSPMapper.h"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>

namespace LiveSpice {

    // ============================================================================
    // JUCE Parameter Descriptor - Defines an audio plugin parameter
    // ============================================================================
    struct JuceParameter {
        std::string id;              // Parameter ID (e.g., "drive")
        std::string name;            // Display name (e.g., "Drive")
        float defaultValue;          // Default value (0.0 - 1.0)
        float minValue;              // Minimum value
        float maxValue;              // Maximum value
        std::string unit;            // Unit suffix (e.g., "%", "dB")
        std::string componentName;   // Source component name
        ComponentType componentType; // Source component type
        
        // Parameter scaling (linear, log, etc.)
        enum class Scaling {
            Linear,
            Logarithmic,
            Exponential
        };
        Scaling scaling = Scaling::Linear;
    };

    // ============================================================================
    // Parameter Generator - Creates JUCE AudioParameters from circuit components
    // ============================================================================
    class ParameterGenerator {
    public:
        ParameterGenerator() {}

        // ========================================================================
        // Extract Parameters from Circuit
        // ========================================================================

        // Find all potentiometers and variable resistors in the circuit
        std::vector<JuceParameter> extractParametersFromCircuit(const Netlist& netlist) {
            std::vector<JuceParameter> parameters;

            for (const auto& pair : netlist.getComponents()) {
                auto comp = pair.second;
                
                if (comp->getType() == ComponentType::Potentiometer ||
                    comp->getType() == ComponentType::VariableResistor) {
                    
                    JuceParameter param = createParameterFromComponent(comp);
                    parameters.push_back(param);
                }
            }

            // Always add bypass parameter to every circuit
            JuceParameter bypassParam;
            bypassParam.id = "bypass";
            bypassParam.name = "Bypass";
            bypassParam.defaultValue = 0.0f;
            bypassParam.minValue = 0.0f;
            bypassParam.maxValue = 1.0f;
            bypassParam.unit = "";
            bypassParam.componentName = "Bypass Switch";
            bypassParam.componentType = ComponentType::Potentiometer;
            bypassParam.scaling = JuceParameter::Scaling::Linear;
            parameters.push_back(bypassParam);

            return parameters;
        }

        // ========================================================================
        // JUCE Code Generation
        // ========================================================================

        // Generate APVTS (AudioProcessorValueTreeState) member variable
        std::string generateAPVTSDeclaration() const {
            return "    juce::AudioProcessorValueTreeState apvts;\n";
        }

        // Generate parameter layout function
        std::string generateParameterLayoutFunction(const std::vector<JuceParameter>& parameters) const {
            std::stringstream ss;
            
            ss << R"(    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

)";

            for (const auto& param : parameters) {
                ss << "        // " << param.componentName << " (" 
                   << getComponentTypeName(param.componentType) << ")\n";
                
                // Use bool for bypass parameter, float for others
                if (param.id == "bypass") {
                    ss << "        layout.add(std::make_unique<juce::AudioParameterBool>(\n";
                    ss << "            juce::ParameterID{\"" << param.id << "\", 1},\n";
                    ss << "            \"" << param.name << "\",\n";
                    ss << "            false));\n\n";  // Default to not bypassed
                } else {
                    ss << "        layout.add(std::make_unique<juce::AudioParameterFloat>(\n";
                    ss << "            juce::ParameterID{\"" << param.id << "\", 1},\n";
                    ss << "            \"" << param.name << "\",\n";
                    ss << "            juce::NormalisableRange<float>(\n";
                    ss << "                " << std::fixed << std::setprecision(1) << param.minValue << "f,\n";
                    ss << "                " << std::fixed << std::setprecision(1) << param.maxValue << "f";
                    
                    // Add logarithmic scaling if needed
                    if (param.scaling == JuceParameter::Scaling::Logarithmic) {
                        ss << ",\n                0.0f, 0.3f"; // Skew factor for log scale
                    }
                    
                    ss << "),\n";
                    ss << "            " << param.defaultValue << "f));\n\n";
                }
            }

            ss << "        return layout;\n";
            ss << "    }\n\n";

            return ss.str();
        }

        // Generate parameter pointer declarations
        std::string generateParameterPointers(const std::vector<JuceParameter>& parameters) const {
            std::stringstream ss;
            
            ss << "    // Parameter pointers for fast access\n";
            for (const auto& param : parameters) {
                if (param.id == "bypass") {
                    ss << "    std::atomic<float>* " << param.id << "Param = nullptr;\n";
                } else {
                    ss << "    std::atomic<float>* " << param.id << "Param = nullptr;\n";
                }
            }
            ss << "\n";

            return ss.str();
        }

        // Generate constructor initialization code
        std::string generateConstructorInit(const std::vector<JuceParameter>& parameters) const {
            std::stringstream ss;
            
            ss << "    // Initialize parameter pointers\n";
            for (const auto& param : parameters) {
                ss << "    " << param.id << "Param = "
                   << "apvts.getRawParameterValue(\"" << param.id << "\");\n";
            }

            return ss.str();
        }

        // Generate usage example in processBlock
        std::string generateParameterUsageExample(const std::vector<JuceParameter>& parameters) const {
            std::stringstream ss;
            
            ss << "    // Get current parameter values\n";
            for (const auto& param : parameters) {
                ss << "    float " << param.id << "Value = " << param.id << "Param->load();\n";
            }
            ss << "\n";

            return ss.str();
        }

        // Generate complete APVTS constructor parameter
        std::string generateAPVTSConstructorParam() const {
            return ", apvts(*this, nullptr, \"Parameters\", createParameterLayout())";
        }

    private:
        // Create a parameter descriptor from a component
        JuceParameter createParameterFromComponent(std::shared_ptr<Component> comp) const {
            JuceParameter param;
            
            param.componentName = comp->getName();
            param.componentType = comp->getType();
            
            // Generate parameter ID from component name
            param.id = sanitizeParameterID(comp->getName());
            param.name = beautifyParameterName(comp->getName());
            
            // Get wipe position (default value)
            std::string wipeStr = comp->getParamValue("Wipe");
            param.defaultValue = wipeStr.empty() ? 0.5f : std::stof(wipeStr);
            
            // Determine parameter range based on component name
            if (isGainParameter(param.name)) {
                // Gain/Drive/Level: 0-100% with logarithmic scaling
                param.minValue = 0.0f;
                param.maxValue = 1.0f;
                param.unit = "%";
                param.scaling = JuceParameter::Scaling::Linear;
            } else if (isToneParameter(param.name)) {
                // Tone: 0-100% linear
                param.minValue = 0.0f;
                param.maxValue = 1.0f;
                param.unit = "%";
                param.scaling = JuceParameter::Scaling::Linear;
            } else {
                // Generic: 0-100%
                param.minValue = 0.0f;
                param.maxValue = 1.0f;
                param.unit = "%";
                param.scaling = JuceParameter::Scaling::Linear;
            }

            return param;
        }

        // Sanitize component name to valid parameter ID
        std::string sanitizeParameterID(const std::string& name) const {
            std::string id;
            for (char c : name) {
                if (std::isalnum(c)) {
                    id += std::tolower(c);
                } else if (c == '_' || c == '-') {
                    id += '_';
                }
            }
            return id.empty() ? "param" : id;
        }

        // Beautify component name for display
        std::string beautifyParameterName(const std::string& name) const {
            // Capitalize first letter, keep rest as-is
            if (name.empty()) return "Parameter";
            
            std::string beautiful = name;
            beautiful[0] = std::toupper(beautiful[0]);
            return beautiful;
        }

        // Check if parameter is a gain/drive/level control
        bool isGainParameter(const std::string& name) const {
            std::string lower = name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            
            return lower.find("drive") != std::string::npos ||
                   lower.find("gain") != std::string::npos ||
                   lower.find("level") != std::string::npos ||
                   lower.find("volume") != std::string::npos;
        }

        // Check if parameter is a tone control
        bool isToneParameter(const std::string& name) const {
            std::string lower = name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            
            return lower.find("tone") != std::string::npos ||
                   lower.find("treble") != std::string::npos ||
                   lower.find("bass") != std::string::npos ||
                   lower.find("mid") != std::string::npos;
        }

        // Get human-readable component type name
        std::string getComponentTypeName(ComponentType type) const {
            switch (type) {
                case ComponentType::Potentiometer: return "Potentiometer";
                case ComponentType::VariableResistor: return "Variable Resistor";
                default: return "Unknown";
            }
        }
    };

} // namespace LiveSpice
