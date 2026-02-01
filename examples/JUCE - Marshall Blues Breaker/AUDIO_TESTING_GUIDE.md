# 🔬 Audio Oscilloscope & Real-Time Analysis Guide

## Overview
We now have two powerful tools for analyzing the Marshall Blues Breaker audio output **without manual listening tests**:

1. **`audio_capture`** - Generates standardized test signals
2. **`audio_oscilloscope`** - Analyzes WAV files with detailed diagnostics

## Workflow

### Step 1: Generate Test Signal
```bash
.\audio_capture generate test_sweep.wav
```
Creates a 5-second frequency sweep (1 kHz → 5 kHz) at low amplitude (-18 dB).

**What it contains:**
- Chirp signal sweeping from 1000 Hz to 5000 Hz
- Constant amplitude for consistent response measurement
- Stereo signal at 44.1 kHz sample rate

### Step 2: Process Through Plugin

**Option A: Use Your Guitar Signal** (What we want to do)
1. Play the test signal into the Marshall Blues Breaker plugin
2. Record the output using:
   - Audacity (Free, open-source)
   - REAPER (Professional)
   - OBS Studio (Streaming software with audio recording)
   - Any DAW (Ableton, Logic, Cubase, etc.)
3. Save output as `plugin_output.wav`

**Option B: Use Generated Test Signal**
1. Open Marshall Blues Breaker.exe
2. Route `test_sweep.wav` → Plugin Input
3. Record output from plugin

### Step 3: Analyze Output

```bash
.\audio_oscilloscope plugin_output.wav
```

### Example Output Analysis

```
======================================================================
🔬 AUDIO OSCILLOSCOPE & ANALYZER
======================================================================

📊 BASIC STATISTICS
----------------------------------------------------------------------
  Duration:           5.003 seconds
  Sample Rate:        44100 Hz
  Peak Level:         0.235467 (-12.58 dB)
  RMS Level:          0.045230 (-26.89 dB)
  Min Level:          -0.314159
  DC Offset:          0.00001234
  Crest Factor:       5.205x

🔴 CLIPPING DETECTION
----------------------------------------------------------------------
  Hard Clipping (>99%):    0 samples (0.0000%)
  Soft Clipping (95-99%):  0 samples (0.0000%)
  ✅ No significant clipping detected

📈 FREQUENCY ANALYSIS (FFT)
----------------------------------------------------------------------
  Estimated Fundamental:  2500.0 Hz
  Frequency Distribution:
    Low (<500 Hz):        15.3%
    Mid (500 Hz-5 kHz):   72.4%
    High (>5 kHz):        12.3%

🎵 HARMONIC DISTORTION ANALYSIS
----------------------------------------------------------------------
  Average Peak:           0.2354
  Estimated THD:          8.34%
  🎸 High distortion (typical for overdrive)

🔊 NOISE FLOOR ANALYSIS
----------------------------------------------------------------------
  Noise Floor Level:      0.000002 (-128.96 dB)
  Signal-to-Noise Ratio:  48.82 dB
  ✅ Good SNR - acceptable for audio

📊 WAVEFORM VISUALIZATION (first 512 samples)
----------------------------------------------------------------------
 #                    #                    #                    #      1.0
 #                    #                    #                    #      0.9
 ##                  ##                   ##                   ##      0.8
 #                    #                    #                    #      0.7
 ##                  ##                   ##                   ##      0.6
-#--------------------#--------------------#--------------------#---  0.5
 #                    #                    #                    #      0.4
 ##                  ##                   ##                   ##      0.3
 #                    #                    #                    #      0.2
                                                                        0.1
                                                                        0.0

⭐ OVERALL QUALITY ASSESSMENT
----------------------------------------------------------------------
  Audio Quality Score:    95/100
  Grade:                  🟢 A (Very Good)

  ✅ No issues detected!
```

## What Each Metric Means

### Peak Level (dB)
- **-12 dB to -6 dB**: Ideal for audio pedals
- **-6 dB to 0 dB**: Loud, may clip on high transients
- **0 dB or higher**: Digital clipping will occur

### RMS Level (dB)
- **-30 to -20 dB**: Good dynamic range
- **-20 to -10 dB**: Typical for processed audio
- **Above -10 dB**: Potentially too loud

### Clipping Detection
- **Hard Clipping**: Samples at 99%+ amplitude (bad!)
- **Soft Clipping**: Samples at 95-99% (usually acceptable for overdrive)
- **No Clipping**: Perfect!

### THD (Total Harmonic Distortion)
- **< 5%**: Very clean (clean boost, mild tone shaping)
- **5-15%**: Slight distortion (typical for light overdrive)
- **15-50%**: Moderate distortion (typical for heavy overdrive)
- **> 50%**: Extreme distortion or noise

### Signal-to-Noise Ratio (SNR)
- **> 60 dB**: Studio quality
- **40-60 dB**: Good quality
- **20-40 dB**: Acceptable
- **< 20 dB**: Poor, lots of noise

### Crest Factor
- **3-6x**: Normal for audio
- **> 10x**: Very dynamic (peaks are much higher than RMS)

## Testing Scenarios

### Scenario 1: Test with Guitar Input
**Goal**: See how the plugin responds to real guitar signal

1. Connect guitar to computer audio input
2. Open Marshall Blues Breaker.exe
3. Play guitar through plugin
4. Record output: `.\record_guitar_output.wav` (use Audacity)
5. Analyze: `.\audio_oscilloscope record_guitar_output.wav`

### Scenario 2: Test Different Settings
**Goal**: Compare plugin behavior with different knob positions

1. Generate test signal: `.\audio_capture generate test_1.wav`
2. Process through plugin with settings A
3. Record as: `output_settings_A.wav`
4. Repeat with different settings (B, C, D)
5. Analyze all: `.\audio_oscilloscope output_settings_A.wav`
6. Compare results

### Scenario 3: Measure Gain Structure
**Goal**: Verify gain is reasonable

1. Generate quiet test signal at -30 dB
2. Process through plugin
3. Check peak level in output
4. Calculate gain: output_dB - input_dB = gain_dB

## Integration with Pedal Testing

**When user says audio has issues:**
- Generate test signal: `.\audio_capture generate issue_test.wav`
- Process through plugin
- Analyze: `.\audio_oscilloscope issue_test.wav`
- Identify problem from metrics (clipping, DC offset, etc.)
- Fix code issue
- Re-test programmatically

**This replaces manual A/B listening!**

## Quick Reference: What to Look For

| Issue | Metric to Check | Expected Value |
|-------|-----------------|-----------------|
| Too loud | Peak Level | Should be -12dB or lower |
| Clipping | Hard Clipping % | Should be 0% |
| Harshness | THD | Should be 5-15% for overdrive |
| Noise | SNR | Should be > 40 dB |
| DC offset | DC Offset | Should be < 0.001 |
| Signal too quiet | Peak Level | Should be > -24 dB |

## File Formats Supported

- **Input**: 16-bit or 32-bit PCM WAV files
- **Sample Rates**: Any (auto-detected)
- **Channels**: Mono or Stereo (averaged for analysis)

## Troubleshooting

### "Failed to open file"
- Check file path and name spelling
- Ensure WAV file exists in current directory
- Use full path if in different directory

### "Not a valid WAV file"
- File must be WAV format (not MP3, FLAC, etc.)
- Use Audacity to convert if needed

### Analysis shows unexpected results
- Check if plugin is processing correctly
- Verify input signal is at proper level
- Generate fresh test signal

## Next Steps

1. **Generate test sweep**: `.\audio_capture generate test_sweep.wav`
2. **Play through plugin and record output**
3. **Analyze results**: `.\audio_oscilloscope plugin_output.wav`
4. **Share the analysis output** and I'll diagnose the audio quality automatically
5. **Make code adjustments** based on findings
6. **Re-test** programmatically (no manual listening needed!)

---

**Ready to test?** Process your guitar signal through the plugin and save the output as a WAV file, then I'll analyze it completely!
