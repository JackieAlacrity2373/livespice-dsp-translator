#ifndef TRANSISTOR_MODELS_H
#define TRANSISTOR_MODELS_H

#include <cmath>
#include <optional>

namespace Nonlinear {

struct BJTCharacteristics {
    float Bf, Br, Is, Vaf, tempCoeffVbe;
    
    static BJTCharacteristics TwoN3904() { return {416.4f, 0.1f, 5.84e-14f, 74.03f, -0.002f}; }
    static BJTCharacteristics TwoN2222() { return {255.9f, 0.1f, 14.34e-14f, 200.0f, -0.002f}; }
    static BJTCharacteristics BC107() { return {312.6f, 0.1f, 8.07e-14f, 95.35f, -0.002f}; }
    static BJTCharacteristics TwoN3906() { return {408.8f, 0.1f, 9.57e-14f, 95.0f, -0.002f}; }
};

struct BJTOperatingPoint {
    float Vbe, Vce, Vbc, Ic, Ib, gm, rce;
    bool isSaturated;
};

class BJTModelEbersMoll {
public:
    BJTModelEbersMoll(const BJTCharacteristics& bjt) : m_bjt(bjt) {}
    
    BJTOperatingPoint solveOperatingPoint(float Vbe, float Vcc, float Rc) const {
        float Vt = 0.026f;
        
        // Simple approximation for demo
        float beta = m_bjt.Bf;
        float expArg = std::clamp(Vbe / (1.2f * Vt), -50.0f, 50.0f);
        float Ic_approx = m_bjt.Is * (std::exp(expArg) - 1.0f);
        float Vce = Vcc - Ic_approx * Rc;
        
        bool sat = Vce < 0.2f;
        
        return {
            Vbe, Vce, Vbe - 0.5f,
            Ic_approx,
            Ic_approx / beta,
            (m_bjt.Is / (1.2f * Vt)) * std::exp(Vbe / (1.2f * Vt)),
            m_bjt.Vaf / Ic_approx,
            sat
        };
    }
    
private:
    BJTCharacteristics m_bjt;
};

struct FETCharacteristics {
    float Kp, Vto, Lambda;
    
    static FETCharacteristics NMOS2N7000() { return {0.5e-3f, 1.5f, 0.02f}; }
    static FETCharacteristics PMOS2N7002() { return {0.5e-3f, -1.5f, 0.02f}; }
};

struct FETOperatingPoint {
    float Id, Vgs, Vds, gm, gds;
    bool isSaturated;
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

class TransistorClippingStage {
public:
    TransistorClippingStage(const BJTCharacteristics& bjt, float Zin, bool isPNP)
        : m_bjt(bjt), m_Zin(Zin), m_isPNP(isPNP), m_model(bjt) {}
    
    float processSample(float inputSignal) {
        auto op = m_model.solveOperatingPoint(
            0.7f,     // Base voltage
            5.0f,     // Vcc
            1000.0f   // Rc
        );
        
        float gain = 10.0f * op.gm * 1000.0f;
        float output = inputSignal * gain;
        
        // Soft clipping using tanh
        return std::tanh(output);
    }
    
private:
    BJTCharacteristics m_bjt;
    float m_Zin;
    bool m_isPNP;
    BJTModelEbersMoll m_model;
};

}  // namespace Nonlinear

#endif
