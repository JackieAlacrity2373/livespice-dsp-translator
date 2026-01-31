/*
  ==============================================================================
    Automated Calibrator - Implementation
  ==============================================================================
*/

#include "AutomatedCalibrator.h"

AutomatedCalibrator::AutomatedCalibrator()
    : fftData(4096)
{
    testSignal.setSize(2, 8192);
}

AutomatedCalibrator::~AutomatedCalibrator()
{
    stop();
}

void AutomatedCalibrator::setProcessors(IAudioProcessor* reference, IAudioProcessor* target)
{
    referenceProcessor = reference;
    targetProcessor = target;
    
    LOG("Calibrator: Reference and target processors set");
}

void AutomatedCalibrator::addParameter(const juce::String& name, float min, float max, float initial)
{
    ParameterConfig param;
    param.name = name;
    param.minValue = min;
    param.maxValue = max;
    param.currentValue = juce::jlimit(min, max, initial);
    param.stepSize = (max - min) * 0.01f;  // 1% of range
    param.enabled = true;
    
    parameters.push_back(param);
    
    LOG("Calibrator: Added parameter '" + name + "' [" + 
        juce::String(min) + " - " + juce::String(max) + "], initial: " + juce::String(initial));
}

void AutomatedCalibrator::setCalibrationSettings(const CalibrationSettings& newSettings)
{
    settings = newSettings;
    testSignal.setSize(2, settings.testDurationSamples);
}

bool AutomatedCalibrator::runCalibration()
{
    if (!referenceProcessor || !targetProcessor)
    {
        LOG("Calibrator: ERROR - Processors not set!");
        return false;
    }
    
    if (parameters.empty())
    {
        LOG("Calibrator: ERROR - No parameters configured!");
        return false;
    }
    
    running = true;
    currentIteration = 0;
    previousError = std::numeric_limits<float>::max();
    
    LOG("=== Starting Automated Calibration ===");
    LOG("Parameters: " + juce::String(parameters.size()));
    LOG("Max Iterations: " + juce::String(settings.maxIterations));
    LOG("Convergence Threshold: " + juce::String(settings.convergenceThreshold));
    
    // Generate test signal once
    generateTestSignal(testSignal, settings.testDurationSamples);
    
    // Allocate output buffers
    referenceOutput.setSize(2, settings.testDurationSamples);
    targetOutput.setSize(2, settings.testDurationSamples);
    
    // Main optimization loop
    while (running && currentIteration < settings.maxIterations)
    {
        if (!runCalibrationStep())
            break;
    }
    
    bool success = (currentIteration < settings.maxIterations);
    
    if (success)
        LOG("=== Calibration CONVERGED after " + juce::String(currentIteration) + " iterations ===");
    else
        LOG("=== Calibration STOPPED at max iterations ===");
    
    LOG("Final Metrics: " + latestMetrics.toString());
    
    if (onCalibrationComplete)
        onCalibrationComplete(success, latestMetrics);
    
    running = false;
    return success;
}

bool AutomatedCalibrator::runCalibrationStep()
{
    currentIteration++;
    
    // Apply current parameters to target
    applyParametersToTarget();
    
    // Process test signal through both processors
    referenceOutput.makeCopyOf(testSignal);
    targetOutput.makeCopyOf(testSignal);
    
    if (referenceProcessor)
        referenceProcessor->processBlock(referenceOutput);
    
    if (targetProcessor)
        targetProcessor->processBlock(targetOutput);
    
    // Calculate error metrics
    latestMetrics = calculateMetrics(referenceOutput, targetOutput);
    
    // Log progress
    if (currentIteration % 10 == 0 || currentIteration == 1)
    {
        LOG("Iteration " + juce::String(currentIteration) + ": " + latestMetrics.toString());
        
        // Log parameter values
        juce::String paramStr = "  Params: ";
        for (const auto& p : parameters)
            paramStr += p.name + "=" + juce::String(p.currentValue, 3) + " ";
        LOG(paramStr);
    }
    
    if (onProgressUpdate)
        onProgressUpdate(currentIteration, latestMetrics);
    
    // Check convergence
    float errorImprovement = previousError - latestMetrics.totalError;
    
    if (std::abs(errorImprovement) < settings.convergenceThreshold && currentIteration > 10)
    {
        LOG("Converged! Error improvement < threshold");
        return false;  // Stop iteration
    }
    
    // Compute gradients for each parameter
    std::vector<float> gradients(parameters.size());
    
    for (size_t i = 0; i < parameters.size(); ++i)
    {
        if (parameters[i].enabled)
            computeGradient((int)i, gradients[i]);
    }
    
    // Update parameters using gradient descent
    updateParameters(gradients);
    
    previousError = latestMetrics.totalError;
    
    return true;  // Continue iteration
}

AutomatedCalibrator::ComparisonMetrics AutomatedCalibrator::compareProcessors()
{
    if (!referenceProcessor || !targetProcessor)
        return ComparisonMetrics();
    
    // Generate and process test signal
    generateTestSignal(testSignal, settings.testDurationSamples);
    
    referenceOutput.makeCopyOf(testSignal);
    targetOutput.makeCopyOf(testSignal);
    
    referenceProcessor->processBlock(referenceOutput);
    targetProcessor->processBlock(targetOutput);
    
    return calculateMetrics(referenceOutput, targetOutput);
}

void AutomatedCalibrator::generateTestSignal(juce::AudioBuffer<float>& buffer, int numSamples)
{
    buffer.clear();
    
    // Generate sine wave test signal
    const float frequency = settings.testFrequency;
    const float amplitude = settings.testAmplitude;
    const float sampleRate = 44100.0f;  // TODO: Get from processor
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float phase = 2.0f * juce::MathConstants<float>::pi * frequency * sample / sampleRate;
            channelData[sample] = amplitude * std::sin(phase);
        }
    }
}

void AutomatedCalibrator::applyParametersToTarget()
{
    if (!targetProcessor)
        return;
    
    // Apply each parameter to the target processor
    for (const auto& param : parameters)
    {
        if (param.enabled)
        {
            targetProcessor->setParameter(param.name, param.currentValue);
        }
    }
}

AutomatedCalibrator::ComparisonMetrics 
AutomatedCalibrator::calculateMetrics(const juce::AudioBuffer<float>& reference,
                                     const juce::AudioBuffer<float>& target)
{
    ComparisonMetrics metrics;
    
    metrics.rmsError = calculateRMSError(reference, target);
    metrics.correlation = calculateCorrelation(reference, target);
    metrics.peakError = calculatePeakError(reference, target);
    metrics.spectralError = calculateSpectralError(reference, target);
    
    // Combined weighted error
    metrics.totalError = 
        metrics.rmsError * settings.rmsWeight +
        (1.0f - metrics.correlation) * settings.correlationWeight +
        metrics.spectralError * settings.spectralWeight;
    
    return metrics;
}

float AutomatedCalibrator::calculateRMSError(const juce::AudioBuffer<float>& a,
                                            const juce::AudioBuffer<float>& b)
{
    float sumSquaredError = 0.0f;
    int totalSamples = 0;
    
    int numChannels = juce::jmin(a.getNumChannels(), b.getNumChannels());
    int numSamples = juce::jmin(a.getNumSamples(), b.getNumSamples());
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* aData = a.getReadPointer(ch);
        const float* bData = b.getReadPointer(ch);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float error = aData[i] - bData[i];
            sumSquaredError += error * error;
            totalSamples++;
        }
    }
    
    return std::sqrt(sumSquaredError / totalSamples);
}

float AutomatedCalibrator::calculateCorrelation(const juce::AudioBuffer<float>& a,
                                               const juce::AudioBuffer<float>& b)
{
    // Pearson correlation coefficient
    float meanA = 0.0f, meanB = 0.0f;
    int totalSamples = 0;
    
    int numChannels = juce::jmin(a.getNumChannels(), b.getNumChannels());
    int numSamples = juce::jmin(a.getNumSamples(), b.getNumSamples());
    
    // Calculate means
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* aData = a.getReadPointer(ch);
        const float* bData = b.getReadPointer(ch);
        
        for (int i = 0; i < numSamples; ++i)
        {
            meanA += aData[i];
            meanB += bData[i];
            totalSamples++;
        }
    }
    
    meanA /= totalSamples;
    meanB /= totalSamples;
    
    // Calculate correlation
    float numerator = 0.0f;
    float denomA = 0.0f, denomB = 0.0f;
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* aData = a.getReadPointer(ch);
        const float* bData = b.getReadPointer(ch);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float devA = aData[i] - meanA;
            float devB = bData[i] - meanB;
            
            numerator += devA * devB;
            denomA += devA * devA;
            denomB += devB * devB;
        }
    }
    
    float denom = std::sqrt(denomA * denomB);
    return (denom > 1e-10f) ? (numerator / denom) : 0.0f;
}

float AutomatedCalibrator::calculatePeakError(const juce::AudioBuffer<float>& a,
                                             const juce::AudioBuffer<float>& b)
{
    float maxError = 0.0f;
    
    int numChannels = juce::jmin(a.getNumChannels(), b.getNumChannels());
    int numSamples = juce::jmin(a.getNumSamples(), b.getNumSamples());
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* aData = a.getReadPointer(ch);
        const float* bData = b.getReadPointer(ch);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float error = std::abs(aData[i] - bData[i]);
            maxError = juce::jmax(maxError, error);
        }
    }
    
    return maxError;
}

float AutomatedCalibrator::calculateSpectralError(const juce::AudioBuffer<float>& a,
                                                  const juce::AudioBuffer<float>& b)
{
    // Simplified spectral comparison using magnitude spectrum
    // In a full implementation, would use FFT on both signals and compare
    
    // For now, return 0 (not implemented)
    // TODO: Implement FFT-based spectral comparison
    juce::ignoreUnused(a, b);
    return 0.0f;
}

void AutomatedCalibrator::computeGradient(int paramIndex, float& gradient)
{
    if (paramIndex < 0 || paramIndex >= (int)parameters.size())
    {
        gradient = 0.0f;
        return;
    }
    
    auto& param = parameters[paramIndex];
    
    // Save current value
    float originalValue = param.currentValue;
    float stepSize = param.stepSize;
    
    // Evaluate error at current value (already computed in latestMetrics)
    float errorCurrent = latestMetrics.totalError;
    
    // Evaluate error at (current + step)
    param.currentValue = juce::jlimit(param.minValue, param.maxValue, 
                                     originalValue + stepSize);
    applyParametersToTarget();
    
    targetOutput.makeCopyOf(testSignal);
    if (targetProcessor)
        targetProcessor->processBlock(targetOutput);
    
    auto metricsPlus = calculateMetrics(referenceOutput, targetOutput);
    
    // Compute numerical gradient
    gradient = (metricsPlus.totalError - errorCurrent) / stepSize;
    
    // Restore original value
    param.currentValue = originalValue;
}

void AutomatedCalibrator::updateParameters(const std::vector<float>& gradients)
{
    for (size_t i = 0; i < parameters.size(); ++i)
    {
        if (!parameters[i].enabled)
            continue;
        
        auto& param = parameters[i];
        
        // Gradient descent update
        float update = -settings.learningRate * gradients[i];
        
        // Adaptive step size
        if (settings.useAdaptiveStep)
        {
            // If gradient is too large, reduce step
            if (std::abs(gradients[i]) > 10.0f)
                update *= 0.1f;
            
            // If gradient is very small, increase step slightly
            if (std::abs(gradients[i]) < 0.01f)
                update *= 1.5f;
        }
        
        param.currentValue = juce::jlimit(param.minValue, param.maxValue,
                                         param.currentValue + update);
    }
}

float AutomatedCalibrator::getProgress() const
{
    if (settings.maxIterations == 0)
        return 0.0f;
    
    return (float)currentIteration / settings.maxIterations;
}

void AutomatedCalibrator::stop()
{
    running = false;
}
