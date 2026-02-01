# JUCE Plugin Generation and Build Guide

## Current Status 

The LiveSpice DSP translation layer **automatically generates and compiles** complete JUCE audio plugins from LiveSpice circuit schematics. Plugins are fully functional with working parametric DSP chains.

**Example**: MXR Distortion+ plugin with Drive/Level controls, input HPF @ 72 Hz, tanh clipping, and output LPF @ 15.9 kHz.

## Generated Plugin Structure

When you run the translator on a LiveSpice file, it creates:
```
JUCE - [Circuit Name]/
├── CircuitProcessor.h       # JUCE AudioProcessor with DSP chain
├── CircuitProcessor.cpp     # Complete DSP implementation
├── CMakeLists.txt          # Build configuration with all JUCE modules
└── build/                   # Build output (created during build)
```

## Quick Build (5 Minutes)

### Generate a Plugin

```powershell
.\livespice-translator.exe "example pedals/MXR Distortion +.schx"
```

### Build the Plugin

```powershell
cd "JUCE - MXR Distortion +"
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Run

```
build\MXR_Distortion___artefacts\Release\Standalone\MXR Distortion +.exe
```

## Build Requirements

**Windows**:
- CMake 3.16+ → https://cmake.org/
- Visual Studio 2019+ (Build Tools sufficient)
- C++17 compiler
- 2-5 GB disk space

**macOS**:
- CMake 3.16+
- Xcode command line tools
- 2-5 GB disk space

**Linux**:
- CMake 3.16+
- GCC or Clang with C++17
- ALSA development headers

## Build Steps (Detailed)

### Prerequisites

1. **Verify CMake installation**:
   ```powershell
   cmake --version
   ```

2. **Check JUCE location**:
   ```
   h:\Live Spice DSP translation layer\third_party\JUCE\
   ```

3. **Ensure C++ compiler in PATH**:
   ```powershell
   # Windows (MSVC)
   cl.exe /?
   # Or GCC
   g++ --version
   ```

### Building

```powershell
# Navigate to generated plugin
cd "JUCE - [Your Circuit]"

# Create build directory
mkdir build
cd build

# Configure (CMake reads CMakeLists.txt)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build (uses CMakeFiles from previous step)
cmake --build . --config Release

# Wait ~2-5 minutes for compilation
```

### Output Locations

**Windows**:
- Standalone: `build\[Name]___artefacts\Release\Standalone\[Name].exe`
- VST3: `build\[Name]___artefacts\Release\VST3\`

**macOS**:
- Standalone: `build/[Name]___artefacts/Release/Standalone/[Name].app`
- AU: `build/[Name]___artefacts/Release/AU/`
- VST3: `build/[Name]___artefacts/Release/VST3/`
- AAX: `build/[Name]___artefacts/Release/AAX/`

**Linux**:
- Standalone: `build/[Name]___artefacts/Release/Standalone/[Name]`
- VST3: `build/[Name]___artefacts/Release/VST3/`

## Generated Code

### CircuitProcessor.h

Defines the JUCE AudioProcessor with:
- `apvts` - AudioProcessorValueTreeState for parameters
- DSP module members: `inputHighPass`, `driveGain`, `clipper`, `outputLowPass`, `levelGain`
- Standard JUCE AudioProcessor methods

### CircuitProcessor.cpp

Implements the complete DSP chain:

**createParameterLayout()**:
- Creates Drive parameter (0-1, default 0.5)
- Creates Level parameter (0-1, default 0.5)

**prepareToPlay()**:
- Initializes all 5 DSP stages
- Sets filter coefficients based on sample rate
- Configures clipping function (tanh soft saturation)

**processBlock()**:
```cpp
// Signal flow:
// Input → HPF → DriveGain → Clipper → LPF → LevelGain → Output
inputHighPass.process(context);      // 72 Hz HPF
driveGain.process(context);          // 0.1x-10x gain
clipper.process(context);            // tanh(x) soft clip
outputLowPass.process(context);      // 15.9 kHz LPF
levelGain.process(context);          // Output volume control
```

### CMakeLists.txt

Build configuration includes:
- JUCE path resolution
- Plugin format definitions (VST3, AU, AAX, Standalone)
- Compiler flags (C++17, optimization)
- All required JUCE module linking

## DSP Chain Details

See **DSP Filters & Processing Chain** section in [SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md) for:
- 5-stage signal flow diagram
- Filter technical specifications
- Parameter mapping and ranges
- Implementation notes

## Customization

### Modify Filter Frequencies

Edit `CircuitProcessor.cpp` in `prepareToPlay()`:

```cpp
// Change input HPF from 72 Hz to 50 Hz
inputHighPass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 50.0f);

// Change output LPF from 15.9 kHz to 20 kHz
outputLowPass.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.0f);
```

### Adjust Clipping Algorithm

Modify the clipping function:

```cpp
// Change from soft tanh to hard clipping
clipper.functionToUse = [](float x) {
    return juce::jlimit(-1.0f, 1.0f, x);  // Hard clip at ±1.0
};
```

### Rebuild After Changes

```powershell
cd build
cmake --build . --config Release
```

## Troubleshooting

### CMake Configuration Fails

**Error**: "Could not find JUCE"
- Verify `../third_party/JUCE/` exists
- Check JUCE path in CMakeLists.txt
- Try absolute path if relative path fails

### Compilation Errors

**C2248**: Cannot access private member
- Check filter coefficient access uses public `coefficients` member
- Avoid accessing private `.state` directly

**C2440**: Type conversion error
- Ensure filter coefficients are properly cast
- Use `makeHighPass(rate, freq)` not state assignment

**LNK2019**: Unresolved external symbol
- Missing JUCE modules in CMakeLists.txt
- Add `juce::juce_audio_devices` and `juce::juce_audio_utils` for standalone builds

### Plugin Won't Load

1. Check architecture (32-bit vs 64-bit)
2. Verify plugin format (VST3, AU, etc.)
3. Confirm correct output directory
4. Check DAW plugin search paths

## Regenerating Plugins

To update a plugin with new circuit analysis:

```powershell
# Re-run translator (overwrites existing files)
.\livespice-translator.exe "example pedals/Your Circuit.schx"

# Rebuild
cd "JUCE - Your Circuit\build"
cmake --build . --config Release
```

## Next Steps

1. **Test in DAW** - Load plugin and verify audio processing
2. **Adjust Parameters** - Modify Drive/Level controls
3. **Fine-tune DSP** - Adjust filter frequencies in CircuitProcessor.cpp
4. **Add UI** - Create custom editor using JUCE graphics
5. **Package** - Create installer for distribution

## Resources

- [JUCE Framework](https://juce.com/)
- [CMake Documentation](https://cmake.org/cmake/help/latest/)
- [JUCE DSP Module](https://docs.juce.com/master/group__juce__dsp.html)
- [JUCE Audio Processors](https://docs.juce.com/master/classAudioProcessor.html)
