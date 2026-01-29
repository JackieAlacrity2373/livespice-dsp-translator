# LiveSpice A/B Tester - Architecture Visual Reference & Quick Start

**Document Type:** Visual Reference & Implementation Quick Start  
**Created:** January 28, 2026

---

## 1. Complete System Architecture Diagram

```
╔════════════════════════════════════════════════════════════════════════════╗
║                        JUCE Audio Application                              ║
║                    (LiveSpice_AB_Tester.exe)                              ║
╠════════════════════════════════════════════════════════════════════════════╣
║                                                                              ║
║  ╔──────────────────────────── MainComponent ────────────────────────────╗ ║
║  ║ (Extends: AudioAppComponent, Timer, MenuBarModel)                    ║ ║
║  ║                                                                        ║ ║
║  ║  ┌──────────────────────────────────────────────────────────────────┐ ║ ║
║  ║  │ Audio Device Management                                          │ ║ ║
║  ║  │  • AudioDeviceManager (JUCE device enumeration)                 │ ║ ║
║  ║  │  • Sample rate: 44.1kHz, 48kHz, 96kHz (device-dependent)      │ ║ ║
║  ║  │  • Buffer size: 512 samples per block                           │ ║ ║
║  ║  └──────────────────────────────────────────────────────────────────┘ ║ ║
║  ║                                                                        ║ ║
║  ║  ┌─────────────────────┐              ┌─────────────────────┐       ║ ║
║  ║  │   SIDE A            │              │   SIDE B            │       ║ ║
║  ║  │ (IAudioProcessor)   │              │ (IAudioProcessor)   │       ║ ║
║  ║  ├─────────────────────┤              ├─────────────────────┤       ║ ║
║  ║  │ PluginHostWrapper   │              │ CircuitProcessor    │       ║ ║
║  ║  │ (External VST3)     │              │ Wrapper<T>          │       ║ ║
║  ║  │                     │              │ (Native DSP)        │       ║ ║
║  ║  │ ┌─────────────────┐ │              │                     │       ║ ║
║  ║  │ │  PluginHost     │ │              │ ┌─────────────────┐ │       ║ ║
║  ║  │ │  • Load VST3    │ │              │ │ Live Spice      │ │       ║ ║
║  ║  │ │  • Process      │ │              │ │ Components      │ │       ║ ║
║  ║  │ │  • APVTS Access │ │              │ │ • Diode clipping│ │       ║ ║
║  ║  │ └─────────────────┘ │              │ │ • Op-amp stage  │ │       ║ ║
║  ║  │        ↓            │              │ │ • RC filters    │ │       ║ ║
║  ║  │ ┌─────────────────┐ │              │ │ • Tone control  │ │       ║ ║
║  ║  │ │External VST3    │ │              │ │ • Drive circuit │ │       ║ ║
║  ║  │ │.dll / .vst3 file│ │              │ └─────────────────┘ │       ║ ║
║  ║  │ │                 │ │              │        ↓            │       ║ ║
║  ║  │ │Live Spice Sim   │ │              │ ┌─────────────────┐ │       ║ ║
║  ║  │ │or Generated     │ │              │ │ APVTS           │ │       ║ ║
║  ║  │ │JUCE Plugin      │ │              │ │ • Drive param   │ │       ║ ║
║  ║  │ └─────────────────┘ │              │ │ • Level param   │ │       ║ ║
║  ║  │        ↓            │              │ │ • Tone param    │ │       ║ ║
║  ║  │ APVTS (2-3 params)  │              │ │ • Default values│ │       ║ ║
║  ║  │ [Live Spice sim]    │              │ └─────────────────┘ │       ║ ║
║  ║  └─────────────────────┘              └─────────────────────┘       ║ ║
║  ║           ↓                                   ↓                      ║ ║
║  ║  ┌─────────────────────────────────────────────────┐               ║ ║
║  ║  │          AudioRouter                            │               ║ ║
║  ║  │  usePluginA flag → Select Active Processor      │               ║ ║
║  ║  │  • True:  Route to SIDE A                       │               ║ ║
║  ║  │  • False: Route to SIDE B                       │               ║ ║
║  ║  └─────────────────────────────────────────────────┘               ║ ║
║  ║           ↓                                                         ║ ║
║  ║  ┌─────────────────────────────────────────────────┐               ║ ║
║  ║  │   Real-time Audio Processing Block              │               ║ ║
║  ║  │   (512 samples @ 44.1kHz = ~11.6ms latency)    │               ║ ║
║  ║  │                                                  │               ║ ║
║  ║  │   selectedProcessor->processBlock(              │               ║ ║
║  ║  │     buffer,                                     │               ║ ║
║  ║  │     midiBuffer                                  │               ║ ║
║  ║  │   );                                            │               ║ ║
║  ║  └─────────────────────────────────────────────────┘               ║ ║
║  ║           ↓                                                         ║ ║
║  ║  ┌─────────────────────────────────────────────────┐               ║ ║
║  ║  │  Audio Output to Device                          │               ║ ║
║  ║  │  • Stereo L/R channels                          │               ║ ║
║  ║  │  • Real-time monitoring                         │               ║ ║
║  ║  │  • Can be switched instantly (spacebar)         │               ║ ║
║  ║  └─────────────────────────────────────────────────┘               ║ ║
║  ║           ↓                                                         ║ ║
║  ║  ┌─────────────────────────────────────────────────┐               ║ ║
║  ║  │    Listeners & Synchronizers                     │               ║ ║
║  ║  │                                                  │               ║ ║
║  ║  │  ┌───────────────────────────────────────────┐  │               ║ ║
║  ║  │  │ ParameterSynchronizer                     │  │               ║ ║
║  ║  │  │ • Monitors APVTS changes                  │  │               ║ ║
║  ║  │  │ • Sync: Drive, Level, Tone                │  │               ║ ║
║  ║  │  │ • Mode: Follower (sync both)              │  │               ║ ║
║  ║  │  │ • MIDI CC 0-119 mapping                   │  │               ║ ║
║  ║  │  └───────────────────────────────────────────┘  │               ║ ║
║  ║  └─────────────────────────────────────────────────┘               ║ ║
║  ║                                                                     ║ ║
║  ║  ┌──────────────────────────────────────────────────────────────┐  ║ ║
║  ║  │  User Interface Layer                                        │  ║ ║
║  ║  │                                                              │  ║ ║
║  ║  │  ┌────────────────────────────────────────────────────────┐ │  ║ ║
║  ║  │  │ ControlPanel                                           │ │  ║ ║
║  ║  │  │ • Drive Slider (0.0 - 1.0)                            │ │  ║ ║
║  ║  │  │   └─ Controls both SIDE A and SIDE B (Follower)       │ │  ║ ║
║  ║  │  │ • Level Slider (0.0 - 1.0)                            │ │  ║ ║
║  ║  │  │   └─ Controls both SIDE A and SIDE B                  │ │  ║ ║
║  ║  │  │ • Tone Slider (0.0 - 1.0) [if available]              │ │  ║ ║
║  ║  │  │   └─ Controls both SIDE A and SIDE B                  │ │  ║ ║
║  ║  │  │ • Real-time value display                             │ │  ║ ║
║  ║  │  │ • Linked to APVTS (bidirectional)                    │ │  ║ ║
║  ║  │  └────────────────────────────────────────────────────────┘ │  ║ ║
║  ║  │                                                              │  ║ ║
║  ║  │  ┌────────────────────────────────────────────────────────┐ │  ║ ║
║  ║  │  │ ABSwitch                                               │ │  ║ ║
║  ║  │  │ • Visual A/B Toggle                                   │ │  ║ ║
║  ║  │  │ • Click to switch                                     │ │  ║ ║
║  ║  │  │ • Spacebar to switch                                  │ │  ║ ║
║  ║  │  │ • Shows: "A" (blue) or "B" (green)                   │ │  ║ ║
║  ║  │  └────────────────────────────────────────────────────────┘ │  ║ ║
║  ║  │                                                              │  ║ ║
║  ║  │  Status Labels:                                             │  ║ ║
║  ║  │  • "Phase: CONFIGURED - Ready for A/B Testing"           │  ║ ║
║  ║  │  • "Currently Using: SIDE B (DSP - MXR Distortion+)"    │  ║ ║ 
║  ║  │  • "Audio: Processing ✓"                                  │  ║ ║
║  ║  └──────────────────────────────────────────────────────────────┘  ║ ║
║  ║                                                                     ║ ║
║  ║  ┌──────────────────────────────────────────────────────────────┐  ║ ║
║  ║  │ Menu Bar (File Menu)                                         │  ║ ║
║  ║  │ • Audio Settings                                            │  ║ ║
║  ║  │ • Logging Level → [Disabled|Error|Warning|Info|Debug]      │  ║ ║
║  ║  │ • View Log File                                            │  ║ ║
║  ║  │ • Quit                                                      │  ║ ║
║  ║  └──────────────────────────────────────────────────────────────┘  ║ ║
║  ║                                                                     ║ ║
║  └─────────────────────────────────────────────────────────────────────┘ ║
║                                                                              ║
╚════════════════════════════════════════════════════════════════════════════╝
```

---

## 1.1 Debug Audio Launch Flag

The A/B Tester includes an optional debug mode for audio diagnostics (extra logs, test tone, buffer probes, and temporary output gain). It is **disabled by default** and only enabled via a launch flag:

- `--debug-audio` (preferred)
- `--debug`

When the flag is **not** present, the app runs with normal processing and minimal logging.

---

## 2. Data Flow Sequence Diagrams

### 2.1 Initialization Sequence
```
main()
  ├─ JUCEApplication::initialise()
  │  └─ createMainComponent()
  │     └─ new MainComponent()
  │        ├─ audioDeviceManager.initialiseWithDefaultDevices(2, 2)
  │        ├─ new ProcessorFactory()
  │        ├─ sideA = nullptr
  │        ├─ sideB = nullptr
  │        ├─ new AudioRouter()
  │        ├─ new ParameterSynchronizer()
  │        ├─ new ControlPanel()
  │        │  └─ (empty, waiting for processors)
  │        ├─ new ABSwitch()
  │        ├─ setAudioChannels(2, 2)
  │        │  └─ audioDeviceManager.addAudioCallback(this)
  │        └─ startTimer(100)
  │           └─ UI updates every 100ms
  │
  └─ GUI ready for user interaction
     State: PRE-CONFIGURED
     Waiting for: Click "Load Plugin A" or "Load DSP Circuit B"
```

### 2.2 Load VST3 Plugin Sequence
```
User: Click "Load Plugin A"
  ├─ File chooser opens
  ├─ User selects: "Live Spice Sim.vst3"
  │
  └─ MainComponent::loadPluginA()
     ├─ filePath = "H:/path/to/Live Spice Sim.vst3"
     ├─ sideA = ProcessorFactory::createFromFile(filePath)
     │  ├─ Check file exists ✓
     │  ├─ Check extension (.vst3) ✓
     │  └─ ProcessorFactory::createFromVST3(filePath)
     │     └─ new PluginHostWrapper(filePath, sampleRate, blockSize)
     │        └─ host = std::make_unique<PluginHost>()
     │           └─ host->loadPlugin(filePath)
     │              ├─ VST3PluginFormat::findAllTypesForFile()
     │              │  └─ Parse .vst3 metadata
     │              ├─ VST3PluginFormat::createInstanceFromDescription()
     │              │  └─ plugin = vst3Format.create(...)
     │              │     └─ External plugin instance created
     │              └─ plugin->prepareToPlay(sampleRate, blockSize)
     │
     ├─ sideA->prepareToPlay(currentSampleRate, currentBlockSize)
     │  └─ Allocate internal buffers
     │
     ├─ router->setProcessors(sideA, sideB)
     ├─ paramSync->setProcessors(sideA, sideB)
     ├─ controlPanel->setProcessors(sideA, sideB)
     │
     ├─ isPluginALoaded = true
     ├─ Update UI label: "Plugin A: Live Spice [Loaded] ✓"
     └─ updateWorkflowPhase()
        └─ If both loaded → Transition to CONFIGURED
           └─ controlPanel->setVisible(true)
```

### 2.3 Load Native Circuit Sequence
```
User: Click "Load DSP Circuit B" or Auto-Load
  ├─ Circuit selector (dropdown or auto)
  ├─ User selects: "MXR Distortion+"
  │
  └─ MainComponent::loadCircuitB()
     ├─ circuitName = "mxr"
     ├─ sideB = ProcessorFactory::createNativeCircuit(circuitName)
     │  ├─ Check name valid ("mxr" → MXRDistortion) ✓
     │  └─ ProcessorFactory::createCircuitProcessorWrapper(circuitName)
     │     └─ new CircuitProcessorWrapper<MXRDistortion>()
     │        └─ processor = std::make_unique<MXRDistortion>()
     │           ├─ CircuitProcessor constructor
     │           ├─ apvts = AudioProcessorValueTreeState(
     │           │     *this, nullptr, "Parameters",
     │           │     createParameterLayout()
     │           │  )
     │           └─ Parameters created:
     │              ├─ "drive" (0.0-1.0, default 0.5)
     │              ├─ "level" (0.0-1.0, default 1.0)
     │              └─ [tone if applicable]
     │
     ├─ sideB->prepareToPlay(currentSampleRate, currentBlockSize)
     │  ├─ Allocate DSP component buffers
     │  ├─ Initialize Diode models
     │  ├─ Initialize Op-Amp model
     │  └─ Initialize RC filters
     │
     ├─ router->setProcessors(sideA, sideB)
     ├─ paramSync->setProcessors(sideA, sideB)
     ├─ controlPanel->setProcessors(sideA, sideB)
     │  ├─ Get sideA parameters: sideA->getParameters()
     │  │  └─ Access sideA's APVTS for parameter list
     │  │
     │  ├─ Get sideB parameters: sideB->getParameters()
     │  │  └─ Access sideB's APVTS for parameter list
     │  │
     │  └─ For each parameter:
     │     ├─ Create Slider for parameter
     │     ├─ Set range 0.0-1.0
     │     ├─ Attach to both APVTS instances
     │     └─ Register with ParameterSynchronizer
     │
     ├─ isPluginBLoaded = true
     ├─ Update UI label: "Plugin B: MXR Distortion+ [Loaded] ✓"
     └─ updateWorkflowPhase()
        └─ If both loaded → Transition to CONFIGURED ★
           ├─ controlPanel->setVisible(true)
           ├─ abSwitch->setVisible(true)
           ├─ g_loggingEnabled = true
           └─ Log: "====== CONFIGURED PHASE ======"
```

### 2.4 Audio Processing Loop Sequence
```
Every ~11.6ms (512 samples @ 44.1kHz):

JUCE Real-Time Audio Callback:
  audioDeviceManager::callback()
    └─ MainComponent::getNextAudioBlock(bufferToFill)
       ├─ Get stereo input buffer (2 channels, 512 samples)
       ├─ Create temp processing buffer
       │
       ├─ Call AudioRouter::processBlock(buffer, midiBuffer)
       │  └─ router->processBlock(buffer, midiBuffer)
       │     ├─ if (useProcessorA)  // True = Use SIDE A (VST3)
       │     │  └─ sideA->processBlock(buffer, midiBuffer)
       │     │     └─ PluginHostWrapper::processBlock()
       │     │        └─ host->processBlock(buffer, midiBuffer)
       │     │           └─ VST3 plugin instance processes:
       │     │              Input:  512 samples stereo
       │     │              Process: Live Spice circuit simulation
       │     │              Output: 512 samples stereo processed
       │     │
       │     └─ else  // False = Use SIDE B (DSP)
       │        └─ sideB->processBlock(buffer, midiBuffer)
       │           └─ CircuitProcessorWrapper<MXR>::processBlock()
       │              └─ mxrProcessor->processBlock(buffer, midiBuffer)
       │                 └─ Process each sample:
       │                    ├─ Read Drive parameter from APVTS
       │                    ├─ Apply drive/distortion stage
       │                    ├─ Apply diode clipping
       │                    ├─ Apply op-amp saturation
       │                    ├─ Read Tone parameter from APVTS
       │                    ├─ Apply tone control (RC filter)
       │                    ├─ Read Level parameter from APVTS
       │                    └─ Apply output level scaling
       │
       ├─ Copy processed buffer to device output
       └─ Return to JUCE (next block ready)

Next Block (user pressed spacebar):
  useProcessorA = false  // Switched to SIDE B
  Next callback uses SIDE B instead (User hears change)
```

### 2.5 Parameter Change Sequence
```
User: Drags Drive slider to 0.7

ControlPanel::sliderValueChanged(driveSlider)
  ├─ New value: 0.7 (normalized 0.0-1.0)
  ├─ Update slider display
  │
  └─ onParameterChanged callback
     ├─ Call ParameterSynchronizer::syncParameter("drive", 0.7)
     │  │
     │  ├─ if (syncMode == Follower)
     │  │  │
     │  │  ├─ sideA->setParameter("drive", 0.7)
     │  │  │  └─ PluginHostWrapper::setParameter()
     │  │  │     └─ apvtsA->getParameter("drive")
     │  │  │        └─ parameter->setValueNotifyingHost(0.7)
     │  │  │           └─ VST3 plugin notified of change
     │  │  │
     │  │  └─ sideB->setParameter("drive", 0.7)
     │  │     └─ CircuitProcessorWrapper::setParameter()
     │  │        └─ apvtsB->getParameter("drive")
     │  │           └─ parameter->setValueNotifyingHost(0.7)
     │  │              └─ CircuitProcessor notified of change
     │  │                 └─ Updates internal state for DSP
     │  │
     │  └─ Both processors' internal state now has drive = 0.7
     │
     └─ Next audio block uses new drive value
        ├─ VST3 processes with new drive
        ├─ DSP processes with new drive
        └─ User hears new tone immediately
```

### 2.6 A/B Toggle Sequence (Spacebar)
```
User: Presses Spacebar key

MainComponent::keyPressed(KeyPress(spaceKey))
  ├─ Check: currentPhase == Configured?  ✓ Yes
  │
  ├─ router->toggle()
  │  └─ useProcessorA = !useProcessorA
  │     └─ Was True (using SIDE A), now False (using SIDE B)
  │
  ├─ abSwitch->setIsA(false)
  │  └─ repaint()  → Visual updates to show "B" is active
  │
  ├─ statusLabel->setText("Currently: SIDE B (DSP)")
  │  └─ repaint()  → Text updates
  │
  └─ return true  (consume key event)

Next Audio Block (immediate):
  MainComponent::getNextAudioBlock()
    └─ router->processBlock()
       └─ if (useProcessorA = false)  ← NEW SELECTION
          └─ sideB->processBlock()  ← Using OTHER processor now
             └─ User HEARS the switch instantly!
```

---

## 3. State Machine Diagram

```
                    ┌─────────────────┐
                    │   STARTUP       │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │ PRE-CONFIGURED  │
                    │                 │
                    │ State:          │
                    │ • sideA = nil   │
                    │ • sideB = nil   │
                    │ • Audio paused  │
                    │                 │
                    │ Actions:        │
                    │ • Load VST3 A   │
                    │ • Load VST3 B   │
                    │ • Load Circuit A│
                    │ • Load Circuit B│
                    │ • Auto-Load     │
                    └────────┬────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
    Loaded A           Loaded B              Loaded Both
        │                    │                    │
        ▼                    ▼                    ▼
  ┌──────────┐         ┌──────────┐         ┌──────────────┐
  │ Partial  │         │ Partial  │         │ CONFIGURED   │
  │ (Wait B) │         │ (Wait A) │         │              │
  └────┬─────┘         └────┬─────┘         │ State:       │
       │                    │               │ • sideA ✓    │
       │                    │               │ • sideB ✓    │
       └────────┬───────────┘               │ • Audio ON   │
                │                           │              │
                └──────────────┬────────────→│ Actions:     │
                  Load Other   │            │ • Adjust    │
                               │            │   parameters│
                               │            │ • A/B toggle│
                               │            │ • Listen    │
                               │            │ • Swap sides│
                               │            │              │
                               ▼            │              │
                        ┌──────────────────►│              │
                        │ (STAY IN STATE)  │              │
                        └──────────────────┘              │
                                           │              │
                                      Unload│              │
                                           ▼              │
                                     ┌────────────────────┘
                                     │
                                     ▼
                              ┌──────────────┐
                              │ PRE-CONFIGURED
                              │ (back to start)
                              │ • Reset UI
                              │ • Clear params
                              │ • Pause audio
                              └──────────────┘
```

---

## 4. Class Hierarchy Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                     JUCE Framework Classes                          │
│                                                                      │
│  AudioAppComponent  Timer  MenuBarModel                            │
│      ▲               ▲          ▲                                   │
│      │               │          │                                   │
│      └───────────────┼──────────┘                                   │
│                      │                                              │
│                      ▼                                              │
│              MainComponent                                          │
│              (Core Audio App)                                       │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│              Audio Processor Abstraction Layer                       │
│                                                                      │
│                     IAudioProcessor                                 │
│                    (Pure Interface)                                 │
│                           ▲                                         │
│                           │                                         │
│           ┌───────────────┴────────────────┐                        │
│           │                                │                        │
│           ▼                                ▼                        │
│    PluginHostWrapper         CircuitProcessorWrapper<T>            │
│    (External VST3)           (Native DSP)                          │
│           │                                │                        │
│           │                                │                        │
│           ├──► PluginHost               ├──► CircuitProcessor      │
│           │    (VST3 Loader)            │    (Base Class)          │
│           │                             │                         │
│           └──► External                 ├──► MXRDistortion        │
│                .vst3/.dll               │    ├── Parameters        │
│                Plugin                   │    └── DSP Components    │
│                                         │                         │
│                                         ├──► BossSD1             │
│                                         │    ├── Parameters        │
│                                         │    └── DSP Components    │
│                                         │                         │
│                                         ├──► BridgeRectifier     │
│                                         │                         │
│                                         └──► ... (other circuits)│
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                Supporting Components                                 │
│                                                                      │
│  ┌──────────────────────┐   ┌──────────────────────┐               │
│  │  AudioRouter         │   │ ParameterSynchronizer│               │
│  │  • Switch A/B        │   │ • Sync parameters    │               │
│  │  • Route audio       │   │ • MIDI CC mapping    │               │
│  │  • No gap switching  │   │ • Follower mode      │               │
│  └──────────────────────┘   └──────────────────────┘               │
│                                                                      │
│  ┌──────────────────────┐   ┌──────────────────────┐               │
│  │  ControlPanel        │   │  ABSwitch            │               │
│  │  • Parameter sliders │   │  • Visual toggle     │               │
│  │  • Labels/values     │   │  • Click handler     │               │
│  │  • Linked controls   │   │  • Keyboard support  │               │
│  └──────────────────────┘   └──────────────────────┘               │
│                                                                      │
│  ┌──────────────────────┐                                          │
│  │  ProcessorFactory    │                                          │
│  │  • Create VST3       │                                          │
│  │  • Create Circuits   │                                          │
│  │  • Auto-detect type  │                                          │
│  └──────────────────────┘                                          │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 5. Quick Reference: Key Methods

### 5.1 MainComponent Key Methods
```cpp
// Lifecycle
MainComponent::MainComponent()
MainComponent::~MainComponent()

// JUCE AudioAppComponent
void prepareToPlay(int samplesPerBlockExpected, double sampleRate)
void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
void releaseResources()

// JUCE Timer
void timerCallback()

// JUCE MenuBarModel
juce::StringArray getMenuBarNames()
juce::PopupMenu getMenuForIndex(int index, const juce::String& name)
void menuItemSelected(int itemID, int menuIndex)

// JUCE Component
void paint(juce::Graphics& g)
void resized()
bool keyPressed(const juce::KeyPress& key)

// Custom Methods
void loadPluginA()
void loadPluginB()
void updateStatus()
void updateWorkflowPhase()
void showAudioSettings()
```

### 5.2 IAudioProcessor Interface Methods
```cpp
// Required (must implement)
virtual void prepareToPlay(double sr, int blockSize) = 0
virtual void releaseResources() = 0
virtual void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) = 0
virtual juce::AudioProcessorValueTreeState* getAPVTS() = 0
virtual std::vector<ParameterInfo> getParameters() const = 0
virtual void setParameter(const juce::String& id, float value) = 0
virtual float getParameter(const juce::String& id) const = 0
virtual juce::String getName() const = 0
virtual bool hasEditor() const = 0
virtual juce::String getProcessorType() const = 0

// Optional (default implementations)
virtual juce::AudioProcessorEditor* getEditor() { return nullptr; }
virtual bool isNativeDSP() const { return false; }
virtual bool isExternalPlugin() const { return false; }
```

### 5.3 ProcessorFactory Static Methods
```cpp
static std::unique_ptr<IAudioProcessor> createFromVST3(
    const juce::String& path, double sr, int blockSize)

static std::unique_ptr<IAudioProcessor> createFromFile(
    const juce::String& path, double sr, int blockSize)

static std::unique_ptr<IAudioProcessor> createNativeCircuit(
    const juce::String& name)

static juce::StringArray getAvailableCircuits()

static void savePreset(IAudioProcessor* proc, const juce::String& path)

static void loadPreset(IAudioProcessor* proc, const juce::String& path)
```

### 5.4 AudioRouter Methods
```cpp
void setProcessors(IAudioProcessor* a, IAudioProcessor* b)
void setUseProcessorA(bool useA)
bool isUsingProcessorA() const
void toggle()
void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&)
juce::String getActiveProcessorName() const
juce::String getInactiveProcessorName() const
```

### 5.5 ParameterSynchronizer Methods
```cpp
void setProcessors(IAudioProcessor* master, IAudioProcessor* slave)
void setSyncMode(SyncMode newMode)
SyncMode getSyncMode() const
void syncAllParameters()
void syncParameter(const juce::String& id, float value)
void parameterChanged(const juce::String& paramID, float newValue) override
void mapMidiCC(int cc, const juce::String& paramId)
void handleMidiCC(int cc, float value)
```

---

## 6. Implementation Checklist

### Phase 1: Foundation
- [ ] Create `IAudioProcessor.h` (pure interface)
- [ ] Create `ProcessorFactory.h/cpp` (factory pattern)
- [ ] Create `PluginHostWrapper.h/cpp` (VST3 wrapper)
- [ ] Create `CircuitProcessorWrapper.h` (template)
- [ ] Create `AudioRouter.h/cpp` (signal routing)
- [ ] Create `ParameterSynchronizer.h/cpp` (param sync)
- [ ] Update `CMakeLists.txt` (add new sources)
- [ ] Update `MainComponent.h/cpp` (use IAudioProcessor)
- [ ] Compile without errors
- [ ] Test with existing VST3 loading

### Phase 2: Native Circuit Integration
- [ ] Copy MXR Distortion+ CircuitProcessor into project
- [ ] Create CircuitProcessorWrapper<MXRDistortion> specialization
- [ ] Add "Load DSP Circuit" button to UI
- [ ] Test loading native circuit
- [ ] Verify APVTS parameters extracted
- [ ] Test audio processing (MXR DSP)
- [ ] Test A/B switching between VST3 and DSP
- [ ] Verify all 3 parameters visible (if applicable)

### Phase 3: Enhancements
- [ ] Fix ControlPanel parameter display
- [ ] Implement linked parameter controls
- [ ] Test real-time parameter updates
- [ ] Implement MIDI CC mapping
- [ ] Add preset save/load
- [ ] Enhance logging system

### Phase 4: Scaling
- [ ] Add remaining 5 circuit types
- [ ] Create circuit selector UI
- [ ] Performance optimization
- [ ] Final integration testing
- [ ] Documentation & user guide

---

## 7. File Organization Reference

```
LiveSpice-AB-Tester/
│
├── REFACTORED_DESIGN_DOCUMENT.md           ◄── Architecture
├── IMPLEMENTATION_ROADMAP.md               ◄── Detailed specs
├── ARCHITECTURE_QUICK_REFERENCE.md         ◄── This file
│
├── CMakeLists.txt
├── build/
│   ├── LiveSpice_AB_Tester_artefacts/
│   │   └── Release/
│   │       └── B Tester.exe                ◄── Built executable
│
└── Source/
    ├── Main.cpp
    │
    ├── MainComponent.h/cpp                 ◄── Core audio component
    │
    ├── IAudioProcessor.h                   ◄── NEW: Interface
    ├── ProcessorFactory.h/cpp              ◄── NEW: Factory
    ├── PluginHostWrapper.h/cpp             ◄── NEW: VST3 wrapper
    ├── CircuitProcessorWrapper.h           ◄── NEW: DSP wrapper
    ├── AudioRouter.h/cpp                   ◄── NEW: Routing
    ├── ParameterSynchronizer.h/cpp         ◄── NEW: Sync
    │
    ├── PluginHost.h/cpp                    ◄── EXISTING: VST3 loader
    ├── ControlPanel.h/cpp                  ◄── REFACTORED: UI
    ├── ABSwitch.h/cpp                      ◄── EXISTING: UI toggle
    │
    └── CircuitProcessors/                  ◄── NEW: Native DSP
        ├── MXR_Distortion/
        │   ├── CircuitProcessor.h/cpp
        │   └── CircuitProcessorEditor.h/cpp
        ├── Boss_SD1/
        ├── Bridge_Rectifier/
        ├── Marshall_Breaker/
        ├── TubeAmplifier/
        └── BJT_Amplifier/
```

---

**Quick Reference Complete**  
**Ready for Phase 1 Implementation**

Use `REFACTORED_DESIGN_DOCUMENT.md` for comprehensive architecture overview.  
Use `IMPLEMENTATION_ROADMAP.md` for detailed technical specifications.  
Use this file for quick visual reference and implementation checklist.
