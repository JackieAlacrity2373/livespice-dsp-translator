# LiveSpice A/B Tester - Implementation Roadmap & Architecture Details

**Document Status:** Implementation Planning Phase  
**Last Updated:** January 28, 2026

---

## 1. Detailed Component Specifications

### 1.1 `IAudioProcessor.h` (New Abstraction Layer)
```cpp
/*
  ============================================================================
    IAudioProcessor - Unified interface for audio processors
    
    Provides abstraction for both:
    - External VST3 plugins (via PluginHostWrapper)
    - Native CircuitProcessor implementations
  ============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>

// Information about a single parameter
struct ParameterInfo
{
    juce::String parameterId;      // Unique ID (e.g., "drive")
    juce::String parameterName;    // Display name (e.g., "Drive")
    float currentValue{0.0f};      // Current value (0.0-1.0)
    float minValue{0.0f};          // Minimum value
    float maxValue{1.0f};          // Maximum value
    float defaultValue{0.5f};      // Default value
};

/**
 * IAudioProcessor - Abstract interface for unified audio processing
 * 
 * This interface allows different types of audio processors
 * (external VST3 plugins, native CircuitProcessor implementations, etc.)
 * to work seamlessly in the A/B Tester.
 */
class IAudioProcessor
{
public:
    virtual ~IAudioProcessor() = default;

    // ========================================================================
    // Lifecycle Management
    // ========================================================================
    
    /**
     * Prepares the processor for audio playback
     * Must be called before any audio processing
     */
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;

    /**
     * Releases resources and stops audio processing
     * Should be called before destruction
     */
    virtual void releaseResources() = 0;

    // ========================================================================
    // Audio Processing
    // ========================================================================
    
    /**
     * Process an audio block
     * Called at real-time audio callback rate
     */
    virtual void processBlock(juce::AudioBuffer<float>& buffer,
                            juce::MidiBuffer& midiMessages) = 0;

    // ========================================================================
    // Parameter Management
    // ========================================================================
    
    /**
     * Get the APVTS for direct parameter access
     * Returns nullptr if not applicable (e.g., parameter-less processors)
     */
    virtual juce::AudioProcessorValueTreeState* getAPVTS() = 0;

    /**
     * Get a list of all parameters
     * Used for UI construction
     */
    virtual std::vector<ParameterInfo> getParameters() const = 0;

    /**
     * Set a parameter by ID
     */
    virtual void setParameter(const juce::String& parameterId, float value) = 0;

    /**
     * Get a parameter value by ID
     */
    virtual float getParameter(const juce::String& parameterId) const = 0;

    // ========================================================================
    // Plugin Information
    // ========================================================================
    
    /**
     * Get human-readable processor name
     */
    virtual juce::String getName() const = 0;

    /**
     * Check if processor has an editor GUI
     */
    virtual bool hasEditor() const = 0;

    /**
     * Get the editor instance if available
     */
    virtual juce::AudioProcessorEditor* getEditor() { return nullptr; }

    // ========================================================================
    // Plugin Type Information
    // ========================================================================
    
    /**
     * Get processor type identifier
     * Examples: "livespice.vst3", "mxr.circuit", "boss.circuit"
     */
    virtual juce::String getProcessorType() const = 0;

    /**
     * Check if this is a native DSP processor
     */
    virtual bool isNativeDSP() const { return false; }

    /**
     * Check if this is an external VST plugin
     */
    virtual bool isExternalPlugin() const { return false; }
};
```

### 1.2 `ProcessorFactory.h/cpp` (Factory Pattern)
```cpp
/*
  ============================================================================
    ProcessorFactory - Factory for creating IAudioProcessor instances
  ============================================================================
*/

#pragma once

#include "IAudioProcessor.h"
#include <memory>
#include <functional>

/**
 * Factory for creating and managing IAudioProcessor instances
 * Supports both external VST3 plugins and native CircuitProcessor implementations
 */
class ProcessorFactory
{
public:
    // ========================================================================
    // VST3 Plugin Loading
    // ========================================================================
    
    /**
     * Create a processor from VST3 file path
     * Returns a PluginHostWrapper containing the loaded plugin
     */
    static std::unique_ptr<IAudioProcessor> createFromVST3(
        const juce::String& vst3Path,
        double initialSampleRate,
        int initialBlockSize);

    /**
     * Create a processor from any supported plugin format
     * Auto-detects format based on file extension
     */
    static std::unique_ptr<IAudioProcessor> createFromFile(
        const juce::String& filePath,
        double initialSampleRate,
        int initialBlockSize);

    // ========================================================================
    // Native CircuitProcessor Loading
    // ========================================================================
    
    /**
     * Create a native CircuitProcessor by name
     * Supported circuits: "mxr", "boss_sd1", "bridge_rectifier", etc.
     */
    static std::unique_ptr<IAudioProcessor> createNativeCircuit(
        const juce::String& circuitName);

    /**
     * List all available native circuits
     */
    static juce::StringArray getAvailableCircuits();

    // ========================================================================
    // Processor Information
    // ========================================================================
    
    /**
     * Get information about a processor without creating an instance
     */
    struct ProcessorInfo
    {
        juce::String name;
        juce::String type;              // "vst3" or "circuit"
        juce::String description;
        std::vector<ParameterInfo> parameters;
        bool available{false};
    };

    static ProcessorInfo getProcessorInfo(const juce::String& identifier);

    // ========================================================================
    // Preset Management
    // ========================================================================
    
    /**
     * Save processor state to preset file
     */
    static void savePreset(IAudioProcessor* processor,
                          const juce::String& presetPath);

    /**
     * Load processor state from preset file
     */
    static void loadPreset(IAudioProcessor* processor,
                          const juce::String& presetPath);

private:
    // Helper methods for different processor types
    static std::unique_ptr<IAudioProcessor> createPluginHostWrapper(
        const juce::String& vst3Path,
        double sampleRate,
        int blockSize);

    static std::unique_ptr<IAudioProcessor> createCircuitProcessorWrapper(
        const juce::String& circuitName);
};
```

### 1.3 `PluginHostWrapper.h/cpp` (VST3 Wrapper)
```cpp
/*
  ============================================================================
    PluginHostWrapper - Wraps external VST3 plugins to IAudioProcessor interface
  ============================================================================
*/

#pragma once

#include "IAudioProcessor.h"
#include "PluginHost.h"
#include <memory>

/**
 * Adapter that wraps PluginHost to provide IAudioProcessor interface
 * Allows external VST3 plugins to be used seamlessly with native processors
 */
class PluginHostWrapper : public IAudioProcessor
{
public:
    PluginHostWrapper(const juce::String& vst3Path,
                     double initialSampleRate,
                     int initialBlockSize);
    ~PluginHostWrapper() override;

    // Lifecycle
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    // Audio
    void processBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midiMessages) override;

    // Parameters
    juce::AudioProcessorValueTreeState* getAPVTS() override;
    std::vector<ParameterInfo> getParameters() const override;
    void setParameter(const juce::String& parameterId, float value) override;
    float getParameter(const juce::String& parameterId) const override;

    // Info
    juce::String getName() const override;
    bool hasEditor() const override;
    juce::AudioProcessorEditor* getEditor() override;

    // Type
    juce::String getProcessorType() const override { return "vst3"; }
    bool isExternalPlugin() const override { return true; }

    // Direct access to underlying PluginHost
    PluginHost* getPluginHost() { return host.get(); }
    juce::AudioProcessor* getUnderlyingProcessor();

private:
    std::unique_ptr<PluginHost> host;
    juce::String pluginName;
    bool isLoaded{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHostWrapper)
};
```

### 1.4 `CircuitProcessorWrapper.h` (Native DSP Wrapper Template)
```cpp
/*
  ============================================================================
    CircuitProcessorWrapper - Template wrapper for native CircuitProcessor classes
  ============================================================================
*/

#pragma once

#include "IAudioProcessor.h"
#include <memory>
#include <utility>

/**
 * Template adapter that wraps native CircuitProcessor implementations
 * to provide the IAudioProcessor interface
 * 
 * Template parameter T should be a class that:
 * - Inherits from juce::AudioProcessor
 * - Implements CircuitProcessor interface
 * - Has createParameterLayout() static method
 * - Has getAPVTS() method
 */
template<typename CircuitProcessorType>
class CircuitProcessorWrapper : public IAudioProcessor
{
public:
    CircuitProcessorWrapper()
        : processor(std::make_unique<CircuitProcessorType>())
    {
    }

    ~CircuitProcessorWrapper() override = default;

    // ====================================================================
    // Lifecycle
    // ====================================================================
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        processor->prepareToPlay(sampleRate, samplesPerBlock);
    }

    void releaseResources() override
    {
        processor->releaseResources();
    }

    // ====================================================================
    // Audio Processing
    // ====================================================================
    
    void processBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midiMessages) override
    {
        processor->processBlock(buffer, midiMessages);
    }

    // ====================================================================
    // Parameter Management
    // ====================================================================
    
    juce::AudioProcessorValueTreeState* getAPVTS() override
    {
        return &processor->getAPVTS();
    }

    std::vector<ParameterInfo> getParameters() const override
    {
        std::vector<ParameterInfo> infos;
        auto* apvts = const_cast<juce::AudioProcessorValueTreeState*>(
            &processor->getAPVTS());

        for (auto* param : apvts->processor.getParameters())
        {
            if (auto* audioParam = dynamic_cast<juce::AudioParameterFloat*>(param))
            {
                ParameterInfo info;
                info.parameterId = audioParam->getParameterID();
                info.parameterName = audioParam->getName(1024);
                info.currentValue = audioParam->get();
                info.minValue = audioParam->getNormalisableRange().start;
                info.maxValue = audioParam->getNormalisableRange().end;
                info.defaultValue = audioParam->getDefaultValue();
                infos.push_back(info);
            }
        }
        return infos;
    }

    void setParameter(const juce::String& parameterId, float value) override
    {
        auto* apvts = &processor->getAPVTS();
        if (auto* param = apvts->getParameter(parameterId))
        {
            param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
        }
    }

    float getParameter(const juce::String& parameterId) const override
    {
        auto* apvts = const_cast<juce::AudioProcessorValueTreeState*>(
            &processor->getAPVTS());
        if (auto* param = apvts->getParameter(parameterId))
        {
            return param->getValue();
        }
        return 0.0f;
    }

    // ====================================================================
    // Plugin Information
    // ====================================================================
    
    juce::String getName() const override
    {
        return processor->getName();
    }

    bool hasEditor() const override
    {
        return processor->hasEditor();
    }

    juce::AudioProcessorEditor* getEditor() override
    {
        return processor->getActiveEditor();
    }

    // ====================================================================
    // Type Information
    // ====================================================================
    
    juce::String getProcessorType() const override
    {
        return "circuit";
    }

    bool isNativeDSP() const override
    {
        return true;
    }

    // ====================================================================
    // Direct Processor Access
    // ====================================================================
    
    CircuitProcessorType* getProcessor() { return processor.get(); }

private:
    std::unique_ptr<CircuitProcessorType> processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        CircuitProcessorWrapper<CircuitProcessorType>)
};
```

### 1.5 `AudioRouter.h/cpp` (Signal Routing)
```cpp
/*
  ============================================================================
    AudioRouter - Routes audio between two IAudioProcessor instances
  ============================================================================
*/

#pragma once

#include "IAudioProcessor.h"
#include <memory>

/**
 * Manages audio signal routing between two processors
 * Allows seamless A/B switching with proper gain staging
 */
class AudioRouter
{
public:
    AudioRouter();
    ~AudioRouter();

    // ====================================================================
    // Configuration
    // ====================================================================
    
    /**
     * Set the two processors to route between
     */
    void setProcessors(IAudioProcessor* processorA, IAudioProcessor* processorB);

    /**
     * Select which processor to use (true = A, false = B)
     */
    void setUseProcessorA(bool useA);

    /**
     * Get current selection
     */
    bool isUsingProcessorA() const { return useProcessorA; }

    /**
     * Toggle between A and B
     */
    void toggle() { setUseProcessorA(!useProcessorA); }

    // ====================================================================
    // Audio Processing
    // ====================================================================
    
    /**
     * Process an audio block through selected processor
     */
    void processBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midiMessages);

    // ====================================================================
    // Utilities
    // ====================================================================
    
    /**
     * Get name of current active processor
     */
    juce::String getActiveProcessorName() const;

    /**
     * Get name of inactive processor
     */
    juce::String getInactiveProcessorName() const;

private:
    IAudioProcessor* processorA{nullptr};
    IAudioProcessor* processorB{nullptr};
    bool useProcessorA{true};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRouter)
};
```

### 1.6 `ParameterSynchronizer.h/cpp` (Parameter Sync)
```cpp
/*
  ============================================================================
    ParameterSynchronizer - Keeps parameters synchronized between processors
  ============================================================================
*/

#pragma once

#include "IAudioProcessor.h"
#include <memory>
#include <vector>

/**
 * Synchronizes parameters between two IAudioProcessor instances
 * Supports different sync modes and optional MIDI CC mapping
 */
class ParameterSynchronizer : public juce::AudioProcessorValueTreeState::Listener
{
public:
    enum class SyncMode
    {
        Follower,      // Master slider controls both processors
        Independent,   // Each processor has its own parameters
        Hybrid         // Toggle between Follower and Independent
    };

    ParameterSynchronizer();
    ~ParameterSynchronizer() override;

    // ====================================================================
    // Setup
    // ====================================================================
    
    /**
     * Set the processors to synchronize
     */
    void setProcessors(IAudioProcessor* masterProcessor,
                      IAudioProcessor* slaveProcessor);

    /**
     * Set the synchronization mode
     */
    void setSyncMode(SyncMode newMode);

    /**
     * Get current sync mode
     */
    SyncMode getSyncMode() const { return syncMode; }

    // ====================================================================
    // Parameter Synchronization
    // ====================================================================
    
    /**
     * Manually sync all parameters from master to slave
     */
    void syncAllParameters();

    /**
     * Sync a single parameter
     */
    void syncParameter(const juce::String& parameterId, float value);

    /**
     * APVTS listener callback
     */
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // ====================================================================
    // MIDI CC Mapping
    // ====================================================================
    
    /**
     * Map MIDI CC to parameter
     * CC range 0-119, parameter ID
     */
    void mapMidiCC(int cc, const juce::String& parameterId);

    /**
     * Handle MIDI CC value change
     */
    void handleMidiCC(int cc, float value);

private:
    IAudioProcessor* master{nullptr};
    IAudioProcessor* slave{nullptr};
    SyncMode syncMode{SyncMode::Follower};

    // MIDI CC mappings
    std::map<int, juce::String> midiCCMap;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterSynchronizer)
};
```

---

## 2. Integration Strategy

### 2.1 Phase 1 Files to Create/Modify

**New Files:**
```
Source/
├── IAudioProcessor.h           (NEW)
├── ProcessorFactory.h/cpp      (NEW)
├── PluginHostWrapper.h/cpp     (NEW)
├── CircuitProcessorWrapper.h   (NEW - header only, template)
├── AudioRouter.h/cpp           (NEW)
├── ParameterSynchronizer.h/cpp (NEW)
└── CircuitProcessors/          (NEW - directory)
```

**Modified Files:**
```
Source/
├── MainComponent.h/cpp         (REFACTOR - use IAudioProcessor)
├── ControlPanel.h/cpp          (REFACTOR - enhanced parameter handling)
└── CMakeLists.txt              (UPDATE - add new sources)
```

**Unchanged Files:**
```
Source/
├── PluginHost.h/cpp            (KEEP - VST3 loading, no changes)
├── ABSwitch.h/cpp              (KEEP - no changes)
└── Main.cpp                    (KEEP - no changes)
```

### 2.2 Compilation Dependencies

```
IAudioProcessor.h (no deps - pure interface)
    ├── ProcessorFactory.h (depends on IAudioProcessor)
    │   ├── PluginHostWrapper.h (depends on IAudioProcessor, PluginHost)
    │   └── CircuitProcessorWrapper.h (depends on IAudioProcessor)
    │
    ├── AudioRouter.h (depends on IAudioProcessor)
    ├── ParameterSynchronizer.h (depends on IAudioProcessor)
    │
    └── MainComponent.h (depends on all above)
        └── ControlPanel.h (depends on IAudioProcessor)
```

### 2.3 Build Command Updates

**Updated CMakeLists.txt entry:**
```cmake
target_sources(LiveSpice_AB_Tester PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    Source/PluginHost.h
    Source/PluginHost.cpp
    
    # NEW ABSTRACTION LAYER
    Source/IAudioProcessor.h
    Source/ProcessorFactory.h
    Source/ProcessorFactory.cpp
    Source/PluginHostWrapper.h
    Source/PluginHostWrapper.cpp
    Source/CircuitProcessorWrapper.h           # Template header only
    Source/AudioRouter.h
    Source/AudioRouter.cpp
    Source/ParameterSynchronizer.h
    Source/ParameterSynchronizer.cpp
    
    # EXISTING UI COMPONENTS
    Source/ControlPanel.h
    Source/ControlPanel.cpp
    Source/ABSwitch.h
    Source/ABSwitch.cpp)
```

---

## 3. Pseudocode Implementation Flow

### 3.1 Application Startup
```
main()
  └─ JUCEApplication::initialise()
     └─ MainComponent::MainComponent()
        ├─ audioDeviceManager.initialiseWithDefaultDevices(2, 2)
        ├─ processorFactory = std::make_unique<ProcessorFactory>()
        ├─ sideA = nullptr  (to be loaded)
        ├─ sideB = nullptr  (to be loaded)
        ├─ router = std::make_unique<AudioRouter>()
        ├─ paramSync = std::make_unique<ParameterSynchronizer>()
        ├─ controlPanel = std::make_unique<ControlPanel>()
        ├─ abSwitch = std::make_unique<ABSwitch>()
        ├─ setAudioChannels(2, 2)
        └─ startTimer(100ms)  // UI update timer
```

### 3.2 User: Click "Load Plugin A" Button
```
User: Click "Load Plugin A" button
  └─ loadPluginA() method called
     ├─ File chooser dialog opens
     ├─ User selects file (e.g., "Live Spice Sim.vst3")
     ├─ sideA = ProcessorFactory::createFromFile(path)
     │  ├─ File extension check (.vst3)
     │  └─ ProcessorFactory::createFromVST3(path)
     │     └─ new PluginHostWrapper(path)
     │        └─ host = std::make_unique<PluginHost>()
     │           └─ host->loadPlugin(path)  // VST3 loading
     ├─ sideA->prepareToPlay(currentSampleRate, blockSize)
     ├─ router->setProcessors(sideA, sideB)
     ├─ paramSync->setProcessors(sideA, sideB)
     ├─ controlPanel->setProcessors(sideA, sideB)
     ├─ controlPanel->createLinkedControls()
     ├─ Update label: "Plugin A: Live Spice [Loaded]"
     └─ updateWorkflowPhase()
```

### 3.3 User: Click "Load DSP Circuit B" Button
```
User: Click "Load DSP Circuit B" (or "Load Native Circuit")
  └─ loadCircuitB() method called
     ├─ Dropdown menu shows available circuits
     ├─ User selects "MXR Distortion+"
     ├─ sideB = ProcessorFactory::createNativeCircuit("mxr")
     │  ├─ Check "mxr" is valid
     │  └─ ProcessorFactory::createCircuitProcessorWrapper("mxr")
     │     └─ new CircuitProcessorWrapper<MXRDistortion>()
     │        └─ processor = std::make_unique<MXRDistortion>()
     ├─ sideB->prepareToPlay(currentSampleRate, blockSize)
     ├─ router->setProcessors(sideA, sideB)
     ├─ paramSync->setProcessors(sideA, sideB)
     ├─ controlPanel->setProcessors(sideA, sideB)
     ├─ controlPanel->createLinkedControls()
     │  ├─ Get parameters from sideA->getParameters()  [APVTS A]
     │  ├─ Get parameters from sideB->getParameters()  [APVTS B]
     │  ├─ For each parameter:
     │  │   └─ Create dual sliders (Drive-A, Drive-B)
     │  └─ Link both to ParameterSynchronizer
     ├─ Update label: "Plugin B: MXR Distortion+ [Loaded]"
     └─ Transition to Configured phase
        ├─ Show controlPanel
        ├─ Show abSwitch
        ├─ Enable audio processing
        └─ Log: "System Ready for A/B Comparison"
```

### 3.4 Audio Processing Loop (Real-time)
```
Every 512 samples @ 44.1 kHz:
  MainComponent::getNextAudioBlock(bufferToFill)
    ├─ Get input buffer from device
    ├─ Create working buffer (copy of input)
    ├─ router->processBlock(workingBuffer, midiBuffer)
    │  ├─ if (useProcessorA)
    │  │   └─ sideA->processBlock(workingBuffer, midiBuffer)
    │  │      └─ PluginHostWrapper::processBlock()
    │  │         └─ host->processBlock()
    │  │            └─ pluginInstance->processBlock()  [VST3]
    │  │               ├─ Input: stereo audio (512 samples)
    │  │               └─ Output: processed stereo audio
    │  │
    │  └─ else
    │      └─ sideB->processBlock(workingBuffer, midiBuffer)
    │         └─ CircuitProcessorWrapper<MXR>::processBlock()
    │            └─ mxrProcessor->processBlock()  [Native DSP]
    │               ├─ Input: stereo audio (512 samples)
    │               ├─ Drive circuit processing
    │               ├─ Level control
    │               └─ Output: processed stereo audio
    │
    ├─ Copy workingBuffer to device output
    └─ Repeat for next block
```

### 3.5 User: Press Spacebar (A/B Toggle)
```
User: Presses Spacebar key
  └─ MainComponent::keyPressed(KeyPress(spaceKey))
     ├─ if (currentPhase == Configured)
     │  ├─ router->toggle()
     │  │  └─ useProcessorA = !useProcessorA
     │  ├─ abSwitch->setIsA(!usePluginA)  // Visual feedback
     │  ├─ statusLabel->setText(...)
     │  └─ Next audio block uses other processor
     │     (User hears instant switch)
     │
     └─ return true  (consume event)

Next Audio Block:
  Processes through OPPOSITE processor
  User hears the other side immediately
```

### 3.6 User: Adjust Parameter Slider
```
User: Drags Drive slider to 0.7
  └─ ControlPanel slider callback
     ├─ onSliderValueChanged(slider)
     ├─ paramSync->syncParameter("drive", 0.7)
     │  ├─ if (syncMode == Follower)
     │  │  ├─ sideA->setParameter("drive", 0.7)
     │  │  │  └─ apvtsA->getParameter("drive")->setValueNotifyingHost(0.7)
     │  │  └─ sideB->setParameter("drive", 0.7)
     │  │     └─ apvtsB->getParameter("drive")->setValueNotifyingHost(0.7)
     │  │
     │  └─ Both processors update their internal state
     │
     └─ Next audio blocks process with new parameter value
        User hears the change applied to active processor
```

---

## 4. Testing Checklist

### 4.1 Unit Tests
- [ ] IAudioProcessor interface compiles
- [ ] PluginHostWrapper compiles with VST3 plugin
- [ ] CircuitProcessorWrapper<MXRDistortion> compiles
- [ ] ProcessorFactory creates wrappers correctly
- [ ] AudioRouter switches between processors
- [ ] ParameterSynchronizer syncs parameters

### 4.2 Integration Tests
- [ ] Load VST3 plugin as sideA
- [ ] Load native circuit as sideB
- [ ] Both processors receive audio
- [ ] Both processors' parameters appear in UI
- [ ] Spacebar toggles audio between sides
- [ ] Parameters update in real-time
- [ ] No crashes on plugin unload

### 4.3 Audio Tests
- [ ] VST3 produces audio output
- [ ] Native circuit produces audio output
- [ ] Audio doesn't have glitches or pops
- [ ] Level is consistent between sides
- [ ] Latency is < 10ms
- [ ] No buffer overflow/underflow

### 4.4 UI Tests
- [ ] All 3 parameters visible (MXR: Drive, Level; Boss: Drive, Level, Tone)
- [ ] Sliders update both processors
- [ ] Visual feedback on toggle
- [ ] Responsive to keyboard input
- [ ] Menu system works
- [ ] Log file captures events

---

## 5. Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| VST3 loading fails | Low | High | Keep PluginHost unchanged, test extensively |
| Parameters not syncing | Medium | High | Implement ParameterSynchronizer, unit test |
| Audio glitches | Medium | High | Test with different block sizes, check latency |
| Memory leaks | Low | Medium | Use smart pointers, run valgrind/ASAN |
| Performance regression | Low | Medium | Profile both versions, compare CPU usage |
| UI responsiveness | Medium | Low | Use timer-based updates, not real-time syncs |

---

## 6. Success Metrics (Measurable)

| Metric | Target | How to Measure |
|--------|--------|-----------------|
| All parameters visible | 100% (3/3) | Count sliders in UI |
| Audio output present | Yes | Listen & record output |
| A/B switch latency | < 50ms | Measure frame timing |
| Parameter update latency | < 1ms | Log timestamps |
| Memory footprint | < 150MB | Task Manager |
| CPU usage (idle) | < 2% | Task Manager |
| No crashes after 10min | Yes | Stress test |
| Compilation warnings | 0 | Build output |
| Code style compliance | 100% | Code review |

---

**Status:** Ready for Phase 1 Implementation  
**Next Step:** Create header files and begin abstraction layer development
