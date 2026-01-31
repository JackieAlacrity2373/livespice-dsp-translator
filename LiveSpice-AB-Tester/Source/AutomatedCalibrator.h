/*
  ==============================================================================
    AutomatedCalibrator - Automated Parameter Optimization Engine
    
    Uses gradient descent to optimize digital pedal parameters to match
    LiveSpice simulation outputs via waveform comparison.
  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <vector>
#include <string>

/**
 * Comparison metrics between reference and target audio signals
 */
struct ComparisonMetrics
{
    float rmsError;         // Root Mean Square error (lower is better)
    float correlation;      // Pearson correlation coefficient (1.0 = perfect match)
    float peakError;        // Maximum absolute difference
    float spectralError;    // Frequency domain error (future implementation)
    float totalError;       // Weighted combination of all metrics
    
    juce::String toString() const
    {
        juce::String s;
        s << "RMS: " << juce::String(rmsError, 4)
          << ", Corr: " << juce::String(correlation, 4)
          << ", Peak: " << juce::String(peakError, 4)
          << ", Total: " << juce::String(totalError, 4);
        return s;
    }
};

/**
 * Parameter configuration for optimization
 */
struct ParameterConfig
{
    std::string name;
    float minValue;
    float maxValue;
    float currentValue;
    float stepSize;      // For gradient computation
    bool enabled;        // Whether to optimize this parameter
    
    ParameterConfig(const std::string& n, float min, float max, float initial)
        : name(n), minValue(min), maxValue(max), currentValue(initial),
          stepSize(0.01f), enabled(true) {}
};

/**
 * Automated calibration engine using gradient descent optimization
 */
class AutomatedCalibrator
{
public:
    /**
     * Calibration settings
     */
    struct CalibrationSettings
    {
        int maxIterations = 100;
        float convergenceThreshold = 1e-4f;
        float learningRate = 0.1f;
        bool useAdaptiveStep = true;
        
        // Test signal configuration
        float testFrequency = 440.0f;      // Hz
        float testAmplitude = 0.5f;        // 0.0 to 1.0
        int testDurationSamples = 8192;
        
        // Metric weights
        float rmsWeight = 0.5f;
        float correlationWeight = 0.3f;
        float spectralWeight = 0.2f;
    };
    
    AutomatedCalibrator();
    ~AutomatedCalibrator();
    
    // Configuration
    void setCalibrationSettings(const CalibrationSettings& settings);
    CalibrationSettings getCalibrationSettings() const { return settings; }
    
    // Parameter management
    void addParameter(const std::string& name, float minValue, float maxValue, float initialValue);
    void removeParameter(const std::string& name);
    void setParameterValue(const std::string& name, float value);
    float getParameterValue(const std::string& name) const;
    std::vector<ParameterConfig> getAllParameters() const { return parameters; }
    
    // Calibration control
    bool isCalibrating() const { return calibrating; }
    void startCalibration();
    void stopCalibration();
    void resetParameters();
    
    // Results
    ComparisonMetrics getLatestMetrics() const { return latestMetrics; }
    int getCurrentIteration() const { return currentIteration; }
    
    // Callbacks
    std::function<void(int iteration, const ComparisonMetrics& metrics)> onProgressUpdate;
    std::function<void(bool converged, const ComparisonMetrics& finalMetrics)> onCalibrationComplete;
    
    // Parameter application callback - implement this to actually set parameters
    std::function<void(const std::string& paramName, float value)> onParameterChange;
    
    // Audio comparison callback - implement this to get processed audio
    std::function<void(juce::AudioBuffer<float>& buffer)> onProcessReference;
    std::function<void(juce::AudioBuffer<float>& buffer)> onProcessTarget;

private:
    // Optimization algorithm
    void runOptimization();
    void computeGradients(std::vector<float>& gradients);
    void updateParameters(const std::vector<float>& gradients);
    ComparisonMetrics evaluateError();
    
    // Metrics calculation
    float calculateRMSError(const juce::AudioBuffer<float>& reference, 
                           const juce::AudioBuffer<float>& target);
    float calculateCorrelation(const juce::AudioBuffer<float>& reference,
                              const juce::AudioBuffer<float>& target);
    float calculatePeakError(const juce::AudioBuffer<float>& reference,
                            const juce::AudioBuffer<float>& target);
    float calculateSpectralError(const juce::AudioBuffer<float>& reference,
                                const juce::AudioBuffer<float>& target);
    
    // Test signal generation
    void generateTestSignal(juce::AudioBuffer<float>& buffer);
    
    // State
    std::vector<ParameterConfig> parameters;
    CalibrationSettings settings;
    ComparisonMetrics latestMetrics;
    int currentIteration = 0;
    bool calibrating = false;
    
    // Audio buffers
    juce::AudioBuffer<float> testSignal;
    juce::AudioBuffer<float> referenceOutput;
    juce::AudioBuffer<float> targetOutput;
    
    // Thread safety
    std::unique_ptr<std::thread> calibrationThread;
    juce::CriticalSection lock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomatedCalibrator)
};
