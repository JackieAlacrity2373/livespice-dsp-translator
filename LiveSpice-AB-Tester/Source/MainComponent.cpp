/*
  ==============================================================================
    Main Component Implementation
  ==============================================================================
*/

#include "MainComponent.h"

MainComponent::MainComponent()
{
    // Initialize plugin hosts
    pluginHostA = std::make_unique<PluginHost>();
    pluginHostB = std::make_unique<PluginHost>();

    // Initialize control panel
    controlPanel = std::make_unique<ControlPanel>();
    addAndMakeVisible(*controlPanel);

    // Initialize A/B switch
    abSwitch = std::make_unique<ABSwitch>();
    addAndMakeVisible(*abSwitch);

    // Initialize calibration system
    calibrator = std::make_unique<AutomatedCalibrator>();
    calibrationPanel = std::make_unique<CalibrationPanel>(*calibrator);
    addAndMakeVisible(*calibrationPanel);

    abSwitch->onSwitch = [this](bool useA) {
        usePluginA = useA;
        updateStatus();
    };

    // Setup load buttons
    loadPluginAButton.onClick = [this] { loadPluginA(); };
    loadPluginBButton.onClick = [this] { loadPluginB(); };
    addAndMakeVisible(loadPluginAButton);
    addAndMakeVisible(loadPluginBButton);

    // Setup labels
    pluginALabel.setText("Plugin A: None", juce::dontSendNotification);
    pluginBLabel.setText("Plugin B: None", juce::dontSendNotification);
    pluginALabel.setJustificationType(juce::Justification::centred);
    pluginBLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pluginALabel);
    addAndMakeVisible(pluginBLabel);

    statusLabel.setText("Ready - Load plugins to begin testing", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(statusLabel);

    // Set window size
    setSize(900, 700);

    // Setup audio
    setAudioChannels(2, 2);

    // Start timer for UI updates
    startTimer(50);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    // Prepare plugin hosts
    if (pluginHostA)
        pluginHostA->prepareToPlay(sampleRate, samplesPerBlockExpected);
    
    if (pluginHostB)
        pluginHostB->prepareToPlay(sampleRate, samplesPerBlockExpected);

    // Allocate temp buffers
    tempBufferA.setSize(2, samplesPerBlockExpected);
    tempBufferB.setSize(2, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    // Process through selected plugin
    if (usePluginA && isPluginALoaded)
    {
        // Copy input to temp buffer
        tempBufferA.makeCopyOf(*bufferToFill.buffer);
        
        // Process through plugin A
        pluginHostA->processBlock(tempBufferA, 
            juce::MidiBuffer(), 
            bufferToFill.startSample, 
            bufferToFill.numSamples);
        
        // Copy processed audio to output
        for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
        {
            bufferToFill.buffer->copyFrom(ch, bufferToFill.startSample,
                tempBufferA, ch, 0, bufferToFill.numSamples);
        }
    }
    else if (!usePluginA && isPluginBLoaded)
    {
        // Copy input to temp buffer
        tempBufferB.makeCopyOf(*bufferToFill.buffer);
        
        // Process through plugin B
        pluginHostB->processBlock(tempBufferB,
            juce::MidiBuffer(),
            bufferToFill.startSample,
            bufferToFill.numSamples);
        
        // Copy processed audio to output
        for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
        {
            bufferToFill.buffer->copyFrom(ch, bufferToFill.startSample,
                tempBufferB, ch, 0, bufferToFill.numSamples);
        }
    }
}

void MainComponent::releaseResources()
{
    if (pluginHostA)
        pluginHostA->releaseResources();
    
    if (pluginHostB)
        pluginHostB->releaseResources();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Draw header
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("LiveSpice A/B Testing Suite", 
        getLocalBounds().removeFromTop(60), 
        juce::Justification::centred);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    
    // Header
    area.removeFromTop(60);
    
    // Status
    statusLabel.setBounds(area.removeFromTop(40).reduced(10, 5));
    
    area.removeFromTop(10);
    
    // Plugin loaders (side by side)
    auto loaderArea = area.removeFromTop(100);
    auto leftLoader = loaderArea.removeFromLeft(getWidth() / 2).reduced(10);
    auto rightLoader = loaderArea.reduced(10);
    
    loadPluginAButton.setBounds(leftLoader.removeFromTop(40));
    pluginALabel.setBounds(leftLoader);
    
    loadPluginBButton.setBounds(rightLoader.removeFromTop(40));
    pluginBLabel.setBounds(rightLoader);
    
    area.removeFromTop(20);
    
    // A/B Switch
    abSwitch->setBounds(area.removeFromTop(80).reduced(10));
    
    area.removeFromTop(10);
    
    // Control panel
    auto controlArea = area.removeFromLeft(getWidth() / 2).reduced(10);
    controlPanel->setBounds(controlArea);
    
    // Calibration panel
    auto calibrationArea = area.reduced(10);
    calibrationPanel->setBounds(calibrationArea);
}

void MainComponent::timerCallback()
{
    // Sync parameters from control panel to both plugins
    if (controlPanel && (isPluginALoaded || isPluginBLoaded))
    {
        auto params = controlPanel->getAllParameters();
        
        if (isPluginALoaded)
            pluginHostA->setParameters(params);
        
        if (isPluginBLoaded)
            pluginHostB->setParameters(params);
    }
}

void MainComponent::loadPluginA()
{
    auto chooser = std::make_shared<juce::FileChooser>("Select LiveSpice VST Plugin",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.dll;*.vst3");

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.exists() && pluginHostA->loadPlugin(file.getFullPathName()))
            {
                isPluginALoaded = true;
                pluginALabel.setText("Plugin A: " + file.getFileNameWithoutExtension(), 
                    juce::dontSendNotification);
                
                // Extract parameters and populate control panel
                auto params = pluginHostA->getParameterList();
                controlPanel->setParameterList(params);
                
                updateStatus();
                setupCalibration();
            }
            else if (file.exists())
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Error",
                    "Failed to load plugin A");
            }
        });
}

void MainComponent::loadPluginB()
{
    auto chooser = std::make_shared<juce::FileChooser>("Select Generated JUCE Plugin",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.dll;*.vst3;*.exe");

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.exists() && pluginHostB->loadPlugin(file.getFullPathName()))
            {
                isPluginBLoaded = true;
                pluginBLabel.setText("Plugin B: " + file.getFileNameWithoutExtension(),
                    juce::dontSendNotification);
                
                updateStatus();
                setupCalibration();
            }
            else if (file.exists())
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Error",
                    "Failed to load plugin B");
            }
        });
}

void MainComponent::updateStatus()
{
    juce::String status;
    
    if (isPluginALoaded && isPluginBLoaded)
    {
        status = "Active: ";
        status += usePluginA ? "Plugin A (LiveSpice VST)" : "Plugin B (Generated Plugin)";
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    }
    else if (isPluginALoaded || isPluginBLoaded)
    {
        status = "Load both plugins for comparison";
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    }
    else
    {
        status = "Ready - Load plugins to begin testing";
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    }
    
    statusLabel.setText(status, juce::dontSendNotification);
}

void MainComponent::setupCalibration()
{
    // Only setup calibration if both plugins are loaded
    if (!isPluginALoaded || !isPluginBLoaded || !calibrator)
        return;
    
    // Get common parameters from both plugins
    auto paramsA = pluginHostA->getParameterList();
    auto paramsB = pluginHostB->getParameterList();
    
    // Add common parameters to calibrator
    // This finds parameters that exist in both plugins
    for (const auto& paramA : paramsA)
    {
        for (const auto& paramB : paramsB)
        {
            // If parameter names match (case-insensitive), add to calibrator
            if (paramA.name.equalsIgnoreCase(paramB.name))
            {
                calibrator->addParameter(
                    paramA.id.toStdString(),
                    paramA.minValue,
                    paramA.maxValue,
                    paramA.defaultValue
                );
                break;
            }
        }
    }
    
    // Wire up parameter change callback
    calibrator->onParameterChange = [this](const std::string& paramName, float value) {
        // Apply to both plugins
        juce::String paramId(paramName);
        if (pluginHostA)
            pluginHostA->setParameter(paramId, value);
        if (pluginHostB)
            pluginHostB->setParameter(paramId, value);
    };
    
    // Wire up audio processing callbacks
    calibrator->onProcessReference = [this](juce::AudioBuffer<float>& buffer) {
        // Process through Plugin A (LiveSpice reference)
        if (pluginHostA)
        {
            juce::MidiBuffer midiBuffer;
            pluginHostA->processBlock(buffer, midiBuffer, 0, buffer.getNumSamples());
        }
    };
    
    calibrator->onProcessTarget = [this](juce::AudioBuffer<float>& buffer) {
        // Process through Plugin B (Digital target)
        if (pluginHostB)
        {
            juce::MidiBuffer midiBuffer;
            pluginHostB->processBlock(buffer, midiBuffer, 0, buffer.getNumSamples());
        }
    };
    
    // Configure calibration settings
    AutomatedCalibrator::CalibrationSettings settings;
    settings.maxIterations = 100;
    settings.convergenceThreshold = 1e-4f;
    settings.learningRate = 0.1f;
    settings.useAdaptiveStep = true;
    settings.testFrequency = 440.0f;  // A4 note
    settings.testAmplitude = 0.5f;
    settings.testDurationSamples = 8192;
    calibrator->setCalibrationSettings(settings);
}
