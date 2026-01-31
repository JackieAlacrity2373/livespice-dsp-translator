#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cmath>

namespace SpiceValidation {

/**
 * Test signal generator for validation
 * Creates standard test waveforms to feed into both SPICE and our DSP solver
 */
class TestSignalGenerator {
public:
    enum class SignalType {
        SineWave,           // Simple sine
        Chirp,              // Frequency sweep
        SquareWave,         // Square wave (for transitions)
        NoiseWhite,         // White noise
        ImpulseResponse     // Single impulse for step response
    };
    
    struct SignalParams {
        float sampleRate = 44100.0f;
        float duration = 1.0f;        // Seconds
        float frequency = 1000.0f;    // Hz
        float amplitude = 0.1f;       // V (peak)
        float startFreq = 20.0f;      // For chirp
        float endFreq = 20000.0f;     // For chirp
    };
    
    /**
     * Generate test signal
     */
    static std::vector<float> generateSignal(SignalType type, const SignalParams& params);
    
    /**
     * Generate logarithmic sweep (for frequency response)
     */
    static std::vector<float> generateLogSweep(float startFreq, float endFreq, 
                                               float durationSec, float sampleRate);
    
    /**
     * Generate stepped input levels (for transfer curve)
     */
    static std::vector<float> generateStepSweep(float minAmplitude, float maxAmplitude, 
                                                float stepSize, float sampleRate);
};

/**
 * SPICE Netlist Generator
 * Creates SPICE .cir files from circuit parameters
 */
class SpiceNetlistGenerator {
public:
    struct DiodeCircuitSpec {
        std::string title;
        std::string diodePartNumber;      // "1N4148", "OA90", etc.
        float sourceResistance = 0.0f;    // Input source impedance
        float loadResistance = 10000.0f;  // Load impedance
        
        enum class Topology {
            BackToBack,   // Two opposing diodes
            Series,       // Single series diode
            Parallel,     // Single parallel diode
        } topology = Topology::BackToBack;
    };
    
    /**
     * Generate SPICE netlist for diode clipping circuit
     * @param spec Circuit specification
     * @param testSignal Input signal description
     * @return SPICE netlist as string
     */
    static std::string generateDiodeTestBench(const DiodeCircuitSpec& spec,
                                              const std::string& signalDescription);
    
    /**
     * Generate transient analysis command
     */
    static std::string generateTransientAnalysis(float endTime, float stepTime);
    
    /**
     * Generate frequency response analysis
     */
    static std::string generateACAnalysis(float startFreq, float endFreq, int points);
    
    /**
     * Get SPICE model for a diode part number
     */
    static std::string getSpiceModel(const std::string& partNumber);
};

/**
 * Validation Results
 */
struct ValidationMetrics {
    std::string circuitName;
    float thd;               // Total Harmonic Distortion %
    float frequencyError;    // ±dB error across range
    float amplitudeError;    // ±dB error
    float peakVoltageError;  // Volts
    float rmsDifference;     // RMS of difference signal
    bool passed;
    std::string notes;
};

/**
 * Comparison Framework
 */
class ComparisonAnalyzer {
public:
    /**
     * Compare two waveforms (our DSP vs SPICE reference)
     */
    static ValidationMetrics compareWaveforms(
        const std::vector<float>& dspOutput,
        const std::vector<float>& spiceReference,
        const std::string& circuitName);
    
    /**
     * Calculate THD (Total Harmonic Distortion)
     */
    static float calculateTHD(const std::vector<float>& signal, 
                             float fundamentalFreq, float sampleRate);
    
    /**
     * Calculate frequency response error
     */
    static float calculateFrequencyResponseError(
        const std::vector<float>& dspOutput,
        const std::vector<float>& spiceOutput,
        float sampleRate);
};

/**
 * Main Validation Runner
 */
class SpiceValidator {
public:
    struct ValidationConfig {
        bool generateNetlists = true;
        bool runNgspice = true;        // Requires ngspice installed
        std::string ngspicePath = "ngspice";
        bool generateReport = true;
        std::string outputDir = "./validation_results";
    };
    
    /**
     * Run full validation suite against MXR Distortion+ circuit
     */
    static std::vector<ValidationMetrics> validateAgainstMXR(
        const ValidationConfig& config);
    
    /**
     * Validate generic diode clipping stage
     */
    static ValidationMetrics validateDiodeStage(
        const SpiceNetlistGenerator::DiodeCircuitSpec& spec,
        const ValidationConfig& config);
    
    /**
     * Generate HTML report of validation results
     */
    static void generateValidationReport(
        const std::vector<ValidationMetrics>& results,
        const std::string& outputPath);
};

} // namespace SpiceValidation
