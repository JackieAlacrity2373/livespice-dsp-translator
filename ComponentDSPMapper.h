#pragma once

#include "LiveSpiceParser.h"
#include "third_party/livespice-components/ComponentModels.h"
#include "third_party/livespice-components/DSPImplementations.h"
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

namespace LiveSpice {

    // ============================================================================
    // Component DSP Mapper - Maps LiveSPICE components to DSP processors
    // ============================================================================
    // This class serves as the integration layer between the circuit analyzer
    // and the LiveSPICE component DSP implementations. It treats LiveSPICE 
    // definitions as the source of truth for component behavior.
    // ============================================================================

    class ComponentDSPMapper {
    public:
        ComponentDSPMapper() {}

        // ========================================================================
        // Component Type Detection & Mapping
        // ========================================================================

        // Detect the LiveSPICE DSP processor type for a component
        enum class DSPProcessorType {
            Resistor,
            Capacitor,
            Inductor,
            Diode,
            BJT,
            JFET,
            OpAmp,
            Triode,
            Pentode,
            Transformer,
            SoftClipper,
            Unknown
        };

        // Map a LiveSPICE component to its DSP processor type
        DSPProcessorType mapComponentToProcessor(std::shared_ptr<Component> comp) const {
            switch (comp->getType()) {
                case ComponentType::Resistor:
                    return DSPProcessorType::Resistor;
                case ComponentType::Capacitor:
                    return DSPProcessorType::Capacitor;
                case ComponentType::Inductor:
                    return DSPProcessorType::Inductor;
                case ComponentType::Diode:
                    return DSPProcessorType::Diode;
                case ComponentType::Transistor:
                    return detectTransistorType(comp);
                case ComponentType::OpAmp:
                    return DSPProcessorType::OpAmp;
                case ComponentType::Transformer:
                    return DSPProcessorType::Transformer;
                default:
                    return DSPProcessorType::Unknown;
            }
        }

        // ========================================================================
        // Component Parameter Extraction (LiveSPICE source of truth)
        // ========================================================================

        // Extract resistor parameters
        struct ResistorParams {
            double resistance = 1000.0;  // Ohms (default 1k)
            double tolerance = 0.05;     // 5% tolerance
        };

        ResistorParams extractResistorParams(std::shared_ptr<Component> comp) const {
            ResistorParams params;
            
            // Try to get resistance from component parameters
            std::string valueStr = comp->getParamValue("Resistance");
            if (valueStr.empty()) valueStr = comp->getParamValue("Value");
            if (valueStr.empty()) valueStr = comp->getParamValue("R");
            
            if (!valueStr.empty()) {
                params.resistance = comp->getParamValueAsDouble("Resistance");
                if (params.resistance == 0.0) {
                    params.resistance = comp->getParamValueAsDouble("Value");
                }
                if (params.resistance == 0.0) {
                    params.resistance = comp->getParamValueAsDouble("R");
                }
            }
            
            return params;
        }

        // Extract capacitor parameters
        struct CapacitorParams {
            double capacitance = 1e-6;   // Farads (default 1µF)
            double esr = 0.1;            // Equivalent Series Resistance (Ohms)
            double tolerance = 0.1;      // 10% tolerance
        };

        CapacitorParams extractCapacitorParams(std::shared_ptr<Component> comp) const {
            CapacitorParams params;
            
            std::string valueStr = comp->getParamValue("Capacitance");
            if (valueStr.empty()) valueStr = comp->getParamValue("Value");
            if (valueStr.empty()) valueStr = comp->getParamValue("C");
            
            if (!valueStr.empty()) {
                params.capacitance = comp->getParamValueAsDouble("Capacitance");
                if (params.capacitance == 0.0) {
                    params.capacitance = comp->getParamValueAsDouble("Value");
                }
                if (params.capacitance == 0.0) {
                    params.capacitance = comp->getParamValueAsDouble("C");
                }
            }
            
            // Extract ESR if specified
            double esrValue = comp->getParamValueAsDouble("ESR");
            if (esrValue > 0.0) {
                params.esr = esrValue;
            }
            
            return params;
        }

        // Extract inductor parameters
        struct InductorParams {
            double inductance = 1e-3;    // Henries (default 1mH)
            double dcResistance = 1.0;   // DC resistance (Ohms)
            double tolerance = 0.1;      // 10% tolerance
        };

        InductorParams extractInductorParams(std::shared_ptr<Component> comp) const {
            InductorParams params;
            
            std::string valueStr = comp->getParamValue("Inductance");
            if (valueStr.empty()) valueStr = comp->getParamValue("Value");
            if (valueStr.empty()) valueStr = comp->getParamValue("L");
            
            if (!valueStr.empty()) {
                params.inductance = comp->getParamValueAsDouble("Inductance");
                if (params.inductance == 0.0) {
                    params.inductance = comp->getParamValueAsDouble("Value");
                }
                if (params.inductance == 0.0) {
                    params.inductance = comp->getParamValueAsDouble("L");
                }
            }
            
            // Extract DC resistance if specified
            double rDC = comp->getParamValueAsDouble("DCR");
            if (rDC == 0.0) rDC = comp->getParamValueAsDouble("DCResistance");
            if (rDC > 0.0) {
                params.dcResistance = rDC;
            }
            
            return params;
        }

        // Extract diode parameters (part number for model lookup)
        struct DiodeParams {
            std::string partNumber = "1N4148";  // Default silicon diode
            double temperature = 25.0;           // Celsius
        };

        DiodeParams extractDiodeParams(std::shared_ptr<Component> comp) const {
            DiodeParams params;
            
            // Check for part number specification
            std::string partNum = comp->getParamValue("PartNumber");
            if (partNum.empty()) partNum = comp->getParamValue("Model");
            if (partNum.empty()) partNum = comp->getParamValue("Type");
            
            if (!partNum.empty()) {
                params.partNumber = partNum;
            }
            
            return params;
        }

        // Extract BJT parameters
        struct BJTParams {
            std::string partNumber = "2N3904";  // Default NPN BJT
            double temperature = 25.0;           // Celsius
        };

        BJTParams extractBJTParams(std::shared_ptr<Component> comp) const {
            BJTParams params;
            
            std::string partNum = comp->getParamValue("PartNumber");
            if (partNum.empty()) partNum = comp->getParamValue("Model");
            if (partNum.empty()) partNum = comp->getParamValue("Type");
            
            if (!partNum.empty()) {
                params.partNumber = partNum;
            }
            
            return params;
        }

        // Extract JFET parameters
        struct JFETParams {
            std::string partNumber = "2N5457";  // Default N-channel JFET
        };

        JFETParams extractJFETParams(std::shared_ptr<Component> comp) const {
            JFETParams params;
            
            std::string partNum = comp->getParamValue("PartNumber");
            if (partNum.empty()) partNum = comp->getParamValue("Model");
            if (partNum.empty()) partNum = comp->getParamValue("Type");
            
            if (!partNum.empty()) {
                params.partNumber = partNum;
            }
            
            return params;
        }

        // Extract OpAmp parameters
        struct OpAmpParams {
            std::string partNumber = "TL072";   // Default dual op-amp
        };

        OpAmpParams extractOpAmpParams(std::shared_ptr<Component> comp) const {
            OpAmpParams params;
            
            std::string partNum = comp->getParamValue("PartNumber");
            if (partNum.empty()) partNum = comp->getParamValue("Model");
            if (partNum.empty()) partNum = comp->getParamValue("Type");
            
            if (!partNum.empty()) {
                params.partNumber = partNum;
            }
            
            return params;
        }

        // Extract Triode parameters
        struct TriodeParams {
            std::string partNumber = "12AX7";   // Default preamp tube
        };

        TriodeParams extractTriodeParams(std::shared_ptr<Component> comp) const {
            TriodeParams params;
            
            std::string partNum = comp->getParamValue("PartNumber");
            if (partNum.empty()) partNum = comp->getParamValue("Model");
            if (partNum.empty()) partNum = comp->getParamValue("Type");
            
            if (!partNum.empty()) {
                params.partNumber = partNum;
            }
            
            return params;
        }

        // ========================================================================
        // DSP Processor Factory Methods
        // ========================================================================

        // Create a DSP processor instance for a component
        std::unique_ptr<LiveSpiceDSP::ResistorProcessor> createResistorProcessor(
            std::shared_ptr<Component> comp) const 
        {
            auto processor = std::make_unique<LiveSpiceDSP::ResistorProcessor>();
            auto params = extractResistorParams(comp);
            processor->prepare(params.resistance);
            return processor;
        }

        std::unique_ptr<LiveSpiceDSP::CapacitorProcessor> createCapacitorProcessor(
            std::shared_ptr<Component> comp) const 
        {
            auto processor = std::make_unique<LiveSpiceDSP::CapacitorProcessor>();
            auto params = extractCapacitorParams(comp);
            processor->prepare(params.capacitance, params.esr);
            return processor;
        }

        std::unique_ptr<LiveSpiceDSP::InductorProcessor> createInductorProcessor(
            std::shared_ptr<Component> comp) const 
        {
            auto processor = std::make_unique<LiveSpiceDSP::InductorProcessor>();
            auto params = extractInductorParams(comp);
            processor->prepare(params.inductance, params.dcResistance);
            return processor;
        }

        std::unique_ptr<LiveSpiceDSP::DiodeProcessor> createDiodeProcessor(
            std::shared_ptr<Component> comp) const 
        {
            auto processor = std::make_unique<LiveSpiceDSP::DiodeProcessor>();
            auto params = extractDiodeParams(comp);
            processor->prepare(params.partNumber, params.temperature);
            return processor;
        }

        std::unique_ptr<LiveSpiceDSP::BJTProcessor> createBJTProcessor(
            std::shared_ptr<Component> comp) const 
        {
            auto processor = std::make_unique<LiveSpiceDSP::BJTProcessor>();
            auto params = extractBJTParams(comp);
            processor->prepare(params.partNumber, params.temperature);
            return processor;
        }

        std::unique_ptr<LiveSpiceDSP::JFETProcessor> createJFETProcessor(
            std::shared_ptr<Component> comp) const 
        {
            auto processor = std::make_unique<LiveSpiceDSP::JFETProcessor>();
            auto params = extractJFETParams(comp);
            processor->prepare(params.partNumber);
            return processor;
        }

        std::unique_ptr<LiveSpiceDSP::OpAmpProcessor> createOpAmpProcessor(
            std::shared_ptr<Component> comp, double sampleRate) const 
        {
            auto processor = std::make_unique<LiveSpiceDSP::OpAmpProcessor>();
            auto params = extractOpAmpParams(comp);
            processor->prepare(params.partNumber, sampleRate);
            return processor;
        }

        std::unique_ptr<LiveSpiceDSP::TriodeProcessor> createTriodeProcessor(
            std::shared_ptr<Component> comp) const 
        {
            auto processor = std::make_unique<LiveSpiceDSP::TriodeProcessor>();
            auto params = extractTriodeParams(comp);
            processor->prepare(params.partNumber);
            return processor;
        }

        // ========================================================================
        // Component Analysis Helpers
        // ========================================================================

        // Get a human-readable description of the component's DSP mapping
        std::string getComponentDSPDescription(std::shared_ptr<Component> comp) const {
            DSPProcessorType procType = mapComponentToProcessor(comp);
            
            switch (procType) {
                case DSPProcessorType::Resistor: {
                    auto params = extractResistorParams(comp);
                    return "Resistor: " + formatResistance(params.resistance);
                }
                case DSPProcessorType::Capacitor: {
                    auto params = extractCapacitorParams(comp);
                    return "Capacitor: " + formatCapacitance(params.capacitance);
                }
                case DSPProcessorType::Inductor: {
                    auto params = extractInductorParams(comp);
                    return "Inductor: " + formatInductance(params.inductance);
                }
                case DSPProcessorType::Diode: {
                    auto params = extractDiodeParams(comp);
                    return "Diode: " + params.partNumber + " (Shockley model)";
                }
                case DSPProcessorType::BJT: {
                    auto params = extractBJTParams(comp);
                    return "BJT: " + params.partNumber + " (Ebers-Moll model)";
                }
                case DSPProcessorType::JFET: {
                    auto params = extractJFETParams(comp);
                    return "FET: " + params.partNumber + " (Quadratic model)";
                }
                case DSPProcessorType::OpAmp: {
                    auto params = extractOpAmpParams(comp);
                    return "Op-Amp: " + params.partNumber + " (Behavioral model)";
                }
                case DSPProcessorType::Triode: {
                    auto params = extractTriodeParams(comp);
                    return "Triode: " + params.partNumber + " (Koren model)";
                }
                default:
                    return "Unknown DSP mapping";
            }
        }

    private:
        // Detect transistor sub-type (BJT, JFET, MOSFET, Triode)
        DSPProcessorType detectTransistorType(std::shared_ptr<Component> comp) const {
            std::string partNum = comp->getParamValue("PartNumber");
            if (partNum.empty()) partNum = comp->getParamValue("Model");
            if (partNum.empty()) partNum = comp->getParamValue("Type");
            std::string partUpper = partNum;
            std::transform(partUpper.begin(), partUpper.end(), partUpper.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });
            std::string nameUpper = comp->getName();
            std::transform(nameUpper.begin(), nameUpper.end(), nameUpper.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });

            // If schematic uses MOSFET-style designators, prefer FET
            if (!nameUpper.empty() && nameUpper[0] == 'M') {
                return DSPProcessorType::JFET;
            }
            
            // Check for tube/triode indicators
            if (partUpper.find("12A") != std::string::npos || 
                partUpper.find("EL") != std::string::npos ||
                partUpper.find("6L6") != std::string::npos ||
                partUpper.find("TRIODE") != std::string::npos) {
                return DSPProcessorType::Triode;
            }
            
            // Check for FET indicators (JFET/MOSFET)
            if (partUpper.find("JFET") != std::string::npos ||
                partUpper.find("FET") != std::string::npos ||
                partUpper.find("MOSFET") != std::string::npos ||
                partUpper.find("NMOS") != std::string::npos ||
                partUpper.find("PMOS") != std::string::npos ||
                partUpper.find("2N7000") != std::string::npos ||
                partUpper.find("BS170") != std::string::npos ||
                partUpper.find("2N5") != std::string::npos) {
                return DSPProcessorType::JFET;
            }
            
            // Default to BJT
            return DSPProcessorType::BJT;
        }

        // Formatting helpers
        std::string formatResistance(double resistance) const {
            if (resistance >= 1e6) {
                return std::to_string(resistance / 1e6) + "MΩ";
            } else if (resistance >= 1e3) {
                return std::to_string(resistance / 1e3) + "kΩ";
            } else {
                return std::to_string(resistance) + "Ω";
            }
        }

        std::string formatCapacitance(double capacitance) const {
            if (capacitance >= 1e-6) {
                return std::to_string(capacitance / 1e-6) + "µF";
            } else if (capacitance >= 1e-9) {
                return std::to_string(capacitance / 1e-9) + "nF";
            } else {
                return std::to_string(capacitance / 1e-12) + "pF";
            }
        }

        std::string formatInductance(double inductance) const {
            if (inductance >= 1.0) {
                return std::to_string(inductance) + "H";
            } else if (inductance >= 1e-3) {
                return std::to_string(inductance / 1e-3) + "mH";
            } else {
                return std::to_string(inductance / 1e-6) + "µH";
            }
        }
    };

} // namespace LiveSpice
