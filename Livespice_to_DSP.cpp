#include "LiveSpiceParser.h"
#include "CircuitAnalyzer.h"
#include "CircuitVisualizer.h"
#include "CircuitDiagnostics.h"
#include "LiveSpiceConnectionMapper.h"
#include "JuceDSPGenerator.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace LiveSpice;

// Helper function to extract circuit name from filename
std::string getCircuitName(const std::string& filepath) {
    // Get filename without path
    size_t lastSlash = filepath.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? filepath : filepath.substr(lastSlash + 1);
    
    // Remove file extension
    size_t lastDot = filename.find_last_of(".");
    if (lastDot != std::string::npos) {
        filename = filename.substr(0, lastDot);
    }
    
    return filename;
}

// Helper function to create valid directory name
std::string createValidDirName(const std::string& name) {
    std::string result = "JUCE - " + name;
    // Replace invalid characters with underscores
    for (auto& c : result) {
        if (c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '-';
        }
    }
    return result;
}

int main(int argc, char* argv[]) {
    try {
        std::string inputFile = "example pedals/MXR Distortion +.schx";

        if (argc > 1) {
            inputFile = argv[1];
        }

        std::cout << "Parsing LiveSpice file: " << inputFile << std::endl;

        // Parse the schematic
        Schematic schematic = SchematicParser::parseFile(inputFile);

        // Get netlist information
        const Netlist& netlist = schematic.getNetlist();
        std::cout << "\n=== NETLIST INFORMATION ===" << std::endl;
        std::cout << "Total Components: " << netlist.getComponentCount() << std::endl;
        std::cout << "Total Wires: " << netlist.getWireCount() << std::endl;

        // Print component list
        std::cout << "\n=== COMPONENTS ===" << std::endl;
        for (const auto& pair : netlist.getComponents()) {
            const auto& comp = pair.second;
            std::cout << "\n" << comp->getName() << " (";

            switch (comp->getType()) {
                case ComponentType::Resistor: std::cout << "Resistor"; break;
                case ComponentType::VariableResistor: std::cout << "Variable Resistor"; break;
                case ComponentType::Capacitor: std::cout << "Capacitor"; break;
                case ComponentType::Inductor: std::cout << "Inductor"; break;
                case ComponentType::Potentiometer: std::cout << "Potentiometer"; break;
                case ComponentType::Diode: std::cout << "Diode"; break;
                case ComponentType::OpAmp: std::cout << "Op-Amp"; break;
                case ComponentType::Speaker: std::cout << "Speaker"; break;
                case ComponentType::Input: std::cout << "Input"; break;
                case ComponentType::Output: std::cout << "Output"; break;
                case ComponentType::Ground: std::cout << "Ground"; break;
                case ComponentType::Rail: std::cout << "Power Rail"; break;
                default: std::cout << "Unknown"; break;
            }
            std::cout << ")" << std::endl;

            auto params = comp->getParams();
            for (const auto& param : params) {
                std::cout << "  " << param.name << " = " << param.value;
                if (!param.unit.empty()) std::cout << " " << param.unit;
                std::cout << std::endl;
            }
        }

        // Analyze the circuit
        std::cout << "\n=== CIRCUIT ANALYSIS ===" << std::endl;
        CircuitAnalyzer analyzer(schematic);
        auto stages = analyzer.analyzeCircuit();

        std::cout << analyzer.generateReport();

        // Generate circuit connectivity report
        std::cout << analyzer.generateConnectivityReport();

        std::cout << "\n### DEBUG: About to call junction mapper ###" << std::endl;
        std::cout.flush();

        // Generate junction-based connectivity analysis
        std::cout << "\n=== JUNCTION-BASED CONNECTIVITY MAPPING ===" << std::endl;
        std::cout << "About to create mapper..." << std::endl;
        std::cout.flush();
        try {
            LiveSpiceConnectionMapper connectionMapper(schematic);
            std::cout << "Mapper created successfully" << std::endl;
            std::cout.flush();
            
            auto junctions = connectionMapper.mapJunctions();
            std::cout << "Total junctions found: " << junctions.size() << std::endl;
            std::cout.flush();
            
            std::string connectivityReport = connectionMapper.generateConnectivityReport();
            std::cout << connectivityReport;
            std::cout.flush();
        } catch (const std::exception& e) {
            std::cout << "ERROR in connection mapper: " << e.what() << std::endl;
        } catch (...) {
            std::cout << "UNKNOWN ERROR in connection mapper" << std::endl;
        }

        // Generate diagnostics if connectivity is broken
        std::cout << "\n=== CIRCUIT EXTRACTION DIAGNOSTICS ===" << std::endl;
        std::cout.flush();
        CircuitDiagnostics diagnostics(schematic);
        std::string diagnosticReport = diagnostics.generateDiagnosticReport();
        std::cout << diagnosticReport;
        std::cout.flush();

        // Save diagnostics to file
        try {
            std::string docPath = std::getenv("USERPROFILE");
            docPath += "\\Documents\\";
            std::string circuitNameForFile = getCircuitName(inputFile);
            for (auto& c : circuitNameForFile) {
                if (c == ' ') c = '_';
            }
            std::string diagnosticsPath = docPath + circuitNameForFile + "_DIAGNOSTICS.txt";
            
            std::ofstream diagnosticsFile(diagnosticsPath);
            if (diagnosticsFile.is_open()) {
                diagnosticsFile << diagnosticReport;
                diagnosticsFile.close();
                std::cout << "\nDiagnostics saved to: " << diagnosticsPath << std::endl;
            }
        } catch (...) {
            // Silent fail for diagnostics
        }

        // Generate visual representation of extracted circuit
        std::cout << "\n=== GENERATING EXTRACTED CIRCUIT VISUALIZATION ===" << std::endl;
        std::cout.flush();
        CircuitVisualizer visualizer(schematic, analyzer);
        std::string extractedDiagram = visualizer.generateFullDiagram();
        
        // Output to console
        std::cout << extractedDiagram;
        std::cout.flush();
        
        // Save to file in Documents folder
        try {
            std::string docPath = std::getenv("USERPROFILE");
            docPath += "\\Documents\\";
            std::string circuitNameForFile = getCircuitName(inputFile);
            // Replace spaces with underscores for filename
            for (auto& c : circuitNameForFile) {
                if (c == ' ') c = '_';
            }
            std::string outputFilePath = docPath + circuitNameForFile + "_EXTRACTED_CIRCUIT.txt";
            
            std::ofstream diagramFile(outputFilePath);
            if (diagramFile.is_open()) {
                diagramFile << extractedDiagram;
                diagramFile.close();
                std::cout << "\n✓ Extracted circuit diagram saved to: " << outputFilePath << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not save diagram to file: " << e.what() << std::endl;
        }

        // Generate DSP configuration
        std::cout << "\n=== DSP CONFIGURATION ===" << std::endl;
        std::cout << "Identified " << stages.size() << " processing stages:" << std::endl;

        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& stage = stages[i];
            std::cout << "\nStage " << (i + 1) << ": " << stage.name << std::endl;
            std::cout << "  Type: ";

            switch (stage.type) {
                case StageType::InputBuffer: std::cout << "Input Buffer"; break;
                case StageType::GainStage: std::cout << "Gain Stage"; break;
                case StageType::OpAmpClipping: std::cout << "Op-Amp Clipping"; break;
                case StageType::LowPassFilter: std::cout << "Low-Pass Filter"; break;
                case StageType::ToneControl: std::cout << "Tone Control"; break;
                case StageType::OutputBuffer: std::cout << "Output Buffer"; break;
                default: std::cout << "Unknown"; break;
            }
            std::cout << std::endl;

            if (!stage.dspParams.empty()) {
                std::cout << "  DSP Parameters:" << std::endl;
                for (const auto& param : stage.dspParams) {
                    std::cout << "    " << param.first << " = " << param.second << std::endl;
                }
            }
        }

        // Generate JUCE DSP plugin code
        std::cout << "\n=== JUCE DSP CODE GENERATION ===" << std::endl;
        
        // Extract circuit name from filename
        std::string circuitName = getCircuitName(inputFile);
        std::string outputDirName = createValidDirName(circuitName);
        
        std::cout << "Circuit Name: " << circuitName << std::endl;
        std::cout << "Output Directory: " << outputDirName << std::endl;
        
        // Create output directory
        try {
            std::filesystem::create_directory(outputDirName);
            std::cout << "Created output directory: " << outputDirName << std::endl;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Warning: Could not create directory: " << e.what() << std::endl;
        }
        
        JuceDSPGenerator juceGen;
        
        // Write plugin files to the output directory (with parameter support)
        juceGen.writePluginFiles(outputDirName, circuitName, stages, schematic.getNetlist());
        std::cout << "Wrote CircuitProcessor.h" << std::endl;
        std::cout << "Wrote CircuitProcessor.cpp" << std::endl;
        
        // Generate CMakeLists.txt
        std::string cmakeContent = juceGen.generateCMakeLists(circuitName, "../../third_party");
        std::ofstream cmakeFile(outputDirName + "/CMakeLists.txt");
        if (cmakeFile.is_open()) {
            cmakeFile << cmakeContent;
            cmakeFile.close();
            std::cout << "Wrote CMakeLists.txt" << std::endl;
        }
        
        // Also print to console for reference
        std::cout << "\n--- CMakeLists.txt ---\n";
        std::cout << cmakeContent << std::endl;
        
        std::cout << "\n=== JUCE PLUGIN GENERATION COMPLETE ===" << std::endl;
        std::cout << "Plugin directory: " << outputDirName << std::endl;
        std::cout << "Build instructions:" << std::endl;
        std::cout << "  cd " << outputDirName << std::endl;
        std::cout << "  mkdir build" << std::endl;
        std::cout << "  cd build" << std::endl;
        std::cout << "  cmake .." << std::endl;
        std::cout << "  cmake --build . --config Release" << std::endl;

        std::cout << "\n=== PARSING COMPLETE ===" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
