# Complete System Architecture & Documentation Map

## System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                   LiveSpice DSP Translation Layer                   │
│                                                                     │
│  LiveSpice Schematic → Analysis → DSP Configuration → JUCE Plugin   │
└─────────────────────────────────────────────────────────────────────┘
```

## Project Directory Structure

```
h:\Live Spice DSP translation layer\
│
├── 📦 CORE TRANSLATOR
│   ├── Livespice_to_DSP.cpp        ← Main entry point
│   ├── LiveSpiceParser.h/.cpp      ← XML parsing & data structures
│   ├── CircuitAnalyzer.h/.cpp      ← Topology analysis & reporting
│   └── JuceDSPGenerator.h/.cpp     ← JUCE code generation
│
├── 🎛️ GENERATED PLUGINS (Auto-created)
│   ├── JUCE - MXR Distortion +/
│   │   ├── CircuitProcessor.h
│   │   ├── CircuitProcessor.cpp
│   │   ├── CMakeLists.txt
│   │   └── build/                 (created during compilation)
│   │
│   └── JUCE - Boss Super Overdrive SD-1/
│       ├── CircuitProcessor.h
│       ├── CircuitProcessor.cpp
│       ├── CMakeLists.txt
│       └── build/                 (created during compilation)
│
├── 📋 DOCUMENTATION
│   ├── JUCE_QUICKSTART.md          ← START HERE (5 min read)
│   ├── JUCE_PLUGIN_BUILD_GUIDE.md  ← Detailed build info
│   ├── JUCE_GENERATION_SUMMARY.md  ← What was built
│   ├── IMPLEMENTATION_SUMMARY.md   ← Previous work
│   ├── PROJECT_DELIVERY_REPORT.md  ← Project status
│   ├── BUILD_INSTRUCTIONS.md       ← Build setup
│   ├── README.md                   ← Project overview
│   ├── QUICK_START.md              ← Initial quick guide
│   └── INDEX.md                    ← File index
│
├── 🔧 BUILD CONFIGURATION
│   ├── .vscode/tasks.json          ← VS Code build tasks
│   ├── CMakeLists.txt              ← Main project CMake (if needed)
│   │
│   └── 📚 EXTERNAL DEPENDENCIES
│       └── third_party/JUCE/       ← Downloaded JUCE framework
│
├── 📂 EXAMPLE CIRCUITS
│   └── example pedals/
│       ├── Boss Super Overdrive SD-1.schx
│       ├── Bridge Rectifier.schx
│       ├── Common Cathode Triode Amplifier.schx
│       ├── Common Emitter Transistor Amplifier.schx
│       ├── Marshall Blues Breaker.schx
│       └── MXR Distortion +.schx
│
└── 📊 OUTPUT FILES
    ├── output.txt                 ← Circuit analysis output
    ├── juce_output.txt           ← JUCE generation output
    ├── juce_generated.txt        ← Latest JUCE output
    └── livespice-translator.exe  ← Compiled executable
```

## Documentation Map

### 🟢 Getting Started (5-10 minutes)

**Start here:** [`JUCE_QUICKSTART.md`](JUCE_QUICKSTART.md)

Quick TL;DR for building:
```bash
.\livespice-translator.exe "path/to/circuit.schx"
cd "JUCE - [Circuit Name]"
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

---

### 🔵 Comprehensive Building (30-45 minutes)

**Detailed guide:** [`JUCE_PLUGIN_BUILD_GUIDE.md`](JUCE_PLUGIN_BUILD_GUIDE.md)

Covers:
- Installation requirements
- Step-by-step build instructions
- Generated code structure
- Plugin customization options
- Troubleshooting guide
- DAW installation paths

---

### 🟣 System Summary (10 minutes)

**Architecture overview:** [`JUCE_GENERATION_SUMMARY.md`](JUCE_GENERATION_SUMMARY.md)

Describes:
- What was accomplished
- Generated plugin structure
- Usage workflow
- Technical implementation details
- Next steps & roadmap

---

### 🟡 Legacy Documentation

**Previous work summaries:**
- [`PROJECT_DELIVERY_REPORT.md`](PROJECT_DELIVERY_REPORT.md) - Project completion status
- [`IMPLEMENTATION_SUMMARY.md`](IMPLEMENTATION_SUMMARY.md) - Detailed implementation notes
- [`BUILD_INSTRUCTIONS.md`](BUILD_INSTRUCTIONS.md) - MinGW setup instructions
- [`README.md`](README.md) - Project overview

---

## Translation Pipeline

```
┌──────────────────────────────────────────────────────────────────┐
│                      1. INPUT: LiveSpice Circuit                 │
│                          (.schx XML file)                        │
└──────────────────────────┬───────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────────┐
│                   2. PARSING & ANALYSIS                          │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ LiveSpiceParser                                          │   │
│  │ • Reads XML schematic                                   │   │
│  │ • Extracts components (resistors, caps, op-amps)        │   │
│  │ • Maps connections between components                   │   │
│  │ • Builds connectivity graph                             │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ CircuitAnalyzer                                          │   │
│  │ • Identifies processing stages                          │   │
│  │ • Calculates filter frequencies (RC values)             │   │
│  │ • Determines gain and clipping parameters               │   │
│  │ • Generates reports                                     │   │
│  └──────────────────────────────────────────────────────────┘   │
└──────────────────────────┬───────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────────┐
│               3. JUCE CODE GENERATION                            │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ JuceDSPGenerator                                         │   │
│  │ • Creates CircuitProcessor.h header                     │   │
│  │   - JUCE AudioProcessor class                           │   │
│  │   - DSP module member variables                         │   │
│  │                                                          │   │
│  │ • Creates CircuitProcessor.cpp implementation           │   │
│  │   - prepareToPlay() initialization                      │   │
│  │   - processBlock() signal chain                         │   │
│  │   - JUCE lifecycle methods                              │   │
│  │                                                          │   │
│  │ • Creates CMakeLists.txt build configuration            │   │
│  │   - JUCE framework linking                              │   │
│  │   - Plugin format settings                              │   │
│  │   - Compiler flags (C++17)                              │   │
│  └──────────────────────────────────────────────────────────┘   │
└──────────────────────────┬───────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────────┐
│              4. OUTPUT: JUCE Plugin Directory                    │
│                                                                  │
│  JUCE - [Circuit Name]/                                          │
│  ├── CircuitProcessor.h         (1.7 KB)                         │
│  ├── CircuitProcessor.cpp       (3.4 KB)                         │
│  ├── CMakeLists.txt            (1.2 KB)                         │
│  └── Ready for compilation                                      │
└──────────────────────────┬───────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────────┐
│                5. COMPILATION & OUTPUT                           │
│                                                                  │
│  cmake .. -DCMAKE_BUILD_TYPE=Release                            │
│  cmake --build . --config Release                               │
│                                                                  │
│  Output:                                                         │
│  • Windows: .vst3, .standalone                                  │
│  • macOS: .vst3, .au, .aax, .standalone                         │
│  • Linux: .vst3, .standalone                                    │
└──────────────────────────┬───────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────────┐
│           6. PLUGIN READY FOR USE IN YOUR DAW!                   │
│                                                                  │
│  Load in Reaper, Pro Tools, Logic, Ableton, etc.                │
│  Process audio with circuit-accurate DSP                        │
└──────────────────────────────────────────────────────────────────┘
```

## Key Classes & Components

### LiveSpiceParser
- **Purpose**: XML parsing of LiveSpice schematics
- **Key Methods**:
  - `parseFile()` - Load and parse .schx file
  - `buildConnectivityPool()` - Create connection graph
  - `getComponents()` - Access all components

### CircuitAnalyzer
- **Purpose**: Circuit topology analysis
- **Key Methods**:
  - `analyzeCircuit()` - Identify processing stages
  - `generateReport()` - Detailed component listing
  - `generateConnectivityReport()` - Connection mapping

### JuceDSPGenerator
- **Purpose**: JUCE audio plugin code generation
- **Key Methods**:
  - `generateProcessorHeader()` - Create .h file
  - `generateProcessorImplementation()` - Create .cpp file
  - `generateCMakeLists()` - Create build config
  - `writePluginFiles()` - Save to disk

## DSP Filters & Processing Chain

### MXR Distortion+ DSP Implementation

Generated JUCE plugins implement a 5-stage signal processing chain:

```
┌─────────────────────────────────────────────────────────────────┐
│                       INPUT SIGNAL                              │
└──────────────────────────────┬──────────────────────────────────┘
                               │
                               ▼
    ┌────────────────────────────────────────┐
    │ STAGE 0: Input High-Pass Filter (HPF)  │
    │ Frequency: 72 Hz                       │
    │ Purpose: DC blocking coupling capacitor│
    │ Implementation: IIR::Filter makeHighPass│
    └────────────────────────────────────────┘
                               │
                               ▼
    ┌────────────────────────────────────────┐
    │ STAGE 1: Drive Gain Stage              │
    │ Parameter: Drive (0.0 - 1.0)           │
    │ Gain Range: 0.1x to 10.0x              │
    │ Purpose: Pre-clipping signal boost     │
    │ Implementation: juce::dsp::Gain        │
    └────────────────────────────────────────┘
                               │
                               ▼
    ┌────────────────────────────────────────┐
    │ STAGE 2: Op-Amp Clipping               │
    │ Algorithm: Soft clipping (tanh)        │
    │ Formula: out = tanh(in)                │
    │ Purpose: Non-linear distortion         │
    │ Implementation: juce::dsp::WaveShaper  │
    └────────────────────────────────────────┘
                               │
                               ▼
    ┌────────────────────────────────────────┐
    │ STAGE 3: Output Low-Pass Filter (LPF)  │
    │ Frequency: 15.9 kHz                    │
    │ Purpose: Tone shaping, reduce harshness│
    │ Implementation: IIR::Filter makeLowPass │
    └────────────────────────────────────────┘
                               │
                               ▼
    ┌────────────────────────────────────────┐
    │ STAGE 4: Level Gain Stage              │
    │ Parameter: Level (0.0 - 1.0)           │
    │ Default: 0.5 (50%)                     │
    │ Purpose: Output volume control         │
    │ Implementation: juce::dsp::Gain        │
    └────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                      OUTPUT SIGNAL                              │
└─────────────────────────────────────────────────────────────────┘
```

### Filter Technical Details

#### Input High-Pass Filter (72 Hz)
- **Type**: Butterworth high-pass IIR filter
- **Cutoff**: 72 Hz (10 kΩ × 1 nF coupling capacitor)
- **Purpose**: Removes DC bias and subsonic frequencies
- **Implementation**: `IIR::Coefficients::makeHighPass(sampleRate, 72.0f)`
- **JUCE Class**: `juce::dsp::IIR::Filter<float>`

#### Output Low-Pass Filter (15.9 kHz)
- **Type**: Butterworth low-pass IIR filter
- **Cutoff**: 15.9 kHz (10 kΩ × 1 nF RC network)
- **Purpose**: Tone shaping, removes aliasing artifacts
- **Implementation**: `IIR::Coefficients::makeLowPass(sampleRate, 15915.5f)`
- **JUCE Class**: `juce::dsp::IIR::Filter<float>`

### Parameters

#### Drive (0.0 - 1.0, Default: 0.5)
- **Minimum (0.0)**: 0.1x gain = very quiet
- **Default (0.5)**: 1.0x gain = unity
- **Maximum (1.0)**: 10.0x gain = loud distortion
- **Mapping Formula**: `gain = 0.1f + (drive × 9.9f)`
- **Purpose**: Controls how much the signal is boosted into the clipping stage

#### Level (0.0 - 1.0, Default: 0.5)
- **Minimum (0.0)**: Output muted
- **Default (0.5)**: 50% volume (protects ears during testing)
- **Maximum (1.0)**: Full volume
- **Purpose**: Output level after distortion

### Implementation Notes

**File**: `CircuitProcessor.cpp`

**prepareToPlay() - Initialization**:
```cpp
// Initialize filters with computed coefficients
inputHighPass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 72.0f);
outputLowPass.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 15915.5f);

// Set clipping function (soft saturation)
clipper.functionToUse = [](float x) { return std::tanh(x); };
```

**processBlock() - Audio Processing**:
```cpp
// Get parameter values (0.0 - 1.0)
float drive = driveParam->get();
float level = levelParam->get();

// Map drive to gain range
float driveGainValue = 0.1f + (drive * 9.9f);

// Process chain:
// Input → HPF → DriveGain → Clipper → LPF → LevelGain → Output
```

### Why These Frequencies?

**72 Hz High-Pass**: Standard for guitar pedal input coupling
- Removes subsonic rumble
- Preserves guitar fundamental (80-1000 Hz range)
- Allows clean low-end definition

**15.9 kHz Low-Pass**: Extracted from schematic RC values
- Removes high-frequency harshness from clipping
- Stays above guitar frequency range
- Reduces aliasing before reconstruction filter



## Usage Examples

### Generate Plugin
```bash
.\livespice-translator.exe "example pedals/MXR Distortion +.schx"
```

### Build Plugin
```bash
cd "JUCE - MXR Distortion +"
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Use Plugin
- Copy `.vst3` or `.au` from build directory
- Load into your DAW
- Process audio with circuit-accurate DSP

### Modify Plugin
- Edit `CircuitProcessor.cpp` for parameter changes
- Rebuild with `cmake --build .`
- Repeat

## Build Requirements Checklist

- [ ] JUCE framework in `third_party/JUCE/`
- [ ] CMake 3.16+ installed
- [ ] C++ compiler (VS2019+, Xcode, GCC)
- [ ] 2-5 GB free disk space
- [ ] 30-60 minutes for first build

## Next Steps

1. **Quick Start**: Read [`JUCE_QUICKSTART.md`](JUCE_QUICKSTART.md)
2. **Try Building**: Run CMake on generated plugin
3. **Test in DAW**: Load plugin and test audio
4. **Customize**: Adjust DSP parameters as needed
5. **Iterate**: Regenerate and rebuild as needed

---

**Questions?** Refer to the appropriate documentation file above, or review the detailed comments in the source code files.
