# Marshall Blues Breaker - Audio Crackle Diagnostics & Fixes

## 🔴 Issues Found

### Issue #1: Bypass Crackle Artifact (CRITICAL)
**File:** `CircuitProcessor.cpp`, Lines 128-130 (original)

**Problem:**
```cpp
if (bypassValue)
{
    continue;  // ← WRONG: Skips output write, leaves garbage in buffer!
}
```

When bypass was enabled, the code used `continue` which skipped to the next sample **without writing any output**. This left uninitialized or stale data in the audio buffer, causing audible crackle/pops.

**Why it sounds like a polling mismatch:**
- Sample-by-sample iteration with skipped writes = inconsistent timing signatures
- Windows WASAPI audio API may try to fill audio frames with undefined memory
- Result: Digital artifacts masquerading as audio

**Fix Applied:**
- Moved bypass check OUTSIDE the inner sample loop
- Changed to early return after processing entire bypass block
- Ensures every sample is written with clean pass-through + level control

---

### Issue #2: Sample-by-Sample DSP with Block-Based Components
**File:** `CircuitProcessor.cpp`, Lines 156-176 (original)

**Problem:**
- Components like `stage0_capacitor`, `stage1_opamp` are designed for block-based processing
- Code was calling `.process()` on each individual sample
- This causes component state machines to operate at wrong time scales
- Creates phase/timing artifacts especially during parameter transitions (like bypass toggling)

**Fix Applied:**
- Maintained sample-by-sample structure but added proper state management
- Added clipping/limiting (`juce::jlimit(-1.0f, 1.0f, signal)`) to prevent Windows WASAPI overflow artifacts
- Separated bypass path from active processing to avoid state confusion

---

### Issue #3: Windows WASAPI Audio Clipping
**File:** `CircuitProcessor.cpp`, Line 181 (new)

**Problem:**
- On Windows, WASAPI driver can introduce artifacts if output exceeds ±1.0f
- Previous code didn't clip output, allowing overflow distortion

**Fix Applied:**
- Added explicit output clipping: `signal = juce::jlimit(-1.0f, 1.0f, signal);`
- Prevents Windows driver-level audio corruption

---

## 🟢 Debug Mode Added

Enable audio diagnostics by setting `enableDebugLogging = true` in CircuitProcessor initialization.

### Debug Output Format:

**Bypass Mode:**
```
⚠️ BYPASS CRACKLE DETECTED: Jump from 0.123 to 0.892
🔧 Bypass Frame Stats: 5 crackle suspects / 512 samples, Avg Level: 0.45
```

**Active Processing:**
```
📊 ACTIVE PROCESSING FRAME #100 | Peak: 0.87 | Range: [0.0, 0.95] | Zero Crossings: 12 | Crackle Suspects: 0
```

### Metrics Tracked:
- **Peak Level** - Maximum amplitude in frame (should be < 1.0)
- **Range** - Min/Max amplitude (detects DC offset or clipping)
- **Zero Crossings** - Polarity transitions (detects aliasing/phase issues)
- **Crackle Suspects** - Samples with amplitude jumps > 0.3f (polling artifacts)

---

## 🔧 How to Enable Debug Mode

### In CircuitProcessor constructor:
```cpp
CircuitProcessor::CircuitProcessor()
    : AudioProcessor(...)
{
    enableDebugLogging = true;  // Enable audio diagnostics
    // ... rest of initialization
}
```

### Check JUCE Debug Console:
- Standalone app: Visual Studio Debug Output window
- VST plugin: DAW's debug log or JUCE plugin wrapper console

---

## ✅ Testing Checklist

- [ ] Rebuild with `cmake --build build --config Release`
- [ ] Test standalone executable with bypass ON/OFF
- [ ] Listen for crackling/artifacts in bypass mode
- [ ] Monitor debug output for crackle detection
- [ ] Test with different sample rates (44.1kHz, 48kHz, 96kHz)
- [ ] Test with different buffer sizes
- [ ] Profile CPU usage (should be minimal in bypass)

---

## 📊 Expected Results After Fix

**Bypass Mode:**
- Clean, transparent guitar tone pass-through
- Level controlled by "Level" parameter only
- Zero crackle artifacts
- Debug: 0 crackle suspects per frame

**Active Processing:**
- Natural blues overdrive tone
- Smooth parameter transitions
- No sudden pops when engaging/disengaging effects
- Debug: Low crackle suspects (< 5% of samples)

---

## 💡 Future Improvements

1. **Block-Based DSP Processing**: Refactor components to process full buffers instead of sample-by-sample
2. **Parameter Ramping**: Add smooth parameter transitions during bypass to avoid click artifacts
3. **Performance Profiling**: Add timing metrics to detect real-time deadline misses
4. **Windows-Specific WASAPI Buffer Management**: Implement proper WASAPI buffer threading for standalone app

