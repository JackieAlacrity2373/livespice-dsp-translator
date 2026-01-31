/*
  ==============================================================================
    Calibration Panel - UI for automated parameter optimization
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AutomatedCalibrator.h"
#include "Logging.h"

class CalibrationPanel : public juce::Component,
                         public juce::Button::Listener,
                         public juce::Timer
{
public:
    CalibrationPanel(AutomatedCalibrator& calibrator)
        : calibrator(calibrator)
    {
        // Start/Stop button
        startStopButton.setButtonText("Start Calibration");
        startStopButton.addListener(this);
        addAndMakeVisible(startStopButton);
        
        // Progress label
        progressLabel.setText("Ready", juce::dontSendNotification);
        progressLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(progressLabel);
        
        // Metrics display
        metricsLabel.setText("No metrics yet", juce::dontSendNotification);
        metricsLabel.setFont(juce::Font(12.0f, juce::Font::plain));
        addAndMakeVisible(metricsLabel);
        
        // Settings
        maxIterationsLabel.setText("Max Iterations:", juce::dontSendNotification);
        addAndMakeVisible(maxIterationsLabel);
        
        maxIterationsSlider.setRange(10, 500, 10);
        maxIterationsSlider.setValue(100);
        maxIterationsSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        maxIterationsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        addAndMakeVisible(maxIterationsSlider);
        
        learningRateLabel.setText("Learning Rate:", juce::dontSendNotification);
        addAndMakeVisible(learningRateLabel);
        
        learningRateSlider.setRange(0.001, 0.5, 0.001);
        learningRateSlider.setValue(0.1);
        learningRateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        learningRateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        addAndMakeVisible(learningRateSlider);
        
        // Parameters display
        parametersLabel.setText("Parameters:", juce::dontSendNotification);
        parametersLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        addAndMakeVisible(parametersLabel);
        
        parameterValuesLabel.setText("", juce::dontSendNotification);
        parameterValuesLabel.setFont(juce::Font(12.0f, juce::Font::plain));
        addAndMakeVisible(parameterValuesLabel);
        
        // Setup callbacks
        calibrator.onProgressUpdate = [this](int iteration, const AutomatedCalibrator::ComparisonMetrics& metrics)
        {
            juce::MessageManager::callAsync([this, iteration, metrics]()
            {
                updateProgress(iteration, metrics);
            });
        };
        
        calibrator.onCalibrationComplete = [this](bool success, const AutomatedCalibrator::ComparisonMetrics& metrics)
        {
            juce::MessageManager::callAsync([this, success, metrics]()
            {
                onCalibrationFinished(success, metrics);
            });
        };
        
        startTimer(100);  // Update UI every 100ms
    }
    
    ~CalibrationPanel() override
    {
        stopTimer();
        startStopButton.removeListener(this);
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey.darker());
        
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText("Automated Calibration", getLocalBounds().removeFromTop(30),
                  juce::Justification::centred, true);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        bounds.removeFromTop(30);  // Title space
        
        // Start button
        startStopButton.setBounds(bounds.removeFromTop(30).reduced(5));
        bounds.removeFromTop(5);
        
        // Progress
        progressLabel.setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(5);
        
        // Metrics
        metricsLabel.setBounds(bounds.removeFromTop(60));
        bounds.removeFromTop(10);
        
        // Settings
        auto settingsArea = bounds.removeFromTop(80);
        
        auto row1 = settingsArea.removeFromTop(35);
        maxIterationsLabel.setBounds(row1.removeFromLeft(120));
        maxIterationsSlider.setBounds(row1);
        
        settingsArea.removeFromTop(5);
        
        auto row2 = settingsArea.removeFromTop(35);
        learningRateLabel.setBounds(row2.removeFromLeft(120));
        learningRateSlider.setBounds(row2);
        
        bounds.removeFromTop(10);
        
        // Parameters
        parametersLabel.setBounds(bounds.removeFromTop(25));
        parameterValuesLabel.setBounds(bounds.removeFromTop(100));
    }
    
    void buttonClicked(juce::Button* button) override
    {
        if (button == &startStopButton)
        {
            if (calibrator.isRunning())
            {
                calibrator.stop();
                startStopButton.setButtonText("Start Calibration");
                progressLabel.setText("Stopped", juce::dontSendNotification);
            }
            else
            {
                startCalibration();
            }
        }
    }
    
    void timerCallback() override
    {
        // Update parameter values display
        if (calibrator.isRunning())
        {
            updateParameterDisplay();
        }
    }

private:
    void startCalibration()
    {
        // Update settings
        AutomatedCalibrator::CalibrationSettings settings;
        settings.maxIterations = (int)maxIterationsSlider.getValue();
        settings.learningRate = (float)learningRateSlider.getValue();
        calibrator.setCalibrationSettings(settings);
        
        // Start in background thread
        startStopButton.setButtonText("Stop Calibration");
        progressLabel.setText("Starting...", juce::dontSendNotification);
        
        std::thread([this]()
        {
            bool success = calibrator.runCalibration();
            juce::ignoreUnused(success);
        }).detach();
    }
    
    void updateProgress(int iteration, const AutomatedCalibrator::ComparisonMetrics& metrics)
    {
        progressLabel.setText("Iteration " + juce::String(iteration), 
                            juce::dontSendNotification);
        
        metricsLabel.setText(
            "RMS Error: " + juce::String(metrics.rmsError, 6) + "\n" +
            "Correlation: " + juce::String(metrics.correlation, 4) + "\n" +
            "Peak Error: " + juce::String(metrics.peakError, 6) + "\n" +
            "Total Error: " + juce::String(metrics.totalError, 6),
            juce::dontSendNotification);
    }
    
    void updateParameterDisplay()
    {
        juce::String paramText;
        
        for (const auto& param : calibrator.getParameters())
        {
            paramText += param.name + ": " + juce::String(param.currentValue, 3) + "\n";
        }
        
        parameterValuesLabel.setText(paramText, juce::dontSendNotification);
    }
    
    void onCalibrationFinished(bool success, const AutomatedCalibrator::ComparisonMetrics& metrics)
    {
        startStopButton.setButtonText("Start Calibration");
        
        if (success)
        {
            progressLabel.setText("Converged!", juce::dontSendNotification);
            LOG("Calibration completed successfully!");
        }
        else
        {
            progressLabel.setText("Max iterations reached", juce::dontSendNotification);
            LOG("Calibration stopped at max iterations");
        }
        
        updateProgress(0, metrics);
        updateParameterDisplay();
    }
    
    AutomatedCalibrator& calibrator;
    
    juce::TextButton startStopButton;
    juce::Label progressLabel;
    juce::Label metricsLabel;
    
    juce::Label maxIterationsLabel;
    juce::Slider maxIterationsSlider;
    
    juce::Label learningRateLabel;
    juce::Slider learningRateSlider;
    
    juce::Label parametersLabel;
    juce::Label parameterValuesLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CalibrationPanel)
};
