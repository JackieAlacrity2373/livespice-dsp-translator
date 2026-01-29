#pragma once
#include <string>
#include <cmath>
#include <map>

namespace LiveSpiceComponents {

// ============================================================================
// COMPONENT BASE CLASSES
// ============================================================================

class Component {
public:
    virtual ~Component() = default;
    
    virtual std::string getComponentType() const = 0;
    virtual std::string getPartNumber() const { return partNumber; }
    virtual void setPartNumber(const std::string& pn) { partNumber = pn; }
    
protected:
    std::string partNumber;
};

// ============================================================================
// PASSIVE COMPONENTS - PARAMETERS & MODELS
// ============================================================================

struct ResistorModel {
    static constexpr const char* componentType = "Resistor";
    double resistance;  // Ohms
    double tolerance;   // Percentage (default 5%)
    
    ResistorModel(double r, double t = 5.0) 
        : resistance(r), tolerance(t) {}
    
    // Ohm's Law: V = I * R
    static inline double calculateVoltage(double current, double resistance) {
        return current * resistance;
    }
    
    static inline double calculateCurrent(double voltage, double resistance) {
        return (resistance > 0) ? voltage / resistance : 0.0;
    }
};

struct CapacitorModel {
    static constexpr const char* componentType = "Capacitor";
    double capacitance;  // Farads
    double esr;          // Equivalent Series Resistance (Ohms)
    double tolerance;    // Percentage
    
    CapacitorModel(double c, double r = 0.1, double t = 10.0)
        : capacitance(c), esr(r), tolerance(t) {}
    
    // i = C * dV/dt
    static inline double calculateCurrent(double dv_dt, double capacitance) {
        return capacitance * dv_dt;
    }
    
    static inline double calculateVoltageChange(double current, double capacitance, double dt) {
        return (capacitance > 0) ? (current / capacitance) * dt : 0.0;
    }
};

struct InductorModel {
    static constexpr const char* componentType = "Inductor";
    double inductance;   // Henry
    double dcResistance; // Ohms (copper resistance)
    double tolerance;    // Percentage
    
    InductorModel(double l, double r = 0.1, double t = 10.0)
        : inductance(l), dcResistance(r), tolerance(t) {}
    
    // V = L * dI/dt
    static inline double calculateVoltage(double di_dt, double inductance) {
        return inductance * di_dt;
    }
    
    static inline double calculateCurrentChange(double voltage, double inductance, double dt) {
        return (inductance > 0) ? (voltage / inductance) * dt : 0.0;
    }
};

// ============================================================================
// DIODE - SHOCKLEY EQUATION & MODELS
// ============================================================================

struct DiodeModel {
    static constexpr const char* componentType = "Diode";
    
    // Shockley equation parameters
    double IS;          // Saturation current (Amps)
    double n;           // Ideality factor (typically 1.0-2.0)
    double Rs;          // Series resistance (Ohms)
    double Cjo;         // Junction capacitance (Farads)
    double Vj;          // Junction potential (Volts)
    double m;           // Grading coefficient
    double Tt;          // Transit time (seconds)
    
    DiodeModel()
        : IS(1e-14), n(1.0), Rs(10.0), Cjo(1e-12), Vj(0.75), m(0.5), Tt(0) {}
    
    // Shockley diode equation: I = IS * (exp(V/(n*Vt)) - 1)
    static inline double calculateCurrent(double voltage, double IS, double n, 
                                         double temp = 298.15) {
        const double q = 1.602176634e-19;  // Coulomb's charge
        const double k = 1.380649e-23;     // Boltzmann constant
        double Vt = k * temp / q;          // Thermal voltage (~26mV at 25°C)
        
        double exponent = voltage / (n * Vt);
        if (exponent > 100.0) return IS * std::exp(100.0);  // Prevent overflow
        if (exponent < -10.0) return -IS;  // Reverse saturation
        
        return IS * (std::exp(exponent) - 1.0);
    }
    
    // Common diode models (standard part numbers)
    static DiodeModel getModel(const std::string& partNumber) {
        // See COMPONENT_REFERENCE_LOOKUP.md for full list
        if (partNumber == "1N4148") {
            DiodeModel m;
            m.IS = 5.84e-14;
            m.n = 1.94;
            m.Rs = 0.7;
            return m;
        }
        if (partNumber == "1N34A") {
            DiodeModel m;
            m.IS = 40e-12;  // Germanium diode - higher IS
            m.n = 1.06;
            m.Rs = 20.0;    // Higher series resistance
            return m;
        }
        // Default
        return DiodeModel();
    }
};

// ============================================================================
// BJT TRANSISTOR - EBERS-MOLL MODEL
// ============================================================================

struct BJTModel {
    static constexpr const char* componentType = "BJT";
    
    // Ebers-Moll parameters
    double Bf;       // Forward beta (current gain)
    double Br;       // Reverse beta
    double Vaf;      // Early voltage (affects slope in active region)
    double Is;       // Saturation current
    double Cje;      // Base-emitter capacitance
    double Cjc;      // Base-collector capacitance
    double Tf;       // Forward transit time
    double Tr;       // Reverse transit time
    
    bool isNPN;      // NPN vs PNP
    
    BJTModel(bool npn = true)
        : Bf(100), Br(1), Vaf(100), Is(1e-14), Cje(1e-12), Cjc(0.5e-12), 
          Tf(0.3e-9), Tr(10e-9), isNPN(npn) {}
    
    // Collector current (Ebers-Moll): Ic = Is * (exp(Vbe/(n*Vt)) - 1) * (1 + Vce/Vaf)
    static inline double calculateCollectorCurrent(double vbe, double vce, 
                                                  const BJTModel& model,
                                                  double temp = 298.15) {
        const double q = 1.602176634e-19;
        const double k = 1.380649e-23;
        double Vt = k * temp / q;
        double n = 1.0;  // Ideality factor
        
        double ic = model.Is * (std::exp(vbe / (n * Vt)) - 1.0);
        ic *= (1.0 + vce / model.Vaf);  // Early effect
        
        return ic;
    }
    
    // Common BJT models
    static BJTModel getModel(const std::string& partNumber) {
        if (partNumber == "2N3904") {
            BJTModel m(true);  // NPN
            m.Bf = 200;
            m.Vaf = 74;
            m.Is = 6.734e-15;
            return m;
        }
        if (partNumber == "2N2222") {
            BJTModel m(true);  // NPN
            m.Bf = 255;
            m.Vaf = 74;
            m.Is = 14.34e-15;
            return m;
        }
        return BJTModel();
    }
};

// ============================================================================
// JFET - QUADRATIC/HYPERBOLIC MODEL
// ============================================================================

struct JFETModel {
    static constexpr const char* componentType = "JFET";
    
    double Vto;      // Threshold voltage (pinch-off)
    double lambda;   // Channel-length modulation
    double Kp;       // Transconductance parameter
    double Cgs;      // Gate-source capacitance
    double Cgd;      // Gate-drain capacitance
    bool isDepleted; // Depletion-mode flag
    
    JFETModel(bool dep = false)
        : Vto(-2.0), lambda(0.02), Kp(0.001), Cgs(1e-12), Cgd(1e-12), isDepleted(dep) {}
    
    // Drain current (Shichman-Hodges): Id = Kp * (Vgs - Vto)^2 * (1 + lambda*Vds)
    static inline double calculateDrainCurrent(double vgs, double vds, 
                                              const JFETModel& model) {
        double vov = vgs - model.Vto;  // Overdrive voltage
        
        if (vov <= 0) return 0.0;  // Off
        
        double id = model.Kp * vov * vov * (1.0 + model.lambda * vds);
        return id;
    }
    
    // Common JFET models
    static JFETModel getModel(const std::string& partNumber) {
        if (partNumber == "2N5457") {
            JFETModel m;
            m.Vto = -0.7;
            m.Kp = 0.0016;
            m.lambda = 0.015;
            return m;
        }
        return JFETModel();
    }
};

// ============================================================================
// OP-AMP - BEHAVIORAL MODEL
// ============================================================================

struct OpAmpModel {
    static constexpr const char* componentType = "OpAmp";
    
    double gain;           // Open-loop gain (V/V)
    double gainBW;         // Gain-bandwidth product (Hz)
    double outputImpedance;// Output impedance (Ohms)
    double inputImpedance; // Input impedance (Ohms)
    double slewRate;       // Slew rate (V/microsecond)
    double maxOutput;      // Rail voltages
    double minOutput;
    
    OpAmpModel()
        : gain(100000), gainBW(1e6), outputImpedance(75), inputImpedance(2e6),
          slewRate(0.5), maxOutput(13.5), minOutput(-13.5) {}
    
    // Common op-amp models
    static OpAmpModel getModel(const std::string& partNumber) {
        if (partNumber == "TL072") {
            OpAmpModel m;
            m.gain = 200000;
            m.gainBW = 13e6;
            m.slewRate = 13.0;
            return m;
        }
        if (partNumber == "LM741") {
            OpAmpModel m;
            m.gain = 200000;
            m.gainBW = 1e6;
            m.slewRate = 0.5;
            return m;
        }
        if (partNumber == "UA741") {
            OpAmpModel m;
            m.gain = 200000;
            m.gainBW = 1e6;
            m.slewRate = 0.5;
            return m;
        }
        return OpAmpModel();
    }
};

// ============================================================================
// VACUUM TUBES - KOREN MODEL
// ============================================================================

struct TriodeModel {
    static constexpr const char* componentType = "Triode";
    
    // Koren triode model parameters
    double mu;        // Amplification factor
    double gamma;     // Grid-cathode curve shape
    double Kp;        // Plate current coefficient
    double Kvb;       // Plate-cathode bias voltage coefficient
    double Rp;        // Internal plate resistance
    
    TriodeModel()
        : mu(20), gamma(1.4), Kp(600), Kvb(300), Rp(800) {}
    
    // Simplified plate current (Koren): Ip = Kp * ((mu*Vc + Vp)/(mu*Kvb + Vp))^gamma
    // Where: Vc=grid-cathode, Vp=plate-cathode
    static inline double calculatePlateCurrentKoren(double vc, double vp, 
                                                   const TriodeModel& model) {
        double numerator = model.mu * vc + vp;
        double denominator = model.mu * model.Kvb + vp;
        
        if (denominator <= 0) return 0.0;
        
        double base = numerator / denominator;
        if (base <= 0) return 0.0;
        
        double ip = model.Kp * std::pow(base, model.gamma);
        return ip;
    }
    
    // Common tube models
    static TriodeModel getModel(const std::string& partNumber) {
        if (partNumber == "12AX7") {
            TriodeModel m;
            m.mu = 100;
            m.gamma = 1.7;
            m.Kp = 600;
            return m;
        }
        if (partNumber == "EL84") {
            TriodeModel m;
            m.mu = 13;
            m.gamma = 1.5;
            m.Kp = 1450;
            return m;
        }
        return TriodeModel();
    }
};

// ============================================================================
// TRANSFORMER - IDEAL MODEL
// ============================================================================

struct TransformerModel {
    static constexpr const char* componentType = "Transformer";
    
    double ratio;            // Primary to secondary turns ratio
    double couplingCoeff;    // Coupling coefficient (0-1)
    double primaryInductance;
    double secondaryInductance;
    
    TransformerModel(double r = 1.0)
        : ratio(r), couplingCoeff(0.99), primaryInductance(0.1), 
          secondaryInductance(0.1 / (r*r)) {}
    
    // Ideal transformer: Vs = (Ns/Np) * Vp = ratio * Vp
    static inline double calculateSecondaryVoltage(double primaryVoltage, double ratio) {
        return primaryVoltage * ratio;
    }
    
    // Power transfer: Is = (Np/Ns) * Ip = (1/ratio) * Ip
    static inline double calculateSecondaryCurrentIdeal(double primaryCurrent, double ratio) {
        return (ratio > 0) ? primaryCurrent / ratio : 0.0;
    }
};

} // namespace LiveSpiceComponents
