/*
  ==============================================================================
    Main Component - A/B Testing Interface
  ==============================================================================
*/

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "Logging.h"
#include "IAudioProcessor.h"
#include "AudioRouter.h"
#include "ParameterSynchronizer.h"
#include "ProcessorFactory.h"
#include "ControlPanel.h"
#include "ABSwitch.h"
#include "SchematicEditorComponent.h"

class MainComponent : public juce::AudioAppComponent,
                      public juce::Timer,
                      public juce::MenuBarModel
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;

    // MenuBarModel implementation
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    // Keyboard input for spacebar
    bool keyPressed(const juce::KeyPress& key) override;

private:
    // Workflow phases
    enum class WorkflowPhase
    {
        PreConfigured,  // Plugins being loaded
        Configured      // Both plugins loaded, ready to compare
    };

    WorkflowPhase currentPhase{WorkflowPhase::PreConfigured};

    // Audio device manager
    juce::AudioDeviceManager audioDeviceManager;
    std::unique_ptr<juce::DocumentWindow> audioSettingsWindow;
    
    // Hardcoded paths for testing (VST3 plugins)
    const juce::String TEST_PLUGIN_A_PATH = "H:\\Live Spice DSP translation layer\\JUCE - MXR Distortion +\\_ The VST is in here\\MXR Distortion +.vst3";
    const juce::String TEST_PLUGIN_B_PATH = "H:\\Live Spice DSP translation layer\\JUCE - Marshall Blues Breaker\\_ The VST is in here\\Marshall Blues Breaker.vst3";

    // Processors via abstraction layer (A = Schematic or VST, B = VST)
    std::unique_ptr<IAudioProcessor> processorA;
    std::unique_ptr<IAudioProcessor> processorB;

    // Audio routing and synchronization
    std::unique_ptr<AudioRouter> audioRouter;
    std::unique_ptr<ParameterSynchronizer> paramSync;

    // UI Components
    std::unique_ptr<ControlPanel> controlPanel;
    std::unique_ptr<ABSwitch> abSwitch;

    // Plugin selection
    juce::TextButton loadPluginAButton{"Load Schematic (A)"};
    juce::TextButton loadPluginBButton{"Load VST3 (B)"};
    juce::TextButton autoLoadButton{"Auto-Load Test (Schematic vs VST3)"};
    juce::Label pluginALabel;
    juce::Label pluginBLabel;

    // Plugin editor windows
    class EditorWindow : public juce::DocumentWindow
    {
    public:
        EditorWindow(const juce::String& name, juce::Colour backgroundColour, int buttons)
            : juce::DocumentWindow(name, backgroundColour, buttons, true) {}
        void closeButtonPressed() override { setVisible(false); }
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorWindow)
    };

    std::unique_ptr<EditorWindow> editorWindowA;
    std::unique_ptr<EditorWindow> editorWindowB;

    // Status display
    juce::Label statusLabel;
    juce::Label phaseLabel;
    juce::Label audioStatusLabel;

    // Audio buffers for processing
    juce::MidiBuffer midiBuffer;
    juce::AudioBuffer<float> inputBuffer;  // Capture input from audio interface
    double currentSampleRate{44100.0};
    int currentBlockSize{512};

    // Current state
    bool isPluginALoaded{false};
    bool isPluginBLoaded{false};

    void loadPluginA();
    void loadPluginB();
    void updateStatus();
    void updateWorkflowPhase();
    void showAudioSettings();
    void syncControlPanel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
