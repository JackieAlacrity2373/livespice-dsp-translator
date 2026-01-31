#include <iostream>
#include <cmath>
#include <iomanip>
#include <vector>
#include <algorithm>

#include "src/DiodeModels.h"

using namespace Nonlinear;

// Test results tracking
struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

std::vector<TestResult> results;

void reportTest(const std::string& name, bool passed, const std::string& message = "") {
    results.push_back({name, passed, message});
    std::cout << (passed ? "✓ PASS" : "✗ FAIL") << ": " << name;
    if (!message.empty()) {
        std::cout << " - " << message;
    }
    std::cout << std::endl;
}

/**
 * Test 1: Diode LUT generation and interpolation
 */
void testDiodeLUT() {
    std::cout << "\n=== TEST 1: Diode LUT Generation ===" << std::endl;
    
    DiodeCharacteristics si1N4148 = DiodeCharacteristics::Si1N4148();
    DiodeLUT lut(si1N4148);
    
    // Test that LUT produces reasonable current values
    float i_at_0V = lut.evaluateCurrent(0.0f);
    float i_at_0_3V = lut.evaluateCurrent(0.3f);
    float i_at_0_6V = lut.evaluateCurrent(0.6f);
    
    bool i_increasing = (i_at_0_3V > i_at_0V) && (i_at_0_6V > i_at_0_3V);
    reportTest("LUT Current Monotonic Increase", i_increasing,
               std::string("I(0V)=") + std::to_string(i_at_0V) + 
               ", I(0.3V)=" + std::to_string(i_at_0_3V) +
               ", I(0.6V)=" + std::to_string(i_at_0_6V));
    
    // Test conductance calculation
    float g_at_0_4V = lut.evaluateConductance(0.4f);
    bool g_positive = g_at_0_4V > 0.0f;
    reportTest("LUT Conductance Positive", g_positive,
               "g(0.4V)=" + std::to_string(g_at_0_4V) + " S");
}

/**
 * Test 2: Newton-Raphson solver convergence
 */
void testNewtonRaphsonSolver() {
    std::cout << "\n=== TEST 2: Newton-Raphson Solver ===" << std::endl;
    
    DiodeCharacteristics si1N4148 = DiodeCharacteristics::Si1N4148();
    DiodeNewtonRaphson solver(si1N4148);
    DiodeNewtonRaphson::SolverConfig config;
    
    float vDiode, iDiode;
    
    // Test case 1: Low voltage
    int iterations = solver.solve(0.1f, config, vDiode, iDiode);
    bool converged_low = (iterations > 0 && iterations < config.maxIterations);
    reportTest("Solver Convergence (0.1V)", converged_low,
               "Converged in " + std::to_string(iterations) + " iterations");
    
    // Test case 2: Mid-range voltage
    iterations = solver.solve(0.5f, config, vDiode, iDiode);
    bool converged_mid = (iterations > 0 && iterations < config.maxIterations);
    reportTest("Solver Convergence (0.5V)", converged_mid,
               "Converged in " + std::to_string(iterations) + " iterations, V=" + 
               std::to_string(vDiode) + "V, I=" + std::to_string(iDiode) + "A");
    
    // Test case 3: High voltage
    iterations = solver.solve(2.0f, config, vDiode, iDiode);
    bool converged_high = (iterations > 0 && iterations <= config.maxIterations);
    reportTest("Solver Convergence (2.0V)", converged_high,
               "Converged in " + std::to_string(iterations) + " iterations");
    
    // Test: Verify Shockley equation satisfaction
    float nVt = si1N4148.n * si1N4148.Vt;
    float expArg = std::clamp(vDiode / nVt, -20.0f, 50.0f);
    float i_shockley = si1N4148.Is * (std::exp(expArg) - 1.0f);
    float residual = std::abs(iDiode - i_shockley);
    bool shockley_satisfied = residual < 1e-12f;
    reportTest("Shockley Equation Satisfied", shockley_satisfied,
               "Residual: " + std::to_string(residual));
}

/**
 * Test 3: Diode clipping stage - Series configuration
 */
void testSeriesDiodeClipping() {
    std::cout << "\n=== TEST 3: Series Diode Clipping ===" << std::endl;
    
    DiodeCharacteristics si1N4148 = DiodeCharacteristics::Si1N4148();
    DiodeClippingStage clipper(si1N4148, DiodeClippingStage::TopologyType::SeriesDiode);
    
    // Test: Small signal passes through
    float out_small = clipper.processSample(0.1f);
    bool small_passes = (std::abs(out_small - 0.1f) < 0.05f);
    reportTest("Series: Small Signal Passes", small_passes,
               "Input: 0.1V, Output: " + std::to_string(out_small) + "V");
    
    // Test: Large signal gets clipped
    float out_large_pos = clipper.processSample(2.0f);
    float out_large_neg = clipper.processSample(-2.0f);
    bool large_clipped = (std::abs(out_large_pos) < 1.0f) && (std::abs(out_large_neg) < 1.0f);
    reportTest("Series: Large Signal Clipped", large_clipped,
               "Input: ±2.0V, Output: +" + std::to_string(out_large_pos) + "V, " +
               std::to_string(out_large_neg) + "V");
}

/**
 * Test 4: Back-to-back diode clipping (symmetric)
 */
void testBackToBackClipping() {
    std::cout << "\n=== TEST 4: Back-to-Back Diode Clipping ===" << std::endl;
    
    DiodeCharacteristics si1N4148 = DiodeCharacteristics::Si1N4148();
    DiodeClippingStage clipper(si1N4148, DiodeClippingStage::TopologyType::BackToBackDiodes);
    
    // Test: Symmetry around zero
    float out_pos = clipper.processSample(0.5f);
    float out_neg = clipper.processSample(-0.5f);
    bool symmetric = std::abs(out_pos + out_neg) < 0.01f;
    reportTest("Back-to-Back: Symmetry", symmetric,
               "+0.5V → " + std::to_string(out_pos) + "V, -0.5V → " + std::to_string(out_neg) + "V");
    
    // Test: Linear region (small signals)
    float out_tiny = clipper.processSample(0.01f);
    bool linear_region = std::abs(out_tiny - 0.01f) < 0.005f;
    reportTest("Back-to-Back: Linear Region", linear_region,
               "Input: 0.01V, Output: " + std::to_string(out_tiny) + "V");
    
    // Test: Soft clipping (threshold region)
    float threshold = clipper.getThresholdVoltage();
    float out_threshold = clipper.processSample(threshold * 1.5f);
    bool soft_clipping = out_threshold < (threshold * 1.5f) && out_threshold > threshold;
    reportTest("Back-to-Back: Soft Clipping", soft_clipping,
               "Input: " + std::to_string(threshold * 1.5f) + "V, Output: " + 
               std::to_string(out_threshold) + "V, Threshold: " + std::to_string(threshold) + "V");
}

/**
 * Test 5: Different diode types
 */
void testDiodeTypes() {
    std::cout << "\n=== TEST 5: Different Diode Types ===" << std::endl;
    
    // Silicon 1N4148 (fast switching) - higher ideality factor
    DiodeClippingStage si1N4148(DiodeCharacteristics::Si1N4148());
    float si_out = si1N4148.processSample(0.3f);  // Lower input for better differentiation
    
    // Germanium OA90 (vintage fuzz) - lower forward voltage
    DiodeClippingStage ge_oa90(DiodeCharacteristics::Ge_OA90());
    float ge_out = ge_oa90.processSample(0.3f);
    
    // Ge has lower forward voltage (~0.3V) than Si (~0.65V), so at same input they clip differently
    bool different_behavior = std::abs(si_out - ge_out) > 0.02f;
    reportTest("Different Diode Types", different_behavior,
               "Si1N4148 @ 0.3V: " + std::to_string(si_out) + "V, Ge_OA90 @ 0.3V: " + std::to_string(ge_out) + "V");
}

/**
 * Test 6: MXR Distortion+ style clipping (symmetric back-to-back)
 */
void testMXRDistortionClipping() {
    std::cout << "\n=== TEST 6: MXR Distortion+ Clipping Simulation ===" << std::endl;
    
    // MXR Distortion+ uses back-to-back 1N4148 diodes
    DiodeCharacteristics diode = DiodeCharacteristics::Si1N4148();
    DiodeClippingStage clipper(diode, DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f);
    
    // Sweep from -1.0 to +1.0V and collect clipping curve
    std::vector<float> input_sweep;
    std::vector<float> output_sweep;
    
    for (float v = -1.0f; v <= 1.0f; v += 0.1f) {
        input_sweep.push_back(v);
        output_sweep.push_back(clipper.processSample(v));
    }
    
    // Check: Output should be bounded
    float max_out = *std::max_element(output_sweep.begin(), output_sweep.end());
    float min_out = *std::min_element(output_sweep.begin(), output_sweep.end());
    bool bounded = (max_out < 1.0f) && (min_out > -1.0f);
    
    reportTest("MXR Clipping: Output Bounded", bounded,
               "Range: [" + std::to_string(min_out) + ", " + std::to_string(max_out) + "]V");
    
    // Print clipping curve for visual inspection
    std::cout << "\nMXR-style Clipping Curve:" << std::endl;
    std::cout << std::setw(10) << "Input (V)" << std::setw(15) << "Output (V)" << std::endl;
    for (size_t i = 0; i < input_sweep.size(); ++i) {
        std::cout << std::setw(10) << std::fixed << std::setprecision(2) << input_sweep[i]
                  << std::setw(15) << std::fixed << std::setprecision(4) << output_sweep[i] << std::endl;
    }
}

/**
 * Test 7: Impedance effects on clipping
 */
void testLoadImpedanceEffect() {
    std::cout << "\n=== TEST 7: Load Impedance Effects ===" << std::endl;
    
    DiodeCharacteristics diode = DiodeCharacteristics::Si1N4148();
    
    // Test with series diode topology - impedance creates voltage divider
    // With low source impedance (1Ω internal), test load impedances: 1k vs 100k
    // This creates: V_diode = I * Rload / (Rs + Rload) * V_applied
    
    // Clipper with 1kΩ load - more current flows, less voltage divider effect
    DiodeClippingStage clipper_low(diode, DiodeClippingStage::TopologyType::SeriesDiode, 1000.0f);
    
    // Clipper with 100kΩ load - less current, more voltage appears across diode
    DiodeClippingStage clipper_high(diode, DiodeClippingStage::TopologyType::SeriesDiode, 100000.0f);
    
    // Use voltage where impedance divider effect is clearly visible (~0.8V mid-range)
    float out_low = clipper_low.processSample(0.8f);
    float out_high = clipper_high.processSample(0.8f);
    
    // Lower load impedance → more current flows → more voltage drop across Rs → lower output
    // Higher load impedance → less current → less drop across Rs → higher output
    bool impedance_effect = out_high > out_low;
    reportTest("Load Impedance Effect", impedance_effect,
               "Low Z (1kΩ): " + std::to_string(out_low) + "V, High Z (100kΩ): " + std::to_string(out_high) + "V");
}

/**
 * Main test runner
 */
int main() {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   DIODE CLIPPING MODEL - TEST SUITE            ║" << std::endl;
    std::cout << "║   Shockley Equation Solver & LUT               ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;
    
    // Run all tests
    testDiodeLUT();
    testNewtonRaphsonSolver();
    testSeriesDiodeClipping();
    testBackToBackClipping();
    testDiodeTypes();
    testMXRDistortionClipping();
    testLoadImpedanceEffect();
    
    // Summary
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║              TEST SUMMARY                      ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;
    
    int passed = 0;
    for (const auto& result : results) {
        if (result.passed) passed++;
    }
    
    std::cout << "Tests Passed: " << passed << "/" << results.size() << std::endl;
    
    if (passed == (int)results.size()) {
        std::cout << "\n✓ ALL TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
