/*
  ==============================================================================
    Automated Calibrator - Parameter optimization via comparison
    
    Automatically adjusts digital pedal parameters to match LiveSpice simulation
    using gradient descent and waveform comparison metrics.
  ==============================================================================
*/

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <functional>
#include "IAudioProcessor.h"
#include "Logging.h"

class AutomatedCalibrator
{
public:
    AutomatedCalibrator();
    ~AutomatedCalibrator();

    // ========================================================================
    // Comparison Metrics
    // ========================================================================
    
    struct ComparisonMetrics
    {
        float rmsError = 0.0f;          // Root mean square error
        float correlation = 0.0f;        // Pearson correlation coefficient
        float peakError = 0.0f;          // Maximum absolute difference
        float spectralError = 0.0f;      // Frequency domain error
        float totalError = 0.0f;         // Combined weighted error metric
        
        juce::String toString() const
        {
            return "RMS Error: " + juce::String(rmsError, 6) + 
                   " | Correlation: " + juce::String(correlation, 4) +
                   " | Peak Error: " + juce::String(peakError, 6) +
                   " | Spectral Error: " + juce::String(spectralError, 6) +
                   " | Total: " + juce::String(totalError, 6);
        }
    };

    // ========================================================================
    // Parameter Configuration
    // ========================================================================
    
    struct ParameterConfig
    {
        juce::String name;               // Parameter name (e.g., "drive", "level")
        float minValue = 0.0f;           // Minimum value to test
        float maxValue = 1.0f;           // Maximum value to test
        float currentValue = 0.5f;       // Current value
        float stepSize = 0.01f;          // Initial step size for gradient descent
        bool enabled = true;             // Whether to optimize this parameter
    };

    // ========================================================================
    // Calibration Settings
    // ========================================================================
    
    struct CalibrationSettings
    {
        int maxIterations = 100;         // Maximum optimization iterations
        float convergenceThreshold = 1e-4f;  // Stop when improvement < threshold
        float learningRate = 0.1f;       // Step size multiplier
        bool useAdaptiveStep = true;     // Adjust step size based on progress
        
        // Test signal parameters
        float testFrequency = 440.0f;    // Hz - test tone frequency
        float testAmplitude = 0.5f;      // 0.0 - 1.0
        int testDurationSamples = 8192;  // Length of test signal
        
        // Metric weights (sum should = 1.0)
        float rmsWeight = 0.5f;
        float correlationWeight = 0.3f;
        float spectralWeight = 0.2f;
    };

    // ========================================================================
    // Main API
    // ========================================================================
    
    /**
     * Initialize calibrator with reference (LiveSpice) and target (Digital) processors
     */
    void setProcessors(IAudioProcessor* reference, IAudioProcessor* target);
    
    /**
     * Add a parameter to optimize
     */
    void addParameter(const juce::String& name, float min, float max, float initial);
    
    /**
     * Configure calibration behavior
     */
    void setCalibrationSettings(const CalibrationSettings& settings);
    
    /**
     * Start automated calibration process
     * Returns true if calibration completed successfully
     */
    bool runCalibration();
    
    /**
     * Run a single calibration step (for real-time/interactive mode)
     * Returns true if more steps needed
     */
    bool runCalibrationStep();
    
    /**
     * Compare current processor outputs without changing parameters
     */
    ComparisonMetrics compareProcessors();
    
    /**
     * Generate test signal (sine, square, sweep, etc.)
     */
    void generateTestSignal(juce::AudioBuffer<float>& buffer, int numSamples);
    
    /**
     * Apply current parameter values to target processor
     */
    void applyParametersToTarget();
    
    /**
     * Get current parameter values
     */
    std::vector<ParameterConfig> getParameters() const { return parameters; }
    
    /**
     * Get latest comparison metrics
     */
    ComparisonMetrics getLatestMetrics() const { return latestMetrics; }
    
    /**
     * Get calibration progress (0.0 - 1.0)
     */
    float getProgress() const;
    
    /**
     * Check if calibration is running
     */
    bool isRunning() const { return running; }
    
    /**
     * Stop calibration
     */
    void stop();
    
    /**
     * Callback for progress updates
     */
    std::function<void(int iteration, const ComparisonMetrics&)> onProgressUpdate;
    
    /**
     * Callback when calibration completes
     */
    std::function<void(bool success, const ComparisonMetrics&)> onCalibrationComplete;

private:
    // ========================================================================
    // Internal Methods
    // ========================================================================
    
    ComparisonMetrics calculateMetrics(const juce::AudioBuffer<float>& reference,
                                       const juce::AudioBuffer<float>& target);
    
    float calculateRMSError(const juce::AudioBuffer<float>& a,
                           const juce::AudioBuffer<float>& b);
    
    float calculateCorrelation(const juce::AudioBuffer<float>& a,
                              const juce::AudioBuffer<float>& b);
    
    float calculatePeakError(const juce::AudioBuffer<float>& a,
                            const juce::AudioBuffer<float>& b);
    
    float calculateSpectralError(const juce::AudioBuffer<float>& a,
                                const juce::AudioBuffer<float>& b);
    
    void computeGradient(int paramIndex, float& gradient);
    
    void updateParameters(const std::vector<float>& gradients);
    
    // ========================================================================
    // Member Variables
    // ========================================================================
    
    IAudioProcessor* referenceProcessor = nullptr;
    IAudioProcessor* targetProcessor = nullptr;
    
    std::vector<ParameterConfig> parameters;
    CalibrationSettings settings;
    ComparisonMetrics latestMetrics;
    
    bool running = false;
    int currentIteration = 0;
    float previousError = std::numeric_limits<float>::max();
    
    // Buffers for processing
    juce::AudioBuffer<float> testSignal;
    juce::AudioBuffer<float> referenceOutput;
    juce::AudioBuffer<float> targetOutput;
    
    // FFT for spectral analysis
    juce::dsp::FFT fft{11};  // 2048 points
    std::vector<float> fftData;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomatedCalibrator)
};
