/*
  ==============================================================================
    AutomatedCalibrator Implementation
  ==============================================================================
*/

#include "AutomatedCalibrator.h"
#include <cmath>
#include <algorithm>

AutomatedCalibrator::AutomatedCalibrator()
{
}

AutomatedCalibrator::~AutomatedCalibrator()
{
    stopCalibration();
}

void AutomatedCalibrator::setCalibrationSettings(const CalibrationSettings& newSettings)
{
    juce::ScopedLock sl(lock);
    settings = newSettings;
    
    // Regenerate test signal with new settings
    testSignal.setSize(2, settings.testDurationSamples);
    generateTestSignal(testSignal);
}

void AutomatedCalibrator::addParameter(const std::string& name, float minValue, float maxValue, float initialValue)
{
    juce::ScopedLock sl(lock);
    
    // Check if parameter already exists
    for (auto& param : parameters)
    {
        if (param.name == name)
        {
            param.minValue = minValue;
            param.maxValue = maxValue;
            param.currentValue = juce::jlimit(minValue, maxValue, initialValue);
            return;
        }
    }
    
    // Add new parameter
    parameters.emplace_back(name, minValue, maxValue, initialValue);
}

void AutomatedCalibrator::removeParameter(const std::string& name)
{
    juce::ScopedLock sl(lock);
    parameters.erase(
        std::remove_if(parameters.begin(), parameters.end(),
            [&name](const ParameterConfig& p) { return p.name == name; }),
        parameters.end());
}

void AutomatedCalibrator::setParameterValue(const std::string& name, float value)
{
    juce::ScopedLock sl(lock);
    for (auto& param : parameters)
    {
        if (param.name == name)
        {
            param.currentValue = juce::jlimit(param.minValue, param.maxValue, value);
            
            // Notify callback
            if (onParameterChange)
                onParameterChange(name, param.currentValue);
            return;
        }
    }
}

float AutomatedCalibrator::getParameterValue(const std::string& name) const
{
    juce::ScopedLock sl(lock);
    for (const auto& param : parameters)
    {
        if (param.name == name)
            return param.currentValue;
    }
    return 0.0f;
}

void AutomatedCalibrator::startCalibration()
{
    if (calibrating)
        return;
    
    calibrating = true;
    currentIteration = 0;
    
    // Setup buffers
    testSignal.setSize(2, settings.testDurationSamples);
    referenceOutput.setSize(2, settings.testDurationSamples);
    targetOutput.setSize(2, settings.testDurationSamples);
    
    generateTestSignal(testSignal);
    
    // Start calibration in background thread
    calibrationThread = std::make_unique<std::thread>([this]() {
        runOptimization();
    });
    calibrationThread->detach();
}

void AutomatedCalibrator::stopCalibration()
{
    calibrating = false;
    
    if (calibrationThread && calibrationThread->joinable())
        calibrationThread->join();
}

void AutomatedCalibrator::resetParameters()
{
    juce::ScopedLock sl(lock);
    for (auto& param : parameters)
    {
        param.currentValue = (param.minValue + param.maxValue) * 0.5f;
        if (onParameterChange)
            onParameterChange(param.name, param.currentValue);
    }
}

void AutomatedCalibrator::runOptimization()
{
    float previousError = std::numeric_limits<float>::max();
    bool converged = false;
    
    for (currentIteration = 0; currentIteration < settings.maxIterations && calibrating; ++currentIteration)
    {
        // Compute gradients
        std::vector<float> gradients;
        computeGradients(gradients);
        
        // Update parameters
        updateParameters(gradients);
        
        // Evaluate current error
        latestMetrics = evaluateError();
        
        // Check convergence
        float improvement = previousError - latestMetrics.totalError;
        if (improvement > 0 && improvement < settings.convergenceThreshold)
        {
            converged = true;
            break;
        }
        
        previousError = latestMetrics.totalError;
        
        // Notify progress
        if (onProgressUpdate)
        {
            juce::MessageManager::callAsync([this, iter = currentIteration, metrics = latestMetrics]() {
                if (onProgressUpdate)
                    onProgressUpdate(iter, metrics);
            });
        }
        
        // Small delay to prevent CPU overload
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    calibrating = false;
    
    // Notify completion
    if (onCalibrationComplete)
    {
        juce::MessageManager::callAsync([this, converged, metrics = latestMetrics]() {
            if (onCalibrationComplete)
                onCalibrationComplete(converged, metrics);
        });
    }
}

void AutomatedCalibrator::computeGradients(std::vector<float>& gradients)
{
    juce::ScopedLock sl(lock);
    gradients.resize(parameters.size());
    
    // Get current error
    float errorCurrent = latestMetrics.totalError;
    
    // Compute numerical gradient for each parameter
    for (size_t i = 0; i < parameters.size(); ++i)
    {
        if (!parameters[i].enabled)
        {
            gradients[i] = 0.0f;
            continue;
        }
        
        float originalValue = parameters[i].currentValue;
        float stepSize = parameters[i].stepSize;
        
        // Perturb parameter
        parameters[i].currentValue = juce::jlimit(
            parameters[i].minValue,
            parameters[i].maxValue,
            originalValue + stepSize);
        
        // Apply parameter change
        if (onParameterChange)
            onParameterChange(parameters[i].name, parameters[i].currentValue);
        
        // Evaluate error with perturbed parameter
        auto metricsPlus = evaluateError();
        
        // Compute gradient
        gradients[i] = (metricsPlus.totalError - errorCurrent) / stepSize;
        
        // Restore original value
        parameters[i].currentValue = originalValue;
        if (onParameterChange)
            onParameterChange(parameters[i].name, originalValue);
    }
}

void AutomatedCalibrator::updateParameters(const std::vector<float>& gradients)
{
    juce::ScopedLock sl(lock);
    
    for (size_t i = 0; i < parameters.size(); ++i)
    {
        if (!parameters[i].enabled)
            continue;
        
        // Gradient descent update
        float update = -settings.learningRate * gradients[i];
        
        // Adaptive step sizing
        if (settings.useAdaptiveStep)
        {
            if (std::abs(gradients[i]) > 10.0f)
                update *= 0.1f;  // Large gradient - reduce step
            else if (std::abs(gradients[i]) < 0.01f)
                update *= 1.5f;  // Small gradient - increase step
        }
        
        // Update and clamp
        parameters[i].currentValue = juce::jlimit(
            parameters[i].minValue,
            parameters[i].maxValue,
            parameters[i].currentValue + update);
        
        // Apply parameter change
        if (onParameterChange)
            onParameterChange(parameters[i].name, parameters[i].currentValue);
    }
}

ComparisonMetrics AutomatedCalibrator::evaluateError()
{
    // Process reference (LiveSpice)
    referenceOutput.makeCopyOf(testSignal);
    if (onProcessReference)
        onProcessReference(referenceOutput);
    
    // Process target (Digital)
    targetOutput.makeCopyOf(testSignal);
    if (onProcessTarget)
        onProcessTarget(targetOutput);
    
    // Calculate all metrics
    ComparisonMetrics metrics;
    metrics.rmsError = calculateRMSError(referenceOutput, targetOutput);
    metrics.correlation = calculateCorrelation(referenceOutput, targetOutput);
    metrics.peakError = calculatePeakError(referenceOutput, targetOutput);
    metrics.spectralError = calculateSpectralError(referenceOutput, targetOutput);
    
    // Compute weighted total error
    metrics.totalError = 
        metrics.rmsError * settings.rmsWeight +
        (1.0f - metrics.correlation) * settings.correlationWeight +
        metrics.spectralError * settings.spectralWeight;
    
    return metrics;
}

float AutomatedCalibrator::calculateRMSError(const juce::AudioBuffer<float>& reference,
                                            const juce::AudioBuffer<float>& target)
{
    float sumSquaredError = 0.0f;
    int totalSamples = 0;
    
    for (int ch = 0; ch < reference.getNumChannels(); ++ch)
    {
        const float* refData = reference.getReadPointer(ch);
        const float* targetData = target.getReadPointer(ch);
        
        for (int i = 0; i < reference.getNumSamples(); ++i)
        {
            float error = refData[i] - targetData[i];
            sumSquaredError += error * error;
            ++totalSamples;
        }
    }
    
    return std::sqrt(sumSquaredError / totalSamples);
}

float AutomatedCalibrator::calculateCorrelation(const juce::AudioBuffer<float>& reference,
                                               const juce::AudioBuffer<float>& target)
{
    // Pearson correlation coefficient
    float sumRef = 0.0f, sumTarget = 0.0f;
    int totalSamples = 0;
    
    // Calculate means
    for (int ch = 0; ch < reference.getNumChannels(); ++ch)
    {
        const float* refData = reference.getReadPointer(ch);
        const float* targetData = target.getReadPointer(ch);
        
        for (int i = 0; i < reference.getNumSamples(); ++i)
        {
            sumRef += refData[i];
            sumTarget += targetData[i];
            ++totalSamples;
        }
    }
    
    float meanRef = sumRef / totalSamples;
    float meanTarget = sumTarget / totalSamples;
    
    // Calculate correlation
    float numerator = 0.0f;
    float denomRef = 0.0f;
    float denomTarget = 0.0f;
    
    for (int ch = 0; ch < reference.getNumChannels(); ++ch)
    {
        const float* refData = reference.getReadPointer(ch);
        const float* targetData = target.getReadPointer(ch);
        
        for (int i = 0; i < reference.getNumSamples(); ++i)
        {
            float diffRef = refData[i] - meanRef;
            float diffTarget = targetData[i] - meanTarget;
            
            numerator += diffRef * diffTarget;
            denomRef += diffRef * diffRef;
            denomTarget += diffTarget * diffTarget;
        }
    }
    
    if (denomRef == 0.0f || denomTarget == 0.0f)
        return 0.0f;
    
    return numerator / std::sqrt(denomRef * denomTarget);
}

float AutomatedCalibrator::calculatePeakError(const juce::AudioBuffer<float>& reference,
                                             const juce::AudioBuffer<float>& target)
{
    float maxError = 0.0f;
    
    for (int ch = 0; ch < reference.getNumChannels(); ++ch)
    {
        const float* refData = reference.getReadPointer(ch);
        const float* targetData = target.getReadPointer(ch);
        
        for (int i = 0; i < reference.getNumSamples(); ++i)
        {
            float error = std::abs(refData[i] - targetData[i]);
            maxError = std::max(maxError, error);
        }
    }
    
    return maxError;
}

float AutomatedCalibrator::calculateSpectralError(const juce::AudioBuffer<float>& reference,
                                                 const juce::AudioBuffer<float>& target)
{
    // Placeholder - future FFT implementation
    juce::ignoreUnused(reference, target);
    return 0.0f;
}

void AutomatedCalibrator::generateTestSignal(juce::AudioBuffer<float>& buffer)
{
    const float sampleRate = 48000.0f;  // Assume standard sample rate
    const float frequency = settings.testFrequency;
    const float amplitude = settings.testAmplitude;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float phase = juce::MathConstants<float>::twoPi * frequency * i / sampleRate;
            data[i] = amplitude * std::sin(phase);
        }
    }
}
