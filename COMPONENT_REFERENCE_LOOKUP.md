# LiveSPICE Component Quick Reference - Lookup Data

**For DSP Implementation - Ready to Extract**

---

## DIODES - Complete Model Data

### Diode Parameter Sets (from Diodes.xml)
```
SILICON DIODES:
1N4001   | IS: 65.4 pA      | n: 1.36    | Application: 1A Rectifier
1N914    | IS: 2.52 nA      | n: 1.752   | Application: Switching
1N4148   | IS: 2.52 nA      | n: 1.752   | Application: Fast switching
1N4448   | IS: 0.1 pA       | n: 2.07    | Application: Ultra-fast (UltraFast)
1N34A    | IS: 2e-7 A       | n: 1.3     | Application: Germanium detector
1N916    | IS: 0.1 pA       | n: 2.07    | Application: Ultra-fast

GERMANIUM DIODES:
Generic  | IS: 1 µA         | n: 1.0     | Application: Generic Ge

ZENER DIODES:
1N4739A  | IS: 9.05 fA      | n: 1.1     | Application: Zener 13.3V
1N4742A  | IS: 6.87 fA      | n: 1.49    | Application: Zener 12V
1N4733   | IS: 1.214 fA     | n: -       | Application: Zener

LEDs:
Red LED      | IS: 93 pA    | n: 3.73    | Vf ≈ 1.8V
Green LED    | IS: 93 pA    | n: 4.61    | Vf ≈ 2.2V
Blue LED     | IS: 93 pA    | n: 7.61    | Vf ≈ 3.2V
```

### Diode Modeling Constants
```
Thermal Voltage: VT = 26.0 mV @ 25°C
Shockley Equation: I = IS * (exp(V/(n*VT)) - 1)
```

### Fast Diodes for Audio Circuits
```
PRIORITY IMPLEMENTATION:
- 1N4148:  Fastest, widely used in audio circuits, hard clipping
- 1N914:   Alternative to 1N4148, slightly different characteristics
- 1N4001:  Power rectifier, slower but common
```

---

## TRANSISTORS - Complete Model Data

### BJT - NPN Silicon (Most Common)
```
STANDARD SMALL-SIGNAL:
2N3904   | IS: 1e-14 A    | BF: 300    | BR: 4      | fT: ~300MHz | Usage: General
2N2222   | IS: 1e-14 A    | BF: 200    | BR: 3      | fT: ~250MHz | Usage: General
2N2222A  | IS: 14.34 fA   | BF: 255.9  | BR: 6.092  | fT: ~250MHz | Usage: Improved
2N4401   | IS: 26.03 fA   | BF: 4292   | BR: 1.01   | fT: ~100MHz | Usage: Very high gain

HIGH-GAIN AUDIO:
2N5088   | IS: 5.911 fA   | BF: 1122   | BR: 1.271  | fT: ~800MHz | Usage: Audio preamp
2N5089   | IS: 5.911 fA   | BF: 1434   | BR: 1.262  | fT: ~800MHz | Usage: Ultra high gain

AUDIO PREAMP (European):
BC107    | IS: 40.72 fA   | BF: 407    | BR: 7.41   | fT: ~100MHz | Usage: Audio
BC108    | IS: 40.72 fA   | BF: 381.7  | BR: 2.359  | fT: ~100MHz | Usage: Audio
BC109    | IS: 40.72 fA   | BF: 677    | BR: 2.209  | fT: ~100MHz | Usage: Audio, high gain
BC549    | IS: 4.679e-14A | BF: 458.7  | BR: 11.57  | fT: ~200MHz | Usage: Low noise audio

GERMANIUM (Vintage):
AC127 (NPN) | IS: 100 nA | BF: 90  | BR: 20 | Soft saturation, warm tone
Generic Ge  | IS: 100 nA | BF: 100 | BR: 10 | Typical Ge characteristics
```

### BJT - PNP Silicon
```
2N3906   | IS: 1e-14 A    | BF: 200    | BR: 4      | fT: ~300MHz | Usage: Complement to 2N3904
2N2907   | IS: 1e-14 A    | BF: 250    | BR: 3      | fT: ~250MHz | Usage: General
2N5087   | IS: 6.734 fA   | BF: 254.1  | BR: 2.683  | fT: ~100MHz | Usage: Audio
2SA1015  | IS: 10.0004 fA | BF: 239.283| BR: 5      | fT: ~200MHz | Usage: Audio

PNP AUDIO (European):
AC128   | IS: 100 nA (Ge) | BF: 90  | BR: 20 | Soft saturation, warm tone
```

### JFET - N-Channel
```
STANDARD:
2N5457   | IS: 1e-14 A    | Beta: 1.125e-3  | Vt0: -1.372 V  | Lambda: 2.3e-3  | Idss: ~12mA
2N5458   | IS: 1e-14 A    | Beta: 488.9e-6  | Vt0: -2.882 V  | Lambda: 3.167e-3 | Idss: ~10mA
2N5952   | IS: 1e-14 A    | Beta: 1.3143e-3 | Vt0: -2.021 V  | Lambda: 4e-3    | Idss: ~15mA

LOW-NOISE:
2N4392   | IS: 4.908e-16 A | Beta: 4.225e-3  | Vt0: -2.208 V  | Lambda: 6.403e-2
J112     | IS: 8.879e-16 A | Beta: 5.972e-3  | Vt0: -2.33 V   | Lambda: 23.198e-3

AUDIO BUFFER:
2N5457 is most common for audio input buffers
```

### JFET - P-Channel
```
J174     | IS: 9.7275e-16 A | Beta: 1.24461e-3 | Vt0: -7.1507 V | Lambda: 2.01968e-2 | Usage: P-channel audio
```

### BJT Formula (Ebers-Moll)
```
Forward Alpha:  aF = BF / (1 + BF)
Reverse Alpha:  aR = BR / (1 + BR)
Forward Diode:  iF = IS * exp(Vbe / VT) - 1
Reverse Diode:  iR = IS * exp(Vbc / VT) - 1
Collector Cur:  Ic = aF*iF - iR
Base Current:   Ib = (1-aF)*iF + (1-aR)*iR
Emitter Cur:    Ie = iF - aR*iR
```

### JFET Regions
```
Cutoff Region (Vgd >= Vt0):
  No current, Id = 0

Linear Region (Vds < |Vgd - Vt0|):
  Id = Beta * (1 + Lambda*|Vds|) * |Vds| * (2*(Vgd - Vt0) - |Vds|)

Saturation Region (Vds >= |Vgd - Vt0|):
  Id = Beta * (1 + Lambda*|Vds|) * (Vgd - Vt0)²
```

---

## DIODE TYPES - SHOCKLEY MODEL

### Basic Equation
```
I = IS * (exp(V / (n * VT)) - 1)

Where:
  IS = Saturation current (temperature dependent)
  n = Ideality factor (1.0 perfect diode, >1 real diode)
  VT = Thermal voltage = k*T/q ≈ 26 mV @ 25°C
```

### Typical n Values by Diode Type
```
Ideal diode:    n = 1.0
Silicon diodes: n = 1.0 to 1.2
Germanium:      n = 1.0 to 1.5
LEDs:           n = 1.5 to 8.0 (higher = softer turn-on)
Zener diodes:   n = 1.0 to 1.5 (reverse biased)
```

### IS Temperature Dependency
```
IS(T) = IS(T0) * (T/T0)^3 * exp( (q*Eg0/k) * (1/T0 - 1/T) )

Practical: At room temp
- Silicon diodes:    IS ≈ 0.1 - 100 pA
- Germanium diodes:  IS ≈ 1 - 100 nA (1000x higher)
```

---

## OPERATIONAL AMPLIFIERS - Complete Data

### Key OpAmp Parameters (from OpAmps.xml)
```
INPUT RESISTANCE (Rin):
  JFET input:     1-10 TΩ (very high impedance)
  Bipolar input:  1-40 MΩ (lower)
  
OUTPUT RESISTANCE (Rout):
  Typical:        10-100 Ω

OPEN-LOOP GAIN (Aol):
  Precision:      200k - 1M
  General:        100k - 300k
  Audio:          100k - 200k

GAIN-BANDWIDTH PRODUCT (GBP):
  Slow:           1 MHz (UA741, TL061)
  General:        3-10 MHz (TL07x, OP27)
  Fast:           35 MHz (AD826)
```

### OpAmp Model Data
```
CLASSIC/PRECISION:
UA741      | Rin: 2 MΩ     | Rout: 75 Ω   | Aol: 200k | GBP: 1 MHz      | Type: Bipolar
LM308      | Rin: 40 MΩ    | Rout: 50 Ω   | Aol: 300k | GBP: 1 MHz      | Type: Bipolar, Low-noise
TL06x      | Rin: 1 TΩ     | Rout: 50 Ω   | Aol: 315k | GBP: 1 MHz      | Type: JFET

AUDIO-GRADE:
TL07x      | Rin: 1 TΩ     | Rout: 50 Ω   | Aol: 200k | GBP: 5.25 MHz   | Type: JFET, Audio standard
OP27       | Rin: 4 MΩ     | Rout: 70 Ω   | Aol: 1.78M| GBP: 8 MHz      | Type: Precision audio

PRECISION:
OP42       | Rin: 1 TΩ     | Rout: 50 Ω   | Aol: 1 M  | GBP: 10 MHz     | Type: Chopper-stab, ultra-low offset
OP134      | Rin: 10 TΩ    | Rout: 10 Ω   | Aol: 1 M  | GBP: 8 MHz      | Type: JFET, Low noise
AD746      | Rin: 200 GΩ   | Rout: 10 Ω   | Aol: 282k | GBP: 13 MHz     | Type: Low distortion

HIGH-SPEED:
AD826      | Rin: 300 kΩ   | Rout: 8 Ω    | Aol: 5.6k | GBP: 35 MHz     | Type: Video, CdB, very fast

LOW-IMPEDANCE INPUT:
TL08x      | Rin: 1 TΩ     | Rout: 50 Ω   | Aol: 100k | GBP: 3 MHz      | Type: CMOS, fast
```

### OpAmp Frequency Response Model
```
Aol(f) = Aol(dc) / (1 + f/fp1)

Where:
  fp1 = GBP / Aol(dc)  (dominant pole)
  
Example for TL071:
  Aol(dc) = 200k
  GBP = 5.25 MHz
  fp1 = 5.25M / 200k = 26.25 Hz
  
Unity-gain bandwidth = GBP = 5.25 MHz (constant)
```

### Typical OpAmp Circuit Applications
```
VOLTAGE FOLLOWER (Unity Gain Buffer):
  Rin ≈ Rin(opamp)    [very high for JFET]
  Rout ≈ Rout(opamp)  [very low, ~50Ω]
  f3dB ≈ GBP

INVERTING AMPLIFIER (gain = -R2/R1):
  Rin ≈ R1            [set by input resistor]
  Rout ≈ Rout(opamp)  [determined by opamp]
  f3dB ≈ GBP / (1 + R2/R1)

NON-INVERTING AMPLIFIER (gain = 1 + R2/R1):
  Rin ≈ Rin(opamp)    [determined by opamp]
  Rout ≈ Rout(opamp)  [very small]
  f3dB ≈ GBP / (1 + R2/R1)
```

---

## VACUUM TUBES - Complete Model Data

### TRIODE MODELS (from Tubes.xml)

#### Preamp Triodes (12-series, 120V nominal)
```
12AT7/ECC81:  Mu: 67.49  | Kg: 419.1  | Kp: 213.96 | Kvb: 300  | Rgk: 1MΩ
              General-purpose preamp, moderate gain

12AU7/ECC82:  Mu: 21.5   | Kg: 1119   | Kp: 400    | Kvb: 300  | Rgk: 1MΩ
              High-current preamp, lower gain, faster response

12AX7/ECC83:  Mu: 83.5   | Kg: 1060   | Kp: 600    | Kvb: 300  | Rgk: 1MΩ
              Highest gain, most versatile, industry standard

12AY7:        Mu: 44.16  | Kg: 1192.4 | Kp: 409.96 | Kvb: 300  | Rgk: 1MΩ
              Intermediate gain

12AZ7:        Mu: 74.08  | Kg: 382    | Kp: 190.11 | Kvb: 300  | Rgk: 1MΩ
              Alternative high-gain preamp

6SL7GT:       Mu: 75.89  | Kg: 1735.2 | Kp: 1725.27| Kvb: 7.0  | Rgk: 2kΩ
              Larger bulb, high-power preamp
```

### PENTODE MODELS (from Tubes.xml) - Power Output

```
EL84-JJ:      Mu: 19.56  | Kg1: 593   | Kg2: 4500  | Kp: 131.34| Kvb: 17.5
              Pmax: ~12W | Rgi: 1kΩ | Application: Guitar amps (Vox, Marshall)

6L6GC-JJ:     Mu: 13.94  | Kg1: 320.2 | Kg2: 4500  | Kp: 33.2  | Kvb: 29.6
              Pmax: ~30W | Rgi: 1kΩ | Application: High-headroom amps

6V6GTA:       Mu: 10.7   | Kg1: 1672  | Kg2: 4500  | Kp: 41.16 | Kvb: 12.7
              Pmax: ~14W | Rgi: 2kΩ | Application: Lower-power amps

EL34-JJ:      Mu: 11.55  | Kg1: 650.2 | Kg2: 6000  | Kp: 51.85 | Kvb: 25.4
              Pmax: ~35W | Rgi: 1kΩ | Application: European amps (Mesa, Engl)

6550:         Mu: 7.9    | Kg1: 890   | Kg2: 4200  | Kp: 60    | Kvb: 24
              Pmax: ~40W | Rgi: 1kΩ | Application: High-power amps

KT88-JJ:      Mu: 12.43  | Kg1: 315.7 | Kg2: 4500  | Kp: 26.72 | Kvb: 36.1
              Pmax: ~50W | Rgi: 1kΩ | Application: Ultra-high-power amps
```

### Triode Koren Model Equations
```
MAIN PLATE CURRENT:
E1 = Ln1Exp( Kp * (1/Mu + Vgk/sqrt(Kvb + Vpk²)) ) * Vpk / Kp
Ip = (E1 > 0) ? 2*(E1^Ex)/Kg : 0

GRID CURRENT (smooth knee):
if (Vgk < Vg - Kn):
    Ig = 0
else if (Vgk > Vg + Kn):
    Ig = (Vgk - Vg) / Rgk
else:
    // Quadratic knee transition
    a = 1 / (4 * Kn * Rgk)
    b = (Kn - Vg) / (2 * Kn * Rgk)
    c = -a*(Vg - Kn)² - b*(Vg - Kn)
    Ig = a*Vgk² + b*Vgk + c

CATHODE CURRENT:
Ik = -(Ip + Ig)

CAPACITANCES (optional):
Cgp = Grid-Plate capacitance (≈ 2.4 pF for 12AX7)
Cgk = Grid-Cathode capacitance (≈ 2.3 pF for 12AX7)
Cpk = Plate-Cathode capacitance (≈ 0.9 pF for 12AX7)
```

### Special Functions
```
Ln1Exp(x) = ln(1 + exp(x))
  For x > 50: Ln1Exp(x) ≈ x (prevents overflow)
  For x < -50: Ln1Exp(x) ≈ exp(x) (prevents underflow)
  Formula: Use special implementation for stability
```

### Tube Model Parameters by Category

**VOLTAGE GAIN (Mu):**
- Low Mu (5-15):  Output tubes, high current, soft characteristics
- Mid Mu (20-50): Output preamp, balanced
- High Mu (60+):  Preamp input, sensitive gain

**KG COEFFICIENT:**
- Affects plate current slope and saturation
- Higher Kg = harder saturation, steeper curves

**KP COEFFICIENT:**
- Affects voltage-dependent behavior
- Kp/Kg ratio determines knee softness

---

## COMPONENT DATABASE SCHEMA

### Recommended C++ Structure
```cpp
struct DiodeModel {
    std::string part_number;
    double IS;           // A (Saturation current)
    double n;            // Ideality factor
    std::string type;    // "Diode", "LED", "Zener"
};

struct BJTModel {
    std::string part_number;
    double IS;           // A (Saturation current)
    double BF;           // Forward beta
    double BR;           // Reverse beta
    std::string type;    // "NPN", "PNP"
};

struct JFETModel {
    std::string part_number;
    double IS;           // A
    double Vt0;          // V (Threshold voltage)
    double Beta;         // A/V² (Transconductance)
    double Lambda;       // 1/V (Channel modulation)
    std::string type;    // "N", "P"
};

struct OpAmpModel {
    std::string part_number;
    double Rin;          // Ω (Input impedance)
    double Rout;         // Ω (Output impedance)
    double Aol;          // V/V (Open-loop gain)
    double GBP;          // Hz (Gain-bandwidth product)
};

struct TriodeModel {
    std::string part_number;
    double Mu;           // Amplification factor
    double Kg, Kp, Kvb;  // Koren coefficients
    double Rgk;          // Ω
    double Kn;           // V
    double Vg;           // V
};

struct ComponentLibrary {
    std::map<std::string, DiodeModel> diodes;
    std::map<std::string, BJTModel> bjts;
    std::map<std::string, JFETModel> jfets;
    std::map<std::string, OpAmpModel> opamps;
    std::map<std::string, TriodeModel> triodes;
};
```

---

## SPICE NETLIST EXAMPLE

### Simple Diode Clipping Circuit
```spice
.title Diode Clipper
* Input → 1N4148 to ground with 10k pullup

V1 IN 0 DC 0 AC 1
R1 IN DIODE 10k
D1 DIODE 0 D1N4148
.model D1N4148 D (IS=2.52n N=1.752)
.end
```

### BJT Amplifier
```spice
.title BJT Amplifier
* Collector supply
VCC 1 0 DC 12
* Base bias
VB BASE 0 DC 0.7
* BJT (2N3904)
Q1 COLL BASE 0 Q2N3904
RC 1 COLL 4.7k
.model Q2N3904 NPN (BF=300 BR=4 IS=1e-14)
.end
```

### OpAmp Inverting Amplifier
```spice
.title OpAmp Inverting Amp
* +15V and -15V supplies
VCC 1 0 DC 15
VEE 2 0 DC -15
* Signal input
VIN IN 0 AC 1
* Resistors: 1k input, 10k feedback
RIN IN NINV 1k
RF NINV OUT 10k
* Ideal OpAmp (simplified)
XAMP INV NINV 1 2 OUT OPAMP
.model OPAMP ... (OpAmp netlist)
.end
```

---

## COMPONENT SELECTION GUIDANCE

### For Guitar Pedals (Most Common)
1. **Clipping diodes:**
   - 1N4148 - Standard, sharp clipping
   - 1N4001 - Softer clipping, warmer
   - 1N34A - Germanium, very soft

2. **Preamp transistors (small-signal):**
   - 2N3904/2N3906 - Standard, widely available
   - BC107-109 - European, audio-grade
   - 2N5088 - High gain, audio applications

3. **Op-amps:**
   - TL072 - Dual, low noise, standard
   - NE5532 - Audio-grade precision
   - LM358 - Single supply, general purpose

4. **Tubes (rare in pedals, used in amps):**
   - 12AX7 - Standard preamp tube
   - EL84 - Standard power amp tube
   - 6L6 - Higher power output

### For DSP Simulation Priority
**TIER 1 (Essential):**
- 1N4148 diode model
- 2N3904 BJT model
- TL072 OpAmp model

**TIER 2 (Important):**
- 1N4001 diode (alternative)
- 1N34A (Ge variant)
- 2N3906 BJT (PNP)
- 2N5457 JFET (buffer)

**TIER 3 (Nice to have):**
- Remaining transistor variants
- Tube models (TL071 audio)
- High-speed OpAmps

---

## Unit Conversion Quick Reference

### Frequency
```
Hz to Angular Freq:  ω = 2π*f
Example: 1 kHz → ω = 6283 rad/s
```

### Impedance (at 1kHz)
```
Resistance:    Z = R (frequency independent)
Capacitance:   Z = 1/(2πfC)
Inductance:    Z = 2πfL
```

### Common Audio Impedances
```
Line level:     10k - 100k Ω
Microphone:     600 Ω - 2k Ω
Speaker:        4 Ω - 8 Ω
Op-amp output:  50 - 100 Ω
Op-amp input:   > 1 MΩ (JFET type)
```

### Power Dissipation
```
P = V * I = V² / R = I² * R

Example: 1 kΩ resistor at 1V
P = 1² / 1000 = 1 mW (safe)
```

---

## References for Further Study

1. **Shockley Diode Equation:**
   - https://en.wikipedia.org/wiki/Diode_modelling

2. **Ebers-Moll BJT Model:**
   - http://people.seas.harvard.edu/~jones/es154/lectures/lecture_3/bjt_models/ebers_moll/

3. **JFET Modeling:**
   - Analogous to Schichman-Hodges MOSFET model

4. **OpAmp Design:**
   - http://www.ecircuitcenter.com/OpModels/OpampModels.htm
   - http://qucs.sourceforge.net/docs/opamp.pdf

5. **Tube Modeling (Koren):**
   - Reference: Standard SPICE library models
   - Koren, Y. "A nonlinear dynamic model of the vacuum tube triode"

6. **LiveSPICE Source:**
   - https://github.com/dsharlet/LiveSPICE/

---

**Last Updated:** January 28, 2026  
**Data Sources:** LiveSPICE repository (Diodes.xml, Transistors.xml, OpAmps.xml, Tubes.xml)  
**Component Count:** 280+ SPICE models documented
