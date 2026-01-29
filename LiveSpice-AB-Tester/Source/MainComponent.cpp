/*
  ==============================================================================
    Main Component Implementation with Audio Device Management
  ==============================================================================
*/

#include "MainComponent.h"

// Initialize global logging
LogLevel g_currentLogLevel = LogLevel::INFO;  // Default to INFO level (minimal logging)
bool g_loggingEnabled = false;
juce::File g_logFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("LiveSpice_AB_Tester.log");

MainComponent::MainComponent()
{
    // Initialize audio device manager with proper error handling
    audioDeviceManager.initialiseWithDefaultDevices(2, 2);
    
    // Initialize abstraction layer components
    audioRouter = std::make_unique<AudioRouter>();
    paramSync = std::make_unique<ParameterSynchronizer>();

    // Initialize control panel (initially hidden, shown when configured)
    controlPanel = std::make_unique<ControlPanel>();
    addAndMakeVisible(*controlPanel);
    controlPanel->setVisible(false);

    // Setup parameter change callback via ParameterSynchronizer
    controlPanel->onParameterChanged = [this](const juce::String& paramId, float value) {
        paramSync->setParameter(paramId, value);
    };

    // Initialize A/B switch
    abSwitch = std::make_unique<ABSwitch>();
    addAndMakeVisible(*abSwitch);

    abSwitch->onSwitch = [this](bool useA) {
        audioRouter->setSelection(useA ? AudioRouter::ProcessorSelection::A 
                                        : AudioRouter::ProcessorSelection::B);
        updateStatus();
    };

    // Setup load buttons
    loadPluginAButton.onClick = [this] { loadPluginA(); };
    loadPluginBButton.onClick = [this] { loadPluginB(); };
    autoLoadButton.onClick = [this] { 
        logDebug("Auto-loading Schematic (A) vs Generated VST3 (B) for comparison", LogLevel::INFO);
        
        // Load Marshall Blues Breaker schematic to processor A
        processorA = ProcessorFactory::create("h:\\Live Spice DSP translation layer\\example pedals\\Marshall Blues Breaker.schx");
        isPluginALoaded = (processorA != nullptr && processorA->isLoaded());
        
        // Load Marshall Blues Breaker VST3 (generated) to processor B
        processorB = ProcessorFactory::createVST3(TEST_PLUGIN_B_PATH);
        isPluginBLoaded = (processorB != nullptr && processorB->isLoaded());
        
        if (isPluginALoaded && isPluginBLoaded)
        {
            logDebug("Successfully loaded both processors for comparison", LogLevel::INFO);
            
            // Create schematic editor UI for processor A
            if (processorA->getType() == "SchematicHost")
            {
                auto* schematicProc = dynamic_cast<SchematicProcessor*>(processorA.get());
                if (schematicProc)
                {
                    logDebug("Creating SchematicEditorComponent for processor A", LogLevel::INFO);
                    
                    // DIAGNOSTIC: Show alert to confirm we reach this code
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::InfoIcon,
                        "Plugin A Debug",
                        "About to create schematic editor window for: " + processorA->getName(),
                        "OK");
                    
                    // Create a window for the schematic editor
                    editorWindowA = std::make_unique<EditorWindow>(
                        "Plugin A - " + processorA->getName() + " (Schematic)",
                        juce::Colours::darkslategrey,
                        juce::DocumentWindow::allButtons);
                    
                    // Create the editor component and set it as window content
                    auto editorComponent = std::make_unique<SchematicEditorComponent>(schematicProc);
                    editorComponent->onParameterChanged = [this](const juce::String& paramId, float value) {
                        if (processorA)
                            processorA->setParameter(paramId, value);
                    };
                    
                    editorWindowA->setContentOwned(editorComponent.release(), true);
                    editorWindowA->setResizable(true, true);
                    editorWindowA->centreWithSize(450, 350);
                    editorWindowA->setVisible(true);
                    editorWindowA->toFront(true);
                    
                    logDebug("SchematicEditorComponent window created and shown", LogLevel::INFO);
                }
                else
                {
                    logDebug("Failed to cast processorA to SchematicProcessor", LogLevel::ERROR);
                }
            }
            else
            {
                logDebug("Processor A type is: " + processorA->getType() + ", expected SchematicHost", LogLevel::WARNING);
            }
            
            // Show VST3 editor for processor B if available
            if (processorB->getType() == "VST3")
            {
                auto* wrapperB = dynamic_cast<PluginHostWrapper*>(processorB.get());
                if (wrapperB && wrapperB->getPluginHost() && wrapperB->getPluginHost()->hasEditor())
                {
                    editorWindowB = std::make_unique<EditorWindow>(
                        "Plugin B - " + processorB->getName(),
                        juce::Colours::darkgrey,
                        juce::DocumentWindow::allButtons);
                    
                    auto editorB = wrapperB->getPluginHost()->createEditor();
                    if (editorB)
                    {
                        editorWindowB->setContentOwned(editorB, true);
                        editorWindowB->setResizable(true, true);
                        editorWindowB->centreWithSize(600, 500);
                        editorWindowB->setVisible(true);
                        editorWindowB->toFront(true);
                    }
                }
            }
            
            // Update parameter synchronizer
            paramSync->setProcessorA(processorA.get());
            paramSync->setProcessorB(processorB.get());
            
            // Prepare processors for playback
            processorA->prepareToPlay(audioDeviceManager.getAudioDeviceSetup().sampleRate, 
                                     audioDeviceManager.getAudioDeviceSetup().bufferSize);
            processorB->prepareToPlay(audioDeviceManager.getAudioDeviceSetup().sampleRate,
                                     audioDeviceManager.getAudioDeviceSetup().bufferSize);
            
            pluginALabel.setText("A: " + processorA->getName() + " [" + processorA->getType() + "]", 
                               juce::dontSendNotification);
            pluginBLabel.setText("B: " + processorB->getName() + " [" + processorB->getType() + "]", 
                               juce::dontSendNotification);
        }
        
        updateStatus();
        updateWorkflowPhase();
    };
    addAndMakeVisible(loadPluginAButton);
    addAndMakeVisible(loadPluginBButton);
    addAndMakeVisible(autoLoadButton);

    // Setup labels
    pluginALabel.setText("Plugin A: Not Loaded", juce::dontSendNotification);
    pluginBLabel.setText("Plugin B: Not Loaded", juce::dontSendNotification);
    pluginALabel.setJustificationType(juce::Justification::centred);
    pluginBLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pluginALabel);
    addAndMakeVisible(pluginBLabel);

    // Phase label
    phaseLabel.setText("Phase: PRE-CONFIGURED - Load both processors to begin", juce::dontSendNotification);
    phaseLabel.setJustificationType(juce::Justification::centred);
    phaseLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    phaseLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    addAndMakeVisible(phaseLabel);

    statusLabel.setText("Ready - Load processors to begin testing", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(statusLabel);

    // Audio status label
    audioStatusLabel.setText("Audio: Ready", juce::dontSendNotification);
    audioStatusLabel.setJustificationType(juce::Justification::centred);
    audioStatusLabel.setFont(juce::Font(12.0f));
    audioStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(audioStatusLabel);

    // Set window size
    setSize(1000, 800);

    // Setup audio
    setAudioChannels(2, 2);

    // Start timer for UI updates
    startTimer(100);
}

MainComponent::~MainComponent()
{
    // Finalize logging
    if (g_loggingEnabled)
    {
        logDebug("====================================================================", LogLevel::INFO);
        logDebug("Session Ended - Application Closed", LogLevel::INFO);
        logDebug("====================================================================", LogLevel::INFO);
        g_loggingEnabled = false;
    }
    
    shutdownAudio();
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Spacebar toggles between A and B
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        if (currentPhase == WorkflowPhase::Configured && audioRouter)
        {
            auto currentSelection = audioRouter->getSelection();
            auto newSelection = (currentSelection == AudioRouter::ProcessorSelection::A) 
                                ? AudioRouter::ProcessorSelection::B 
                                : AudioRouter::ProcessorSelection::A;
            
            audioRouter->setSelection(newSelection);
            updateStatus();
            
            if (abSwitch)
                abSwitch->setIsA(newSelection == AudioRouter::ProcessorSelection::A);
        }
        return true;  // Consume the key event
    }
    return false;
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return {"File"};
}

juce::PopupMenu MainComponent::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;

    if (menuName == "File")
    {
        menu.addItem(1, "Audio Settings");
        menu.addSeparator();
        
        // Logging level submenu
        juce::PopupMenu logMenu;
        logMenu.addItem(10, "Disabled", true, g_currentLogLevel == LogLevel::NONE);
        logMenu.addItem(11, "Errors Only", true, g_currentLogLevel == LogLevel::ERROR);
        logMenu.addItem(12, "Warnings & Errors", true, g_currentLogLevel == LogLevel::WARNING);
        logMenu.addItem(13, "Info Level (Default)", true, g_currentLogLevel == LogLevel::INFO);
        logMenu.addItem(14, "Debug (Verbose)", true, g_currentLogLevel == LogLevel::DEBUG);
        menu.addSubMenu("Logging Level", logMenu);
        
        menu.addItem(15, "View Log File");
        menu.addSeparator();
        menu.addItem(2, "Quit");
    }

    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    juce::ignoreUnused(topLevelMenuIndex);

    if (menuItemID == 1)
    {
        showAudioSettings();
    }
    else if (menuItemID == 10)
    {
        g_currentLogLevel = LogLevel::NONE;
    }
    else if (menuItemID == 11)
    {
        g_currentLogLevel = LogLevel::ERROR;
    }
    else if (menuItemID == 12)
    {
        g_currentLogLevel = LogLevel::WARNING;
    }
    else if (menuItemID == 13)
    {
        g_currentLogLevel = LogLevel::INFO;
    }
    else if (menuItemID == 14)
    {
        g_currentLogLevel = LogLevel::DEBUG;
    }
    else if (menuItemID == 15)
    {
        // Open log file with default app
        g_logFile.startAsProcess();
    }
    else if (menuItemID == 2)
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
}

void MainComponent::showAudioSettings()
{
    // Close previous window if open
    audioSettingsWindow = nullptr;
    
    // Create a custom window that will update settings on close
    class AudioSettingsWindow : public juce::DocumentWindow
    {
    public:
        AudioSettingsWindow(MainComponent* owner)
            : juce::DocumentWindow("Audio Settings",
                juce::Colours::darkgrey,
                juce::DocumentWindow::allButtons),
              mainComponent(owner)
        {
        }
        
        void closeButtonPressed() override
        {
            // Update the audio status label when window closes
            if (mainComponent && mainComponent->audioDeviceManager.getCurrentAudioDevice())
            {
                auto device = mainComponent->audioDeviceManager.getCurrentAudioDevice();
                mainComponent->audioStatusLabel.setText(
                    juce::String("Audio: ") + device->getName() +
                    " (" + juce::String(device->getCurrentSampleRate()) + " Hz)",
                    juce::dontSendNotification);
            }
            setVisible(false);
        }
        
    private:
        MainComponent* mainComponent;
    };
    
    audioSettingsWindow = std::make_unique<AudioSettingsWindow>(this);

    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        audioDeviceManager, 0, 2, 0, 2, false, false, true, false);

    audioSettingsWindow->setContentOwned(selector.release(), true);
    audioSettingsWindow->centreWithSize(500, 400);
    audioSettingsWindow->setResizable(true, true);
    audioSettingsWindow->setVisible(true);
    audioSettingsWindow->toFront(true);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    // Store sample rate for later use
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;
    
    // Allocate input capture buffer
    inputBuffer.setSize(2, samplesPerBlockExpected);
    
    // Prepare audio router
    if (audioRouter)
        audioRouter->prepareToPlay(sampleRate, samplesPerBlockExpected);
    
    // Prepare processors
    if (processorA && processorA->isLoaded())
        processorA->prepareToPlay(sampleRate, samplesPerBlockExpected);
    
    if (processorB && processorB->isLoaded())
        processorB->prepareToPlay(sampleRate, samplesPerBlockExpected);

    audioStatusLabel.setText(
        juce::String("Audio: ") + 
        juce::String(sampleRate, 0) + " Hz, " +
        juce::String(samplesPerBlockExpected) + " samples",
        juce::dontSendNotification);
    
    logDebug("prepareToPlay: " + juce::String(sampleRate) + " Hz, " + 
             juce::String(samplesPerBlockExpected) + " samples", LogLevel::DEBUG);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Only process if we're in configured phase
    if (currentPhase != WorkflowPhase::Configured)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    // Get audio buffer - this is BOTH input AND output with AudioAppComponent
    auto& buffer = *bufferToFill.buffer;
    
    // Important: When using AudioAppComponent with 2 input and 2 output channels,
    // JUCE automatically reads input and places it in the first 2 channels,
    // so we need to copy it before processing
    
    // Copy current input to our input buffer for reference
    for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), inputBuffer.getNumChannels()); ++channel)
    {
        const auto* readPtr = buffer.getReadPointer(channel);
        auto* writePtr = inputBuffer.getWritePointer(channel);
        std::copy(readPtr, readPtr + bufferToFill.numSamples, writePtr);
    }
    
    // Process through audio router (routes to A or B based on selection)
    if (audioRouter)
    {
        audioRouter->processBlock(buffer, midiBuffer, 
                                 processorA.get(), processorB.get());
    }
    
    // Log periodically for debugging
    static int audioCounter = 0;
    if (audioCounter++ % 1000 == 0)
    {
        auto selection = audioRouter ? audioRouter->getSelection() : AudioRouter::ProcessorSelection::A;
        
        // Check if we're getting input
        float maxInputLevel = 0.0f;
        for (int channel = 0; channel < inputBuffer.getNumChannels(); ++channel)
        {
            auto maxValue = inputBuffer.getMagnitude(channel, 0, inputBuffer.getNumSamples());
            maxInputLevel = juce::jmax(maxInputLevel, maxValue);
        }
        
        logDebug("Audio: Processing via " + 
                juce::String(selection == AudioRouter::ProcessorSelection::A ? "A" : "B") +
                " - " + juce::String(bufferToFill.numSamples) + " samples, InputLevel: " +
                juce::String(maxInputLevel, 3), 
                LogLevel::DEBUG);
    }
}

void MainComponent::releaseResources()
{
    if (audioRouter)
        audioRouter->releaseResources();
        
    if (processorA && processorA->isLoaded())
        processorA->releaseResources();
    
    if (processorB && processorB->isLoaded())
        processorB->releaseResources();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    
    // Phase indicator at top
    phaseLabel.setBounds(area.removeFromTop(40).reduced(10, 5));
    
    // Audio status
    audioStatusLabel.setBounds(area.removeFromTop(25).reduced(10, 2));
    
    area.removeFromTop(10);
    
    // Main status
    statusLabel.setBounds(area.removeFromTop(40).reduced(10, 5));
    
    area.removeFromTop(10);
    
    // Plugin loaders (side by side) + Auto-load button
    auto loaderArea = area.removeFromTop(140);
    auto leftLoader = loaderArea.removeFromLeft(getWidth() / 2).reduced(10);
    auto rightLoader = loaderArea.removeFromRight(getWidth() / 2).reduced(10);
    auto centerLoader = loaderArea.reduced(10);
    
    loadPluginAButton.setBounds(leftLoader.removeFromTop(40));
    pluginALabel.setBounds(leftLoader.removeFromTop(30));
    
    loadPluginBButton.setBounds(rightLoader.removeFromTop(40));
    pluginBLabel.setBounds(rightLoader.removeFromTop(30));
    
    autoLoadButton.setBounds(centerLoader.removeFromTop(40).reduced(100, 5));
    
    area.removeFromTop(20);
    
    // A/B Switch (only visible when configured)
    if (currentPhase == WorkflowPhase::Configured)
    {
        abSwitch->setBounds(area.removeFromTop(80).reduced(10));
        abSwitch->setVisible(true);
    }
    else
    {
        abSwitch->setVisible(false);
    }
    
    area.removeFromTop(10);
    
    // Control panel (only visible when configured)
    if (currentPhase == WorkflowPhase::Configured)
    {
        controlPanel->setBounds(area.reduced(10));
        controlPanel->setVisible(true);
    }
    else
    {
        controlPanel->setVisible(false);
    }
}

void MainComponent::timerCallback()
{
    updateWorkflowPhase();
    
    // Start logging when entering Configured phase
    if (currentPhase == WorkflowPhase::Configured && !g_loggingEnabled)
    {
        g_loggingEnabled = true;
        // Clear old log file and start fresh
        g_logFile.deleteFile();
        logDebug("====================================================================", LogLevel::INFO);
        logDebug("LiveSpice A/B Testing Suite - Session Started", LogLevel::INFO);
        logDebug("====================================================================", LogLevel::INFO);
        logDebug("Logging Level: " + juce::String((int)g_currentLogLevel), LogLevel::INFO);
    }
    
    // Only sync control panel once, when transitioning to Configured phase
    static bool controlPanelSynced = false;
    if (currentPhase == WorkflowPhase::Configured && !controlPanelSynced)
    {
        logDebug("MainComponent::timerCallback - Syncing control panel for first time", LogLevel::DEBUG);
        syncControlPanel();
        controlPanelSynced = true;
    }
    else if (currentPhase == WorkflowPhase::PreConfigured)
    {
        controlPanelSynced = false;
        // Stop logging when returning to PreConfigured phase
        if (g_loggingEnabled)
        {
            logDebug("Returning to PRE-CONFIGURED phase - logging paused", LogLevel::INFO);
            g_loggingEnabled = false;
        }
    }
}

void MainComponent::updateWorkflowPhase()
{
    WorkflowPhase newPhase = (isPluginALoaded && isPluginBLoaded) 
        ? WorkflowPhase::Configured 
        : WorkflowPhase::PreConfigured;

    if (newPhase != currentPhase)
    {
        currentPhase = newPhase;
        resized(); // Trigger layout update
        
        if (currentPhase == WorkflowPhase::Configured)
        {
            phaseLabel.setText("Phase: CONFIGURED - Both plugins loaded, ready for A/B comparison", 
                juce::dontSendNotification);
            phaseLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
            syncControlPanel();
        }
        else
        {
            phaseLabel.setText("Phase: PRE-CONFIGURED - Load both plugins to begin", 
                juce::dontSendNotification);
            phaseLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
        }
    }
}

void MainComponent::syncControlPanel()
{
    if (!paramSync)
        return;
        
    logDebug("syncControlPanel: Getting merged parameters", LogLevel::DEBUG);
    auto params = paramSync->getMergedParameters();
    
    // Check if parameters match between A and B
    if (processorA && processorA->isLoaded() && processorB && processorB->isLoaded())
    {
        auto paramsA = processorA->getParameters();
        auto paramsB = processorB->getParameters();
        
        if (paramsA.size() != paramsB.size())
        {
            logDebug("WARNING: Parameter mismatch - A has " + juce::String(paramsA.size()) + 
                     " parameters, B has " + juce::String(paramsB.size()) + " parameters", 
                     LogLevel::WARNING);
            
            // Show warning in status
            juce::String warning = "⚠ Parameter mismatch: A(" + juce::String(paramsA.size()) + 
                                  " params) vs B(" + juce::String(paramsB.size()) + " params)";
            statusLabel.setText(warning, juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
        }
    }
    
    // Convert IAudioProcessor::ParameterInfo to ControlPanel's ParameterInfo
    std::vector<::ParameterInfo> cpParams;
    for (const auto& param : params)
    {
        ::ParameterInfo cpParam;
        cpParam.id = param.id;
        cpParam.name = param.name;
        cpParam.value = param.currentValue;
        cpParam.minValue = 0.0f;
        cpParam.maxValue = 1.0f;
        cpParam.defaultValue = param.defaultValue;
        cpParams.push_back(cpParam);
    }
    
    logDebug("syncControlPanel: Passing " + juce::String(cpParams.size()) + 
             " parameters to control panel", LogLevel::DEBUG);
    controlPanel->setParameterList(cpParams);
}

void MainComponent::loadPluginA()
{
    auto chooser = std::make_shared<juce::FileChooser>("Select LiveSpice Schematic (.schx)",
        juce::File("h:\\Live Spice DSP translation layer\\example pedals\\"),
        "*.schx");

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.exists())
            {
                processorA = ProcessorFactory::createNativeDSP(file.getFullPathName());
                
                if (processorA && processorA->isLoaded())
                {
                    logDebug("Successfully loaded Schematic A: " + processorA->getName(), LogLevel::INFO);
                    isPluginALoaded = true;
                    pluginALabel.setText("A: " + processorA->getName() + " [" + processorA->getType() + "]", 
                        juce::dontSendNotification);
                    
                    updateStatus();
                    updateWorkflowPhase();
                    syncControlPanel();
                }
                else
                {
                    logDebug("Failed to load schematic: " + file.getFullPathName(), LogLevel::ERROR);
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Failed to Load Schematic",
                        "Could not load the schematic file. Check the log for details.");
                }
            }
        });
}

void MainComponent::loadPluginB()
{
    auto chooser = std::make_shared<juce::FileChooser>("Select VST3 Plugin for B",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.vst3");

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.exists())
            {
                processorB = ProcessorFactory::createVST3(file.getFullPathName());
                
                if (processorB && processorB->isLoaded())
                {
                    logDebug("Successfully loaded Processor B: " + processorB->getName(), LogLevel::INFO);
                    isPluginBLoaded = true;
                    pluginBLabel.setText("B: " + processorB->getName() + " [" + processorB->getType() + "]", 
                        juce::dontSendNotification);
                    
                    // Show VST3 editor if available
                    if (processorB->getType() == "VST3")
                    {
                        auto* wrapper = dynamic_cast<PluginHostWrapper*>(processorB.get());
                        if (wrapper && wrapper->getPluginHost() && wrapper->getPluginHost()->hasEditor())
                        {
                            editorWindowB = std::make_unique<EditorWindow>(
                                "Plugin B - " + processorB->getName(),
                                juce::Colours::darkgrey,
                                juce::DocumentWindow::allButtons);
                            
                            auto editor = wrapper->getPluginHost()->createEditor();
                            if (editor)
                            {
                                editorWindowB->setContentOwned(editor, true);
                                editorWindowB->setResizable(true, true);
                                editorWindowB->centreWithSize(400, 300);
                                editorWindowB->setVisible(true);
                                editorWindowB->toFront(true);
                            }
                        }
                    }
                    
                    // Update parameter synchronizer
                    paramSync->setProcessorB(processorB.get());
                    
                    // Prepare for playback
                    processorB->prepareToPlay(audioDeviceManager.getAudioDeviceSetup().sampleRate,
                                             audioDeviceManager.getAudioDeviceSetup().bufferSize);
                    
                    updateStatus();
                    updateWorkflowPhase();
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Error",
                        "Failed to load VST3 plugin");
                }
            }
        });
}

void MainComponent::updateStatus()
{
    juce::String status;
    
    if (isPluginALoaded && isPluginBLoaded)
    {
        auto selection = audioRouter->getSelection();
        status = "Active: ";
        status += (selection == AudioRouter::ProcessorSelection::A) 
                  ? ("A - " + processorA->getName() + " [" + processorA->getType() + "]")
                  : ("B - " + processorB->getName() + " [" + processorB->getType() + "]");
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    }
    else if (isPluginALoaded || isPluginBLoaded)
    {
        status = "Load both processors for comparison";
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    }
    else
    {
        status = "Ready - Load processors to begin testing";
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    }
    
    statusLabel.setText(status, juce::dontSendNotification);
}
