# Quick Start Guide

## For Immediate Testing (5 Minutes)

The LiveSpice DSP translation layer is fully functional with working JUCE plugins.

### Step 1: Generate a Plugin

```powershell
cd "h:\\Live Spice DSP translation layer"
.\\livespice-translator.exe "example pedals/MXR Distortion +.schx"
```

This creates a directory: `JUCE - MXR Distortion +` with all plugin source code.

### Step 2: Build the Plugin

```powershell
cd "JUCE - MXR Distortion +"
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Step 3: Run the Plugin

**Standalone (Windows)**:
```
build\\MXR_Distortion___artefacts\\Release\\Standalone\\MXR Distortion +.exe
```

**In Your DAW**:
- VST3: Copy from `build\\...\\VST3\\Release`
- AU (macOS): Copy from `build\\...\\AU\\Release`
- AAX (macOS): Copy from `build\\...\\AAX\\Release`

## What You Get

✅ Working distortion effect with:
- Drive control (0-100%, ranges 0.1x to 10x gain)
- Level control (0-100%, default 50%)
- Input high-pass filter @ 72 Hz (DC blocking)
- Op-amp soft clipping (tanh)
- Output low-pass filter @ 15.9 kHz (tone shaping)

## Build Requirements

- **CMake** 3.16+: https://cmake.org/
- **JUCE Framework**: Already in `third_party/JUCE/`
- **C++ Compiler**: MSVC 2019+, GCC, or Clang (C++17 support)
- **Visual Studio** (Windows): For compiler + MSBuild

## Alternative Circuits

Generate and build other circuits:

```powershell
# Boss Super Overdrive
.\\livespice-translator.exe "example pedals/Boss Super Overdrive SD-1.schx"

# Marshall Blues Breaker
.\\livespice-translator.exe "example pedals/Marshall Blues Breaker.schx"
```

## Troubleshooting

**CMake not found**: Install from https://cmake.org/

**Compiler errors**: Ensure C++17 support installed

**Build still failing**: Check [JUCE_PLUGIN_BUILD_GUIDE.md](JUCE_PLUGIN_BUILD_GUIDE.md) for detailed help

## Next Steps

- Read [SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md) to understand the signal chain
- Modify DSP parameters in `CircuitProcessor.cpp`
- Rebuild with `cmake --build .`
- Try it in your DAW!
