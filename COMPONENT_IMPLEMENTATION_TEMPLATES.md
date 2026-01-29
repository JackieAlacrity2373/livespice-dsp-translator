# LiveSPICE Component DSP Implementation Templates

**C++ Code Stubs for DSP Implementation**

---

## Table of Contents
1. [Component Base Classes](#component-base-classes)
2. [Passive Components](#passive-components)
3. [Diode Implementation](#diode-implementation)
4. [BJT Implementation](#bjt-implementation)
5. [JFET Implementation](#jfet-implementation)
6. [OpAmp Implementation](#opamp-implementation)
7. [Tube Implementation](#tube-implementation)
8. [Component Library System](#component-library-system)

---

## Component Base Classes

### Base Component Interface
```cpp
// Component.h
#pragma once
#include <vector>
#include <string>

namespace Spice {

class Node;

class Component {
public:
    virtual ~Component() = default;
    
    // Pure virtual - all components must implement these
    virtual void analyze(double dt) = 0;
    virtual double getVoltage() const = 0;
    virtual double getCurrent() const = 0;
    
    // Getters
    virtual const std::string& getPartNumber() const { return partNumber; }
    virtual const std::string& getName() const { return name; }
    
protected:
    std::string name;
    std::string partNumber;
    double voltage = 0.0;
    double current = 0.0;
};

// Two-terminal component (most common)
class TwoTerminal : public Component {
public:
    TwoTerminal(Node* anode, Node* cathode) 
        : anode(anode), cathode(cathode) {}
    
    double getVoltageAcross() const;
    
protected:
    Node* anode;
    Node* cathode;
};

// Three-terminal component (transistors)
class ThreeTerminal : public Component {
public:
    ThreeTerminal(Node* n1, Node* n2, Node* n3) 
        : n1(n1), n2(n2), n3(n3) {}
    
protected:
    Node* n1;
    Node* n2;
    Node* n3;
};

} // namespace Spice
```

---

## Passive Components

### Resistor
```cpp
// Resistor.h
#pragma once
#include "Component.h"

namespace Spice {

class Resistor : public TwoTerminal {
public:
    Resistor(Node* anode, Node* cathode, double resistance);
    
    void analyze(double dt) override;
    double getVoltage() const override { return voltage; }
    double getCurrent() const override { return current; }
    
    double getResistance() const { return R; }
    void setResistance(double newR) { R = newR; }
    
private:
    double R;  // Resistance in Ohms
};

} // namespace Spice
```

```cpp
// Resistor.cpp
#include "Resistor.h"
#include "Node.h"

namespace Spice {

Resistor::Resistor(Node* anode, Node* cathode, double resistance)
    : TwoTerminal(anode, cathode), R(resistance) {
    name = "R1";
}

void Resistor::analyze(double dt) {
    // Ohm's law: I = V / R
    voltage = anode->getVoltage() - cathode->getVoltage();
    
    if (R > 0.0) {
        current = voltage / R;
    } else {
        // Short circuit
        current = 0.0;  // Would be handled by solver
    }
}

} // namespace Spice
```

### Capacitor
```cpp
// Capacitor.h
#pragma once
#include "Component.h"

namespace Spice {

class Capacitor : public TwoTerminal {
public:
    Capacitor(Node* anode, Node* cathode, double capacitance);
    
    void analyze(double dt) override;
    double getVoltage() const override { return voltage; }
    double getCurrent() const override { return current; }
    
    double getCapacitance() const { return C; }
    void setCapacitance(double newC) { C = newC; }
    
private:
    double C;                    // Capacitance in Farads
    double previousVoltage = 0.0;
    double previousCurrent = 0.0;
};

} // namespace Spice
```

```cpp
// Capacitor.cpp
#include "Capacitor.h"
#include "Node.h"

namespace Spice {

Capacitor::Capacitor(Node* anode, Node* cathode, double capacitance)
    : TwoTerminal(anode, cathode), C(capacitance) {
    name = "C1";
}

void Capacitor::analyze(double dt) {
    // i = C * dV/dt
    voltage = anode->getVoltage() - cathode->getVoltage();
    
    double dV = voltage - previousVoltage;
    
    // Numerical differentiation
    if (dt > 0.0) {
        current = C * (dV / dt);
    }
    
    previousVoltage = voltage;
    previousCurrent = current;
}

} // namespace Spice
```

### Inductor
```cpp
// Inductor.h
#pragma once
#include "Component.h"

namespace Spice {

class Inductor : public TwoTerminal {
public:
    Inductor(Node* anode, Node* cathode, double inductance);
    
    void analyze(double dt) override;
    double getVoltage() const override { return voltage; }
    double getCurrent() const override { return current; }
    
    double getInductance() const { return L; }
    void setInductance(double newL) { L = newL; }
    
private:
    double L;                   // Inductance in Henries
    double currentState = 0.0;
    double previousCurrent = 0.0;
};

} // namespace Spice
```

```cpp
// Inductor.cpp
#include "Inductor.h"
#include "Node.h"

namespace Spice {

Inductor::Inductor(Node* anode, Node* cathode, double inductance)
    : TwoTerminal(anode, cathode), L(inductance) {
    name = "L1";
}

void Inductor::analyze(double dt) {
    // V = L * di/dt
    // Uses state variable for current
    
    voltage = anode->getVoltage() - cathode->getVoltage();
    
    // Numerical integration of current
    if (dt > 0.0 && L > 0.0) {
        double di = voltage * dt / L;
        currentState += di;
        current = currentState;
    }
    
    previousCurrent = current;
}

} // namespace Spice
```

---

## Diode Implementation

### Diode Model Header
```cpp
// Diode.h
#pragma once
#include "Component.h"
#include <cmath>

namespace Spice {

class DiodeModel {
public:
    // Standard SPICE parameters
    double IS = 1e-12;    // Saturation current (A)
    double n = 1.0;       // Ideality factor
    
    // Additional parameters (for completeness)
    double VT = 0.026;    // Thermal voltage (V) @ 25°C
    
    // Load from library
    static DiodeModel loadModel(const std::string& partNumber);
};

class Diode : public TwoTerminal {
public:
    Diode(Node* anode, Node* cathode, const DiodeModel& model);
    
    void analyze(double dt) override;
    double getVoltage() const override { return voltage; }
    double getCurrent() const override { return current; }
    
    const DiodeModel& getModel() const { return model; }
    void setModel(const DiodeModel& newModel) { model = newModel; }
    
private:
    DiodeModel model;
    
    // Shockley equation helper
    double exponentialClip(double x);
};

} // namespace Spice
```

### Diode Implementation
```cpp
// Diode.cpp
#include "Diode.h"
#include "Node.h"
#include <algorithm>
#include <cmath>

namespace Spice {

// Thermal voltage constant at 25°C
const double k_B = 1.38064852e-23;  // Boltzmann constant
const double q = 1.60217662e-19;    // Electron charge
const double T = 298.15;             // Temperature (K)
const double VT = k_B * T / q;       // ≈ 0.0259 V

Diode::Diode(Node* anode, Node* cathode, const DiodeModel& model)
    : TwoTerminal(anode, cathode), model(model) {
    name = "D1";
}

double Diode::exponentialClip(double x) {
    // Prevent numerical overflow
    const double MAX_EXP = 100.0;
    
    if (x > MAX_EXP) {
        return std::exp(MAX_EXP) * (1.0 + x - MAX_EXP);
    } else if (x < -MAX_EXP) {
        return 0.0;
    } else {
        return std::exp(x);
    }
}

void Diode::analyze(double dt) {
    // Shockley diode equation:
    // I = IS * (exp(V / (n * VT)) - 1)
    
    voltage = anode->getVoltage() - cathode->getVoltage();
    
    double denominator = model.n * VT;
    double exponent = voltage / denominator;
    
    // exp(V / (n*VT)) - 1
    double expTerm = exponentialClip(exponent) - 1.0;
    
    current = model.IS * expTerm;
    
    // Limit current for stability
    const double MAX_CURRENT = 1.0;
    current = std::clamp(current, -MAX_CURRENT, MAX_CURRENT);
}

// Load model from part number
DiodeModel DiodeModel::loadModel(const std::string& partNumber) {
    DiodeModel model;
    
    // Diode library lookup
    if (partNumber == "1N4148") {
        model.IS = 2.52e-9;     // 2.52 nA
        model.n = 1.752;
    } else if (partNumber == "1N4001") {
        model.IS = 65.4e-12;    // 65.4 pA
        model.n = 1.36;
    } else if (partNumber == "1N34A") {
        model.IS = 2e-7;        // 200 nA (Germanium)
        model.n = 1.3;
    } else if (partNumber == "Si") {
        // Generic silicon
        model.IS = 1e-12;
        model.n = 1.0;
    } else if (partNumber == "Ge") {
        // Generic germanium
        model.IS = 1e-6;
        model.n = 1.0;
    }
    
    return model;
}

} // namespace Spice
```

---

## BJT Implementation

### BJT Model Header
```cpp
// BipolarJunctionTransistor.h
#pragma once
#include "Component.h"
#include "Diode.h"

namespace Spice {

enum class BJTType { NPN, PNP };

class BJTModel {
public:
    double IS = 1e-12;    // Saturation current (A)
    double BF = 100.0;    // Forward beta (current gain)
    double BR = 1.0;      // Reverse beta
    double VT = 0.026;    // Thermal voltage
    
    static BJTModel loadModel(const std::string& partNumber);
};

class BipolarJunctionTransistor : public ThreeTerminal {
public:
    BipolarJunctionTransistor(Node* collector, Node* base, Node* emitter,
                             BJTType type, const BJTModel& model);
    
    void analyze(double dt) override;
    double getVoltage() const override;
    double getCurrent() const override;
    
    double getCollectorCurrent() const { return ic; }
    double getBaseCurrent() const { return ib; }
    double getEmitterCurrent() const { return ie; }
    
private:
    BJTType type;
    BJTModel model;
    
    // Terminal currents
    double ic = 0.0;  // Collector current
    double ib = 0.0;  // Base current
    double ie = 0.0;  // Emitter current
    
    double exponentialClip(double x);
};

} // namespace Spice
```

### BJT Implementation (Ebers-Moll Model)
```cpp
// BipolarJunctionTransistor.cpp
#include "BipolarJunctionTransistor.h"
#include "Node.h"
#include <cmath>
#include <algorithm>

namespace Spice {

const double VT = 0.026;  // Thermal voltage at 25°C

BipolarJunctionTransistor::BipolarJunctionTransistor(
    Node* collector, Node* base, Node* emitter,
    BJTType type, const BJTModel& model)
    : ThreeTerminal(collector, base, emitter), type(type), model(model) {
    name = "Q1";
}

double BipolarJunctionTransistor::exponentialClip(double x) {
    const double MAX_EXP = 100.0;
    
    if (x > MAX_EXP) {
        return std::exp(MAX_EXP) * (1.0 + x - MAX_EXP);
    } else if (x < -MAX_EXP) {
        return 0.0;
    } else {
        return std::exp(x);
    }
}

void BipolarJunctionTransistor::analyze(double dt) {
    // Ebers-Moll BJT model
    
    int sign = (type == BJTType::NPN) ? 1 : -1;
    
    double Vc = n1->getVoltage();  // Collector
    double Vb = n2->getVoltage();  // Base
    double Ve = n3->getVoltage();  // Emitter
    
    // Voltage drops
    double Vbc = sign * (Vb - Vc);
    double Vbe = sign * (Vb - Ve);
    
    // Forward and reverse alphas
    double aF = model.BF / (1.0 + model.BF);
    double aR = model.BR / (1.0 + model.BR);
    
    // Diode equations
    double iF = model.IS * (exponentialClip(Vbe / VT) - 1.0);
    double iR = model.IS * (exponentialClip(Vbc / VT) - 1.0);
    
    // Terminal currents (Ebers-Moll)
    ic = aF * iF - iR;
    ib = (1.0 - aF) * iF + (1.0 - aR) * iR;
    ie = iF - aR * iR;
    
    // Apply sign convention for PNP
    if (type == BJTType::PNP) {
        ic = -ic;
        ib = -ib;
        ie = -ie;
    }
}

double BipolarJunctionTransistor::getVoltage() const {
    return n1->getVoltage() - n3->getVoltage();  // Collector-Emitter
}

double BipolarJunctionTransistor::getCurrent() const {
    return ic;  // Collector current
}

BJTModel BJTModel::loadModel(const std::string& partNumber) {
    BJTModel model;
    
    if (partNumber == "2N3904") {
        model.IS = 1e-14;
        model.BF = 300.0;
        model.BR = 4.0;
    } else if (partNumber == "2N3906") {
        model.IS = 1e-14;
        model.BF = 200.0;
        model.BR = 4.0;
    } else if (partNumber == "BC107") {
        model.IS = 40.72e-15;
        model.BF = 407.0;
        model.BR = 7.41;
    }
    
    return model;
}

} // namespace Spice
```

---

## JFET Implementation

### JFET Model Header
```cpp
// JFET.h
#pragma once
#include "Component.h"

namespace Spice {

enum class JFETType { N_CHANNEL, P_CHANNEL };

class JFETModel {
public:
    double IS = 1e-14;        // Gate-source saturation current (A)
    double n = 1.0;           // Gate emission coefficient
    double Vt0 = -2.0;        // Threshold voltage (V)
    double Beta = 1e-4;       // Transconductance (A/V²)
    double Lambda = 0.0;      // Channel length modulation (1/V)
    
    static JFETModel loadModel(const std::string& partNumber);
};

class JFET : public ThreeTerminal {
public:
    JFET(Node* drain, Node* gate, Node* source,
         JFETType type, const JFETModel& model);
    
    void analyze(double dt) override;
    double getVoltage() const override;
    double getCurrent() const override;
    
    double getDrainCurrent() const { return id; }
    
private:
    JFETType type;
    JFETModel model;
    double id = 0.0;  // Drain current
    
    // Check if operating in linear or saturation region
    bool isInSaturation(double Vgs, double Vds) const;
};

} // namespace Spice
```

### JFET Implementation
```cpp
// JFET.cpp
#include "JFET.h"
#include "Node.h"
#include <cmath>
#include <algorithm>

namespace Spice {

JFET::JFET(Node* drain, Node* gate, Node* source,
           JFETType type, const JFETModel& model)
    : ThreeTerminal(drain, gate, source), type(type), model(model) {
    name = "J1";
}

bool JFET::isInSaturation(double Vgs, double Vds) const {
    // Saturation when |Vds| >= |Vgs - Vt0|
    return std::abs(Vds) >= std::abs(Vgs - model.Vt0);
}

void JFET::analyze(double dt) {
    // JFET quadratic model
    
    double Vd = n1->getVoltage();   // Drain
    double Vg = n2->getVoltage();   // Gate
    double Vs = n3->getVoltage();   // Source
    
    // Terminal voltages
    double Vds = Vd - Vs;
    double Vgs = Vg - Vs;
    double Vgd = Vg - Vd;
    
    // Reverse voltage for p-channel
    if (type == JFETType::P_CHANNEL) {
        Vds = -Vds;
        Vgs = -Vgs;
        Vgd = -Vgd;
    }
    
    // Pinch-off check
    if (Vgs < model.Vt0) {
        // Cutoff region
        id = 0.0;
    } else {
        // Conduct
        double Vgd_Vt = Vgs - model.Vt0;
        double absVds = std::abs(Vds);
        
        if (absVds < std::abs(Vgd_Vt)) {
            // LINEAR REGION
            id = std::copysign(1.0, Vds) * 
                 (Vgs >= model.Vt0) * 
                 model.Beta * (1.0 + model.Lambda * absVds) *
                 absVds * (2.0 * Vgd_Vt - 1.0);
        } else {
            // SATURATION REGION
            id = std::copysign(1.0, Vds) * 
                 (Vgs >= model.Vt0) * 
                 model.Beta * (1.0 + model.Lambda * absVds) *
                 (Vgd_Vt * Vgd_Vt);
        }
    }
    
    // Reverse for p-channel
    if (type == JFETType::P_CHANNEL) {
        id = -id;
    }
}

double JFET::getVoltage() const {
    return n1->getVoltage() - n3->getVoltage();  // Drain-Source
}

double JFET::getCurrent() const {
    return id;
}

JFETModel JFETModel::loadModel(const std::string& partNumber) {
    JFETModel model;
    
    if (partNumber == "2N5457") {
        model.IS = 1e-14;
        model.Vt0 = -1.372;
        model.Beta = 1.125e-3;
        model.Lambda = 2.3e-3;
    } else if (partNumber == "J112") {
        model.IS = 8.879e-16;
        model.Vt0 = -2.33;
        model.Beta = 5.972e-3;
        model.Lambda = 23.198e-3;
    }
    
    return model;
}

} // namespace Spice
```

---

## OpAmp Implementation

### OpAmp Model Header
```cpp
// OpAmp.h
#pragma once
#include "Component.h"

namespace Spice {

class OpAmpModel {
public:
    double Rin = 1e6;      // Input resistance (Ω)
    double Rout = 100.0;   // Output resistance (Ω)
    double Aol = 1e6;      // Open-loop gain (V/V)
    double GBP = 1e6;      // Gain-bandwidth product (Hz)
    double VccPositive = 15.0;   // +Vcc
    double VccNegative = -15.0;  // -Vcc
    
    static OpAmpModel loadModel(const std::string& partNumber);
};

class OpAmp : public Component {
public:
    // 5-terminal device: +in, -in, output, +Vcc, -Vcc
    OpAmp(Node* posInput, Node* negInput, Node* output,
          Node* vccPos, Node* vccNeg, const OpAmpModel& model);
    
    void analyze(double dt) override;
    double getVoltage() const override { return vout; }
    double getCurrent() const override { return iout; }
    
private:
    Node* posInput;
    Node* negInput;
    Node* output;
    Node* vccPos;
    Node* vccNeg;
    
    OpAmpModel model;
    
    // Internal nodes
    double vout = 0.0;
    double iout = 0.0;
    
    // Pole compensation filter state
    double integratorState = 0.0;
    
    double saturate(double value);
};

} // namespace Spice
```

### OpAmp Implementation
```cpp
// OpAmp.cpp
#include "OpAmp.h"
#include "Node.h"
#include <algorithm>
#include <cmath>

namespace Spice {

OpAmp::OpAmp(Node* posInput, Node* negInput, Node* output,
             Node* vccPos, Node* vccNeg, const OpAmpModel& model)
    : posInput(posInput), negInput(negInput), output(output),
      vccPos(vccPos), vccNeg(vccNeg), model(model) {
    name = "X1";
}

double OpAmp::saturate(double value) {
    // Limit output to rail voltages
    double maxOut = model.VccPositive - 2.0;  // 2V dropout
    double minOut = model.VccNegative + 2.0;
    
    return std::clamp(value, minOut, maxOut);
}

void OpAmp::analyze(double dt) {
    // Behavioral op-amp model with frequency response
    
    double vp = posInput->getVoltage();
    double vn = negInput->getVoltage();
    double vdiff = vp - vn;
    
    // Input voltage drop across Rin
    double inputCurrent = vdiff / model.Rin;
    
    // Open-loop DC gain stage
    double dcOutput = model.Aol * vdiff;
    
    // Single-pole frequency response
    // Pole frequency: fp = GBP / Aol
    double fp = model.GBP / model.Aol;
    double wc = 2.0 * M_PI * fp;  // Cutoff frequency
    
    // First-order system: tau = 1/wc
    double tau = 1.0 / wc;
    
    // Integration step (simple Euler method)
    if (dt > 0.0) {
        double deriv = (dcOutput - integratorState) / tau;
        integratorState += deriv * dt;
    }
    
    double filteredOutput = integratorState;
    
    // Output resistance
    double voutUnloaded = saturate(filteredOutput);
    
    // Output current calculation (from load perspective)
    iout = (voutUnloaded - output->getVoltage()) / model.Rout;
    
    vout = voutUnloaded;
}

OpAmpModel OpAmpModel::loadModel(const std::string& partNumber) {
    OpAmpModel model;
    
    if (partNumber == "TL072") {
        model.Rin = 1e12;
        model.Rout = 50.0;
        model.Aol = 200e3;
        model.GBP = 5.25e6;
    } else if (partNumber == "UA741") {
        model.Rin = 2e6;
        model.Rout = 75.0;
        model.Aol = 200e3;
        model.GBP = 1e6;
    } else if (partNumber == "OP27") {
        model.Rin = 4e6;
        model.Rout = 70.0;
        model.Aol = 1.78e6;
        model.GBP = 8e6;
    }
    
    return model;
}

} // namespace Spice
```

---

## Tube Implementation

### Triode Model Header
```cpp
// Triode.h
#pragma once
#include "Component.h"

namespace Spice {

enum class TriodeModel { KOREN, LANGMUIR, DEMPWOLF_ZOLZER };

class TriodeModelParams {
public:
    // Common
    double Mu = 100.0;    // Amplification factor
    
    // Koren model
    double Kg = 1060.0;   // Grid coefficient
    double Kp = 600.0;    // Plate coefficient
    double Kvb = 300.0;   // Bias voltage
    double Rgk = 1e6;     // Grid-cathode resistance (Ω)
    double Kn = 0.5;      // Knee size (V)
    double Vg = 0.33;     // Grid voltage offset (V)
    double Ex = 1.4;      // Exponent
    
    // Capacitances
    double Cgp = 2.4e-12; // Grid-plate (F)
    double Cgk = 2.3e-12; // Grid-cathode (F)
    double Cpk = 0.9e-12; // Plate-cathode (F)
    
    static TriodeModelParams loadModel(const std::string& partNumber);
};

class Triode : public ThreeTerminal {
public:
    Triode(Node* plate, Node* grid, Node* cathode,
           TriodeModel modelType, const TriodeModelParams& params);
    
    void analyze(double dt) override;
    double getVoltage() const override;
    double getCurrent() const override;
    
    double getPlateCurrent() const { return ip; }
    double getGridCurrent() const { return ig; }
    
private:
    TriodeModel modelType;
    TriodeModelParams params;
    
    double ip = 0.0;  // Plate current
    double ig = 0.0;  // Grid current
    double ik = 0.0;  // Cathode current
    
    // Koren model implementation
    void analyzeKoren(double Vpk, double Vgk);
    
    double Ln1Exp(double x);
};

} // namespace Spice
```

### Triode Implementation (Koren Model)
```cpp
// Triode.cpp
#include "Triode.h"
#include "Node.h"
#include <cmath>
#include <algorithm>

namespace Spice {

Triode::Triode(Node* plate, Node* grid, Node* cathode,
               TriodeModel modelType, const TriodeModelParams& params)
    : ThreeTerminal(plate, grid, cathode), modelType(modelType), params(params) {
    name = "V1";
}

double Triode::Ln1Exp(double x) {
    // ln(1 + exp(x)) with numerical stability
    const double THRESHOLD = 50.0;
    
    if (x > THRESHOLD) {
        // For large x: ln(1 + exp(x)) ≈ x
        return x;
    } else if (x < -THRESHOLD) {
        // For small x: ln(1 + exp(x)) ≈ exp(x)
        return std::exp(x);
    } else {
        return std::log(1.0 + std::exp(x));
    }
}

void Triode::analyzeKoren(double Vpk, double Vgk) {
    // Koren model - most accurate for audio tubes
    
    // E1 calculation
    double sqrtTerm = std::sqrt(params.Kvb + Vpk * Vpk);
    double argument = params.Kp * (1.0 / params.Mu + Vgk / sqrtTerm);
    double E1 = Ln1Exp(argument) * Vpk / params.Kp;
    
    // Plate current
    if (E1 > 0.0) {
        double exp_term = std::pow(E1, params.Ex);
        ip = 2.0 * exp_term / params.Kg;
    } else {
        ip = 0.0;
    }
    
    // Grid current with smooth knee
    double knee = params.Kn;
    double vgOffset = params.Vg;
    
    if (Vgk < vgOffset - knee) {
        // Cutoff
        ig = 0.0;
    } else if (Vgk > vgOffset + knee) {
        // Ohmic
        ig = (Vgk - vgOffset) / params.Rgk;
    } else {
        // Smooth knee region
        double a = 1.0 / (4.0 * knee * params.Rgk);
        double b = (knee - vgOffset) / (2.0 * knee * params.Rgk);
        double c = -a * (vgOffset - knee) * (vgOffset - knee) 
                   - b * (vgOffset - knee);
        
        ig = a * Vgk * Vgk + b * Vgk + c;
    }
    
    // Cathode current (conservation)
    ik = -(ip + ig);
}

void Triode::analyze(double dt) {
    // Get terminal voltages
    double Vp = n1->getVoltage();   // Plate
    double Vg = n2->getVoltage();   // Grid
    double Vk = n3->getVoltage();   // Cathode
    
    // Relative voltages
    double Vpk = Vp - Vk;
    double Vgk = Vg - Vk;
    
    // Choose model
    switch (modelType) {
        case TriodeModel::KOREN:
            analyzeKoren(Vpk, Vgk);
            break;
        
        case TriodeModel::LANGMUIR:
            // Simplified Child-Langmuir model
            {
                double Ed = params.Mu * Vgk + Vpk;
                if (Ed > 0.0) {
                    ip = params.Kg * std::pow(Ed, 1.5);
                } else {
                    ip = 0.0;
                }
                ig = 0.0;
                ik = -ip;
            }
            break;
        
        case TriodeModel::DEMPWOLF_ZOLZER:
            // Complex model with capacitances
            // TODO: Implement Dempwolf-Zolzer equations
            break;
    }
}

double Triode::getVoltage() const {
    return n1->getVoltage() - n3->getVoltage();  // Plate-Cathode
}

double Triode::getCurrent() const {
    return ip;  // Plate current
}

TriodeModelParams TriodeModelParams::loadModel(const std::string& partNumber) {
    TriodeModelParams params;
    
    if (partNumber == "12AX7") {
        params.Mu = 83.5;
        params.Kg = 1060.0;
        params.Kp = 600.0;
        params.Kvb = 300.0;
        params.Rgk = 1e6;
        params.Vg = 0.33;
        params.Kn = 0.5;
    } else if (partNumber == "12AT7") {
        params.Mu = 67.49;
        params.Kg = 419.1;
        params.Kp = 213.96;
        params.Kvb = 300.0;
        params.Rgk = 1e6;
        params.Vg = 0.33;
        params.Kn = 0.5;
    } else if (partNumber == "6SL7GT") {
        params.Mu = 75.89;
        params.Kg = 1735.2;
        params.Kp = 1725.27;
        params.Kvb = 7.0;
        params.Rgk = 2000.0;
    }
    
    return params;
}

} // namespace Spice
```

---

## Component Library System

### Library Manager Header
```cpp
// ComponentLibrary.h
#pragma once
#include <map>
#include <string>
#include <memory>

namespace Spice {

class ComponentLibrary {
public:
    // Load libraries from XML files
    bool loadDiodeLibrary(const std::string& filename);
    bool loadTransistorLibrary(const std::string& filename);
    bool loadOpAmpLibrary(const std::string& filename);
    bool loadTubeLibrary(const std::string& filename);
    
    // Lookup component models
    DiodeModel* getDiodeModel(const std::string& partNumber);
    BJTModel* getBJTModel(const std::string& partNumber);
    JFETModel* getJFETModel(const std::string& partNumber);
    OpAmpModel* getOpAmpModel(const std::string& partNumber);
    TriodeModelParams* getTriodeModel(const std::string& partNumber);
    
private:
    std::map<std::string, DiodeModel> diodes;
    std::map<std::string, BJTModel> bjts;
    std::map<std::string, JFETModel> jfets;
    std::map<std::string, OpAmpModel> opamps;
    std::map<std::string, TriodeModelParams> triodes;
};

} // namespace Spice
```

### XML Parser Example
```cpp
// ComponentLibrary.cpp
#include "ComponentLibrary.h"
#include <pugixml.hpp>
#include <iostream>

namespace Spice {

bool ComponentLibrary::loadDiodeLibrary(const std::string& filename) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());
    
    if (!result) {
        std::cerr << "Failed to load XML: " << result.description() << std::endl;
        return false;
    }
    
    for (pugi::xml_node component = doc.child("Library").child("Component");
         component; component = component.next_sibling("Component")) {
        
        // Check if it's a diode component
        std::string type = component.attribute("_Type").value();
        if (type.find("Diode") == std::string::npos) continue;
        
        DiodeModel model;
        std::string partNumber = component.attribute("PartNumber").value();
        
        // Parse parameters
        const char* isAttr = component.attribute("IS").value();
        const char* nAttr = component.attribute("n").value();
        
        if (isAttr && strlen(isAttr) > 0) {
            model.IS = parseQuantity(isAttr);  // Helper function
        }
        if (nAttr && strlen(nAttr) > 0) {
            model.n = std::stod(nAttr);
        }
        
        diodes[partNumber] = model;
    }
    
    return true;
}

DiodeModel* ComponentLibrary::getDiodeModel(const std::string& partNumber) {
    auto it = diodes.find(partNumber);
    if (it != diodes.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace Spice
```

---

## Helper Utilities

### Quantity Parser
```cpp
// QuantityParser.h
#pragma once
#include <string>

namespace Spice {

// Parse quantities like "1.5 kΩ" or "2.52 nA"
double parseQuantity(const std::string& str);

// Unit multipliers
const double TERA = 1e12;
const double GIGA = 1e9;
const double MEGA = 1e6;
const double KILO = 1e3;
const double MILLI = 1e-3;
const double MICRO = 1e-6;
const double NANO = 1e-9;
const double PICO = 1e-12;
const double FEMTO = 1e-15;

} // namespace Spice
```

```cpp
// QuantityParser.cpp
#include "QuantityParser.h"
#include <cctype>
#include <sstream>

namespace Spice {

double parseQuantity(const std::string& str) {
    // Parse "1.5 kΩ" format
    std::istringstream iss(str);
    double value = 0.0;
    iss >> value;
    
    std::string unit;
    iss >> unit;
    
    if (unit.empty()) return value;
    
    // Get first character (prefix)
    char prefix = unit[0];
    double multiplier = 1.0;
    
    switch (prefix) {
        case 'T': multiplier = TERA; break;
        case 'G': multiplier = GIGA; break;
        case 'M': 
            // Check if mega or milli
            if (unit.length() > 1 && std::tolower(unit[1]) == 'u') {
                multiplier = MICRO;  // µ or mu
            } else {
                multiplier = MEGA;
            }
            break;
        case 'k': multiplier = KILO; break;
        case 'm': multiplier = MILLI; break;
        case 'u':
        case 'µ': multiplier = MICRO; break;
        case 'n': multiplier = NANO; break;
        case 'p': multiplier = PICO; break;
        case 'f': multiplier = FEMTO; break;
    }
    
    return value * multiplier;
}

} // namespace Spice
```

---

## Testing Template

```cpp
// test_components.cpp
#include <cassert>
#include <iostream>
#include "Resistor.h"
#include "Capacitor.h"
#include "Diode.h"
#include "Node.h"

using namespace Spice;

void testResistor() {
    Node n1("n1"), n2("n2");
    n1.setVoltage(5.0);
    n2.setVoltage(0.0);
    
    Resistor r(&n1, &n2, 1000.0);
    r.analyze(0.001);
    
    // Should be I = V/R = 5/1000 = 5mA
    assert(std::abs(r.getCurrent() - 0.005) < 1e-6);
    std::cout << "✓ Resistor test passed" << std::endl;
}

void testDiode() {
    Node n1("n1"), n2("n2");
    n1.setVoltage(0.7);   // Forward bias
    n2.setVoltage(0.0);
    
    DiodeModel model = DiodeModel::loadModel("1N4148");
    Diode d(&n1, &n2, model);
    d.analyze(0.001);
    
    // Should conduct (positive current)
    assert(d.getCurrent() > 0.0);
    std::cout << "✓ Diode test passed" << std::endl;
}

int main() {
    testResistor();
    testDiode();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
```

---

**Note:** These are code templates designed to be adapted for your specific DSP framework. Adjust as needed for your signal processing architecture.

**Next Steps:**
1. Implement Node class (manages nodal analysis)
2. Create MNA solver (Modified Nodal Analysis)
3. Integrate with audio sample processing loop
4. Test each component individually
5. Build circuit examples (filters, amplifiers, clipping stages)

---

**Generated:** January 28, 2026  
**Based on:** LiveSPICE source code analysis  
**Status:** Ready for implementation
