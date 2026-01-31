#include "TransistorModels.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Nonlinear {

/**
 * BJTAmplifierStage::shockleyBJT - Ebers-Moll base-emitter equation
 * 
 * Ic = beta * Ib = Is * (exp(Vbe / (n * Vt)) - 1)
 * 
 * With Early voltage correction:
 * Ic = Ic0 * (1 + Vce / Vaf)
 */
float BJTAmplifierStage::shockleyBJT(float vbe) {
    // Thermal voltage
    float nVt = m_param.nBE * m_param.Vt;
    
    // Clamp exponent to prevent overflow
    float expArg = std::clamp(vbe / nVt, -50.0f, 50.0f);
    
    // Base-emitter saturation current effect
    float ibe = m_param.Is * (std::exp(expArg) - 1.0f);
    
    // Collector current with forward gain
    float ic = m_param.Bf * ibe;
    
    // Early voltage effect (output impedance)
    float ic_with_early = ic * (1.0f + m_biasPoint.Vce / m_param.Vat);
    
    return ic_with_early;
}

/**
 * BJTAmplifierStage::processVbe
 * 
 * Calculates collector current from base-emitter input voltage
 */
float BJTAmplifierStage::processVbe(float vbeInput, float vceOutput) {
    // Clamp input to reasonable range
    vbeInput = std::clamp(vbeInput, -0.5f, 1.0f);
    
    // Update bias point
    m_biasPoint.Vbe = vbeInput;
    m_biasPoint.Vce = vceOutput;
    
    // Calculate collector current
    m_biasPoint.Ic = shockleyBJT(vbeInput);
    
    // Base current
    m_biasPoint.Ib = m_biasPoint.Ic / m_param.Bf;
    
    // Transconductance (small-signal gain)
    float nVt = m_param.nBE * m_param.Vt;
    m_biasPoint.gm = m_biasPoint.Ic / nVt;
    
    // Output impedance
    m_biasPoint.rce = m_param.Vat / (m_biasPoint.Ic + 1e-12f);
    
    // Determine operating mode
    if (vceOutput < 0.2f) {
        m_biasPoint.mode = BiasMode::Saturation;
    } else if (vbeInput < 0.5f) {
        m_biasPoint.mode = BiasMode::Cutoff;
    } else {
        m_biasPoint.mode = BiasMode::Active;
    }
    
    return m_biasPoint.Ic;
}

/**
 * BJTAmplifierStage::processInputVoltage
 * 
 * Full circuit simulation: input voltage -> output voltage at collector
 * 
 * Circuit: Vcc --[Rc]-- Collector --[Output]
 *                           |
 *                        (BJT)
 *                           |
 *                         Emitter (Ground)
 */
float BJTAmplifierStage::processInputVoltage(float vinput) {
    // Input impedance (base divider)
    float Zbase = 10000.0f; // Typical input impedance
    float baseVoltage = vinput * 0.5f; // Voltage divider
    
    // Process base-emitter junction
    float ic = processVbe(baseVoltage);
    
    // Collector voltage from supply and collector current
    // Vce = Vcc - Ic * Rc
    float vCollector = m_Vcc - ic * m_Rc;
    
    // Output voltage with load resistance
    float vOutput = vCollector * m_Rload / (m_Rc + m_Rload);
    
    return std::clamp(vOutput, 0.0f, m_Vcc);
}

/**
 * FETOverdriveStage::shockleyFET
 * 
 * MOSFET quadratic model:
 * Id = Kp * ((Vgs - Vto)^2 / 2) * (1 + Lambda * Vds)
 * 
 * For saturation: Id = Kp/2 * (Vgs - Vto)^2
 * For triode: Id = Kp * ((Vgs - Vto) * Vds - Vds^2/2)
 */
float FETOverdriveStage::shockleyFET(float vgs, float vds) {
    // Clamp inputs
    vgs = std::clamp(vgs, -5.0f, 5.0f);
    vds = std::clamp(vds, 0.0f, 10.0f);
    
    // Check if in saturation region
    bool inSaturation = std::abs(vds) >= std::abs(vgs - m_param.Vto);
    
    float id;
    
    if (inSaturation) {
        // Saturation region: square-law behavior
        float overdrive = vgs - m_param.Vto;
        
        if (overdrive <= 0) {
            id = 0.0f; // Off state
        } else {
            // Id = Kp/2 * (Vgs - Vto)^2 * (1 + Lambda*Vds)
            id = m_param.Kp * overdrive * overdrive * 0.5f * (1.0f + m_param.Lambda * vds);
        }
    } else {
        // Triode region: linear behavior
        float overdrive = vgs - m_param.Vto;
        if (overdrive <= 0) {
            id = 0.0f;
        } else {
            // Id = Kp * ((Vgs - Vto) * Vds - Vds^2/2)
            id = m_param.Kp * (overdrive * vds - vds * vds * 0.5f);
        }
    }
    
    return std::max(id, 0.0f); // Prevent negative current
}

/**
 * FETOverdriveStage::processVgs
 * 
 * Calculate drain current from gate-source voltage
 */
float FETOverdriveStage::processVgs(float vgsInput) {
    // Default drain-source voltage for DC analysis
    float vds = 5.0f;
    
    return shockleyFET(vgsInput, vds);
}

/**
 * FETOverdriveStage::getDrainCurrent
 * 
 * Get drain current for given gate and drain voltages
 */
float FETOverdriveStage::getDrainCurrent(float vgs, float vds) {
    return shockleyFET(vgs, vds);
}

/**
 * FETOverdriveStage::processInputVoltage
 * 
 * Full circuit simulation: input voltage -> output voltage at drain
 * 
 * Circuit: Vcc --[Rd]-- Drain --[Output]
 *                         |
 *                      (FET)
 *                         |
 *                       Source (Ground)
 */
float FETOverdriveStage::processInputVoltage(float vinput) {
    // Input couples through capacitor (AC coupling assumed)
    // Gate voltage from input signal
    float vgs = vinput * 0.7f; // Voltage divider / coupling
    
    // Solve for drain voltage iteratively
    // Vcc = Vds + Id * Rd
    // Id = f(Vgs, Vds)
    
    // Initial guess
    float vds = 5.0f;
    
    // Iterative Newton-Raphson to find equilibrium
    for (int iter = 0; iter < 5; ++iter) {
        float id = shockleyFET(vgs, vds);
        float vds_new = 5.0f - id * m_Rd;
        
        if (std::abs(vds_new - vds) < 0.001f) {
            break;
        }
        vds = vds_new;
    }
    
    // Clamp to valid range
    return std::clamp(vds, 0.0f, 5.0f);
}

/**
 * HybridTransistorDiodeStage::processBJTClipperCascade
 * 
 * BJT preamp (gain) -> Diode clipper (soft limiting)
 * 
 * Combines amplification with soft clipping for natural overdrive
 */
float HybridTransistorDiodeStage::processBJTClipperCascade(float input, float feedbackAmount) {
    // Stage 1: BJT amplification
    float bjt_output = m_bjtStage.processInputVoltage(input);
    
    // Apply feedback for tone shaping
    float with_feedback = bjt_output * (1.0f - feedbackAmount * 0.3f);
    
    // Stage 2: Diode clipping (if we had access to clipper)
    // For now, return BJT output with soft limiting
    float output = with_feedback;
    
    // Apply soft limiting using tanh
    float softLimit = std::tanh(output * 2.0f) * 0.5f;
    
    return softLimit;
}

/**
 * HybridTransistorDiodeStage::processFETOverdriveCascade
 * 
 * FET overdrive stage (smooth saturation) -> Tone control
 * 
 * FET naturally compresses, providing overdrive characteristics
 */
float HybridTransistorDiodeStage::processFETOverdriveCascade(float input, float toneControl) {
    // Stage 1: FET processing
    float fet_output = m_fetStage.processInputVoltage(input);
    
    // Stage 2: Tone shaping based on frequency content
    // Simple single-pole filter simulation
    float tone_shaped = fet_output * (0.5f + toneControl * 0.5f);
    
    // Soft clipping to prevent excessive distortion
    float clipped = std::tanh(tone_shaped * 1.5f) * 0.67f;
    
    return clipped;
}

} // namespace Nonlinear
