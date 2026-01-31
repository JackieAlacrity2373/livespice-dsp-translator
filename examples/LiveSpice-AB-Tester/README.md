# LiveSpice A/B Testing Suite

A JUCE-based audio plugin comparison tool for validating LiveSpice circuit translations.

## Important Note

**Phase 1**: This version compares two standalone executable plugins side-by-side.  
For VST hosting capabilities, use your DAW to load both plugins and compare there.

## Features

### Core Functionality
- **Dual Plugin Execution**: Run two standalone plugins simultaneously
  - **Plugin A**: First translation to compare
  - **Plugin B**: Second translation or variant to compare
  
- **Unified Control Panel**: Single set of controls that adjusts both plugins in real-time
  - Automatic parameter discovery from loaded plugins
  - Synchronized parameter changes across both instances
  - Visual parameter values with sliders

- **A/B Switching**: Instant toggling between plugins during playback
  - Seamless switching without audio glitches
  - Visual indication of active plugin
  - Keyboard shortcut support (Spacebar to toggle)

- **Real-Time Audio Processing**: 
  - Low-latency audio routing
  - Supports stereo input/output
  - Configurable sample rates and buffer sizes

### Testing Workflow

1. **Load Plugins**:
   - Click "Load LiveSpice VST (A)" to select the original circuit VST
   - Click "Load Generated Plugin (B)" to select the translated JUCE plugin

2. **Configure Parameters**:
   - Adjust any parameter using the unified control panel
   - Changes apply to both plugins simultaneously
   - Visual feedback shows current values

3. **Compare Output**:
   - Toggle A/B switch to hear differences
   - Switch instantly during playback for direct comparison
   - Monitor active plugin in status bar

4. **Analysis**:
   - Listen for frequency response differences
   - Check for gain/level matching
   - Verify clipping/distortion characteristics

## Build Instructions

### Prerequisites
- CMake 3.16 or later
- Visual Studio 2022 Build Tools (Windows)
- JUCE v7.x (already included in `third_party/JUCE`)

### Windows Build

```powershell
cd "LiveSpice-AB-Tester"
mkdir build
cd build

# Configure
& "C:\Program Files\CMake\bin\cmake.exe" .. -G "Visual Studio 17 2022" -A x64

# Build Release
& "C:\Program Files\CMake\bin\cmake.exe" --build . --config Release

# Executable location
.\LiveSpice_AB_Tester_artefacts\Release\LiveSpice A/B Tester.exe
```

### Quick Build Script

```powershell
# From workspace root
cd "LiveSpice-AB-Tester"

if (Test-Path build) { Remove-Item -Recurse -Force build }
mkdir build
cd build

& "C:\Program Files\CMake\bin\cmake.exe" .. -G "Visual Studio 17 2022" -A x64
& "C:\Program Files\CMake\bin\cmake.exe" --build . --config Release

# Run
Start-Process ".\LiveSpice_AB_Tester_artefacts\Release\LiveSpice A/B Tester.exe"
```

## Usage Guide

### Supported Plugin Formats

**Plugin A (LiveSpice):**
- VST (.dll)
- VST3 (.vst3)

**Plugin B (Generated):**
- VST3 (.vst3)
- Standalone (.exe) - *Note: Standalone hosting requires VST3 export*

### Parameter Mapping

The control panel automatically detects parameters from Plugin A and attempts to map them to Plugin B:
- Parameters matched by exact name
- Values normalized to 0.0-1.0 range
- Unmapped parameters show warning indicators

### Keyboard Shortcuts

- **Spacebar**: Toggle A/B switch
- **Ctrl+A**: Load Plugin A
- **Ctrl+B**: Load Plugin B
- **Ctrl+R**: Reset all parameters to defaults

## Technical Details

### Architecture

```
┌─────────────────────────────────────────┐
│         MainComponent (UI)              │
│  ┌─────────┐  ┌──────────┐  ┌────────┐ │
│  │ Plugin  │  │ Control  │  │   A/B  │ │
│  │ Loaders │  │  Panel   │  │ Switch │ │
│  └─────────┘  └──────────┘  └────────┘ │
└─────────────────────────────────────────┘
           │              │
    ┌──────┴──────┐  ┌───┴────┐
    │ PluginHost  │  │ Plugin │
    │     A       │  │ Host B │
    └─────────────┘  └────────┘
           │              │
    ┌──────┴──────────────┴──────┐
    │    Audio Device Manager    │
    └────────────────────────────┘
```

### Audio Processing Flow

1. Input audio received from audio device
2. Buffer copied to temporary storage
3. Routed to active plugin (A or B based on switch)
4. Processed audio returned to output
5. Parameters synchronized every 50ms (20Hz timer)

### Parameter Synchronization

- Timer-based polling (50ms interval)
- Get all parameter values from ControlPanel
- Set values on both PluginHost A and B
- Handles missing parameters gracefully

## Troubleshooting

### Plugin Won't Load
- Verify plugin file exists and is valid VST/VST3
- Check file permissions
- Try running as administrator
- Review debug console for error messages

### Parameters Don't Match
- Plugins must have identical parameter names for auto-mapping
- Check parameter case sensitivity
- Manually verify parameter ranges in plugin editors

### Audio Glitches During Switching
- Increase buffer size in audio settings
- Ensure both plugins have same latency
- Check CPU usage in task manager

### No Sound Output
- Verify audio device is selected and configured
- Check plugin bypass states
- Confirm input/output routing in audio settings

## Development Notes

### Adding New Features

**Custom Parameter Mapping:**
Edit `ControlPanel::setParameterList()` to implement custom mapping rules

**Latency Compensation:**
Modify `PluginHost::processBlock()` to add delay compensation

**Spectrum Analyzer:**
Add `juce_dsp` FFT components to MainComponent for visual frequency comparison

## Known Limitations

1. **Standalone Plugin Hosting**: .exe standalone plugins cannot be directly hosted
   - Solution: Export as VST3 instead
   
2. **Parameter Name Matching**: Requires exact parameter name matches
   - Solution: Implement fuzzy matching or manual mapping UI

3. **Latency**: No automatic latency compensation
   - Workaround: Manually align recordings for comparison

## Future Enhancements

- [ ] Waveform/spectrum visualization
- [ ] Recording and export to WAV files
- [ ] Parameter preset save/load
- [ ] Automated difference analysis (THD, frequency response)
- [ ] Support for more plugin formats (AU, AAX)
- [ ] MIDI input for testing with keyboard/controller

## License

Part of the LiveSpice DSP Translation Layer project.
See main project README for licensing information.

## Related Projects

- **LiveSpice**: Original circuit simulation software
- **JUCE**: Cross-platform C++ audio framework
- **LiveSpice Translator**: Main translation layer project

---

**Version**: 1.0.0  
**Last Updated**: January 28, 2026  
**Author**: LiveSpice DSP Team
