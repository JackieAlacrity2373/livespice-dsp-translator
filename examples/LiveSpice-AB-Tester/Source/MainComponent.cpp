/*
  ==============================================================================
    Main Component Implementation with Audio Device Management
  ==============================================================================
*/

#include "MainComponent.h"
#include <cmath>

namespace
{
    bool isDebugAudioEnabled()
    {
        if (auto* app = juce::JUCEApplication::getInstance())
        {
            const auto args = app->getCommandLineParameters();
            return args.containsIgnoreCase("--debug-audio") || args.containsIgnoreCase("--debug");
        }
        return false;
    }

    const bool kDebugAudioEnabled = isDebugAudioEnabled();
}

// Initialize global logging
LogLevel g_currentLogLevel = kDebugAudioEnabled ? LogLevel::DEBUG : LogLevel::INFO;
bool g_loggingEnabled = true;
juce::File g_logFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("LiveSpice_AB_Tester.log");

MainComponent::MainComponent()
{
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
        
        // Update schematic editor UI if it exists
        if (schematicEditor)
        {
            schematicEditor->updateParameterUI(paramId, value);
        }
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
        logDebug("Starting to load Processor A (Schematic)...", LogLevel::INFO);
        processorA = ProcessorFactory::create("h:\\Live Spice DSP translation layer\\example pedals\\Marshall Blues Breaker.schx");
        isPluginALoaded = (processorA != nullptr && processorA->isLoaded());
        
        if (processorA)
        {
            logDebug("Processor A created: Type=" + processorA->getType() + 
                    ", Name=" + processorA->getName() + 
                    ", Loaded=" + juce::String(isPluginALoaded ? "YES" : "NO"), 
                    LogLevel::INFO);
        }
        else
        {
            logDebug("ERROR: Processor A is NULL!", LogLevel::ERROR);
        }
        
        // Load Marshall Blues Breaker VST3 (generated) to processor B
        logDebug("Starting to load Processor B (VST3)...", LogLevel::INFO);
        processorB = ProcessorFactory::createVST3(TEST_PLUGIN_B_PATH);
        isPluginBLoaded = (processorB != nullptr && processorB->isLoaded());
        
        if (processorB)
        {
            logDebug("Processor B created: Type=" + processorB->getType() + 
                    ", Name=" + processorB->getName() + 
                    ", Loaded=" + juce::String(isPluginBLoaded ? "YES" : "NO"), 
                    LogLevel::INFO);
        }
        else
        {
            logDebug("ERROR: Processor B is NULL!", LogLevel::ERROR);
        }
        
        if (isPluginALoaded && isPluginBLoaded)
        {
            logDebug("Successfully loaded both processors for comparison", LogLevel::INFO);
            
            // Create schematic editor UI for processor A
            logDebug("Checking if Processor A type is SchematicHost...", LogLevel::INFO);
            if (processorA->getType() == "SchematicHost")
            {
                logDebug("Type matches SchematicHost, attempting dynamic_cast...", LogLevel::INFO);
                auto* schematicProc = dynamic_cast<SchematicProcessor*>(processorA.get());
                if (schematicProc)
                {
                    logDebug("Successfully cast to SchematicProcessor, creating editor window...", LogLevel::INFO);
                    
                    try
                    {
                        // Create a window for the schematic editor
                        editorWindowA = std::make_unique<EditorWindow>(
                            "Plugin A - " + processorA->getName() + " (Schematic)",
                            juce::Colours::darkslategrey,
                            juce::DocumentWindow::allButtons);
                        
                        logDebug("EditorWindow created, now creating SchematicEditorComponent...", LogLevel::INFO);
                        
                        // Create the editor component and set it as window content
                        auto editorComponent = std::make_unique<SchematicEditorComponent>(schematicProc);
                        
                        logDebug("SchematicEditorComponent created, setting up callback...", LogLevel::INFO);
                        
                        // Store pointer for UI updates
                        schematicEditor = editorComponent.get();
                        
                        editorComponent->onParameterChanged = [this](const juce::String& paramId, float value) {
                            if (processorA)
                            {
                                processorA->setParameter(paramId, value);
                                // Also sync to processor B via parameter synchronizer
                                paramSync->setParameter(paramId, value, false);  // false = don't notify again
                            }
                        };
                        
                        logDebug("Setting content to window...", LogLevel::INFO);
                        editorWindowA->setContentOwned(editorComponent.release(), true);
                        editorWindowA->setResizable(true, true);
                        editorWindowA->centreWithSize(500, 450);  // Increased height to fit 3 controls
                        
                        logDebug("Making window visible...", LogLevel::INFO);
                        editorWindowA->setVisible(true);
                        editorWindowA->toFront(true);
                        
                        logDebug("SchematicEditorComponent window successfully created and shown!", LogLevel::INFO);
                    }
                    catch (const std::exception& e)
                    {
                        logDebug("EXCEPTION creating schematic editor: " + juce::String(e.what()), LogLevel::ERROR);
                    }
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
            processorA->prepareToPlay(deviceManager.getAudioDeviceSetup().sampleRate, 
                                     deviceManager.getAudioDeviceSetup().bufferSize);
            processorB->prepareToPlay(deviceManager.getAudioDeviceSetup().sampleRate,
                                     deviceManager.getAudioDeviceSetup().bufferSize);
            
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

    // Audio device label - show which output device is being used
    auto* currentDevice = deviceManager.getCurrentAudioDevice();
    juce::String deviceName = currentDevice ? currentDevice->getName() : "No device";
    audioDeviceLabel.setText("Output Device: " + deviceName, juce::dontSendNotification);
    audioDeviceLabel.setJustificationType(juce::Justification::centred);
    audioDeviceLabel.setFont(juce::Font(11.0f));
    audioDeviceLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    addAndMakeVisible(audioDeviceLabel);

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
            auto& dm = mainComponent->deviceManager;
            if (dm.getCurrentAudioDevice())
            {
                auto device = dm.getCurrentAudioDevice();
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

    // Use AudioAppComponent's device manager
    auto& dm = deviceManager;
    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        dm, 0, 2, 0, 2, false, false, true, false);

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
    
    auto* currentDevice = deviceManager.getCurrentAudioDevice();
    if (!currentDevice)
    {
        logDebug("ERROR in prepareToPlay: No audio device! Audio callback may not work!", LogLevel::ERROR);
        return;
    }

    if (kDebugAudioEnabled)
    {
        logDebug("=== AUDIO DEVICE INFO ===", LogLevel::INFO);
        logDebug("Device: " + currentDevice->getName(), LogLevel::INFO);
        logDebug("Input channels: " + juce::String(currentDevice->getInputChannelNames().size()), LogLevel::INFO);
        logDebug("Output channels: " + juce::String(currentDevice->getOutputChannelNames().size()), LogLevel::INFO);
        
        auto inputChannels = currentDevice->getInputChannelNames();
        for (int i = 0; i < inputChannels.size(); ++i)
        {
            logDebug("  Input " + juce::String(i+1) + ": " + inputChannels[i], LogLevel::DEBUG);
        }
        
        auto outputChannels = currentDevice->getOutputChannelNames();
        for (int i = 0; i < outputChannels.size(); ++i)
        {
            logDebug("  Output " + juce::String(i+1) + ": " + outputChannels[i], LogLevel::DEBUG);
        }
    }
    
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
    
    if (kDebugAudioEnabled)
    {
        logDebug("prepareToPlay: " + juce::String(sampleRate) + " Hz, " + 
                 juce::String(samplesPerBlockExpected) + " samples", LogLevel::DEBUG);
    }
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (kDebugAudioEnabled)
    {
        // DIAGNOSTIC: Check if this callback is being called
        static int callCount = 0;
        if (callCount++ == 0)
        {
            juce::Logger::writeToLog("=== getNextAudioBlock CALLBACK IS BEING CALLED ===");
            logDebug("=== Audio callback started! ===", LogLevel::INFO);
        }
    }
    
    // Get audio buffer - this is BOTH input AND output with AudioAppComponent
    auto& buffer = *bufferToFill.buffer;
    
    // ALWAYS track input level regardless of phase - this shows if Focusrite is working
    float inputLevelBefore = 0.0f;
    float minSample = 1.0f, maxSample = -1.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        inputLevelBefore = juce::jmax(inputLevelBefore, buffer.getMagnitude(ch, bufferToFill.startSample, bufferToFill.numSamples));
        
        if (kDebugAudioEnabled)
        {
            // Track min/max for diagnostics
            auto* chData = buffer.getReadPointer(ch, bufferToFill.startSample);
            for (int i = 0; i < bufferToFill.numSamples; ++i)
            {
                minSample = juce::jmin(minSample, chData[i]);
                maxSample = juce::jmax(maxSample, chData[i]);
            }
        }
    }
    inputLevel.store(inputLevelBefore);  // Update meter even if not configured
    
    if (kDebugAudioEnabled)
    {
        // Log input level periodically to help debug
        static int levelLogCounter = 0;
        if (levelLogCounter++ % 500 == 0)
        {
            auto* device = deviceManager.getCurrentAudioDevice();
            juce::String deviceName = device ? device->getName() : "NONE";
            logDebug("INPUT: Level=" + juce::String(inputLevelBefore, 6) + 
                    " Min=" + juce::String(minSample, 6) + 
                    " Max=" + juce::String(maxSample, 6) + 
                    " Device=" + deviceName, 
                    LogLevel::DEBUG);
        }
        
        // Alert if input changes significantly
        static float lastInputLevel = 0.0f;
        if (std::abs(inputLevelBefore - lastInputLevel) > 0.001f && levelLogCounter % 100 == 0)
        {
            logDebug("INPUT CHANGED: " + juce::String(lastInputLevel, 4) + " -> " + juce::String(inputLevelBefore, 4), LogLevel::INFO);
        }
        lastInputLevel = inputLevelBefore;
    }
    
    // If not configured, pass audio through without processing (don't silence!)
    // This allows you to hear the input from the Focusrite even during setup
    if (currentPhase != WorkflowPhase::Configured)
    {
        // Update all meters to show input passthrough
        processedLevel.store(inputLevelBefore);
        outputLevel.store(inputLevelBefore);
        deviceOutputLevel.store(inputLevelBefore);
        
        if (kDebugAudioEnabled)
        {
            // Repaint meters
            static int repaintCounter = 0;
            if (repaintCounter++ % 5 == 0)
                repaint(10, 95, getWidth() - 20, 60);
        }
        
        return;  // Pass through unchanged
    }
    
    if (kDebugAudioEnabled)
    {
        // DIAGNOSTIC: Generate a 1kHz test tone every 10 seconds to verify output works
        static int testToneCounter = 0;
        if (testToneCounter++ > 480000)  // After ~10 seconds at 48kHz
        {
            testToneCounter = 0;
            logDebug("=== GENERATING TEST TONE - IF YOU HEAR A BEEP, AUDIO OUTPUT IS WORKING ===", LogLevel::INFO);
            
            // Generate a 1kHz sine wave test tone
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* samples = buffer.getWritePointer(ch);
                for (int i = 0; i < bufferToFill.numSamples; ++i)
                {
                    // 1kHz sine tone, 0.5 amplitude
                    static int sampleIdx = 0;
                    samples[i] = 0.5f * sinf(2.0f * 3.14159f * 1000.0f * sampleIdx / 48000.0f);
                    sampleIdx++;
                }
            }
            return;
        }
        
        // DIAGNOSTIC: Log buffer configuration
        static int diagCounter = 0;
        if (diagCounter++ % 1000 == 0)
        {
            logDebug("BUFFER STATE: NumChannels=" + juce::String(buffer.getNumChannels()) + 
                    ", NumSamples=" + juce::String(bufferToFill.numSamples) + 
                    ", StartSample=" + juce::String(bufferToFill.startSample), LogLevel::DEBUG);
        }
    }
    
    // Copy current input to our input buffer for reference
    for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), inputBuffer.getNumChannels()); ++channel)
    {
        const auto* readPtr = buffer.getReadPointer(channel);
        auto* writePtr = inputBuffer.getWritePointer(channel);
        std::copy(readPtr, readPtr + bufferToFill.numSamples, writePtr);
    }
    
    // Input level already captured at start of function - use it for processing display
    processedLevel.store(inputLevel.load());  // Will update after processing
    
    // Process through audio router (routes to A or B based on selection)
    if (audioRouter)
    {
        audioRouter->processBlock(buffer, midiBuffer, 
                                 processorA.get(), processorB.get());
    }
    
    if (kDebugAudioEnabled)
    {
        // EMERGENCY TEST: Amplify output by 2x (debug only)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* channelData = buffer.getWritePointer(ch, bufferToFill.startSample);
            for (int i = 0; i < bufferToFill.numSamples; ++i)
            {
                channelData[i] *= 2.0f;
            }
        }
    }
    
    // CRITICAL DEBUG: Check output level AFTER processing and amplification
    float outputLevelAfter = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        outputLevelAfter = juce::jmax(outputLevelAfter, buffer.getMagnitude(ch, bufferToFill.startSample, bufferToFill.numSamples));
    }
    
    // Update level displays for UI - show complete audio chain
    processedLevel.store(outputLevelAfter);   // What came out of DSP processing
    outputLevel.store(outputLevelAfter);      // What's in the buffer now
    deviceOutputLevel.store(outputLevelAfter); // What will be sent to device (same as buffer for now)
    
    if (kDebugAudioEnabled)
    {
        // Repaint level meters every 5 audio blocks for smooth updates
        static int repaintCounter = 0;
        if (repaintCounter++ % 5 == 0)
            repaint(10, 95, getWidth() - 20, 60);  // Repaint just the meter area at the top
        
        // DIAGNOSTIC: Check if buffer samples are actually written
        static int diagCounter = 0;
        if (diagCounter++ % 1000 == 0)
        {
            float sample0_ch0 = buffer.getNumChannels() > 0 ? *buffer.getReadPointer(0) : 0.0f;
            float sample0_ch1 = buffer.getNumChannels() > 1 ? *buffer.getReadPointer(1) : 0.0f;
            logDebug("BUFFER SAMPLES: Ch0[0]=" + juce::String(sample0_ch0, 6) + 
                    ", Ch1[0]=" + juce::String(sample0_ch1, 6), LogLevel::DEBUG);
        }
        
        // Log periodically for debugging
        static int audioCounter = 0;
        if (audioCounter++ % 500 == 0)  // More frequent logging
        {
            auto selection = audioRouter ? audioRouter->getSelection() : AudioRouter::ProcessorSelection::A;
            
            logDebug("AUDIO FLOW: Input=" + juce::String(inputLevelBefore, 4) + 
                    " -> [" + juce::String(selection == AudioRouter::ProcessorSelection::A ? "A" : "B") + "] -> " +
                    "Output=" + juce::String(outputLevelAfter, 4) + 
                    " (Channels: " + juce::String(buffer.getNumChannels()) + 
                    ", Samples: " + juce::String(bufferToFill.numSamples) + ")", 
                    LogLevel::INFO);
        }
        
        // EMERGENCY DIAGNOSTIC: Force output to ensure buffer is being written
        // If startSample is non-zero, we need to handle it properly
        if (bufferToFill.startSample != 0)
        {
            logDebug("WARNING: startSample=" + juce::String(bufferToFill.startSample) + " (non-zero!)", LogLevel::WARNING);
        }
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
    paintLevelMeters(g);
}

void MainComponent::paintLevelMeters(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Position meters at the TOP after phase/device labels, before status
    // Calculate position: 40 (phase) + 20 (device) + 25 (audio status) + 10 (spacing) = 95
    auto meterArea = juce::Rectangle<int>(10, 95, bounds.getWidth() - 20, 60);
    
    // Divide into 4 sections for complete audio chain
    int meterWidth = meterArea.getWidth() / 4;
    
    auto drawMeter = [&g](juce::Rectangle<int> area, float level, const juce::String& label, juce::Colour colour)
    {
        // Draw background
        g.setColour(juce::Colours::black);
        g.fillRect(area);
        
        // Draw border
        g.setColour(juce::Colours::white);
        g.drawRect(area, 2);
        
        // Draw level bar (fill proportional to level, max 1.0)
        int barWidth = (int)(area.getWidth() * juce::jlimit(0.0f, 1.0f, level * 10.0f));  // Scale 10x for visibility
        juce::Rectangle<int> barArea = area.reduced(4);
        barArea.setWidth(barWidth);
        
        g.setColour(colour);
        g.fillRect(barArea);
        
        // Draw label and value
        g.setColour(juce::Colours::white);
        g.setFont(12.0f);
        g.drawFittedText(label, area.removeFromTop(18), juce::Justification::centred, 1);
        
        // Draw numeric value
        g.setFont(11.0f);
        g.drawFittedText(juce::String(level, 4), area, juce::Justification::centredBottom, 1);
    };
    
    // Draw four meters showing complete audio chain
    drawMeter(meterArea.removeFromLeft(meterWidth).reduced(3), inputLevel, "1. INPUT", juce::Colours::cyan);
    drawMeter(meterArea.removeFromLeft(meterWidth).reduced(3), processedLevel, "2. DSP OUT", juce::Colours::yellow);
    drawMeter(meterArea.removeFromLeft(meterWidth).reduced(3), outputLevel, "3. BUFFER", juce::Colours::orange);
    drawMeter(meterArea.removeFromLeft(meterWidth).reduced(3), deviceOutputLevel, "4. DEVICE", juce::Colours::lime);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    
    // Phase indicator at top
    phaseLabel.setBounds(area.removeFromTop(40).reduced(10, 5));
    
    // Audio device display
    audioDeviceLabel.setBounds(area.removeFromTop(20).reduced(10, 2));
    
    // Audio status
    audioStatusLabel.setBounds(area.removeFromTop(25).reduced(10, 2));
    
    area.removeFromTop(10);
    
    // Reserve space for level meters (painted in paint method)
    area.removeFromTop(60);  // Height of level meters
    
    area.removeFromTop(10);
    
    // Main status
    statusLabel.setBounds(area.removeFromTop(40).reduced(10, 5));
    
    area.removeFromTop(10);
    
    // Plugin loaders (side by side)
    auto loaderArea = area.removeFromTop(80);
    auto leftLoader = loaderArea.removeFromLeft(getWidth() / 2).reduced(10);
    auto rightLoader = loaderArea.reduced(10);
    
    loadPluginAButton.setBounds(leftLoader.removeFromTop(40));
    pluginALabel.setBounds(leftLoader.removeFromTop(30));
    
    loadPluginBButton.setBounds(rightLoader.removeFromTop(40));
    pluginBLabel.setBounds(rightLoader.removeFromTop(30));
    
    // Auto-load button on its own row
    auto autoLoadArea = area.removeFromTop(50).reduced(10, 5);
    autoLoadButton.setBounds(autoLoadArea);
    
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
            juce::String warning = "âš  Parameter mismatch: A(" + juce::String(paramsA.size()) + 
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
                    processorB->prepareToPlay(deviceManager.getAudioDeviceSetup().sampleRate,
                                             deviceManager.getAudioDeviceSetup().bufferSize);
                    
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











