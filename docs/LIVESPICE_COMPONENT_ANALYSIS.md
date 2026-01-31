# LiveSPICE Component Reference Extraction Report

**Analysis Date:** January 28, 2026  
**Repository:** https://github.com/dsharlet/LiveSPICE/  
**Language:** C# with XML configuration files  
**License:** MIT  

---

## Executive Summary

LiveSPICE is a real-time audio SPICE circuit simulator written in C#. The component system is well-structured with:
- **7 major component categories** (Passive, Active Semiconductor, Op-Amps, Tubes, Transformers, Sources)
- **XML-based component library system** for easy model specification
- **Computer Algebra integration** for symbolic circuit analysis
- **Modular design** suitable for DSP implementation extraction

---

## Repository Structure Overview

### Key Directories
```
LiveSPICE/
├── Circuit/                          # Core circuit simulation engine
│   ├── Components/                   # Component definitions & models
│   │   ├── VacuumTubes/             # Triode and Pentode models
│   │   ├── *.cs                     # Individual component classes
│   │   ├── *.xml                    # SPICE model libraries
│   │   └── Readme.txt               # Library documentation
│   ├── Simulation/                  # Simulation algorithms
│   ├── Spice/                       # SPICE format parsing
│   └── Analysis.cs                  # Circuit analysis engine
├── ComputerAlgebra/                 # Symbolic math library (submodule)
├── LiveSPICE/                       # Main UI application
└── Tests/                           # Test suite
```

---

## Component Types & Definitions

### 1. RESISTOR
**File:** `Circuit/Components/Resistor.cs`

#### Class Definition
- **Type:** Two-terminal passive component
- **Base Class:** `TwoTerminal`
- **Property:** Linear V = R*i relationship

#### Parameters
| Parameter | Type | Units | Default | Range | Description |
|-----------|------|-------|---------|-------|-------------|
| Resistance | Quantity | Ω (Ohm) | 100 | - | Ohmic resistance value |

#### Mathematical Model
```csharp
// V = R*i relationship
i = (Anode.V - Cathode.V) / R
```

#### Analysis Implementation
- **Node Equations:** Adds to MNA matrix as passive component
- **Special Case:** If R = 0, becomes a Conductor (short circuit)
- **Method:** `Analyze(Analysis Mna, Node Anode, Node Cathode, Expression R)`

#### Code Location
- Implementation: Lines 1-50 (class definition, parameters)
- Analysis: Lines 51-80 (Analyze method)
- Layout: Lines 81-100+ (LayoutSymbol method)

---

### 2. CAPACITOR
**File:** `Circuit/Components/Capacitor.cs`

#### Class Definition
- **Type:** Two-terminal passive component
- **Base Class:** `TwoTerminal`
- **Property:** Dynamic i = C*dV/dt relationship

#### Parameters
| Parameter | Type | Units | Default | Range | Description |
|-----------|------|-------|---------|-------|-------------|
| Capacitance | Quantity | F (Farad) | 100µF | - | Capacitance value |

#### Mathematical Model
```csharp
// i = C*dV/dt
i = C * D(V, t)
```

#### Analysis Implementation
- **Method:** Derivative-based using time-domain analysis
- **Integration:** Time-derivative operator D() applied to voltage across capacitor
- **MNA Integration:** Added as passive component with time-dependent current

#### Code Location
- Implementation: Lines 1-40
- Analysis & derivative: Lines 41-65

---

### 3. INDUCTOR
**File:** `Circuit/Components/Inductor.cs`

#### Class Definition
- **Type:** Two-terminal passive component
- **Base Class:** `TwoTerminal`
- **Property:** Dynamic V = L*di/dt relationship

#### Parameters
| Parameter | Type | Units | Default | Range | Description |
|-----------|------|-------|---------|-------|-------------|
| Inductance | Quantity | H (Henry) | 100µH | - | Inductance value |

#### Mathematical Model
```csharp
// V = L*di/dt
V = L * D(i, t)
```

#### Analysis Implementation
- **State Variable:** Current `i` is defined as unknown in MNA
- **Equation:** Voltage equation added to matrix: Anode.V - Cathode.V = L*di/dt
- **Method:** `Analyze(Analysis Mna, Node Anode, Node Cathode, Expression L)`

#### Code Location
- Implementation: Lines 1-40
- Unknown current: Lines 45-50
- Analysis equations: Lines 51-65

---

### 4. DIODE
**File:** `Circuit/Components/Diode.cs`

#### Class Definition
- **Type:** Two-terminal nonlinear semiconductor
- **Base Class:** `TwoTerminal`
- **Model:** Shockley diode equation

#### Parameters
| Parameter | Type | Units | Default | Range | Description |
|-----------|------|-------|---------|-------|-------------|
| IS | Quantity | A (Ampere) | 1e-12 | 1e-16 to 1e-6 | Saturation current (reverse leakage) |
| n | Quantity | None | 1 | 1.0 to 4.0 | Gate emission coefficient (ideality factor) |
| Type | Enum | - | Diode | {Diode, LED, Zener} | Diode type (affects symbol only) |

#### Mathematical Model
```csharp
// Shockley diode equation
i = IS * LinExpm1(Vac / (n * VT))
// where VT = k*T/q (thermal voltage, ~26mV at 25°C)
// LinExpm1 = exp(x) - 1 for numerical stability
```

#### Diode Type Definitions
- **Silicon (Si):** IS=1pA, n=1.0 (generic)
- **Germanium (Ge):** IS=1µA, n=1.0
- **Zener:** IS=1fA, n=1.0 (high-impedance)
- **Red LED:** IS=93pA, n=3.73
- **Green LED:** IS=93pA, n=4.61
- **Blue LED:** IS=93pA, n=7.61

#### Standard SPICE Models Included
| Part Number | IS | n | Type | Application |
|------------|----|----|------|-------------|
| 1N4001 | 65.4 pA | 1.36 | Diode | Rectifier |
| 1N914 | 2.52 nA | 1.752 | Diode | High-speed switching |
| 1N4148 | 2.52 nA | 1.752 | Diode | Fast switching |
| 1N4448 | 0.1 pA | 2.07 | Diode | Ultra-fast |
| 1N34A | 2e-7 A | 1.3 | Diode | Germanium (old) |
| 1N4739A | 9.05 fA | 1.1 | Zener | Regulation |
| 1N4742A | 6.87 fA | 1.49 | Zener | Regulation |

#### Code Location
- Class & parameters: Lines 1-60
- Analysis method: Lines 61-90
- Model: Reference to Shockley equation (lines 20-25)

#### Library File
- **File:** `Circuit/Components/Diodes.xml` (80+ predefined models)
- **Format:** XML with _Type, IS, n, Type attributes

---

### 5. BIPOLAR JUNCTION TRANSISTOR (BJT)
**File:** `Circuit/Components/BipolarJunctionTransistor.cs`

#### Class Definition
- **Type:** Three-terminal active semiconductor
- **Base Class:** `Component`
- **Model:** Ebers-Moll BJT model
- **Terminals:** Collector (C), Base (B), Emitter (E)
- **Types:** NPN, PNP

#### Parameters
| Parameter | Type | Units | Default | Range | Description |
|-----------|------|-------|---------|-------|-------------|
| IS | Quantity | A | 1e-12 | - | Saturation current |
| BF | Quantity | None | 100 | 10 to 1000+ | Forward common-emitter current gain |
| BR | Quantity | None | 1 | 0.1 to 50 | Reverse common-emitter current gain |
| Type | Enum | - | NPN | {NPN, PNP} | BJT polarity type |

#### Mathematical Model (Ebers-Moll)
```csharp
// Forward and reverse base currents
aF = BF / (1 + BF)    // Forward alpha
aR = BR / (1 + BR)    // Reverse alpha

// Diode equations for BE and BC junctions
iF = IS * LinExpm1(Vbe / VT)    // Forward current
iR = IS * LinExpm1(Vbc / VT)    // Reverse current

// Terminal currents
ic = aF*iF - iR          // Collector current
ib = (1-aF)*iF + (1-aR)*iR    // Base current
ie = iF - aR*iR          // Emitter current
```

#### Standard SPICE Models Included
**High-performance small-signal transistors:**
| Part | Type | IS | BF | BR | Fmax | Application |
|------|------|----|----|----|----|-------------|
| 2N2222 | NPN | 1e-14 A | 200 | 3 | ~250MHz | General purpose |
| 2N2222A | NPN | 14.34 fA | 255.9 | 6.092 | - | Improved 2N2222 |
| 2N3904 | NPN | 1e-14 A | 300 | 4 | ~300MHz | Standard small-signal |
| 2N3906 | PNP | 1e-14 A | 200 | 4 | ~300MHz | Standard small-signal |
| 2N5088 | NPN | 5.911 fA | 1122 | 1.271 | ~800MHz | High gain |
| 2N5089 | NPN | 5.911 fA | 1434 | 1.262 | ~800MHz | Very high gain |
| BC107 | NPN | 40.72 fA | 407 | 7.41 | - | Audio preamp |
| BC108 | NPN | 40.72 fA | 381.7 | 2.359 | - | Audio preamp |
| BC549 | NPN | 4.679e-14 A | 458.7 | 11.57 | ~200MHz | Low noise |

**Power transistors (Germanium):**
| Part | Type | IS | BF | BR | Application |
|------|------|----|----|----|----|
| AC127 | NPN | 100 nA | 90 | 20 | Germanium audio |
| AC128 | PNP | 100 nA | 90 | 20 | Germanium audio |

#### Code Location
- Class definition: Lines 1-80
- Ebers-Moll model: Lines 90-140
- Terminal current equations: Lines 140-170
- Analysis entry: Lines 170-200

#### Library File
- **File:** `Circuit/Components/Transistors.xml` (45+ BJT models)

---

### 6. JUNCTION FIELD-EFFECT TRANSISTOR (JFET)
**File:** `Circuit/Components/JunctionFieldEffectTransistor.cs`

#### Class Definition
- **Type:** Three-terminal active semiconductor
- **Base Class:** `Component`
- **Model:** Quadratic JFET with saturation
- **Terminals:** Drain (D), Gate (G), Source (S)
- **Types:** N-channel, P-channel

#### Parameters
| Parameter | Type | Units | Default | Range | Description |
|-----------|------|-------|---------|-------|-------------|
| IS | Quantity | A | 1e-14 | - | Gate-source/Gate-drain saturation current |
| n | Quantity | None | 1 | - | Gate emission coefficient |
| Vt0 (VTO) | Quantity | V | -2.0 | -5 to 0 | Threshold voltage (pinch-off) |
| Beta | Quantity | A/V² | 1e-4 | - | Transconductance parameter |
| Lambda | Quantity | 1/V | 0 | 0 to 0.1 | Channel length modulation |

#### Mathematical Model
```csharp
// Gate-source diode junction
Diode.Analyze(Mna, Gate, Source, IS, n)
Diode.Analyze(Mna, Gate, Drain, IS, n)

// Drain-source voltage & gate-drain voltage
Vgds = Gate.V - Min(Source.V, Drain.V)
Vds = Drain.V - Source.V
AbsVds = Abs(Vds)

// Linear vs Saturation regions
if (AbsVds < (Vgds - Vt0)):
    // Linear region
    id = Sign(Vds) * (Vgds >= Vt0) * Beta * (1 + Lambda*AbsVds) * AbsVds * (2*(Vgds-Vt0) - 1)
else:
    // Saturation region
    id = Sign(Vds) * (Vgds >= Vt0) * Beta * (1 + Lambda*AbsVds) * (Vgds-Vt0)²
```

#### Standard SPICE Models Included
| Part | Type | IS | Beta | Vt0 | Lambda | Idss | Application |
|------|------|----|----|------|--------|------|-------------|
| 2N5952 | N | 1e-14 A | 1.3143e-3 | -2.021 V | 4e-3 | ~15mA | VHF |
| 2N5457 | N | 1e-14 A | 1.125e-3 | -1.372 V | 2.3e-3 | ~12mA | General |
| 2N5458 | N | 1e-14 A | 488.9e-6 | -2.882 V | 3.167e-3 | ~10mA | Low capacitance |
| 2N4392 | N | 4.908e-16 A | 4.225e-3 | -2.208 V | 6.403e-2 | - | Low noise |
| J112 | N | 8.879e-16 A | 5.972e-3 | -2.33 V | 23.198e-3 | - | General purpose |
| J174 | P | 9.7275e-16 A | 1.24461e-3 | -7.1507 V | 2.01968e-2 | - | P-channel |

#### Code Location
- Class definition: Lines 1-100
- Gate diodes: Lines 140-150
- Drain current logic: Lines 150-190
- Analysis method: Lines 190-220

#### Library File
- **File:** `Circuit/Components/Transistors.xml` (10+ JFET models)

---

### 7. OPERATIONAL AMPLIFIER
**File:** `Circuit/Components/OpAmp.cs`

#### Class Definition
- **Type:** Multi-terminal active component (5 terminals)
- **Base Class:** `IdealOpAmp`
- **Model:** Single-pole frequency response with saturation
- **Terminals:** Positive input (+), Negative input (-), Output, Vcc+, Vcc-

#### Parameters
| Parameter | Type | Units | Default | Range | Description |
|-----------|------|-------|---------|-------|-------------|
| Rin | Quantity | Ω | 1 MΩ | 1kΩ to 10TΩ | Input resistance |
| Rout | Quantity | Ω | 100 | 10 to 1000 | Output resistance |
| Aol | Quantity | None | 1e6 | 100k to 1M | Open-loop DC gain |
| GBP | Quantity | Hz | 1 MHz | 1MHz to 35MHz | Gain-bandwidth product |

#### Mathematical Model
```csharp
// Input stage with differential input resistor
Resistor.Analyze(Mna, Negative, Positive, Rin)

// Voltage gain with pole compensation
Rp1 = 1000 Ω
Ip1 = (VRin * Aol) / Rp1
Resistor.Analyze(Mna, pp1, np1, Rp1)
Capacitor.Analyze(Mna, pp1, np1, 1/(2π*Rp1*GBP/Aol))

// Output buffering
Iout = (pp1.V - Out.V) / Rout

// Saturation/Clipping
if (Vcc+/Vcc- connected):
    Diode.Analyze(Mna, pp1, Vcc+2V, 8e-16, 1)
    Diode.Analyze(Mna, Vcc-2V, pp1, 8e-16, 1)
```

#### Standard OpAmp Models in Library
**Classic/Precision OpAmps:**
| Part | Rin | Rout | Aol | GBP | Noise | Application |
|------|-----|------|-----|-----|-------|-------------|
| UA741 | 2 MΩ | 75 Ω | 200k | 1 MHz | - | Industry standard (1970s) |
| LM308 | 40 MΩ | 50 Ω | 300k | 1 MHz | Low noise | Precision integrators |
| TL061 | 1 TΩ | 50 Ω | 315k | 1 MHz | Low noise | JFET input |
| TL071 | 1 TΩ | 50 Ω | 200k | 5.25 MHz | Low noise | JFET input audio |

**High-speed OpAmps:**
| Part | Rin | Rout | Aol | GBP | Slew Rate | Application |
|------|-----|------|-----|-----|-----------|-------------|
| TL08x | 1 TΩ | 50 Ω | 100k | 3 MHz | - | Fast CMOS |
| AD826 | 300 kΩ | 8 Ω | 5.6k | 35 MHz | - | Video circuits |

**High-impedance input OpAmps:**
| Part | Rin | Rout | Aol | GBP | Input Type | Application |
|------|-----|------|-----|-----|-----------|-------------|
| 3404A | 1 TΩ | 50 Ω | 100k | 1.2 MHz | MOSFET | Ultra high impedance |
| OP42 | 1 TΩ | 50 Ω | 1 M | 10 MHz | Chopper-stabilized | Precision DC |
| OP134 | 10 TΩ | 10 Ω | 1 M | 8 MHz | JFET | Audio applications |

#### Code Location
- Class definition: Lines 1-60
- Input stage: Lines 80-95
- Pole compensation: Lines 95-105
- Output buffering: Lines 105-115
- Saturation: Lines 120-135

#### Library File
- **File:** `Circuit/Components/OpAmps.xml` (20+ OpAmp models)

---

### 8. VACUUM TUBES - TRIODE
**File:** `Circuit/Components/VacuumTubes/Triode.cs`

#### Class Definition
- **Type:** Three-terminal nonlinear active component
- **Base Class:** `Component`
- **Models:** 3 available (Child-Langmuir, Koren, Dempwolf-Zolzer)
- **Terminals:** Plate (P), Grid (G), Cathode (K)

#### Parameters
**Common Parameters:**
| Parameter | Type | Units | Default | Description |
|-----------|------|-------|---------|-------------|
| Mu | double | - | 100 | Voltage gain (amplification factor) |
| Model | Enum | - | Koren | {ChildLangmuir, Koren, DempwolfZolzer} |

**Child-Langmuir Model:**
| Parameter | Type | Units | Default | Description |
|-----------|------|-------|---------|-------------|
| K | double | - | 1.73e-6 | Generalized perveance |

**Koren Model (Most accurate):**
| Parameter | Type | Units | Default | Description |
|-----------|------|-------|---------|-------------|
| Ex | double | - | 1.4 | Exponent (0.8 to 2.0) |
| Kg | double | - | 1060 | Grid gm coefficient |
| Kp | double | - | 600 | Plate coefficient |
| Kvb | double | - | 300 | VB bias parameter |
| Rgk | Quantity | Ω | 1 MΩ | Grid-cathode resistance |
| Kn | Quantity | V | 0.5 | Knee size |
| Vg | Quantity | V | 0.33 | Grid voltage offset |

**Dempwolf-Zolzer Model (Dynamic, with capacitances):**
| Parameter | Type | Units | Default | Description |
|-----------|------|-------|---------|-------------|
| Gamma | double | - | 1.26 | Exponent |
| G | double | - | 2.242e-3 | Plate current coefficient |
| Gg | double | - | 6.177e-4 | Grid current coefficient |
| C | double | - | 3.4 | Plate exponent |
| Cg | double | - | 9.901 | Grid exponent |
| Xi | double | - | 1.314 | Grid exponent |
| Ig0 | Quantity | A | 8.025e-8 | Grid current offset |
| Cgp | Quantity | F | 2.4 pF | Grid-plate capacitance |
| Cgk | Quantity | F | 2.3 pF | Grid-cathode capacitance |
| Cpk | Quantity | F | 0.9 pF | Plate-cathode capacitance |

#### Triode Mathematical Models

**Child-Langmuir Model (Simplest):**
```csharp
Ed = Mu * Vgk + Vpk
ip = (Ed > 0) ? K * Ed^1.5 : 0
ig = 0
ik = -(ip + ig)
```

**Koren Model (Standard in audio):**
```csharp
// Grid-plate interaction
E1 = Ln1Exp(Kp * (1/Mu + Vgk/sqrt(Kvb + Vpk²))) * Vpk / Kp

// Plate current with grid clipping
ip = (E1 > 0) ? 2*(E1^Ex)/Kg : 0

// Grid current with smooth knee
if (Vgk < Vg - Kn):
    ig = 0
else if (Vgk > Vg + Kn):
    ig = (Vgk - Vg) / Rgk
else:
    // Smooth transition
    ig = quadratic(Vgk)
```

**Dempwolf-Zolzer Model (Most accurate):**
```csharp
// Grid current (exponential)
exg = Cg * Vgk
ig = (exg > -50) ? Gg*(Ln1Exp(exg)/Cg)^Xi : 0 + Ig0

// Cathode current (exponential)
exk = C * ((Vpk/Mu) + Vgk)
ik = (exk > -50) ? -G*(Ln1Exp(exk)/C)^Gamma : 0

// Plate current (conservation)
ip = -(ik + ig)
```

#### Standard Triode Models in Library
**Preamp tubes (12-series for 120V rails):**
| Part | Mu | Kg | Kp | Kvb | Rgk | Model | Application |
|------|----|----|----|----|-----|-------|-------------|
| 12AT7 | 67.49 | 419.1 | 213.96 | 300 | 1MΩ | Koren | Phase inverter, preamp |
| 12AU7 | 21.5 | 1119 | 400 | 300 | 1MΩ | Koren | High current preamp |
| 12AX7 | 83.5 | 1060 | 600 | 300 | 1MΩ | Dempwolf-Zolzer | High-gain preamp (industry standard) |
| 12AY7 | 44.16 | 1192.4 | 409.96 | 300 | 1MΩ | Koren | Intermediate gain |
| 12AZ7 | 74.08 | 382 | 190.11 | 300 | 1MΩ | Koren | Output tube design |

**Output tubes (6-series for 330V rails):**
| Part | Mu | Kg | Kp | Kvb | Rgk | Application |
|------|----|----|----|----|-----|-------------|
| 6L6GC | - | - | - | - | - | Power tube (6W) |
| 6V6GTA | - | - | - | - | - | Power tube (4W) |
| 6SL7GT | 75.89 | 1735.2 | 1725.27 | 7.0 | 2kΩ | High-power preamp |

#### Code Location
- Class definition: Lines 1-120
- Model selection: Lines 120-160
- Koren model: Lines 220-260
- Dempwolf-Zolzer model: Lines 270-310
- Capacitance simulation: Lines 350-365

#### Library File
- **File:** `Circuit/Components/Tubes.xml` (20+ triode and pentode models)

---

### 9. VACUUM TUBES - PENTODE
**File:** `Circuit/Components/VacuumTubes/Pentode.cs` (referenced in Tubes.xml)

#### Class Definition
- **Type:** Five-terminal nonlinear active component
- **Terminals:** Plate (P), Screen Grid (G2), Control Grid (G1), Suppressor (G3), Cathode (K)
- **Models:** Koren-based with screen grid modeling

#### Parameters (Superset of Triode)
| Parameter | Type | Units | Typical | Description |
|-----------|------|-------|---------|-------------|
| Mu | double | - | 10-20 | Screen amplification factor |
| Kg1 | double | - | 300-1700 | Control grid coefficient |
| Kg2 | double | - | 4000-6000 | Screen grid coefficient |
| Kp | double | - | 25-130 | Plate coefficient |
| Kvb | double | - | 10-40 | Bias voltage parameter |
| Rgi | Quantity | Ω | 1-2 kΩ | Internal grid resistance |
| Ex | double | - | 1.06-1.35 | Exponent |

#### Pentode Models in Library
**Power output pentodes:**
| Part | Mu | Kg1 | Kg2 | Kp | Kvb | Rgi | Pmax | Application |
|------|----|----|----|----|-----|-----|------|-------------|
| 6L6GC-JJ | 13.94 | 320.2 | 4500 | 33.2 | 29.6 | 1kΩ | ~30W | Power amp |
| 6V6GTA | 10.7 | 1672 | 4500 | 41.16 | 12.7 | 2kΩ | ~14W | Power amp |
| EL34-JJ | 11.55 | 650.2 | 6000 | 51.85 | 25.4 | 1kΩ | ~35W | European power |
| EL84-JJ | 19.56 | 593 | 4500 | 131.34 | 17.5 | 1kΩ | ~12W | Guitar amp |
| 6550 | 7.9 | 890 | 4200 | 60 | 24 | 1kΩ | ~40W | High-power |
| KT88-JJ | 12.43 | 315.7 | 4500 | 26.72 | 36.1 | 1kΩ | ~50W | Ultra-high-power |

---

### 10. TRANSFORMER
**File:** `Circuit/Components/Transformer.cs`

#### Class Definition
- **Type:** Four-terminal linear component
- **Base Class:** `Component`
- **Model:** Ideal transformer (no losses)
- **Terminals:** Primary Anode (PA), Primary Cathode (PC), Secondary Anode (SA), Secondary Cathode (SC)

#### Parameters
| Parameter | Type | Units | Default | Description |
|-----------|------|-------|---------|-------------|
| Turns | Ratio | N1:N2 | 1:1 | Primary:Secondary turns ratio |

#### Mathematical Model
```csharp
// Ideal transformer equations
Ip = Mna.AddUnknown("i" + Name + "p")
Is = Mna.AddUnknown("i" + Name + "s")

// Current equation: Ip * turns = Is
Ip * turns = Is

// Voltage equation: Vp = turns * Vs
Vp = turns * Vs

// Conservation of power: Vp*Ip = Vs*Is
```

#### Code Location
- Class definition: Lines 1-40
- Analysis: Lines 50-80
- Symbol layout: Lines 85-120

#### Supported Transformer Types
- **Step-up:** turns > 1 (voltage increases, current decreases)
- **Step-down:** turns < 1 (voltage decreases, current increases)
- **Isolation:** turns = 1 (same voltage, current)

---

### 11. CENTER-TAP TRANSFORMER
**File:** `Circuit/Components/CenterTapTransformer.cs`

#### Class Definition
- **Type:** Five-terminal transformer variant
- **Terminals:** PA, PC (primary), SA, ST, SC (secondary with center tap)
- **Terminals:** Primary A, Primary C, Secondary A, Secondary Tap (CT), Secondary C

#### Use Cases
- Full-wave rectification
- Push-pull amplifier drive
- Balanced audio outputs

---

## Component Support Summary Table

| Component | File | Model Type | Nonlinear | Dynamic | Library Models |
|-----------|------|-----------|-----------|---------|-----------------|
| Resistor | Resistor.cs | Linear | No | No | - |
| Capacitor | Capacitor.cs | Linear | No | Yes | - |
| Inductor | Inductor.cs | Linear | No | Yes | - |
| Diode | Diode.cs | Shockley | Yes | No | 80+ (Diodes.xml) |
| BJT | BipolarJunctionTransistor.cs | Ebers-Moll | Yes | No | 45+ (Transistors.xml) |
| JFET | JunctionFieldEffectTransistor.cs | Quadratic+Lambda | Yes | No | 10+ (Transistors.xml) |
| OpAmp | OpAmp.cs | Behavioral + Poles | Yes | Yes | 20+ (OpAmps.xml) |
| Triode | Triode.cs | 3 models | Yes | Yes | 20+ (Tubes.xml) |
| Pentode | Pentode.cs | Koren | Yes | Yes | 10+ (Tubes.xml) |
| Transformer | Transformer.cs | Ideal | No | No | - |

---

## Circuit Analysis Method (MNA - Modified Nodal Analysis)

### Core Analysis Class
**File:** `Circuit/Analysis.cs`

The system uses **Modified Nodal Analysis (MNA)** for circuit solving:

```csharp
public class Analysis
{
    // Primary methods
    void Analyze(Circuit circuit)           // Solve circuit
    void AddUnknown(string name)             // Create variable
    void AddEquation(Expression lhs, Expression rhs)  // Add constraint
    void AddPassiveComponent(Node anode, Node cathode, Expression i)  // Add branch
    void AddTerminal(Terminal t, Expression i)  // Add current injection
}
```

### MNA Process
1. **Nodal equations:** KCL at each node → current balance
2. **Component equations:** Component-specific constraints
3. **System matrix:** Conductance/admittance matrix
4. **Solver:** Computer algebra system solves for node voltages

---

## Data Format & File Organization

### XML Component Library Format
```xml
<?xml version="1.0" encoding="utf-8"?>
<Library [Category="category"]>
  <Component
    _Type="Circuit.ClassName, Circuit, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null"
    ParamName="value"
    ParamName="value"
    Name="InstanceName"
    PartNumber="ModelNumber"
    Description="Description text"
  />
</Library>
```

### Example: Diode Library Entry
```xml
<Component _Type="Circuit.Diode, Circuit, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null"
           IS="2.52 nA" n="1.752"
           Type="Diode" Name="D1" PartNumber="1N4148"
           Description="Fast switching diode" />
```

### Example: Transistor Library Entry
```xml
<Component _Type="Circuit.BipolarJunctionTransistor, Circuit, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null"
           IS="1e-14" BF="300" BR="4"
           Type="NPN" Name="Q1" PartNumber="2N3904" />
```

### Example: OpAmp Library Entry
```xml
<Component _Type="Circuit.OpAmp, Circuit, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null"
           Rin="1 TΩ" Rout="50Ω"
           Aol="200k" GBP="5.25 MHz"
           Name="X1" PartNumber="TL07x" />
```

---

## DSP Implementation Recommendations

### Priority 1 - Core Audio Components (Essential)
1. **Diode** - Essential for distortion, clipping, peak detection
2. **Resistor/Capacitor/Inductor** - Filter design (tone controls)
3. **OpAmp** - Gain staging, filtering, summing

**Why:** Most common in vintage pedal designs (Boss, MXR, Marshall)

### Priority 2 - Semiconductor Transistors (Important)
4. **BJT (2N3904, 2N3906, BC107-109)** - Amplification, overdrive circuits
5. **JFET (2N5457, J112)** - Analog switches, high-impedance buffers

**Why:** Common in gain circuits, buffer stages, tone sections

### Priority 3 - Vacuum Tubes (Special Effects)
6. **Triode (12AX7, EL84)** - Tone coloration, saturation characteristics
7. **Pentode (6L6, EL34)** - Power amp emulation

**Why:** Critical for authentic vintage guitar amp modeling

### Priority 4 - Complex Components (Advanced)
8. **Transformer** - Audio coupling, impedance matching
9. **OpAmp Library** - Precision designs (TL072, OP27, OP134)

**Why:** Refined vintage designs often use quality OpAmps and output transformers

---

## Key Mathematical Constants & Functions

### Thermal Voltage
- **VT = k·T/q**
- At 25°C: VT ≈ 26 mV
- Used in Shockley equation and all exponential diode/BJT models

### Special Functions Used
- **LinExpm1(x):** `exp(x) - 1` with numerical stability
  - Avoids overflow for large x
  - Avoids underflow for small x
- **Ln1Exp(x):** `ln(1 + exp(x))` with numerical stability
  - Used in tube models for smooth characteristics

### Unit System
- **Voltage:** Volts (V)
- **Current:** Amperes (A)
- **Resistance:** Ohms (Ω)
- **Capacitance:** Farads (F)
- **Inductance:** Henries (H)
- **Frequency:** Hertz (Hz)

### Standard Unit Prefixes Supported
- p (pico, 1e-12), n (nano, 1e-9), µ (micro, 1e-6)
- m (milli, 1e-3), k (kilo, 1e3), M (mega, 1e6)
- G (giga, 1e9), T (tera, 1e12)

---

## File Organization for Reference

```
LiveSPICE/Circuit/Components/
├── BaseComponents.cs              # TwoTerminal, OneTerminal base classes
├── Resistor.cs                   # Ohmic resistor
├── Capacitor.cs                  # Linear capacitor
├── Inductor.cs                   # Linear inductor
├── Diode.cs                      # Shockley diode model
├── Diodes.xml                    # 80+ diode SPICE models
├── BipolarJunctionTransistor.cs  # BJT Ebers-Moll model
├── JunctionFieldEffectTransistor.cs  # JFET quadratic model
├── Transistors.xml               # 60+ BJT and JFET SPICE models
├── OpAmp.cs                      # Behavioral op-amp model
├── OpAmps.xml                    # 20+ op-amp SPICE models
├── Transformer.cs                # Ideal transformer
├── CenterTapTransformer.cs       # Center-tap variant
├── VacuumTubes/
│   ├── Triode.cs                # Triode (3 models: CL, Koren, DZ)
│   ├── Pentode.cs               # Pentode power tubes
│   └── Diode.cs                 # Tube-rectifier diode
├── Tubes.xml                     # 30+ tube SPICE models
├── Definitions.cs                # Named voltage/current definitions
├── Readme.txt                    # SPICE library loading documentation
└── ... (other components: Current/Voltage sources, etc.)
```

---

## Integration with C++ DSP Project

### Mapping LiveSPICE Models to C++ Classes

1. **Component Parameter Class Structure**
   ```cpp
   struct ComponentParams {
       // Diode
       float IS;      // Saturation current (A)
       float n;       // Ideality factor
       
       // BJT
       float BF;      // Forward beta
       float BR;      // Reverse beta
       
       // OpAmp
       float Rin;     // Input resistance (Ω)
       float Rout;    // Output resistance (Ω)
       float Aol;     // Open-loop gain
       float GBP;     // Gain-bandwidth product (Hz)
   };
   ```

2. **Component Model Implementations**
   - Create separate .cpp files for each component type
   - Implement mathematical models (Shockley, Ebers-Moll, etc.)
   - Use lookup tables for complex functions (Ln1Exp, LinExpm1)

3. **Library Loading**
   - Parse XML component definitions
   - Build component databases in-memory
   - Allow quick model lookup by part number

4. **Audio Processing Pipeline**
   - Per-sample component evaluation
   - State variables for energy-storing components (L, C, tubes)
   - Real-time DSP using component models

---

## Additional Resources

### References Used in Code
- **Diodes:** Shockley diode model (http://en.wikipedia.org/wiki/Diode_modelling)
- **BJTs:** Ebers-Moll model (http://people.seas.harvard.edu/~jones/es154/)
- **OpAmps:** 
  - http://www.ecircuitcenter.com/OpModels/OpampModels.htm
  - http://qucs.sourceforge.net/docs/opamp.pdf
- **Tubes (Koren model):** Referenced in standard SPICE libraries
- **Tubes (Dempwolf-Zolzer):** Referenced in code comments

### External SPICE Model Sources
- LTspice standard models (referenced in comments)
- Manufacturer datasheets (referenced for specific part numbers)
- Standard SPICE simulation tools (NGSPICE, LTspice)

---

## Summary Table: Component Parameter Counts

| Component Type | Basic Params | Total Variants | Critical Params | Difficulty |
|---|---|---|---|---|
| Resistor | 1 | N/A | R | Trivial |
| Capacitor | 1 | N/A | C | Trivial |
| Inductor | 1 | N/A | L | Trivial |
| Diode | 3 | 80+ | IS, n | Low |
| BJT | 3 | 45+ | IS, BF, BR | Medium |
| JFET | 5 | 10+ | Vt0, Beta, Lambda | Medium-High |
| OpAmp | 4 | 20+ | Rin, Rout, Aol, GBP | High |
| Triode | 10-15 | 20+ | Mu, Kg, Kp | Very High |
| Pentode | 7 | 10+ | Mu, Kg1, Kg2, Kp | Very High |
| Transformer | 1 | N/A | Turns ratio | Low |

---

## Notes for Implementation

### Phase 1: Basic Components (Weeks 1-2)
- [ ] Resistor, Capacitor, Inductor
- [ ] Basic diode model
- [ ] Simple BJT model
- [ ] Test with basic RC/RL filters

### Phase 2: Semiconductor Library (Weeks 3-4)
- [ ] Full diode Shockley model with lookup tables
- [ ] Full BJT Ebers-Moll model
- [ ] JFET model with region detection
- [ ] Build component database from XML

### Phase 3: Op-Amps & Advanced (Weeks 5-6)
- [ ] OpAmp behavioral model with frequency response
- [ ] Transformer ideal model
- [ ] System-level integration testing

### Phase 4: Tubes & Refinement (Weeks 7-8)
- [ ] Koren triode model (primary)
- [ ] Dempwolf-Zolzer triode (secondary)
- [ ] Pentode model
- [ ] Performance optimization

---

**Report Generated:** January 28, 2026  
**Repository Analyzed:** https://github.com/dsharlet/LiveSPICE/master  
**Language:** C# with XML configuration  
**Total Components Documented:** 10 primary types, 280+ SPICE models in libraries
