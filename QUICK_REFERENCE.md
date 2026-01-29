# Quick Reference (Condensed)

Full details are in README.md and JUCE_PLUGIN_BUILD_GUIDE.md.

## Generate Plugin
```powershell
.\livespice-translator.exe "example pedals/MXR Distortion +.schx"
```

## Build Plugin
```powershell
cd "JUCE - MXR Distortion +"
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Output Paths
- Standalone: `build\[Name]___artefacts\Release\Standalone\[Name].exe`
- VST3: `build\[Name]___artefacts\Release\VST3\`# Quick Reference

## Commands

### Generate Plugin
```powershell
.\livespice-translator.exe "example pedals/MXR Distortion +.schx"
```

### Build Plugin
```powershell
cd "JUCE - MXR Distortion +\build"
cmake ..
cmake --build . --config Release
```

### Run Plugin (Standalone)
```powershell
.\MXR_Distortion___artefacts\Release\Standalone\MXR Distortion +.exe
```

### Build Translator
```bash
g++ -std=c++17 -Wall -Wextra -O2 Livespice_to_DSP.cpp LiveSpiceParser.cpp CircuitAnalyzer.cpp CircuitVisualizer.cpp CircuitDiagnostics.cpp LiveSpiceConnectionMapper.cpp JuceDSPGenerator.cpp -o livespice-translator -lm
```

## Key Paths

| Item | Path |
|------|------|
| Schematics | `example pedals/*.schx` |
| JUCE Framework | `third_party/JUCE/` |
| Generated Plugins | `JUCE - [Name]/` |
| Build Outputs | `JUCE - [Name]/build/[Name]___artefacts/Release/` |
| Plugin Code | `JUCE - [Name]/CircuitProcessor.h/.cpp` |

## Plugin Output Locations

**Standalone**:
```
build\[Name]___artefacts\Release\Standalone\[Name].exe
```

**VST3** (Windows):
```
build\[Name]___artefacts\Release\VST3
```

**AU/AAX** (macOS):
```
build/[Name]___artefacts/Release/AU/
build/[Name]___artefacts/Release/AAX/
```

## DSP Parameters (MXR Distortion+)

| Parameter | Range | Default | Effect |
|-----------|-------|---------|--------|
| Drive | 0-100% | 50% | Boost into clipping (0.1x-10x) |
| Level | 0-100% | 50% | Output volume |

## Filter Frequencies

| Filter | Frequency | Purpose |
|--------|-----------|---------|
| Input HPF | 72 Hz | DC blocking, coupling |
| Output LPF | 15.9 kHz | Tone shaping |

## Troubleshooting

| Issue | Solution |
|-------|----------|
| CMake not found | Install from https://cmake.org/ |
| Compiler errors | Ensure C++17 support |
| Plugin won't load | Check architecture (32/64-bit) match |
| Build fails | Check [JUCE_PLUGIN_BUILD_GUIDE.md](JUCE_PLUGIN_BUILD_GUIDE.md) |

## Documentation Files

- **[README.md](README.md)** - Project overview
- **[QUICK_START.md](QUICK_START.md)** - 5-minute setup guide
- **[SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md)** - How everything works (includes DSP chain details)
- **[JUCE_PLUGIN_BUILD_GUIDE.md](JUCE_PLUGIN_BUILD_GUIDE.md)** - Detailed build help
- **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - This file (commands & quick facts)
