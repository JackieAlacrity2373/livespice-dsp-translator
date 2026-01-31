/*
  ==============================================================================
    Main Component - A/B Testing Interface
  ==============================================================================
*/

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginHost.h"
#include "ControlPanel.h"
#include "ABSwitch.h"

class MainComponent : public juce::AudioAppComponent,
                      public juce::Timer
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

private:
    // Audio device manager
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;

    // Plugin hosts (A = LiveSpice VST, B = Generated JUCE Plugin)
    std::unique_ptr<PluginHost> pluginHostA;
    std::unique_ptr<PluginHost> pluginHostB;

    // UI Components
    std::unique_ptr<ControlPanel> controlPanel;
    std::unique_ptr<ABSwitch> abSwitch;

    // Plugin selection
    juce::TextButton loadPluginAButton{"Load LiveSpice VST (A)"};
    juce::TextButton loadPluginBButton{"Load Generated Plugin (B)"};
    juce::Label pluginALabel;
    juce::Label pluginBLabel;

    // Status display
    juce::Label statusLabel;

    // Audio buffers for processing
    juce::AudioBuffer<float> tempBufferA;
    juce::AudioBuffer<float> tempBufferB;

    // Current state
    bool isPluginALoaded{false};
    bool isPluginBLoaded{false};
    bool usePluginA{true}; // true = A, false = B

    void loadPluginA();
    void loadPluginB();
    void updateStatus();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
