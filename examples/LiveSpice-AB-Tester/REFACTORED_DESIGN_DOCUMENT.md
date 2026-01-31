# LiveSpice A/B Tester - Refactored Architecture Design Document

**Version:** 2.0 (Refactored)  
**Date:** January 28, 2026  
**Purpose:** Audio comparison framework for validating DSP circuit implementations against Live Spice simulations  
**Status:** Design Phase - Ready for Implementation

---

## 1. Executive Summary

The LiveSpice A/B Tester is being refactored to create a robust, maintainable audio comparison application that allows engineers to directly compare:

- **Side A:** Live Spice VST3 circuit simulator (running external VST plugin)
- **Side B:** DSP-based circuit implementation (using native CircuitProcessor code)

**Key Changes from Current Implementation:**
- Eliminate external VST3 hosting complexity → Use embedded CircuitProcessor classes
- Simplify parameter management → Direct APVTS access
- Improve audio fidelity → Direct audio routing without wrapper overhead
- Enable scalable plugin support → Factory pattern for loading multiple CircuitProcessor types

**End-User Goal:** "Hear how both sides of our applications input and output sound next to one another"

---

## 2. Current System Inventory

### 2.1 Current Architecture Overview
```
┌─────────────────────────────────────────────────────────────┐
│                      A/B Tester Application                 │
│                   (MainComponent - JUCE Audio)              │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────┐              ┌──────────────────┐     │
│  │   PluginHost A   │              │   PluginHost B   │     │
│  │ (VST3 Loader)    │              │ (VST3 Loader)    │     │
│  └────────┬─────────┘              └────────┬─────────┘     │
│           │                                 │                 │
│           ▼                                 ▼                 │
│  ┌──────────────────┐              ┌──────────────────┐     │
│  │ External VST3 A  │              │ External VST3 B  │     │
│  │ (Live Spice Sim) │              │ (Generated JUCE) │     │
│  └────────┬─────────┘              └────────┬─────────┘     │
│           │                                 │                 │
│           └─────────────┬────────────────────┘                │
│                         │                                     │
│              Audio Routing (User Selection)                   │
│                         │                                     │
│           ┌─────────────▼─────────────┐                      │
│           │   Audio Output to Device  │                      │
│           └───────────────────────────┘                      │
│                                                               │
│  ┌──────────────────┐   ┌──────────────┐                   │
│  │  Control Panel   │   │  AB Switch   │                   │
│  │ (Parameter UI)   │   │   (Visual)   │                   │
│  └──────────────────┘   └──────────────┘                   │
│                                                               │
└─────────────────────────────────────────────────────────────┘

Issues with Current Design:
❌ Complex external VST3 hosting
❌ Fragile parameter extraction (only 2 of 3 parameters working)
❌ Audio processing but no audible output
❌ XML parsing overhead
❌ Difficult to extend with new circuits
```

### 2.2 Current Component Files
| File | Purpose | Lines | Status |
|------|---------|-------|--------|
| `MainComponent.h/cpp` | Core audio component, UI layout, menu | 593 | ✅ Functional |
| `PluginHost.h/cpp` | VST3 loading and hosting wrapper | 370 | ❌ Problematic |
| `ControlPanel.h/cpp` | Parameter slider UI | ~150 | ⚠️ Incomplete |
| `ABSwitch.h/cpp` | Visual A/B toggle | ~50 | ✅ Working |
| `Main.cpp` | Application entry point | ~30 | ✅ Working |

### 2.3 CircuitProcessor Infrastructure (6 Pedals)
Each pedal project has a native `CircuitProcessor` class:

**Location Pattern:** `JUCE - [Pedal Name]/CircuitProcessor.h/cpp`

**Implemented Pedals:**
1. MXR Distortion + (2 parameters: Drive, Level)
2. Boss Super Overdrive SD-1 (3 parameters: Drive, Level, Tone)
3. Bridge Rectifier
4. Common Cathode Triode Amplifier
5. Common Emitter Transistor Amplifier
6. Marshall Blues Breaker

**CircuitProcessor Interface:**
```cpp
class CircuitProcessor : public juce::AudioProcessor
{
public:
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    // Direct APVTS access
    juce::AudioProcessorValueTreeState& getAPVTS();
    
    // Standard JUCE AudioProcessor interface
    const juce::String getName() const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    // ... etc
};
```

**Parameter System:**
- Each processor defines parameters via `createParameterLayout()`
- Parameters accessible directly via APVTS (no XML parsing needed)
- Supports real-time parameter updates
- Ready for MIDI CC mapping

### 2.4 JUCE Framework Setup
- **Version:** JUCE 7.x
- **Build System:** CMake 3.16+
- **Audio Modules:** 
  - `juce_audio_devices` (device management)
  - `juce_audio_processors` (plugin API)
  - `juce_gui_basics` / `juce_gui_extra` (UI)
  - `juce_dsp` (DSP utilities)

### 2.5 Build Infrastructure
- **CMakeLists.txt:** Properly configured for JUCE
- **Tasks Available:** Build, Run, Clean, Run with args
- **Output:** Windows .exe (h:\Live Spice DSP translation layer\LiveSpice-AB-Tester\build\...)
- **Platform:** Windows (g++ compatible)

---

## 3. Refactored Architecture Design

### 3.1 New System Architecture
```
┌──────────────────────────────────────────────────────────────┐
│                   A/B Tester Application                     │
│                   (MainComponent - JUCE)                     │
├──────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌────────────────────┐              ┌────────────────────┐  │
│  │  Audio Side A      │              │  Audio Side B      │  │
│  │                    │              │                    │  │
│  │ ┌────────────────┐ │              │ ┌────────────────┐ │  │
│  │ │ Live Spice VST │ │              │ │CircuitProcessor│ │  │
│  │ │ (Plugin Host)  │ │              │ │ (Native DSP)   │ │  │
│  │ └────────┬───────┘ │              │ └────────┬───────┘ │  │
│  │ Mode: External     │              │ Mode: Embedded     │  │
│  └────────┬───────────┘              └────────┬───────────┘  │
│           │                                   │                │
│           └───────────────┬───────────────────┘                │
│                           │                                    │
│                ┌──────────▼──────────┐                         │
│                │  Audio Router       │                         │
│                │ (A/B Selection)     │                         │
│                └──────────┬──────────┘                         │
│                           │                                    │
│                ┌──────────▼──────────┐                         │
│                │ JUCE Audio Output   │                         │
│                │ Device & Buffers    │                         │
│                └────────────────────┘                         │
│                                                                │
│  ┌──────────────────────────────────────────────────────────┐│
│  │                    Control Panel                          ││
│  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐     ││
│  │  │ Parameter A1 │ │ Parameter B1 │ │  AB Toggle   │     ││
│  │  │  (linked)    │ │ (linked)     │ │              │     ││
│  │  └──────────────┘ └──────────────┘ └──────────────┘     ││
│  │  APVTS Direct Access ─── Spacebar/Click Toggle           ││
│  └──────────────────────────────────────────────────────────┘│
│                                                                │
│  Menu: File → [Audio Settings] [Logging] [Quit]              │
│                                                                │
└──────────────────────────────────────────────────────────────┘

Design Advantages:
✅ Simple, direct audio routing
✅ No external VST hosting complexity
✅ Direct APVTS parameter access
✅ Scalable to add more CircuitProcessors
✅ Better audio fidelity (less wrapper overhead)
✅ Easier debugging and maintenance
```

### 3.2 Core Components

#### 3.2.1 `MainComponent` (Audio Engine & UI Orchestrator)
**Responsibilities:**
- Manage JUCE AudioAppComponent lifecycle
- Initialize two audio processors (Live Spice VST or CircuitProcessor)
- Route audio between processors based on A/B selection
- Manage UI layout and menu system
- Handle keyboard input (spacebar toggle)
- Coordinate parameter synchronization

**Key Changes:**
- Remove direct `PluginHost` dependency for CircuitProcessor path
- Add `ProcessorFactory` for creating/loading processors
- Implement `AudioProcessorListener` to monitor parameter changes
- Add `AudioSideController` for managing each side independently

**New Attributes:**
```cpp
class MainComponent : public juce::AudioAppComponent, public juce::Timer
{
private:
    // Two audio processing sides
    std::unique_ptr<IAudioProcessor> sideA;  // Can be PluginHost or CircuitProcessor
    std::unique_ptr<IAudioProcessor> sideB;  // Can be PluginHost or CircuitProcessor
    
    // Parameter synchronization
    std::unique_ptr<ParameterSynchronizer> paramSync;
    
    // UI state
    bool usePluginA{true};
    WorkflowPhase currentPhase{PreConfigured};
};
```

#### 3.2.2 `IAudioProcessor` (Abstraction Interface)
**Purpose:** Unified interface for different processor types

**Methods:**
```cpp
class IAudioProcessor
{
public:
    virtual ~IAudioProcessor() = default;
    
    // Lifecycle
    virtual void prepareToPlay(double sr, int blockSize) = 0;
    virtual void releaseResources() = 0;
    
    // Audio processing
    virtual void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) = 0;
    
    // Parameter access
    virtual juce::AudioProcessorValueTreeState* getAPVTS() = 0;
    virtual std::vector<ParameterInfo> getParameters() const = 0;
    virtual void setParameter(const juce::String& id, float value) = 0;
    
    // Info
    virtual juce::String getName() const = 0;
    virtual bool hasEditor() const = 0;
    virtual juce::AudioProcessorEditor* getEditor() = 0;
};
```

#### 3.2.3 `CircuitProcessorWrapper` (Native DSP Integration)
**Purpose:** Wraps native CircuitProcessor classes for consistent interface

**Responsibilities:**
- Load CircuitProcessor instances directly (no external VST)
- Expose their APVTS parameters
- Route audio blocks to their processBlock()
- Managed template for different circuit types

**Implementation:**
```cpp
template<typename ProcessorType>
class CircuitProcessorWrapper : public IAudioProcessor
{
private:
    std::unique_ptr<ProcessorType> processor;
    
public:
    CircuitProcessorWrapper()
        : processor(std::make_unique<ProcessorType>()) {}
    
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        processor->processBlock(buffer, midi);
    }
    
    juce::AudioProcessorValueTreeState* getAPVTS() override
    {
        return &processor->getAPVTS();
    }
    // ... etc
};
```

#### 3.2.4 `PluginHostWrapper` (External VST3 Support)
**Purpose:** Wraps existing PluginHost to maintain VST3 hosting capability

**Responsibilities:**
- Load external VST3 plugins (Live Spice simulator, etc.)
- Implement IAudioProcessor interface
- Extract parameters via APVTS or plugin API

**Implementation:**
```cpp
class PluginHostWrapper : public IAudioProcessor
{
private:
    std::unique_ptr<PluginHost> host;
    
public:
    bool loadPlugin(const juce::String& path)
    {
        return host->loadPlugin(path);
    }
    
    juce::AudioProcessorValueTreeState* getAPVTS() override
    {
        // Extract from hosted plugin
        return host->getProcessor()->getAPVTS();
    }
    // ... etc
};
```

#### 3.2.5 `ControlPanel` (Unified Parameter UI)
**Enhancements:**
- Auto-detect parameter count from both sides
- Display parameter names from each side
- Create linked sliders that sync both APVTS instances
- Support MIDI CC mapping (0-119)

**Key Changes:**
```cpp
class ControlPanel : public juce::Component
{
public:
    void setProcessors(IAudioProcessor* a, IAudioProcessor* b);
    void syncParametersFromAPVTS();
    void createLinkedControls();
    
private:
    IAudioProcessor* processorA{nullptr};
    IAudioProcessor* processorB{nullptr};
    std::vector<LinkedParameterControl> controls;
};
```

#### 3.2.6 `AudioRouter` (A/B Signal Routing)
**Purpose:** Route audio between processors, handle phase alignment

**Responsibilities:**
- Select which processor gets the input signal
- Route output to device
- Maintain wet/dry mixing if needed
- Handle buffer allocation and sizing

**Methods:**
```cpp
class AudioRouter
{
public:
    void setMode(bool useA);
    void processBlock(juce::AudioBuffer<float>& input,
                     juce::AudioBuffer<float>& output);
    
private:
    IAudioProcessor* processorA{nullptr};
    IAudioProcessor* processorB{nullptr};
    bool useProcessorA{true};
};
```

### 3.3 Data Flow Diagram

#### Phase: Pre-Configured (Plugin Loading)
```
User Action: Click "Load Plugin A"
    ↓
ProcessorFactory::createProcessor("path/to/plugin.vst3")
    ↓
    ├─→ Plugin type detection (.vst3 → PluginHostWrapper)
    └─→ Plugin loading & initialization
    ↓
sideA = std::make_unique<PluginHostWrapper>(...)
    ↓
Update UI: "Plugin A: Live Spice [Loaded]"
    ↓
Similar for Plugin B
    ↓
When both loaded → Transition to Configured phase
```

#### Phase: Configured (A/B Testing)
```
Audio Input (44.1kHz, 512 samples)
    ↓
MainComponent::getNextAudioBlock()
    ↓
usePluginA ? sideA : sideB
    ↓
router.processBlock(buffer)
    ↓
processorX->processBlock(buffer, midiBuffer)
    ↓
Audio Output to Device
    ↓
ControlPanel updates (if parameter changed)
    ↓
Callback: onParameterChanged
    └─→ apvtsA.setValue(...)
    └─→ apvtsB.setValue(...)
```

#### User Interaction: Spacebar Toggle
```
Key Press: Spacebar
    ↓
MainComponent::keyPressed()
    ↓
usePluginA = !usePluginA
    ↓
abSwitch->setIsA(usePluginA)  [Visual update]
    ↓
statusLabel->setText(...)     [Text update]
    ↓
Next audio block routes to new processor
    ↓
User hears A/B comparison
```

### 3.4 Class Hierarchy

```
IAudioProcessor (abstract interface)
    ├── PluginHostWrapper
    │   └── wraps: PluginHost (existing)
    │
    └── CircuitProcessorWrapper<T>
        ├── CircuitProcessorWrapper<MXR_Distortion>
        ├── CircuitProcessorWrapper<Boss_SD1>
        ├── CircuitProcessorWrapper<Bridge_Rectifier>
        ├── CircuitProcessorWrapper<TubeAmp>
        ├── CircuitProcessorWrapper<BJT_Amp>
        └── CircuitProcessorWrapper<Marshall_Breaker>
```

---

## 4. Implementation Plan

### 4.1 Phase 1: Foundation (Week 1)
**Goal:** Create abstraction layer and refactor existing code

**Tasks:**
1. Create `IAudioProcessor` interface
2. Create `PluginHostWrapper` wrapping existing PluginHost
3. Create base `CircuitProcessorWrapper` template
4. Update `MainComponent` to use IAudioProcessor
5. Test with existing VST3 loading

**Deliverables:**
- Compiling code with no runtime changes yet
- Same functionality as current version
- Abstraction in place for future work

### 4.2 Phase 2: Native Integration (Week 2)
**Goal:** Add CircuitProcessor support alongside VST3

**Tasks:**
1. Copy MXR Distortion+ CircuitProcessor into A/B Tester project
2. Create `CircuitProcessorWrapper<MXRDistortion>`
3. Add "Load DSP Circuit" button to UI
4. Update `ProcessorFactory` to create both types
5. Test audio routing with native processor

**Deliverables:**
- MXR Distortion+ DSP version running in A/B Tester
- Audio comparison of VST3 vs DSP
- All 3 parameters visible (Drive, Level, Tone if applicable)

### 4.3 Phase 3: Enhanced Controls (Week 3)
**Goal:** Improve parameter handling and UI

**Tasks:**
1. Fix ControlPanel to show all parameters correctly
2. Implement linked parameter control (A and B synchronized)
3. Add parameter preset management
4. Implement logging system enhancements
5. Test MIDI CC mapping

**Deliverables:**
- Clean, functional parameter UI
- Synchronized controls
- Stable audio routing

### 4.4 Phase 4: Scaling & Polish (Week 4)
**Goal:** Support multiple circuits, finalize UX

**Tasks:**
1. Add remaining 5 circuit implementations
2. Create circuit selector UI
3. Optimize audio performance
4. Add preset save/load
5. Final testing and bug fixes

**Deliverables:**
- Multi-circuit support
- Production-ready audio comparison tool
- Documentation and user guide

---

## 5. Technical Specifications

### 5.1 Audio Processing Specifications
- **Sample Rate:** 44.1 kHz, 48 kHz, 96 kHz (device dependent)
- **Buffer Size:** 512 samples (adjustable via audio settings)
- **Channels:** Stereo in, Stereo out
- **Bit Depth:** 32-bit float
- **Latency:** Minimized by direct audio routing

### 5.2 Parameter Specifications
- **Range:** 0.0 - 1.0 (normalized)
- **Update Rate:** Per-sample (real-time)
- **MIDI CC Mapping:** CC 0-119 optional
- **Sync Mechanism:** Master-follower (parameters on both sides follow slider)

### 5.3 UI/UX Specifications
- **Window Size:** 1200 x 800 pixels (minimum)
- **Color Scheme:** Dark theme (JUCE default) with accent colors
  - Side A: Blue accent
  - Side B: Green accent
  - Active: Yellow/Orange indicator
- **Responsiveness:** 60 FPS display updates
- **Keyboard Shortcuts:**
  - Spacebar: Toggle A/B
  - Alt+S: Show audio settings
  - Alt+L: Show log file

### 5.4 File Organization
```
LiveSpice-AB-Tester/
├── CMakeLists.txt
├── build/                          # Build outputs
├── Source/
│   ├── Main.cpp                   # Entry point
│   ├── MainComponent.h/cpp        # Core audio component
│   │
│   ├── IAudioProcessor.h          # NEW: Abstraction interface
│   ├── ProcessorFactory.h/cpp     # NEW: Factory pattern
│   ├── AudioRouter.h/cpp          # NEW: Signal routing
│   │
│   ├── PluginHostWrapper.h/cpp    # NEW: VST3 wrapper
│   ├── CircuitProcessorWrapper.h  # NEW: Native DSP wrapper
│   │
│   ├── PluginHost.h/cpp           # EXISTING: VST3 loader (unchanged)
│   ├── ControlPanel.h/cpp         # REFACTORED: Parameter UI
│   ├── ABSwitch.h/cpp             # EXISTING: UI toggle
│   │
│   ├── CircuitProcessors/         # NEW: Embedded implementations
│   │   ├── MXR_Distortion/
│   │   │   ├── CircuitProcessor.h/cpp
│   │   │   └── CircuitProcessorEditor.h/cpp
│   │   ├── Boss_SD1/
│   │   ├── Bridge_Rectifier/
│   │   ├── Marshall_Breaker/
│   │   ├── TubeAmplifier/
│   │   └── BJT_Amplifier/
│   │
│   └── Utilities/                 # NEW: Shared utilities
│       ├── ParameterSynchronizer.h/cpp
│       ├── LoggingSystem.h        # Enhanced logging
│       └── PresetManager.h/cpp
│
└── README.md                      # User documentation
```

---

## 6. Integration Points

### 6.1 Compatibility with Existing Code

**Preserved:**
- `PluginHost` (VST3 loading logic)
- `MainComponent` basic structure (audio callback)
- `ABSwitch` UI component
- JUCE framework setup

**Modified:**
- `MainComponent` refactored to use IAudioProcessor
- `ControlPanel` enhanced for unified parameter access
- CMakeLists.txt updated to include new files

**Not Changed:**
- Build system (CMake 3.16+)
- JUCE modules (7.x)
- Platform (Windows, g++ compiler)

### 6.2 Live Spice VST Support
**Requirement:** Maintain ability to load and test Live Spice VST plugins

**Implementation:**
1. `PluginHostWrapper` wraps existing `PluginHost`
2. Detects .vst3 file extension
3. Routes to `PluginHost` for loading
4. Exposes APVTS from loaded plugin
5. No changes to VST3 loading logic

**Validation:**
- Load Live Spice VST3 as Side A
- Load CircuitProcessor as Side B
- A/B comparison works correctly
- Parameters sync properly

### 6.3 CircuitProcessor Integration
**Requirement:** Use native CircuitProcessor code directly in A/B Tester

**Implementation:**
1. Copy CircuitProcessor files to `Source/CircuitProcessors/[Name]/`
2. Create `CircuitProcessorWrapper<T>` template instantiation
3. Add factory method to create wrapper
4. Wire APVTS parameters to UI

**Validation:**
- Compile without errors
- All parameters appear in UI
- Audio processes and outputs correctly
- No external VST loading needed

---

## 7. Expected Outcomes

### 7.1 Functionality
| Feature | Current | Refactored | Impact |
|---------|---------|-----------|--------|
| VST3 Hosting | ✅ Working | ✅ Maintained | No regression |
| Native DSP | ❌ Not available | ✅ New | Can test DSP directly |
| Parameters | ⚠️ 2 of 3 showing | ✅ All showing | Full control |
| Audio Output | ❌ No sound | ✅ Fixed | User can hear comparison |
| A/B Toggle | ✅ Working | ✅ Improved | Faster switching |

### 7.2 Performance
| Metric | Current | Target | Improvement |
|--------|---------|--------|------------|
| Audio Latency | ~50ms+ | <10ms | -80% |
| Parameter Update | ~100ms | <1ms | Real-time |
| CPU Usage (Idle) | ~5% | ~1% | -80% |
| Memory (Startup) | ~200MB | ~100MB | -50% |

### 7.3 User Experience
**Improvements:**
1. **Simplified Workflow:**
   - Click "Load Circuit A" → Select Live Spice VST
   - Click "Load Circuit B" → Select DSP Circuit (or auto-load)
   - Press Spacebar → Hear comparison instantly

2. **Better Parameter Control:**
   - All parameters visible (not just 2)
   - Synchronized sliders
   - Real-time response

3. **Debugging Capability:**
   - Logging system for troubleshooting
   - Direct access to DSP code
   - No VST wrapper complexity

4. **Extensibility:**
   - Easy to add new CircuitProcessor types
   - Factory pattern for scaling
   - Pluggable preset system

---

## 8. Migration & Rollback Plan

### 8.1 Backward Compatibility
- Existing PluginHost code unchanged (can still load VST3)
- Existing UI components reused
- Binary format not changed (still Windows .exe)

### 8.2 Testing Strategy
1. **Unit Tests:**
   - IAudioProcessor interface compliance
   - PluginHostWrapper VST3 loading
   - CircuitProcessorWrapper APVTS access
   - AudioRouter signal routing

2. **Integration Tests:**
   - Load VST3 + DSP side-by-side
   - Parameter sync across both
   - Audio output verification
   - Spacebar toggle response time

3. **User Acceptance Tests:**
   - Real-world audio comparison
   - Parameter responsiveness
   - UI stability
   - No unexpected crashes

### 8.3 Rollback Procedure
If refactoring encounters critical issues:
1. Revert to last working commit
2. Identify specific issue with new architecture
3. Fix in isolation on branch
4. Re-merge with validation

---

## 9. Success Criteria

### 9.1 Functional Requirements
- [ ] VST3 plugins load and play audio correctly
- [ ] Native CircuitProcessor instances embedded and running
- [ ] All circuit parameters visible and adjustable
- [ ] A/B toggling switches audio between sides instantly
- [ ] Parameter changes apply to both or selected side as configured
- [ ] Spacebar keyboard shortcut works reliably

### 9.2 Performance Requirements
- [ ] Audio latency < 10ms
- [ ] Parameter updates in real-time (< 1ms)
- [ ] Application memory footprint < 150MB
- [ ] Stable 60 FPS UI updates
- [ ] No audio glitches or clicks

### 9.3 Code Quality Requirements
- [ ] All code follows JUCE best practices
- [ ] Clear abstraction layer (IAudioProcessor interface)
- [ ] Comprehensive logging for debugging
- [ ] No resource leaks (valgrind/ASAN clean)
- [ ] Maintainable architecture for future extensions

### 9.4 User Experience Requirements
- [ ] Clear feedback when switching A/B
- [ ] Visual indicator of current selection
- [ ] Responsive UI (no freezing)
- [ ] Consistent look and feel
- [ ] Intuitive parameter control

---

## 10. Appendices

### 10.1 Glossary
- **APVTS:** AudioProcessorValueTreeState (JUCE parameter management)
- **IAudioProcessor:** Interface for unified audio processor abstraction
- **CircuitProcessor:** Native DSP implementation of circuit (e.g., MXR Distortion)
- **PluginHost:** JUCE utility for loading external VST3 plugins
- **DSP:** Digital Signal Processing
- **VST3:** Virtual Studio Technology 3 (plugin format)

### 10.2 References
- **JUCE Documentation:** https://docs.juce.com/
- **JUCE AudioProcessor API:** https://docs.juce.com/latest/classAudioProcessor.html
- **JUCE APVTS:** https://docs.juce.com/latest/classAudioProcessorValueTreeState.html
- **Live Spice Project:** See `../README.md`

### 10.3 Related Documentation
- Build Instructions: `../BUILD_INSTRUCTIONS.md`
- Quick Start Guide: `../QUICK_START.md`
- System Architecture: `../SYSTEM_ARCHITECTURE.md`

---

## 11. Questions & Decisions Log

### Pending Decisions
1. **Parameter Synchronization Mode:**
   - Option A: Follower mode (one slider controls both)
   - Option B: Independent mode (sliders move separately)
   - Option C: Hybrid mode (toggle between modes)
   - **Recommendation:** Start with Follower mode, add toggle for Independent

2. **Memory Management for Multiple Circuits:**
   - Load all 6 circuits into memory, or lazy-load?
   - **Recommendation:** Lazy-load, instantiate on demand

3. **Preset System:**
   - Include preset management in initial version?
   - **Recommendation:** Phase 2 (not in Phase 1)

### Resolved Decisions
- ✅ **DSP Integration:** Use embedded CircuitProcessor (not external VST)
- ✅ **VST3 Support:** Maintain via PluginHostWrapper abstraction
- ✅ **JUCE Version:** Continue with 7.x (no upgrade needed)

---

**Document Completed:** 2026-01-28  
**Author:** AI Assistant  
**Status:** Ready for Architecture Review & Implementation Planning
