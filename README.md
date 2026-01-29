# LiveSpice to DSP Translation Layer

A C++ framework for parsing LiveSpice guitar pedal schematics and translating them into circuit-accurate DSP components for JUCE audio plugins.

## Current Status ✅

**Working**: Translation layer + Auto-generated JUCE plugins with full DSP implementation
- LiveSpice XML parser (component extraction, connectivity analysis)
- Circuit topology analyzer (stage identification)
- JUCE plugin code generator (complete with parametric DSP chain)
- Standalone plugins building and functional
- MXR Distortion+ plugin: ~4.4 MB standalone executable

## Condensed Documentation Map (Start Here)

- **Project overview & quick start:** README.md (this file)
- **System architecture:** SYSTEM_ARCHITECTURE.md
- **Plugin build details:** JUCE_PLUGIN_BUILD_GUIDE.md
- **A/B Tester module:** LiveSpice-AB-Tester/README.md
- **Component analysis index:** README_DOCUMENTATION.md

## Quick Start (5 Minutes)

### 1. Generate a Plugin
```powershell
cd "h:\Live Spice DSP translation layer"
.\livespice-translator.exe "example pedals/MXR Distortion +.schx"
```

### 2. Build the Plugin
```powershell
cd "JUCE - MXR Distortion +"
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 3. Test the Plugin
Find the executable at:
```
build\MXR_Distortion___artefacts\Release\Standalone\MXR Distortion +.exe
```

## Quick Reference (Condensed)

### Build Translator
```powershell
g++ -std=c++17 -Wall -Wextra -O2 Livespice_to_DSP.cpp LiveSpiceParser.cpp CircuitAnalyzer.cpp CircuitVisualizer.cpp CircuitDiagnostics.cpp LiveSpiceConnectionMapper.cpp JuceDSPGenerator.cpp -o livespice-translator -lm
```

### Key Paths
- Schematics: `example pedals/*.schx`
- Generated plugins: `JUCE - [Circuit Name]/`
- JUCE framework: `third_party/JUCE/`

### VST3 Output (Windows)
```
JUCE - [Circuit Name]\build\[Name]___artefacts\Release\VST3\
```

## Project Structure

```
├── Livespice_to_DSP.cpp           # Main translator program
├── LiveSpiceParser.h/.cpp         # XML parsing & data structures
├── CircuitAnalyzer.h/.cpp         # Circuit analysis & stage detection
├── CircuitVisualizer.h/.cpp       # Debug visualization output
├── JuceDSPGenerator.h/.cpp        # JUCE code generation
├── example pedals/                # Test schematic files (.schx)
├── JUCE - [Circuit Name]/         # Auto-generated plugin directories
└── third_party/JUCE/              # JUCE framework
```

## System Architecture

For detailed documentation, see [SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md).

```
LiveSpice Schematic (.schx) → LiveSpiceParser → CircuitAnalyzer → JuceDSPGenerator → JUCE Plugin
```

## Compilation (Translator)

### Using MinGW (Windows)
```powershell
g++ -std=c++17 -Wall -Wextra -O2 Livespice_to_DSP.cpp LiveSpiceParser.cpp CircuitAnalyzer.cpp CircuitVisualizer.cpp CircuitDiagnostics.cpp LiveSpiceConnectionMapper.cpp JuceDSPGenerator.cpp -o livespice-translator -lm
```

### Using VS Code
1. Press `Ctrl+Shift+B` to open build menu
2. Select "build (g++)" task
3. Run with `.\livespice-translator.exe`

## Building Generated Plugins

See [JUCE_PLUGIN_BUILD_GUIDE.md](JUCE_PLUGIN_BUILD_GUIDE.md) for:
- Installation requirements
- Step-by-step build instructions
- Plugin customization
- Troubleshooting

## DSP Implementation

Generated plugins include a complete 5-stage DSP chain. See the **DSP Filters & Chain** section in [SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md) for details.

## Testing

Example circuits in `example pedals/`:
- `MXR Distortion +.schx` - Simple distortion (recommended)
- `Boss Super Overdrive SD-1.schx` - Multi-stage
- `Marshall Blues Breaker.schx` - Complex
- `Common Cathode Triode Amplifier.schx` - Tube circuit
- `Common Emitter Transistor Amplifier.schx` - Transistor circuit

## Dependencies

**For Translator**:
- C++17 compiler
- Standard library only

**For Plugin Building**:
- CMake 3.16+
- JUCE framework
- C++17 compiler
- Platform SDKs (Visual Studio, Xcode, etc.)

## Known Limitations

- Linear circuit analysis only (resistors, capacitors, ideal op-amps)
- Basic stage identification heuristics
- No implicit feedback loop solving

## Future Improvements

- Non-linear component modeling (diode Shockley, transistor models)
- Wave Digital Filter (WDF) generation
- State-space extraction
- More sophisticated topology pattern matching
- Temperature compensation

## References

- [LiveSpice](https://www.livespice.org/)
- [JUCE DSP Module](https://docs.juce.com/master/group__juce__dsp.html)
- [Virtual Analog Modeling](https://www.willpirkle.com/)
- [Wave Digital Filters](https://www.dsprelated.com/freebooks/filters/)
