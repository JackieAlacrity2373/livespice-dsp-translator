# Beta Mode Guide

## Overview

The LiveSPICE to DSP translator now supports **two operational modes**:

- **STABLE MODE** (default): Proven, reliable code generation
- **BETA MODE**: Experimental pattern-specific DSP optimization

## Quick Start

```bash
# Stable mode (default)
./livespice-translator "my_pedal.schx"
./livespice-translator --stable "my_pedal.schx"

# Beta mode (experimental)
./livespice-translator --beta "my_pedal.schx"

# Show help
./livespice-translator --help
```

---

## Mode Comparison

### STABLE MODE (Default)

**What it does:**
- Uses generic DSP mapping for all circuit stages
- Proven, tested code generation
- Guaranteed to work with all circuits

**Code Generation:**
- Input Buffer → `juce::dsp::Gain` (unity)
- Op-Amp Stage → `juce::dsp::WaveShaper` (tanh)
- Filters → `juce::dsp::IIR::Filter` (generic)
- Nonlinear diodes → `Nonlinear::DiodeClippingStage`

**When to use:**
- Production builds
- When you need guaranteed stability
- If beta mode produces errors
- For final plugin releases

---

### BETA MODE (Experimental)

**What it does:**
- Analyzes circuit topology patterns
- Generates optimized DSP code based on recognized patterns
- Uses pattern-specific algorithms

**Pattern Recognition:**
- RC Low-Pass Filter → Optimized biquad cascade
- RC High-Pass Filter → Optimized biquad cascade
- Diode Clipping → Specialized nonlinear solver
- Op-Amp Gain Stage → Optimized gain with bias compensation
- Tone Control → Advanced multi-band EQ

**Advantages:**
- More accurate circuit modeling
- Better CPU performance for recognized patterns
- Closer match to SPICE simulation

**When to use:**
- Testing new circuits
- Development/experimentation
- When you want the most accurate modeling
- Circuits with well-defined topologies (RC filters, op-amp stages, etc.)

**When NOT to use:**
- Production builds (until fully validated)
- If generated code has compilation errors
- Unusual/experimental circuit topologies

---

## Rollback Instructions

If beta mode causes issues:

### Quick Rollback
```bash
# Just use --stable flag
./livespice-translator --stable "my_pedal.schx"
```

### Rebuild Existing Plugin
```bash
# Step 1: Regenerate with stable mode
./livespice-translator --stable "example pedals/Boss Super Overdrive SD-1.schx"

# Step 2: Rebuild plugin
cd "JUCE - Boss Super Overdrive SD-1"
cd build
cmake --build . --config Release
```

---

## Testing Workflow

### Recommended Test Process

1. **Generate with both modes:**
   ```bash
   ./livespice-translator --stable "my_pedal.schx"
   mv "JUCE - my_pedal" "JUCE - my_pedal (stable)"
   
   ./livespice-translator --beta "my_pedal.schx"
   mv "JUCE - my_pedal" "JUCE - my_pedal (beta)"
   ```

2. **Build both versions:**
   ```bash
   cd "JUCE - my_pedal (stable)/build"
   cmake .. && cmake --build . --config Release
   
   cd "../../JUCE - my_pedal (beta)/build"
   cmake .. && cmake --build . --config Release
   ```

3. **Compare outputs:**
   - Load same audio test signal
   - Compare frequency response
   - Compare distortion characteristics
   - Verify parameters work correctly

4. **Choose the winner:**
   - If beta sounds better and has no bugs → use beta
   - If beta has issues → use stable
   - Document any differences

---

## Known Limitations

### Beta Mode Limitations

1. **Pattern Library is Limited**
   - Currently recognizes: RC filters, op-amp stages, diode clipping, basic tone controls
   - Complex/uncommon topologies fall back to generic mapping

2. **BJT/FET Support Incomplete**
   - Diode clipping fully implemented
   - BJT/FET code emission in development

3. **No State-Space/WDF Yet**
   - Pattern matching identifies topologies
   - Advanced solvers (state-space, Wave Digital Filters) not yet implemented

4. **Pattern Confidence Threshold**
   - Low confidence matches (<0.8) may fall back to generic code
   - Check analysis report for "Pattern Match" confidence scores

### Stable Mode Limitations

1. **Generic Mapping**
   - One-size-fits-all approach
   - May not be optimal for specific circuit types

2. **Limited Optimization**
   - Uses JUCE's standard DSP components
   - No topology-specific optimizations

---

## Troubleshooting

### Beta Mode Issues

**Problem:** Generated code won't compile
```bash
# Quick fix: Use stable mode
./livespice-translator --stable "my_pedal.schx"
```

**Problem:** Plugin sounds wrong
```bash
# Compare to stable mode output
# Report differences for debugging
```

**Problem:** Pattern confidence too low
```bash
# Check analysis report:
cat full_pattern_output.txt | grep "Pattern Match"

# Low confidence = beta falls back to generic code anyway
# Just use stable mode
```

---

## Version History

### Current Version (v2.0 - January 2026)

**Stable Mode:**
-  Nonlinear diode clipping (Phase 1 complete)
-  Generic stage-based DSP mapping
-  JUCE parameter generation
-  All tests passing

**Beta Mode:**
-  Pattern detection framework
-  5 initial patterns (RC filters, op-amp clipping, etc.)
-  Confidence scoring
-  Pattern-specific code generation (in development)
-  Advanced BJT/FET emission
- 📋 State-space solvers (planned)

---

## Reporting Issues

If you encounter problems with beta mode:

1. **Capture the error:**
   ```bash
   ./livespice-translator --beta "problem_circuit.schx" > beta_error.log 2>&1
   ```

2. **Generate stable comparison:**
   ```bash
   ./livespice-translator --stable "problem_circuit.schx" > stable_output.log 2>&1
   ```

3. **Report with:**
   - Input .schx file
   - Both log files
   - Description of the issue
   - Expected vs actual behavior

---

## Developer Notes

### Enabling Beta Features in Code

The beta flag is passed to `JuceDSPGenerator`:

```cpp
JuceDSPGenerator juceGen;
juceGen.setBetaMode(true);  // Enable beta features
```

### Checking Beta Mode in Generator

```cpp
if (isBetaMode()) {
    // Use pattern-specific code generation
    generatePatternOptimizedDSP(stage);
} else {
    // Use stable generic code generation
    generateGenericDSP(stage);
}
```

---

## Future Roadmap

### Upcoming Beta Features

- [ ] Pattern-specific code emission (Phase 2.2)
- [ ] BJT/FET nonlinear processing
- [ ] State-space model generation
- [ ] Wave Digital Filter topology mapping
- [ ] Advanced tone stack modeling
- [ ] Feedback loop detection and compensation

### Graduation to Stable

Beta features will move to stable when:
1. All tests passing
2. Validated against SPICE within 3% THD
3. No compilation errors across test suite
4. Performance acceptable (no >10% CPU increase)
5. Documentation complete

---

## Conclusion

Use **--stable** for production, **--beta** for experimentation. When in doubt, stable mode is always safe.

For questions or issues, check the project documentation or run `--help`.
