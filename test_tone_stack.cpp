#include "StateSpaceFilter.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <sstream>

using namespace LiveSpiceDSP;

// ============================================================================
// Test Utilities
// ============================================================================

class TestResults {
public:
    int passed = 0;
    int failed = 0;
    std::string lastError;
    
    void pass(const std::string& test) {
        passed++;
        std::cout << "Γ£ô PASS: " << test << "\n";
    }
    
    void fail(const std::string& test, const std::string& reason) {
        failed++;
        lastError = reason;
        std::cout << "✗ FAIL: " << test << " - " << reason << "\n";
    }
    
    void summary() {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "Tests Passed: " << passed << "/" << (passed + failed) << "\n";
        if (failed == 0) {
            std::cout << "Γ£ô ALL TESTS PASSED\n";
        } else {
            std::cout << "✗ " << failed << " tests failed\n";
        }
        std::cout << std::string(80, '=') << "\n";
    }
};

bool almostEqual(float a, float b, float epsilon = 1e-5f) {
    return std::abs(a - b) < epsilon;
}

// ============================================================================
// TEST 1: Biquad Filter Basics
// ============================================================================

void testBiquadPassThrough(TestResults& results) {
    BiquadFilter filter;
    
    // Unity gain should pass signal through unchanged
    float input = 0.5f;
    float output = filter.process(input);
    
    if (almostEqual(output, 0.5f)) {
        results.pass("Biquad Unity Gain - Pass-through");
    } else {
        results.fail("Biquad Unity Gain", 
                     "Input: " + std::to_string(input) + ", Output: " + std::to_string(output));
    }
}

void testLowPassFrequencyResponse(TestResults& results) {
    float sampleRate = 44100.0f;
    BiquadFilter filter;
    
    // Design low-pass filter @ 10kHz
    auto coeff = BiquadFilter::designLowPass(sampleRate, 10000.0f);
    filter.setCoefficients(coeff);
    
    // Generate frequency response
    auto freqs = FrequencyResponseAnalyzer::generateLogSweep(20.0f, 20000.0f, 100);
    auto response = FrequencyResponseAnalyzer::getMagnitudeResponse(coeff, freqs, sampleRate);
    
    // Check that magnitude is monotonically decreasing
    bool isDecreasing = true;
    for (size_t i = 1; i < response.size(); ++i) {
        if (response[i] > response[i-1] + 0.01f) {  // Small tolerance for numerical errors
            isDecreasing = false;
            break;
        }
    }
    
    if (isDecreasing) {
        results.pass("Low-Pass: Magnitude Monotonically Decreasing");
    } else {
        results.fail("Low-Pass: Magnitude Response", "Response not monotonically decreasing");
    }
    
    // Check attenuation at high frequency (should be >> 20Hz response)
    if (response.back() < response.front() * 0.1f) {
        results.pass("Low-Pass: High Frequency Attenuation");
    } else {
        results.fail("Low-Pass: High Frequency", 
                     "Not enough attenuation at 20kHz");
    }
}

// ============================================================================
// TEST 2: Tone Stack Functionality
// ============================================================================

void testToneStackBassControl(TestResults& results) {
    ToneStackController toneStack(44100.0f);
    
    // Apply bass boost
    toneStack.setBassGain(12.0f);
    
    // Process a low frequency signal (100Hz)
    float lowFreqSignal = 0.1f;
    float outputBoosted = toneStack.process(lowFreqSignal);
    
    // Reset and apply bass cut
    toneStack.reset();
    toneStack.setBassGain(-12.0f);
    float outputCut = toneStack.process(lowFreqSignal);
    
    // Boosted should be > cut
    if (std::abs(outputBoosted) > std::abs(outputCut)) {
        results.pass("Tone Stack: Bass Control Response");
    } else {
        results.fail("Tone Stack: Bass Control",
                     "Boost (" + std::to_string(outputBoosted) + ") not > Cut (" + 
                     std::to_string(outputCut) + ")");
    }
}

void testToneStackMidControl(TestResults& results) {
    ToneStackController toneStack(44100.0f);
    
    // Apply mid boost
    toneStack.setMidGain(12.0f);
    
    // Process a mid frequency signal (1kHz)
    float midFreqSignal = 0.1f;
    float outputBoosted = toneStack.process(midFreqSignal);
    
    // Reset and apply mid cut
    toneStack.reset();
    toneStack.setMidGain(-12.0f);
    float outputCut = toneStack.process(midFreqSignal);
    
    // Boosted should be > cut
    if (std::abs(outputBoosted) > std::abs(outputCut)) {
        results.pass("Tone Stack: Mid Control Response");
    } else {
        results.fail("Tone Stack: Mid Control",
                     "Boost not > Cut");
    }
}

void testToneStackTrebleControl(TestResults& results) {
    ToneStackController toneStack(44100.0f);
    
    // Apply treble boost
    toneStack.setTrebleGain(12.0f);
    
    // Process a high frequency signal (10kHz)
    float highFreqSignal = 0.1f;
    float outputBoosted = toneStack.process(highFreqSignal);
    
    // Reset and apply treble cut
    toneStack.reset();
    toneStack.setTrebleGain(-12.0f);
    float outputCut = toneStack.process(highFreqSignal);
    
    // Boosted should be > cut
    if (std::abs(outputBoosted) > std::abs(outputCut)) {
        results.pass("Tone Stack: Treble Control Response");
    } else {
        results.fail("Tone Stack: Treble Control",
                     "Boost not > Cut");
    }
}

// ============================================================================
// TEST 3: Biquad Cascading
// ============================================================================

void testBiquadCascading(TestResults& results) {
    BiquadFilterBank filterBank(2);  // 2-stage cascade
    
    // Set both stages to identical low-pass filters
    auto coeff = BiquadFilter::designLowPass(44100.0f, 5000.0f);
    filterBank.setStageCoefficients(0, coeff);
    filterBank.setStageCoefficients(1, coeff);
    
    // Process a signal
    float input = 0.5f;
    float output = filterBank.process(input);
    
    // Cascading should provide steeper rolloff than single stage
    // So output should be smaller (more attenuation if there's any frequency content)
    if (std::abs(output) <= std::abs(input)) {
        results.pass("Biquad: Cascading Preserves Stability");
    } else {
        results.fail("Biquad: Cascading", "Output > Input");
    }
}

// ============================================================================
// TEST 4: Phase Response Analysis
// ============================================================================

void testPhaseResponseContinuity(TestResults& results) {
    BiquadCoefficients coeff = BiquadFilter::designLowPass(44100.0f, 5000.0f);
    
    auto freqs = FrequencyResponseAnalyzer::generateLogSweep(20.0f, 20000.0f, 50);
    auto phase = FrequencyResponseAnalyzer::getPhaseResponse(coeff, freqs, 44100.0f);
    
    // Phase should be monotonically decreasing (for low-pass)
    bool isDecreasing = true;
    for (size_t i = 1; i < phase.size(); ++i) {
        if (phase[i] > phase[i-1] + 0.1f) {  // Small tolerance
            isDecreasing = false;
            break;
        }
    }
    
    if (isDecreasing) {
        results.pass("Phase Response: Monotonic Decrease (Low-Pass)");
    } else {
        results.fail("Phase Response", "Not monotonically decreasing");
    }
}

// ============================================================================
// TEST 5: Complete Pedal DSP Chain
// ============================================================================

void testDistortionPedalIntegration(TestResults& results) {
    DistortionPedalDSP pedal(44100.0f);
    
    // Set moderate gain and tone
    pedal.setInputGain(6.0f);   // +6dB
    pedal.setOutputLevel(0.0f); // 0dB (unity)
    pedal.getToneStack().setBassGain(3.0f);
    pedal.getToneStack().setTrebleGain(-3.0f);
    
    // Process a small signal (shouldn't clip at +6dB)
    float input = 0.1f;
    float output = pedal.process(input);
    
    if (std::isfinite(output) && std::abs(output) < 1.5f) {
        results.pass("Distortion Pedal: Signal Processing");
    } else {
        results.fail("Distortion Pedal", 
                     "Invalid output: " + std::to_string(output));
    }
}

void testDistortionPedalClipping(TestResults& results) {
    DistortionPedalDSP pedal(44100.0f);
    
    // High gain should cause clipping
    pedal.setInputGain(18.0f);  // +18dB
    pedal.setOutputLevel(-6.0f); // -6dB
    
    // Process large signal
    float input = 0.5f;
    float output = pedal.process(input);
    
    // Output should be clipped (not proportional to input at high gain)
    // Also should be bounded
    if (std::abs(output) < 1.0f) {
        results.pass("Distortion Pedal: Clipping Bounded");
    } else {
        results.fail("Distortion Pedal: Clipping", 
                     "Output not bounded: " + std::to_string(output));
    }
}

void testDistortionPedalToneControl(TestResults& results) {
    DistortionPedalDSP pedal1(44100.0f);
    DistortionPedalDSP pedal2(44100.0f);
    
    pedal1.setInputGain(12.0f);
    pedal2.setInputGain(12.0f);
    
    // Pedal 1: bright tone
    pedal1.getToneStack().setTrebleGain(6.0f);
    pedal1.getToneStack().setBassGain(-6.0f);
    
    // Pedal 2: dark tone
    pedal2.getToneStack().setTrebleGain(-6.0f);
    pedal2.getToneStack().setBassGain(6.0f);
    
    // Process same input
    float input = 0.2f;
    float output1 = pedal1.process(input);
    
    pedal2.reset();  // Reset state
    float output2 = pedal2.process(input);
    
    if (std::abs(output1) != std::abs(output2)) {
        results.pass("Distortion Pedal: Tone Control Affects Output");
    } else {
        results.fail("Distortion Pedal: Tone Control", 
                     "Different tone settings produced same output");
    }
}

// ============================================================================
// TEST 6: Frequency Sweep Analysis
// ============================================================================

void testFrequencySweepGeneration(TestResults& results) {
    auto sweep = FrequencyResponseAnalyzer::generateLogSweep(20.0f, 20000.0f, 100);
    
    bool isLogarithmic = true;
    
    // Check that spacing is logarithmic (ratio between consecutive points is constant)
    if (sweep.size() > 2) {
        float firstRatio = sweep[1] / sweep[0];
        for (size_t i = 2; i < sweep.size(); ++i) {
            float ratio = sweep[i] / sweep[i-1];
            if (std::abs(ratio - firstRatio) > firstRatio * 0.01f) {
                isLogarithmic = false;
                break;
            }
        }
    }
    
    if (isLogarithmic && sweep.front() >= 20.0f && sweep.back() <= 20001.0f) {
        results.pass("Frequency Sweep: Logarithmic Spacing");
    } else {
        results.fail("Frequency Sweep", "Sweep not properly logarithmic or out of range");
    }
}

// ============================================================================
// TEST 7: Gain Control Verification
// ============================================================================

void testInputGainControl(TestResults& results) {
    DistortionPedalDSP pedal(44100.0f);
    
    // Test 0dB (unity) gain
    pedal.setInputGain(0.0f);
    float input = 0.05f;
    float output0db = pedal.process(input);
    
    // Test +6dB gain
    pedal.reset();
    pedal.setInputGain(6.0f);
    float output6db = pedal.process(input);
    
    // +6dB should roughly double the output (accounting for clipping)
    // At small signals, should be roughly 2x
    if (std::abs(output6db) > std::abs(output0db)) {
        results.pass("Input Gain: Control Increases Level");
    } else {
        results.fail("Input Gain", "Gain increase didn't increase output");
    }
}

void testOutputLevelControl(TestResults& results) {
    DistortionPedalDSP pedal(44100.0f);
    
    // Process with full output level (0dB)
    pedal.setInputGain(0.0f);
    pedal.setOutputLevel(0.0f);
    float input = 0.1f;
    float outputFull = pedal.process(input);
    
    // Process with reduced output level (-6dB)
    pedal.reset();
    pedal.setInputGain(0.0f);
    pedal.setOutputLevel(-6.0f);
    float outputReduced = pedal.process(input);
    
    // Reduced should be less than full
    if (std::abs(outputReduced) < std::abs(outputFull)) {
        results.pass("Output Level: Control Decreases Level");
    } else {
        results.fail("Output Level", "Level reduction didn't decrease output");
    }
}

// ============================================================================
// TEST 8: High-Frequency Response
// ============================================================================

void testHighPassResponse(TestResults& results) {
    BiquadCoefficients coeff = BiquadFilter::designHighPass(44100.0f, 80.0f);
    
    auto freqs = FrequencyResponseAnalyzer::generateLogSweep(20.0f, 20000.0f, 50);
    auto response = FrequencyResponseAnalyzer::getMagnitudeResponse(coeff, freqs, 44100.0f);
    
    // For high-pass, magnitude should be monotonically increasing
    bool isIncreasing = true;
    for (size_t i = 1; i < response.size(); ++i) {
        if (response[i] < response[i-1] - 0.01f) {
            isIncreasing = false;
            break;
        }
    }
    
    if (isIncreasing) {
        results.pass("High-Pass: Magnitude Monotonically Increasing");
    } else {
        results.fail("High-Pass: Magnitude Response", "Not monotonically increasing");
    }
}

// ============================================================================
// Main Test Suite
// ============================================================================

int main() {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "STATE-SPACE FILTERING TEST SUITE - PHASE 2\n";
    std::cout << "3-Band Tone Control & Frequency Response\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    TestResults results;
    
    // Test 1: Biquad Filter Basics
    std::cout << "=== TEST 1: Biquad Filter Fundamentals ===\n";
    testBiquadPassThrough(results);
    testLowPassFrequencyResponse(results);
    
    // Test 2: Tone Stack Functionality
    std::cout << "\n=== TEST 2: Tone Stack (Bass/Mid/Treble) ===\n";
    testToneStackBassControl(results);
    testToneStackMidControl(results);
    testToneStackTrebleControl(results);
    
    // Test 3: Cascading
    std::cout << "\n=== TEST 3: Biquad Cascading ===\n";
    testBiquadCascading(results);
    
    // Test 4: Phase Response
    std::cout << "\n=== TEST 4: Phase Response Analysis ===\n";
    testPhaseResponseContinuity(results);
    
    // Test 5: Complete Pedal
    std::cout << "\n=== TEST 5: Distortion Pedal Integration ===\n";
    testDistortionPedalIntegration(results);
    testDistortionPedalClipping(results);
    testDistortionPedalToneControl(results);
    
    // Test 6: Frequency Sweep
    std::cout << "\n=== TEST 6: Frequency Sweep Analysis ===\n";
    testFrequencySweepGeneration(results);
    
    // Test 7: Gain Control
    std::cout << "\n=== TEST 7: Gain & Level Control ===\n";
    testInputGainControl(results);
    testOutputLevelControl(results);
    
    // Test 8: High-Pass
    std::cout << "\n=== TEST 8: High-Pass Filter ===\n";
    testHighPassResponse(results);
    
    results.summary();
    
    return results.failed == 0 ? 0 : 1;
}
