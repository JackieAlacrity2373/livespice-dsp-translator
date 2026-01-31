#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <iomanip>

#include "src/SpiceValidation.h"
#include "src/DiodeModels.h"
#include "src/LiveSpiceParser.h"

using namespace SpiceValidation;
using namespace Nonlinear;

/**
 * Main Validation Tool
 * Tests our diode solver against SPICE reference
 */
int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   DIODE SOLVER - SPICE VALIDATION TOOL                    ║" << std::endl;
    std::cout << "║   Comparing DSP Implementation vs. SPICE Reference         ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // Step 1: Generate test signals
    std::cout << "STEP 1: Generate Test Signals" << std::endl;
    std::cout << "─────────────────────────────\n" << std::endl;
    
    TestSignalGenerator::SignalParams params;
    params.sampleRate = 44100.0f;
    params.amplitude = 0.1f;
    
    // Test at different frequencies
    std::vector<float> testFrequencies = {100.0f, 1000.0f, 5000.0f};
    
    for (float freq : testFrequencies) {
        params.frequency = freq;
        params.duration = 0.05f;  // 50ms
        auto signal = TestSignalGenerator::generateSignal(
            TestSignalGenerator::SignalType::SineWave, params);
        
        std::cout << "  ✓ Generated " << freq << " Hz test signal (" 
                  << signal.size() << " samples)" << std::endl;
    }
    
    // Step 2: Generate SPICE netlists
    std::cout << "\nSTEP 2: Generate SPICE Netlists" << std::endl;
    std::cout << "───────────────────────────────\n" << std::endl;
    
    SpiceNetlistGenerator::DiodeCircuitSpec mxrSpec;
    mxrSpec.title = "MXR Distortion+ Clipping Stage";
    mxrSpec.diodePartNumber = "1N4148";
    mxrSpec.sourceResistance = 1000.0f;
    mxrSpec.loadResistance = 100000.0f;
    mxrSpec.topology = SpiceNetlistGenerator::DiodeCircuitSpec::Topology::BackToBack;
    
    std::string netlist = SpiceNetlistGenerator::generateDiodeTestBench(
        mxrSpec, "PWL(0 0 0.001 0.1 0.005 -0.1 0.01 0)  * Triangle wave");
    
    std::ofstream netlFile("mxr_test.cir");
    netlFile << netlist;
    netlFile << "\n";
    netlFile << ".tran 0 0.01 0 1e-6\n";
    netlFile << ".control\n";
    netlFile << "run\n";
    netlFile << "set wr_vecnames\n";
    netlFile << "option numdgt=7\n";
    netlFile << "wrdata mxr_output.txt time v(2) v(3)\n";
    netlFile << "quit\n";
    netlFile << ".endc\n";
    netlFile << ".end\n";
    netlFile.close();
    
    std::cout << "  ✓ Generated SPICE netlist: mxr_test.cir" << std::endl;
    std::cout << "  ✓ SPICE model: " << mxrSpec.diodePartNumber << std::endl;
    std::cout << "  ✓ Topology: Back-to-back (symmetric clipping)" << std::endl;
    
    // Step 3: Run our DSP implementation
    std::cout << "\nSTEP 3: Run DSP Implementation" << std::endl;
    std::cout << "──────────────────────────────\n" << std::endl;
    
    auto diode = DiodeCharacteristics::Si1N4148();
    DiodeClippingStage clipper(diode, DiodeClippingStage::TopologyType::BackToBackDiodes, 100000.0f);
    
    // Process test signals through our solver
    std::vector<std::vector<float>> dspResults;
    
    for (float freq : testFrequencies) {
        params.frequency = freq;
        params.duration = 0.05f;
        auto inputSignal = TestSignalGenerator::generateSignal(
            TestSignalGenerator::SignalType::SineWave, params);
        
        std::vector<float> outputSignal(inputSignal.size());
        for (size_t i = 0; i < inputSignal.size(); ++i) {
            outputSignal[i] = clipper.processSample(inputSignal[i]);
        }
        
        dspResults.push_back(outputSignal);
        
        // Find peak values
        float peakInput = 0.0f, peakOutput = 0.0f;
        for (float sample : inputSignal) {
            peakInput = std::max(peakInput, std::abs(sample));
        }
        for (float sample : outputSignal) {
            peakOutput = std::max(peakOutput, std::abs(sample));
        }
        
        std::cout << "  ✓ " << freq << " Hz: Input Peak=" << std::fixed 
                  << std::setprecision(4) << peakInput << "V → Output Peak=" 
                  << peakOutput << "V (Clipping: " 
                  << ((peakInput > peakOutput) ? "YES" : "NO") << ")" << std::endl;
    }
    
    // Step 4: Validation Metrics
    std::cout << "\nSTEP 4: Validation Setup" << std::endl;
    std::cout << "────────────────────────\n" << std::endl;
    
    SpiceValidator::ValidationConfig config;
    config.generateNetlists = true;
    config.generateReport = true;
    config.outputDir = "./validation_results";
    
    std::cout << "  ✓ Validation config ready" << std::endl;
    std::cout << "  ✓ Generate SPICE netlists: " << (config.generateNetlists ? "YES" : "NO") << std::endl;
    std::cout << "  ✓ Output directory: " << config.outputDir << std::endl;
    
    // Step 5: Run full validation
    std::cout << "\nSTEP 5: Run Validation Suite" << std::endl;
    std::cout << "────────────────────────────\n" << std::endl;
    
    auto results = SpiceValidator::validateAgainstMXR(config);
    
    // Step 6: Display Results
    std::cout << "\nSTEP 6: Validation Results" << std::endl;
    std::cout << "─────────────────────────\n" << std::endl;
    
    for (const auto& result : results) {
        std::cout << "Circuit: " << result.circuitName << std::endl;
        std::cout << "  THD: " << std::fixed << std::setprecision(2) << result.thd << "%\n";
        std::cout << "  Amplitude Error: " << (result.amplitudeError * 100) << "%\n";
        std::cout << "  RMS Difference: " << result.rmsDifference << "V\n";
        std::cout << "  Peak Error: " << result.peakVoltageError << "V\n";
        std::cout << "  Status: " << (result.passed ? "✓ PASS" : "✗ FAIL") << "\n";
        std::cout << "  Notes: " << result.notes << "\n\n";
    }
    
    // Step 7: Instructions
    std::cout << "NEXT STEPS:" << std::endl;
    std::cout << "──────────\n" << std::endl;
    
    std::cout << "1. Install ngspice (if not already installed):\n";
    std::cout << "   - Download from: http://ngspice.sourceforge.net\n";
    std::cout << "   - Add ngspice to PATH\n\n";
    
    std::cout << "2. Run SPICE simulation:\n";
    std::cout << "   > ngspice -b mxr_test.cir -o mxr_output.log\n\n";
    
    std::cout << "3. Compare results:\n";
    std::cout << "   - Check mxr_output.txt for SPICE results\n";
    std::cout << "   - Our DSP output shown above\n";
    std::cout << "   - Metrics display difference\n\n";
    
    std::cout << "4. Validate against hardware (optional):\n";
    std::cout << "   - Use spectrum analyzer on real MXR Distortion+\n";
    std::cout << "   - Feed in 1kHz sine wave\n";
    std::cout << "   - Compare THD and harmonics\n\n";
    
    std::cout << "✓ Validation tool ready for SPICE comparison\n" << std::endl;
    
    return 0;
}
