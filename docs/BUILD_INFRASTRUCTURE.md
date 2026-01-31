# Build Infrastructure Documentation

**Date:** January 31, 2026  
**Status:** ✅ Fully Functional (CMake + Visual Studio)

---

## Overview

The LiveSpice DSP Translation Layer has a **complete, pre-configured build infrastructure** using CMake and Visual Studio. The system is already set up and functional.

---

## Current Build Status

### ✅ What's Already Working

```
✓ CMake configuration        - CMakeLists.txt fully configured
✓ Visual Studio solution     - LiveSpice_DSP_Translator.sln exists
✓ Compiled executable        - livespice-translator.exe (567 KB, generated 1/29/2026)
✓ MSVC projects             - All .vcxproj files generated
✓ Build directories         - build/ with Release, Debug, x64 subdirectories
✓ Compilation warnings      - /W4 (MSVC) and -Wall -Wextra (GCC/Clang) enabled
✓ C++17 standard            - Properly configured for modern C++
```

---

## File Structure

```
h:\Live Spice DSP translation layer\
├── CMakeLists.txt                      ← Main CMake configuration
├── livespice-translator.exe            ← Compiled executable (567 KB)
│
├── build/                              ← CMake-generated Visual Studio project
│   ├── LiveSpice_DSP_Translator.sln    ← Visual Studio Solution file
│   ├── livespice-translator.vcxproj    ← Main project file
│   ├── ALL_BUILD.vcxproj               ← Build all target
│   ├── ZERO_CHECK.vcxproj              ← CMake check target
│   ├── CMakeCache.txt                  ← CMake configuration cache
│   ├── cmake_install.cmake             ← Installation rules
│   ├── CMakeFiles/                     ← CMake generated files
│   ├── bin/                            ← Output binaries
│   ├── Release/                        ← Release build artifacts
│   └── x64/                            ← 64-bit platform files
│
└── [Source files]
    ├── Livespice_to_DSP.cpp            ← Main entry point
    ├── LiveSpiceParser.cpp
    ├── CircuitAnalyzer.cpp
    ├── CircuitVisualizer.cpp
    ├── CircuitDiagnostics.cpp
    ├── LiveSpiceConnectionMapper.cpp
    ├── JuceDSPGenerator.cpp
    └── TopologyPatterns.cpp
```

---

## CMake Configuration Details

**File:** [CMakeLists.txt](CMakeLists.txt)

### Key Settings

```cmake
cmake_minimum_required(VERSION 3.15)      # CMake 3.15 or higher required
project(LiveSpice_DSP_Translator)         # Project name
set(CMAKE_CXX_STANDARD 17)                # C++17 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)       # C++17 mandatory
```

### Compiler Flags

**MSVC (Visual Studio):**
```
/W4                                       ← Highest warning level
```

**GCC/Clang:**
```
-Wall -Wextra -Wpedantic                  ← Strict warnings
-lm (if not Apple)                        ← Link math library
```

### Source Files Compiled

```cmake
add_executable(livespice-translator
    Livespice_to_DSP.cpp
    LiveSpiceParser.cpp
    CircuitAnalyzer.cpp
    CircuitVisualizer.cpp
    CircuitDiagnostics.cpp
    LiveSpiceConnectionMapper.cpp
    JuceDSPGenerator.cpp
    TopologyPatterns.cpp
)
```

### Output Configuration

```cmake
RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_BINARY_DIR}/bin
Platform: Windows (.exe suffix added)
```

---

## Visual Studio Solution Details

**File:** [build/LiveSpice_DSP_Translator.sln](build/LiveSpice_DSP_Translator.sln)

### Projects in Solution

1. **ALL_BUILD** - Meta-target that builds everything
2. **ZERO_CHECK** - CMake configuration check (runs on each build)
3. **livespice-translator** - Main translator executable

### Build Configurations

```
Debug|x64               ← Debug symbols, no optimization
Release|x64             ← Optimized, no debug symbols
MinSizeRel|x64          ← Optimized for size
RelWithDebInfo|x64      ← Optimized + debug symbols
```

### Project Dependencies

```
ALL_BUILD
  └── depends on: ZERO_CHECK
  └── depends on: livespice-translator

livespice-translator
  └── depends on: ZERO_CHECK
```

---

## How to Build

### Method 1: Using Visual Studio IDE (Recommended for GUI)

```powershell
# 1. Open the solution file
"h:\Live Spice DSP translation layer\build\LiveSpice_DSP_Translator.sln"

# 2. In Visual Studio:
#    - Select Release|x64 configuration (top toolbar)
#    - Press Ctrl+Shift+B to build all
#    - Or: Build → Build Solution

# 3. Output will be in:
#    build\Release\bin\livespice-translator.exe
```

### Method 2: Using MSBuild (Command Line)

```powershell
cd "h:\Live Spice DSP translation layer\build"

# Build Release configuration
msbuild LiveSpice_DSP_Translator.sln /p:Configuration=Release /p:Platform=x64 /m:4

# Output:
# build\Release\livespice-translator.exe
```

### Method 3: Using CMake Directly (If Rebuilding from Scratch)

```powershell
cd "h:\Live Spice DSP translation layer"

# Generate Visual Studio project files (if needed)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Release

# Output:
# build\Release\livespice-translator.exe
```

### Method 4: Using G++ (Command Line - No Visual Studio)

```powershell
cd "h:\Live Spice DSP translation layer"

g++ -std=c++17 -Wall -Wextra -O2 `
    Livespice_to_DSP.cpp `
    LiveSpiceParser.cpp `
    CircuitAnalyzer.cpp `
    CircuitVisualizer.cpp `
    CircuitDiagnostics.cpp `
    LiveSpiceConnectionMapper.cpp `
    JuceDSPGenerator.cpp `
    TopologyPatterns.cpp `
    -o livespice-translator.exe `
    -lm

# Output:
# livespice-translator.exe (in current directory)
```

---

## Running the Translator

### Current Executable (Already Built)

```powershell
cd "h:\Live Spice DSP translation layer"

# Generate JUCE plugin from LiveSpice schematic
.\livespice-translator.exe "example pedals/MXR Distortion +.schx"

# Output:
# JUCE - MXR Distortion +/  (new directory with plugin source code)
```

### Generate Multiple Circuits

```powershell
# Boss Super Overdrive SD-1
.\livespice-translator.exe "example pedals/Boss Super Overdrive SD-1.schx"

# Boss DS-1 (if available)
.\livespice-translator.exe "example pedals/Boss DS-1.schx"

# Fuzz Face (if available)
.\livespice-translator.exe "example pedals/Fuzz Face.schx"
```

### List Available Example Circuits

```powershell
Get-ChildItem "example pedals" -Filter "*.schx" | Select-Object Name
```

---

## Build Requirements

### Software

```
✓ CMake 3.15+                    - Configuration tool
✓ Visual Studio 2019+            - IDE and compiler
  OR
✓ Build Tools for Visual Studio  - Command-line compiler
  OR
✓ G++/Clang                      - Alternative compilers
```

### Check Your System

```powershell
# Check CMake
cmake --version

# Check Visual Studio / MSBuild
where msbuild

# Check G++ (if using that)
where g++
```

---

## Compilation Statistics

### Executable Details

| Property | Value |
|----------|-------|
| **Filename** | livespice-translator.exe |
| **Size** | 567 KB |
| **Last Built** | January 29, 2026, 6:29 PM |
| **Location** | h:\Live Spice DSP translation layer\ |
| **Platform** | Windows x64 |
| **C++ Standard** | C++17 |

### Source Files Compiled

| File | Purpose |
|------|---------|
| Livespice_to_DSP.cpp | Main entry point, CLI handling |
| LiveSpiceParser.cpp | XML parsing and component extraction |
| CircuitAnalyzer.cpp | Topology analysis and stage detection |
| CircuitVisualizer.cpp | Debug visualization output |
| CircuitDiagnostics.cpp | Diagnostic reporting |
| LiveSpiceConnectionMapper.cpp | Connection analysis |
| JuceDSPGenerator.cpp | JUCE plugin code generation |
| TopologyPatterns.cpp | Circuit pattern matching |

---

## Incremental Builds

### Rebuild Specific Changes

If you've modified source files, rebuild with:

```powershell
# Using MSBuild
cd "h:\Live Spice DSP translation layer\build"
msbuild livespice-translator.vcxproj /p:Configuration=Release

# Using CMake
cmake --build build --config Release --target livespice-translator
```

### Clean Build

```powershell
# Remove all generated files
cd "h:\Live Spice DSP translation layer"
Remove-Item -Recurse -Force build

# Regenerate and rebuild
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

---

## Troubleshooting

### Build Fails with "MSBuild not found"

**Solution:** Install Build Tools for Visual Studio 2022
- Download: https://visualstudio.microsoft.com/downloads/
- Select: "Build Tools for Visual Studio 2022"
- During install: Check "Desktop development with C++"

### Build Fails with "CMake not found"

**Solution:** Install CMake
- Download: https://cmake.org/download/
- Choose: Windows x64 Installer
- Run installer and add CMake to PATH

### Compilation Errors in C++17 Features

**Check:** Your compiler supports C++17
- MSVC: Visual Studio 2017 Update 5 or later
- GCC: Version 7.0 or later
- Clang: Version 5.0 or later

### "livespice-translator.exe not found" After Build

**Check:** Build output directory
```powershell
# If using Visual Studio:
# build\Release\livespice-translator.exe

# If using G++:
# livespice-translator.exe (in root directory)

# Check cmake configuration
cmake -B build -G "Visual Studio 17 2022" -A x64
```

---

## Integration with VS Code Build Tasks

The workspace already includes build tasks in [.vscode/tasks.json](.vscode/tasks.json):

```json
{
    "label": "build (g++)",
    "type": "shell",
    "command": "g++",
    "args": ["-std=c++17", "-Wall", "-Wextra", "-O2", 
             "*.cpp", "-o", "livespice-translator", "-lm"],
    "group": {"kind": "build", "isDefault": true}
},
{
    "label": "build & run",
    "type": "shell",
    "command": "bash",
    "args": ["-c", "g++ ... && ./livespice-translator"]
}
```

Press **Ctrl+Shift+B** in VS Code to run the build task.

---

## Next Steps

Now that the build infrastructure is documented, proceed with:

1. **Phase 2 Implementation**: Topology Pattern Matching (in progress)
   - Enhanced pattern detection system
   - 15+ circuit topologies
   - Smart DSP strategy selection

2. **Testing**: Build and run with example circuits
   ```powershell
   .\livespice-translator.exe "example pedals/MXR Distortion +.schx"
   ```

3. **Integration**: Add new patterns to [TopologyPatterns.h](TopologyPatterns.h)

4. **Validation**: Generate plugins and verify pattern matching

---

## Summary

✅ **Build infrastructure is complete and functional**
- CMake configuration ready
- Visual Studio solution generated
- Executable compiled (567 KB)
- Multiple build methods available
- Ready for pattern matching enhancement

**No additional tools need to be installed** - the system is ready to build and test immediately.
