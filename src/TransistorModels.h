#pragma once

#include <cmath>

namespace Nonlinear {

/**
 * TransistorModels.h - BJT and FET models for Phase 1 Part 2
 * 
 * Implements standard transistor equations:
 * - BJT: Ebers-Moll model with temperature compensation
 * - FET: Quadratic model with channel modulation
 * 
 * Integration with diode models for hybrid circuits
 */

/**
 * BJT Transistor Parameters (Ebers-Moll Model)
 */
struct BJTCharacteristics {
    float Is;              // Saturation current (A)
    float Vt;              // Thermal voltage ≈ 0.026V @ 25°C
    float nBE;             // Ideality factor (typically 1.0)
    float nBC;             // Ideality factor (typically 1.0)
    float Bf;              // Forward current gain (hFE)
    float Br;              // Reverse current gain
    float Rb;              // Base resistance (Ω)
    float Vat;             // Early voltage effect (V)
    float tempCoeff;       // Temperature coefficient
    
    // Predefined transistor types
    static BJTCharacteristics TwoN2222() {
        return {
            .Is = 1.4e-14f,
            .Vt = 0.026f,
            .nBE = 1.0f,
            .nBC = 1.0f,
            .Bf = 255.0f,
            .Br = 6.433f,
            .Rb = 100.0f,
            .Vat = 74.03f,
            .tempCoeff = -0.002f
        };
    }
    
    static BJTCharacteristics TwoN3904() {
        return {
            .Is = 6.193e-15f,
            .Vt = 0.026f,
            .nBE = 1.0f,
            .nBC = 1.0f,
            .Bf = 416.4f,
            .Br = 0.75f,
            .Rb = 200.0f,
            .Vat = 200.0f,
            .tempCoeff = -0.002f
        };
    }
    
    static BJTCharacteristics TwoN5088() {
        return {
            .Is = 5.911e-15f,
            .Vt = 0.026f,
            .nBE = 1.0f,
            .nBC = 1.0f,
            .Bf = 800.0f,
            .Br = 1.0f,
            .Rb = 50.0f,
            .Vat = 300.0f,
            .tempCoeff = -0.002f
        };
    }
};

/**
 * BJT Operating Point
 */
struct BJTOperatingPoint {
    float Vbe;
    float Vce;
    float Ib;
    float Ic;
    float gm;              // Transconductance
    float rce;             // Output impedance
    bool isSaturated;
};

/**
 * FET Transistor Parameters (MOSFET/JFET Quadratic Model)
 */
struct FETCharacteristics {
    float Vto;             // Threshold voltage (V)
    float Kp;              // Transconductance parameter (A/V²)
    float Lambda;          // Channel modulation parameter (1/V)
    float Vef;             // Early voltage (V)
    float Rs;              // Source resistance (Ω)
    float Rd;              // Drain resistance (Ω)
    float Cgs;             // Gate-source capacitance (F)
    float Cgd;             // Gate-drain capacitance (F)
    
    // Predefined FET types
    static FETCharacteristics TwoN7000() {
        return {
            .Vto = 1.5f,
            .Kp = 0.00357f,
            .Lambda = 0.04f,
            .Vef = 100.0f,
            .Rs = 5.0f,
            .Rd = 5.0f,
            .Cgs = 1.8e-11f,
            .Cgd = 0.5e-12f
        };
    }
    
    static FETCharacteristics J201() {
        return {
            .Vto = -0.4f,
            .Kp = 0.003f,
            .Lambda = 0.02f,
            .Vef = 50.0f,
            .Rs = 10.0f,
            .Rd = 10.0f,
            .Cgs = 2.0e-12f,
            .Cgd = 1.5e-12f
        };
    }
};

/**
 * BJT Common Emitter Amplifier Stage
 */
class BJTAmplifierStage {
public:
    enum class BiasMode {
        Active, Saturation, Cutoff
    };
    
    BJTAmplifierStage(const BJTCharacteristics& param, float collectorResistance = 10000.0f,
                      float loadResistance = 100000.0f, float supplyVoltage = 9.0f)
        : m_param(param), m_Rc(collectorResistance), m_Rload(loadResistance), 
          m_Vcc(supplyVoltage), m_temperature(25.0f) {}
    
    float processVbe(float vbeInput, float vceOutput = 5.0f);
    float processInputVoltage(float vinput);
    BJTOperatingPoint getCurrentBiasPoint() const;
    void setTemperature(float tempC) { m_temperature = tempC; }
    float shockleyBJT(float vbe);
    float getThresholdVoltage() const { return m_param.Vat * 0.1f; }
    
private:
    BJTCharacteristics m_param;
    float m_Rc, m_Rload, m_Vcc, m_temperature;
    BJTOperatingPoint m_biasPoint;
};

/**
 * FET Overdrive Stage
 */
class FETOverdriveStage {
public:
    enum class ChannelType { NMOS, PMOS, JFET };
    
    FETOverdriveStage(const FETCharacteristics& param, float drainResistance = 10000.0f,
                      float loadResistance = 100000.0f, ChannelType type = ChannelType::NMOS)
        : m_param(param), m_Rd(drainResistance), m_Rload(loadResistance), m_type(type) {}
    
    float processVgs(float vgsInput);
    float processInputVoltage(float vinput);
    float getDrainCurrent(float vgs, float vds);
    
private:
    FETCharacteristics m_param;
    float m_Rd, m_Rload;
    ChannelType m_type;
    float shockleyFET(float vgs, float vds);
};

/**
 * Hybrid Transistor-Diode Stage
 */
class HybridTransistorDiodeStage {
public:
    HybridTransistorDiodeStage(const BJTCharacteristics& bjt,
                               const FETCharacteristics& fet = FETCharacteristics::TwoN7000())
        : m_bjtStage(bjt), m_fetStage(fet) {}
    
    float processBJTClipperCascade(float input, float feedbackAmount = 0.5f);
    float processFETOverdriveCascade(float input, float toneControl = 0.5f);
    
private:
    BJTAmplifierStage m_bjtStage;
    FETOverdriveStage m_fetStage;
};

} // namespace Nonlinear

namespace Nonlinear {

/**
 * FET Operating Point
 */
struct FETOperatingPoint {
    float Id;              // Drain current (A)
    float Vgs;             // Gate-source voltage (V)
    float Vds;             // Drain-source voltage (V)
    float gm;              // Transconductance (A/V)
    float gds;             // Output conductance (A/V)
    bool isSaturated;      // True if in saturation region
};

class FETModelQuadratic {
public:
    FETModelQuadratic(const FETCharacteristics& fet) : m_fet(fet) {}
    
    FETOperatingPoint solveOperatingPoint(float Vgs, float Vds, float Rd) const {
        float Vgs_eff = Vgs - m_fet.Vto;
        
        if (Vgs_eff <= 0.0f) {
            return {1e-12f, Vgs, Vds, 0.0f, 0.0f, false};
        }
        
        float Vds_sat = Vgs_eff / 2.0f;
        bool sat = Vds >= Vds_sat;
        
        float Id = sat ? 
            0.5f * m_fet.Kp * Vgs_eff * Vgs_eff * (1.0f + m_fet.Lambda * Vds) :
            m_fet.Kp * (Vgs_eff * Vds - 0.5f * Vds * Vds);
        
        float gm = sat ? m_fet.Kp * Vgs_eff : m_fet.Kp * Vds;
        float gds = sat ? 0.5f * m_fet.Kp * Vgs_eff * Vgs_eff * m_fet.Lambda : m_fet.Kp * (Vgs_eff - Vds);
        
        return {Id, Vgs, Vds, gm, gds, sat};
    }
    
private:
    FETCharacteristics m_fet;
};

// TransistorClippingStage - Phase 1 placeholder, superseded by Phase 2 tone control
// Disabled due to incomplete BJTModelEbersMoll implementation
/*
class TransistorClippingStage {
public:
    TransistorClippingStage(const BJTCharacteristics& bjt, float Zin, bool isPNP)
        : m_bjt(bjt), m_Zin(Zin), m_isPNP(isPNP) {}
    
    float processSample(float inputSignal) {
        // TODO: Implement proper BJT clipping stage
        return inputSignal * 0.1f;
    }
    
private:
    BJTCharacteristics m_bjt;
    float m_Zin;
    bool m_isPNP;
};
*/

}  // namespace Nonlinear
