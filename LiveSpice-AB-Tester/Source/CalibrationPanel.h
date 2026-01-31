/*
  ==============================================================================
    CalibrationPanel - UI Component for Automated Calibration
    
    Provides controls and real-time feedback for the automated calibration system
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AutomatedCalibrator.h"

class CalibrationPanel : public juce::Component,
                        public juce::Timer
{
public:
    explicit CalibrationPanel(AutomatedCalibrator& calibratorRef)
        : calibrator(calibratorRef)
    {
        // Title
        titleLabel.setText("Automated Calibration", juce::dontSendNotification);
        titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
        titleLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel);
        
        // Start/Stop button
        startStopButton.setButtonText("Start Calibration");
        startStopButton.onClick = [this]() {
            if (!calibrator.isCalibrating())
            {
                startCalibration();
            }
            else
            {
                stopCalibration();
            }
        };
        addAndMakeVisible(startStopButton);
        
        // Progress label
        progressLabel.setText("Ready", juce::dontSendNotification);
        progressLabel.setJustificationType(juce::Justification::centred);
        progressLabel.setFont(juce::Font(14.0f));
        addAndMakeVisible(progressLabel);
        
        // Metrics display
        metricsLabel.setText("Metrics:\n---", juce::dontSendNotification);
        metricsLabel.setJustificationType(juce::Justification::left);
        metricsLabel.setFont(juce::Font(12.0f, juce::Font::plain));
        addAndMakeVisible(metricsLabel);
        
        // Settings sliders
        maxIterationsLabel.setText("Max Iterations:", juce::dontSendNotification);
        maxIterationsLabel.attachToComponent(&maxIterationsSlider, true);
        maxIterationsSlider.setRange(10, 500, 10);
        maxIterationsSlider.setValue(100);
        maxIterationsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        addAndMakeVisible(maxIterationsSlider);
        
        learningRateLabel.setText("Learning Rate:", juce::dontSendNotification);
        learningRateLabel.attachToComponent(&learningRateSlider, true);
        learningRateSlider.setRange(0.001, 0.5, 0.001);
        learningRateSlider.setValue(0.1);
        learningRateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        addAndMakeVisible(learningRateSlider);
        
        // Parameter values display
        parametersLabel.setText("Parameters:", juce::dontSendNotification);
        parametersLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        parametersLabel.setJustificationType(juce::Justification::left);
        addAndMakeVisible(parametersLabel);
        
        parameterValuesLabel.setText("---", juce::dontSendNotification);
        parameterValuesLabel.setFont(juce::Font(12.0f));
        parameterValuesLabel.setJustificationType(juce::Justification::left);
        addAndMakeVisible(parameterValuesLabel);
        
        // Setup callbacks
        calibrator.onProgressUpdate = [this](int iteration, const ComparisonMetrics& metrics) {
            onProgress(iteration, metrics);
        };
        
        calibrator.onCalibrationComplete = [this](bool converged, const ComparisonMetrics& finalMetrics) {
            onComplete(converged, finalMetrics);
        };
        
        // Start timer for UI updates
        startTimer(100);
    }
    
    ~CalibrationPanel() override
    {
        stopTimer();
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff3a3a3a));
        
        // Draw border
        g.setColour(juce::Colours::grey);
        g.drawRect(getLocalBounds(), 2);
        
        // Draw sections
        g.setColour(juce::Colour(0xff4a4a4a));
        auto area = getLocalBounds().reduced(10);
        area.removeFromTop(40); // Title
        g.fillRect(area.removeFromTop(120).reduced(0, 5)); // Metrics section
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        
        // Title
        titleLabel.setBounds(area.removeFromTop(40));
        
        // Start/Stop button
        startStopButton.setBounds(area.removeFromTop(40).reduced(100, 5));
        
        area.removeFromTop(10);
        
        // Progress
        progressLabel.setBounds(area.removeFromTop(30));
        
        area.removeFromTop(10);
        
        // Metrics display
        metricsLabel.setBounds(area.removeFromTop(110).reduced(5));
        
        area.removeFromTop(10);
        
        // Settings
        maxIterationsSlider.setBounds(area.removeFromTop(30).reduced(120, 0));
        area.removeFromTop(5);
        learningRateSlider.setBounds(area.removeFromTop(30).reduced(120, 0));
        
        area.removeFromTop(15);
        
        // Parameters
        parametersLabel.setBounds(area.removeFromTop(25).reduced(5, 0));
        parameterValuesLabel.setBounds(area.reduced(5));
    }
    
    void timerCallback() override
    {
        // Update parameter values display
        if (calibrator.isCalibrating())
        {
            updateParameterDisplay();
        }
    }

private:
    AutomatedCalibrator& calibrator;
    
    // UI Components
    juce::Label titleLabel;
    juce::TextButton startStopButton;
    juce::Label progressLabel;
    juce::Label metricsLabel;
    
    juce::Label maxIterationsLabel;
    juce::Slider maxIterationsSlider;
    juce::Label learningRateLabel;
    juce::Slider learningRateSlider;
    
    juce::Label parametersLabel;
    juce::Label parameterValuesLabel;
    
    void startCalibration()
    {
        // Update settings from UI
        auto settings = calibrator.getCalibrationSettings();
        settings.maxIterations = static_cast<int>(maxIterationsSlider.getValue());
        settings.learningRate = static_cast<float>(learningRateSlider.getValue());
        calibrator.setCalibrationSettings(settings);
        
        // Start calibration
        calibrator.startCalibration();
        
        // Update UI
        startStopButton.setButtonText("Stop Calibration");
        startStopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
        progressLabel.setText("Calibrating...", juce::dontSendNotification);
        
        // Disable settings during calibration
        maxIterationsSlider.setEnabled(false);
        learningRateSlider.setEnabled(false);
    }
    
    void stopCalibration()
    {
        calibrator.stopCalibration();
        
        // Update UI
        startStopButton.setButtonText("Start Calibration");
        startStopButton.setColour(juce::TextButton::buttonColourId, 
            getLookAndFeel().findColour(juce::TextButton::buttonColourId));
        
        // Re-enable settings
        maxIterationsSlider.setEnabled(true);
        learningRateSlider.setEnabled(true);
    }
    
    void onProgress(int iteration, const ComparisonMetrics& metrics)
    {
        // Update progress label
        progressLabel.setText("Iteration: " + juce::String(iteration), juce::dontSendNotification);
        
        // Update metrics display
        juce::String metricsText;
        metricsText << "Metrics:\n"
                   << "  RMS Error: " << juce::String(metrics.rmsError, 4) << "\n"
                   << "  Correlation: " << juce::String(metrics.correlation, 4) << "\n"
                   << "  Peak Error: " << juce::String(metrics.peakError, 4) << "\n"
                   << "  Total Error: " << juce::String(metrics.totalError, 4);
        
        metricsLabel.setText(metricsText, juce::dontSendNotification);
    }
    
    void onComplete(bool converged, const ComparisonMetrics& finalMetrics)
    {
        juce::ignoreUnused(finalMetrics);
        
        // Update UI
        startStopButton.setButtonText("Start Calibration");
        startStopButton.setColour(juce::TextButton::buttonColourId,
            getLookAndFeel().findColour(juce::TextButton::buttonColourId));
        
        progressLabel.setText(converged ? "Converged!" : "Max iterations reached", 
            juce::dontSendNotification);
        
        // Re-enable settings
        maxIterationsSlider.setEnabled(true);
        learningRateSlider.setEnabled(true);
    }
    
    void updateParameterDisplay()
    {
        auto params = calibrator.getAllParameters();
        
        juce::String paramText;
        for (const auto& param : params)
        {
            paramText << param.name.c_str() << ": " 
                     << juce::String(param.currentValue, 3) << "\n";
        }
        
        if (paramText.isEmpty())
            paramText = "No parameters configured";
        
        parameterValuesLabel.setText(paramText, juce::dontSendNotification);
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CalibrationPanel)
};
