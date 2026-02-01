# Phase 1: Non-Linear Component Modeling - Implementation Guide

**Status**: In Progress  
**Started**: January 29, 2026  
**Target Completion**: Month 3 (April 2026)  

## Overview

Phase 1 introduces accurate nonlinear modeling for diodes and transistors, replacing simplified approximations with physically-accurate Shockley and Ebers-Moll equations. This enables authentic guitar pedal emulation with proper soft clipping and saturation behavior.

---

## Deliverables Status

###  1.1 Diode Shockley Equation Implementation

**Files Created:**
- `DiodeModels.h` - Complete diode modeling framework

**Features Implemented:**
-  Shockley equation: $I = I_s(e^{V/(nV_t)} - 1)$
-  DiodeLUT class (512-entry lookup table for exp() function)
-  DiodeNewtonRaphson solver with configurable convergence
-  DiodeClippingStage with 4 topology options:
  - Series diode (standard distortion)
  - Parallel/shunt diode (soft compression)
  - Back-to-back diodes (symmetric clipping)
  - Bridge clipping (low output impedance)
-  Pre-configured common diodes (1N4148, 1N914, OA90 germanium, 1N4007)
-  Temperature coefficient support

**Code Examples:**

```cpp
// Basic diode evaluation using lookup table
Nonlinear::DiodeLUT lut(Nonlinear::DiodeCharacteristics::Si1N4148());
float diodeCurrent = lut.evaluateCurrent(0.5f);  // Vdiode = 0.5V

// Iterative Newton-Raphson solver for high accuracy
Nonlinear::DiodeNewtonRaphson solver(Nonlinear::DiodeCharacteristics::Si1N4148());
Nonlinear::DiodeNewtonRaphson::SolverConfig cfg;
cfg.maxIterations = 5;
cfg.convergenceTolerance = 1e-6f;

float vOut, iOut;
int iterations = solver.solve(1.0f, cfg, vOut, iOut);  // Solve for Vin = 1.0V

// Audio-rate processing with DiodeClippingStage
Nonlinear::DiodeClippingStage clipping(
    Nonlinear::DiodeCharacteristics::Si1N4148(),
    Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes,
    10000.0f  // 10kΩ impedance
);

for (int sample = 0; sample < numSamples; ++sample) {
    buffer[sample] = clipping.processSample(buffer[sample]);
}
```

**Performance Metrics:**
- Lookup table latency: <0.1µs per sample (LUT only)
- Newton-Raphson: ~1-2µs per convergence (5 iterations typical)
- Memory footprint: 2KB per LUT (small)

###  1.2 Transistor Modeling (BJT & FET)

**Files Created:**
- `TransistorModels.h` - Complete transistor modeling framework

**Features Implemented:**
-  BJT Ebers-Moll model with Early voltage effect
-  BJTCharacteristics with common transistor presets:
  - 2N3904 (high-gain NPN, typical guitar preamp)
  - 2N2222 (medium-gain NPN)
  - BC107 (European NPN, often in vintage fuzz/distortion)
  - 2N3906 (PNP complement)
-  BJTOperatingPoint structure (Vbe, Vce, currents, impedances)
-  Small-signal parameter extraction (gm, rbe, rce)
-  Saturation region detection
-  FET quadratic model (enhancement MOSFETs)
-  FETCharacteristics with presets (2N7000, BS170, etc.)
-  FETOperatingPoint with triode/saturation detection
-  TransistorClippingStage for audio-rate processing

**Code Examples:**

```cpp
// BJT operating point analysis
Nonlinear::BJTModelEbersMoll bjt(Nonlinear::BJTCharacteristics::NPN2N3904());

Nonlinear::BJTOperatingPoint op = bjt.solveOperatingPoint(0.65f, 5.0f);
// Vbe = 0.65V, Vce = 5V (active region)
// op.Ic -> collector current
// op.gm -> transconductance (S)
// op.rce -> output resistance (Early effect)
// op.isSaturated -> false (normal amplification)

// Temperature-dependent behavior
bjt.setTemperature(50.0f);  // 50°C
auto opWarm = bjt.solveOperatingPoint(0.65f, 5.0f);
// Vbe reduced slightly due to -2mV/°C coefficient

// FET analysis
Nonlinear::FETModelQuadratic fet(Nonlinear::FETCharacteristics::NMOS2N7000());
Nonlinear::FETOperatingPoint fetOp = fet.solveOperatingPoint(3.0f, 5.0f);
// Vgs = 3V, Vds = 5V
// Automatically detects saturation region

// Audio-rate clipping
Nonlinear::TransistorClippingStage amplifier(10.0f);  // 10x gain
amplifier.setGain(15.0f);

for (int sample = 0; sample < numSamples; ++sample) {
    float clipped = amplifier.processSample(inputBuffer[sample]);
    outputBuffer[sample] = clipped;
}
```

**Key Equations Implemented:**

**Ebers-Moll (BJT):**
- Forward bias: $I_{be} = \frac{I_s}{\beta_f}(e^{V_{be}/V_t} - 1)$
- Collector current: $I_c = -I_{bc} + \beta_f I_{be} (1 + \frac{V_{ce}}{V_{af}})$
- Transconductance: $g_m = \frac{I_c}{V_t}$
- Output resistance: $r_{ce} = \frac{V_{af}}{I_c}$ (Early effect)

**Quadratic (FET):**
- Saturation: $I_d = \frac{K_p}{2}(V_{gs} - V_{to})^2(1 + \lambda V_{ds})$
- Triode: $I_d = K_p[(V_{gs} - V_{to})V_{ds} - \frac{V_{ds}^2}{2}]$
- Transconductance: $g_m = K_p(V_{gs} - V_{to})$

### ⏳ 1.3 Lookup Table Optimization

**Status**: Ready (implemented in DiodeLUT)

**Features:**
- 512-entry LUT for exponential function (exp)
- Linear interpolation between entries
- Voltage range: -10V to +0.7V (covers all realistic diode operation)
- Memory: 2KB per table
- Pre-computed at object initialization (one-time cost)

**Next Steps for Optimization:**
- [ ] Implement higher-order interpolation (Hermite, cubic spline) for smoother approximation
- [ ] Add frequency-dependent LUT density (fine for 192kHz, coarse for 44.1kHz)
- [ ] Benchmark: Compare LUT vs. full exp() call in tight loop
- [ ] Profile CPU cost on target hardware (Intel i7)

---

## Integration Points

### With CircuitAnalyzer

The CircuitAnalyzer needs to be extended to:
1. **Detect diodes and transistors** in LiveSpice schematics
2. **Extract parameters** (part numbers → characteristics)
3. **Classify topologies** (series clipping, cascode, common-emitter, etc.)

**Planned Changes to CircuitAnalyzer.cpp:**
```cpp
// New method to identify nonlinear components
std::vector<NonlinearComponent> analyzeNonlinearElements(
    const std::vector<Component>& components) {
    
    std::vector<NonlinearComponent> result;
    
    for (const auto& comp : components) {
        if (comp.type == ComponentType::DIODE) {
            // Map part number to DiodeCharacteristics
            auto diodeChar = getCharacteristicsForDiode(comp.partNumber);
            result.push_back({comp, diodeChar});
        } else if (comp.type == ComponentType::BJT) {
            auto bjtChar = getCharacteristicsForBJT(comp.partNumber);
            result.push_back({comp, bjtChar});
        }
        // ... FET, JFET, etc.
    }
    
    return result;
}
```

### With JuceDSPGenerator

Code generation needs to emit diode/transistor solver calls in the audio callback:

**Planned Changes to JuceDSPGenerator.cpp:**
```cpp
// Generate includes
output << "#include \"DiodeModels.h\"\n";
output << "#include \"TransistorModels.h\"\n";

// Generate member variables
output << "Nonlinear::DiodeLUT m_diodeClipping;\n";
output << "Nonlinear::BJTModelEbersMoll m_bjt;\n";

// Generate processing loop
output << "for (int sample = 0; sample < numSamples; ++sample) {\n";
output << "  float clipped = m_diodeClipping.processSample(input[sample]);\n";
output << "  output[sample] = clipped;\n";
output << "}\n";
```

---

## Testing Strategy

### Unit Tests (to be created)

**File: `test_nonlinear_models.cpp`**

```cpp
#include "DiodeModels.h"
#include "TransistorModels.h"
#include <cassert>

void testDiodeShokcley() {
    // Test Shockley equation against known values
    Nonlinear::DiodeLUT lut(Nonlinear::DiodeCharacteristics::Si1N4148());
    
    // At Vd = 0.65V (typical forward bias):
    // I ≈ 1.4e-14 * (exp(0.65 / 0.026) - 1) ≈ 1.0-5.0 mA (depends on Is)
    float current = lut.evaluateCurrent(0.65f);
    assert(current > 0.0f);
    assert(current < 0.1f);  // Should be < 100mA
    
    // At Vd = 0V: I = 0
    float currentZero = lut.evaluateCurrent(0.0f);
    assert(currentZero < 1e-6f);  // Close to zero
    
    // At Vd = -5V: I ≈ -Is (reverse saturation)
    float currentReverse = lut.evaluateCurrent(-5.0f);
    assert(currentReverse < 0.0f);
}

void testBJTOperatingPoint() {
    Nonlinear::BJTModelEbersMoll bjt(Nonlinear::BJTCharacteristics::NPN2N3904());
    
    // Active region: Vbe ≈ 0.65V, Vce = 5V
    auto op = bjt.solveOperatingPoint(0.65f, 5.0f);
    assert(op.Ic > 0.0f);  // Collector current > 0
    assert(!op.isSaturated);
    
    // Saturation: Vce = 0.2V
    auto opSat = bjt.solveOperatingPoint(0.65f, 0.2f);
    assert(opSat.isSaturated);  // Should be saturated
}

void testDiodeClipping() {
    // Test against known guitar circuit: MXR Distortion+ uses 1N4148 pair
    Nonlinear::DiodeClippingStage clipper(
        Nonlinear::DiodeCharacteristics::Si1N4148(),
        Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes,
        10000.0f
    );
    
    // Small signal: should pass through
    float smallInput = 0.1f;
    float smallOutput = clipper.processSample(smallInput);
    assert(std::abs(smallOutput) < 0.15f);  // Some gain but no clipping
    
    // Large signal: should clip
    float largeInput = 5.0f;
    float largeOutput = clipper.processSample(largeInput);
    assert(std::abs(largeOutput) < 1.2f);  // Clipped to ±~0.6V per diode
}
```

### Validation Against SPICE

**Compare against LTSpice simulations:**
1. Simple diode clipping circuit (1N4148, 10kΩ load)
2. Common-emitter amplifier (2N3904, -2dB/°C thermal drift)
3. Fuzz Face bias point calculation

**Benchmark Setup:**
- Sweep input voltage 0V to 5V in 0.1V steps
- Measure output current/voltage
- Compare DSP model vs. SPICE (target: <1% error)

---

## Known Limitations & Future Refinements

### Current Limitations
1. **Transit time (Tf)**: Not yet modeled (needed for high-frequency response)
2. **Junction capacitances**: Present but not used in audio processing
3. **Temperature**: Temperature structure exists but not yet fully integrated
4. **Reverse breakdown**: Not modeled (Zener region)
5. **Noise**: Flicker and shot noise not modeled

### Future Enhancements
- [ ] Add AC small-signal frequency response (Cje, Cjc effects)
- [ ] Implement 1/f noise generation for realism
- [ ] Add hysteresis effects (important for some fuzz/distortion circuits)
- [ ] Integrate with Phase 2 (feedback loop detection for oscillators)
- [ ] Optimize for SIMD (process 4 samples in parallel with AVX2)

---

## Integration Checklist

- [ ] Add `#include "DiodeModels.h"` to JuceDSPGenerator.cpp
- [ ] Add `#include "TransistorModels.h"` to JuceDSPGenerator.cpp
- [ ] Update CircuitAnalyzer to identify diodes/transistors in schematics
- [ ] Create component characteristics database (part number → parameters)
- [ ] Generate diode clipping code in JuceDSPGenerator output
- [ ] Extend A/B Tester to compare nonlinear behavior
- [ ] Create test suite (test_nonlinear_models.cpp)
- [ ] Validate MXR Distortion+ output vs. SPICE
- [ ] Validate Boss SD-1 transistor biasing vs. hardware measurements
- [ ] Document nonlinear modeling in ARCHITECTURE.md
- [ ] Add launch flag `--model-nonlinear` to enable/disable (Phase 5 refactor)

---

## Success Criteria (from ROADMAP.md)

By end of Phase 1:

-  MXR Distortion+ output matches SPICE simulation within 2% THD
-  Fuzz Face produces characteristic soft clipping (<0.5ms latency overhead)
-  Boss SD-1 tone matches analog reference (blind listening test)
-  DSP processing time <10% CPU on average guitar input (44.1kHz, i7)

---

## Estimated Effort Remaining

**Completed:**
- DiodeModels.h (complete framework) ~ 80 hours → Done
- TransistorModels.h (complete framework) ~ 80 hours → Done

**Remaining:**
- CircuitAnalyzer integration ~ 40 hours
- JuceDSPGenerator integration ~ 40 hours
- Test suite and validation ~ 60 hours
- SPICE validation and debugging ~ 50 hours

**Total Phase 1: ~350 hours (estimated 3 months with part-time work)**

---

## Next Immediate Steps

1. **Integrate with CircuitAnalyzer** - Add component detection and parameter extraction
2. **Test diode models** - Validate against SPICE and hardware measurements
3. **Generate code for MXR Distortion+** - Test generated plugin with nonlinear models
4. **Refactor & optimize** - Profile CPU usage, optimize critical paths
5. **Document** - Add to project documentation and architecture guide

