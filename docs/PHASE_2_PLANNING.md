# Phase 2 Planning: CircuitAnalyzer and JuceDSPGenerator Integration

## Overview

Phase 1 is complete with all nonlinear models validated. Phase 2 focuses on integrating these models into the existing LiveSPICE-to-JUCE pipeline.

**Goal:** Enable automatic detection and modeling of nonlinear components in LiveSPICE schematics, then generate accurate JUCE plugins with real-time nonlinear DSP processing.

---

## Phase 2 Deliverables

### 1. CircuitAnalyzer Integration (HIGH PRIORITY)

**Current State:** CircuitAnalyzer.cpp exists and parses LiveSPICE schematics

**Required Enhancements:**

#### 1a. Component Detection
Add method to `CircuitAnalyzer` class:
```cpp
struct NonlinearComponentInstance {
    std::string componentType;  // "DIODE", "BJT", "FET"
    std::string designator;     // "D1", "Q1", "M1"
    std::string partNumber;     // "1N4148", "2N3904"
    float baseVoltage;          // DC operating point
    float loadResistance;       // Circuit context
};

std::vector<NonlinearComponentInstance> analyzeNonlinearElements();
```

**Implementation Steps:**
1. Parse component designators from schematic
   - Regex: D* = diode, Q* = BJT, M* = FET
   - Extract VALUE field = part number

2. Look up in component database
   ```cpp
   auto diodeOpt = getDiodeDB().lookup(partNumber);
   if (diodeOpt.has_value()) {
       // Found diode characteristics
   }
   ```

3. Calculate DC operating points
   - Analyze circuit topology for each component
   - Determine bias voltages from surrounding resistors
   - Store in NonlinearComponentInstance

#### 1b. Integration with Existing CircuitStage
Link to existing `CircuitStage` structure:
```cpp
struct CircuitStage {
    // ... existing fields ...
    
    std::vector<NonlinearComponentInstance> nonlinearComponents;
    std::map<std::string, DiodeCharacteristics> diodeModels;
    std::map<std::string, BJTCharacteristics> bjtModels;
    std::map<std::string, FETCharacteristics> fetModels;
};
```

**File to Modify:** `CircuitAnalyzer.cpp`, `CircuitAnalyzer.h`  
**Estimated Effort:** 2-3 hours

---

### 2. JuceDSPGenerator Integration (HIGH PRIORITY)

**Current State:** JuceDSPGenerator.cpp generates JUCE audio processor code

**Required Enhancements:**

#### 2a. Code Emission for Models
Add methods to `JuceDSPGenerator`:
```cpp
void emitNonlinearIncludes(std::ofstream& out);
void emitNonlinearMembers(std::ofstream& out, const CircuitStage& stage);
void emitNonlinearProcessing(std::ofstream& out, const CircuitStage& stage);
```

**Implementation:**

1. **Emit includes at top of generated file:**
   ```cpp
   void emitNonlinearIncludes(std::ofstream& out) {
       out << "#include \"DiodeModels.h\"\n";
       out << "#include \"TransistorModels.h\"\n";
       out << "#include \"ComponentCharacteristicsDatabase.h\"\n";
       out << "using namespace Nonlinear;\n";
   }
   ```

2. **Emit member variables in AudioProcessor class:**
   ```cpp
   void emitNonlinearMembers(std::ofstream& out, const CircuitStage& stage) {
       for (const auto& comp : stage.nonlinearComponents) {
           if (comp.componentType == "DIODE") {
               out << "DiodeClippingStage " << comp.designator << "_clipper;\n";
           } else if (comp.componentType == "BJT") {
               out << "BJTModelEbersMoll " << comp.designator << "_model;\n";
           } else if (comp.componentType == "FET") {
               out << "FETModelQuadratic " << comp.designator << "_model;\n";
           }
       }
   }
   ```

3. **Emit processing code in processBlock():**
   ```cpp
   void emitNonlinearProcessing(std::ofstream& out, const CircuitStage& stage) {
       out << "// Nonlinear processing\n";
       for (const auto& comp : stage.nonlinearComponents) {
           if (comp.componentType == "DIODE") {
               out << "sample = D1_clipper.processSample(sample);\n";
           }
       }
   }
   ```

**File to Modify:** `JuceDSPGenerator.cpp`, `JuceDSPGenerator.h`  
**Estimated Effort:** 2-3 hours

#### 2b. Constructor Initialization
Initialize model instances with characteristics:
```cpp
// In generated AudioProcessor constructor
D1_clipper = DiodeClippingStage(
    DiodeCharacteristics::Si1N4148(),
    DiodeClippingStage::TopologyType::BackToBackDiodes,
    10000.0f  // load impedance
);
```

---

### 3. Test Integration (MEDIUM PRIORITY)

**Create integration test:**

Create file: `test_circuit_nonlinear_integration.cpp`

```cpp
#include "CircuitAnalyzer.h"
#include "JuceDSPGenerator.h"
#include "ComponentCharacteristicsDatabase.h"

void testMXRDistortionPlusGeneration() {
    // Parse MXR Distortion+ schematic
    CircuitAnalyzer analyzer("MXR Distortion +.schx");
    auto stages = analyzer.analyzeCircuit();
    
    // Find nonlinear components
    auto nonlinear = stages[0].nonlinearComponents;
    assert(nonlinear.size() > 0);  // Should find diodes
    
    // Generate JUCE plugin
    JuceDSPGenerator generator(stages);
    std::string generatedCode = generator.generatePluginCode();
    
    // Verify code contains nonlinear processing
    assert(generatedCode.find("DiodeClippingStage") != std::string::npos);
}
```

**File to Create:** `test_circuit_nonlinear_integration.cpp`  
**Estimated Effort:** 1-2 hours

---

### 4. Validation Against Benchmarks (MEDIUM PRIORITY)

**Compare outputs:**

1. **SPICE Baseline**
   - Run MXR Distortion+ schematic in LTspice
   - Export I-V curves for key diodes
   - Record distortion spectrum

2. **Generated Plugin Validation**
   - Load generated JUCE plugin
   - Feed same test signals
   - Compare:
     - Clipping threshold (within 5%)
     - THD (Total Harmonic Distortion) within 3%
     - Frequency response (±2dB)

3. **Hardware Measurement (Optional)**
   - Measure actual MXR Distortion+ pedal
   - Compare against plugin output
   - Validate tone accuracy

**Create:** `validation_report.md`  
**Estimated Effort:** 3-4 hours

---

## Implementation Roadmap

### Week 1: Core Integration
- [ ] Day 1-2: CircuitAnalyzer component detection (2-3 hours)
- [ ] Day 3-4: JuceDSPGenerator code emission (2-3 hours)
- [ ] Day 5: Integration testing (1-2 hours)

### Week 2: Validation
- [ ] Day 6-7: SPICE comparison and tuning (3-4 hours)
- [ ] Day 8: Documentation and cleanup (1-2 hours)
- [ ] Total Phase 2: 8-10 hours

---

## Key Integration Points

### CircuitAnalyzer → NonlinearModels
```
CircuitAnalyzer parses "D1=1N4148" 
    ↓
Queries DiodeDatabase::getInstance().lookup("1N4148")
    ↓
Returns DiodeCharacteristics with Is, n, Vt, Rs
    ↓
Stores in CircuitStage.nonlinearComponents
```

### CircuitStage → JuceDSPGenerator
```
JuceDSPGenerator receives CircuitStage
    ↓
Iterates nonlinearComponents
    ↓
For each component, emits:
  - Member variable: DiodeClippingStage m_clipper;
  - Constructor call: m_clipper(characteristics);
  - Process call: sample = m_clipper.processSample(sample);
    ↓
Generates complete JUCE plugin with nonlinear DSP
```

---

## Example: MXR Distortion+ Integration

### Current LiveSPICE Schematic
```xml
<component>
  <designator>D1</designator>
  <value>1N914</value>
  <x>100</x><y>50</y>
</component>
```

### After CircuitAnalyzer Enhancement
```cpp
NonlinearComponentInstance {
    componentType: "DIODE",
    designator: "D1",
    partNumber: "1N914",
    baseVoltage: 0.65,
    loadResistance: 10000.0
}
```

### Generated JUCE Plugin (Partial)
```cpp
#include "DiodeModels.h"
using namespace Nonlinear;

class MXRDistortionAudioProcessor : public juce::AudioProcessor {
private:
    DiodeClippingStage D1_clipper;
    
public:
    MXRDistortionAudioProcessor() 
        : D1_clipper(DiodeCharacteristics::Si1N914(),
                     DiodeClippingStage::TopologyType::BackToBackDiodes,
                     10000.0f) {}
    
    void processBlock(juce::AudioBuffer<float>& buffer, ...) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            // Apply nonlinear diode clipping
            auto signal = buffer.getSample(0, sample);
            signal = D1_clipper.processSample(signal);
            buffer.setSample(0, sample, signal);
        }
    }
};
```

---

## Dependencies & Resources

### Required Files (Already Created)
-  [DiodeModels.h](DiodeModels.h) - Diode framework
-  [TransistorModels.h](TransistorModels.h) - BJT/FET framework
-  [ComponentCharacteristicsDatabase.h](ComponentCharacteristicsDatabase.h) - Database

### Files to Modify
-  CircuitAnalyzer.cpp - Add component detection
-  CircuitAnalyzer.h - Add NonlinearComponentInstance struct
-  JuceDSPGenerator.cpp - Add code emission methods
-  JuceDSPGenerator.h - Add method signatures

### New Test Files
-  test_circuit_nonlinear_integration.cpp
-  validation_report.md

---

## Success Criteria

Phase 2 is complete when:

1.  CircuitAnalyzer detects all diodes, BJTs, FETs in test schematics
2.  JuceDSPGenerator emits correct C++ code with nonlinear processing
3.  Generated plugin compiles and runs without errors
4.  MXR Distortion+ plugin output matches SPICE within 3% THD
5.  All integration tests passing
6.  Documentation complete

---

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| CircuitAnalyzer doesn't parse all components | Medium | High | Test with multiple pedal schematics |
| Generated code has memory leaks | Low | High | Add valgrind/ASAN to testing |
| Performance regression | Medium | Medium | Profile each component before/after |
| SPICE comparison diverges | Low | Medium | Start with simple circuits (single diode) |

---

## Conclusion

Phase 2 is well-scoped with clear deliverables and integration points. The existing Phase 1 code provides a solid foundation for seamless integration. Estimated timeline: **7-10 working hours** to complete integration, validation, and documentation.

**Next Step:** Begin CircuitAnalyzer enhancement when ready.
