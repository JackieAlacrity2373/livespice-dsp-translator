# Before/After Test Case Definition

Date: 2026-01-29

## Build Status
- Build completed with warnings only (missing initializer for `ConnectionNode::connectedComponents` in LiveSpiceParser.cpp).
- No new errors were introduced by the nonlinear integration changes.

## Test Case 1: Circuit Analysis Report (Nonlinear Component Detection)

### Purpose
Verify that nonlinear components are detected and reported per stage after integration.

### Input
- Schematic: example pedals/MXR Distortion +.schx (or Boss Super Overdrive SD-1.schx)

### Steps
1. Run the translator to generate the circuit analysis report.
2. Locate the **Identified Stages** section.

### Before (Expected Baseline)
- Stages list **no** “Nonlinear Components” section.

### After (Expected)
- Each stage containing diodes/transistors includes a “Nonlinear Components” section with entries:
  - component name (e.g., `D1`)
  - type (e.g., `DIODE`)
  - part number (e.g., `1N4148`)

### Pass Criteria
- At least one stage shows “Nonlinear Components” and lists the expected diode/transistor names from the schematic.

---

## Test Case 2: Generated JUCE Code (Nonlinear Model Emission)

### Purpose
Ensure generated JUCE code includes the nonlinear component models when a stage contains diodes.

### Input
- Schematic: example pedals/MXR Distortion +.schx
- Generated file: JUCE - MXR Distortion +/CircuitProcessor.h and CircuitProcessor.cpp

### Steps
1. Generate JUCE plugin output for the schematic.
2. Inspect the generated processor header and implementation.

### Before (Expected Baseline)
- `CircuitProcessor.h` **does not** include:
  - `#include "../../DiodeModels.h"`
  - `#include "../../TransistorModels.h"`
  - `#include "../../ComponentCharacteristicsDatabase.h"`
- No `Nonlinear::DiodeClippingStage` members are declared.

### After (Expected)
- Header includes nonlinear model headers.
- `CircuitProcessor` declares one `Nonlinear::DiodeClippingStage` member per detected diode.
- Constructor initializes each clipper with a part number from the schematic.
- `processBlock` includes a line like:
  - `signal = D1_clipper.processSample(signal);`

### Pass Criteria
- Generated code contains at least one nonlinear diode clipper declaration and usage.

---

## Test Case 3: Regression (Existing DSP Paths)

### Purpose
Ensure existing LiveSpice DSP processing still appears for op-amp clipping stages.

### Input
- Schematic with op-amp clipping (e.g., Boss Super Overdrive SD-1)

### Before (Expected Baseline)
- Op-amp clipping stages use `LiveSpiceDSP::DiodeProcessor` and `LiveSpiceDSP::OpAmpProcessor` in `processBlock`.

### After (Expected)
- The same LiveSpice DSP processing remains.
- Nonlinear diode clipping is **added** after the stage processing (not replacing it).

### Pass Criteria
- Both LiveSpice DSP and nonlinear diode clipping are present for the same stage.

---

## Notes
- The nonlinear integration currently focuses on diodes. BJT/FET model emission is planned for the next step.
- If a schematic lacks a part number, default diode model `1N4148` is used.
