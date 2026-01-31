#include "DiodeModels.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Nonlinear {

/**
 * DiodeClippingStage::processSample - Main entry point for sample processing
 * 
 * Solves the implicit circuit equation depending on topology:
 * Series:      Vout = Vdiode + Id*Rs (limited current divider)
 * Parallel:    Vout = Vdiode (diode forward voltage)
 * BackToBack:  Symmetric clipping around zero
 * 
 * Uses Newton-Raphson method for implicit equation solving
 */
float DiodeClippingStage::processSample(float inputSample) {
    switch (m_topology) {
        case TopologyType::SeriesDiode:
            return solveSeriesDiodeCircuit(inputSample);
            
        case TopologyType::ParallelDiode:
            return solveParallelDiodeCircuit(inputSample);
            
        case TopologyType::BackToBackDiodes:
            return solveBackToBackDiodes(inputSample);
            
        case TopologyType::BridgeClipping: {
            // Bridge rectifier topology - clips both polarities symmetrically
            float abs_in = std::abs(inputSample);
            float abs_out = solveParallelDiodeCircuit(abs_in);
            return inputSample < 0 ? -abs_out : abs_out;
        }
        
        default:
            return inputSample;
    }
}

/**
 * solveSeriesDiodeCircuit - Series configuration
 * 
 * Circuit equation:
 *   V_applied = V_diode + I_diode * Rs + I_diode * R_load
 *   
 * Where:
 *   I_diode = Is * (exp(V_diode/(n*Vt)) - 1)
 *   
 * Implicit equation solved via Newton-Raphson
 */
float DiodeClippingStage::solveSeriesDiodeCircuit(float vApplied) {
    // Clamp applied voltage to reasonable range
    vApplied = std::clamp(vApplied, -5.0f, 5.0f);
    
    // Calculate forward voltage for reference
    float Vt = m_diode.n * m_diode.Vt;
    float Vf = Vt * std::log(1e-6f / m_diode.Is + 1.0f);  // ~0.65V for 1N4148
    float totalZ = m_diode.Rs + m_impedance;
    
    // Handle positive voltages (forward bias)
    if (vApplied > Vf * 1.5f) {
        // Strong forward bias: voltage divider between diode and load
        // High current flows, diode conducts strongly
        float clipped = Vf + (vApplied - Vf) * m_impedance / totalZ;
        return std::clamp(clipped, 0.6f, 0.95f);
    }
    
    // Handle negative voltages (reverse bias) - symmetric response
    if (vApplied < -Vf * 1.5f) {
        // Reverse bias: very high impedance, mirror of positive case
        // Current is minimal, diode blocks
        float absOutput = Vf + (-vApplied - Vf) * m_impedance / totalZ;
        float clipped = -std::clamp(absOutput, 0.6f, 0.95f);
        return clipped;
    }
    
    // Mid-range: use Newton-Raphson solver
    DiodeNewtonRaphson::SolverConfig config;
    config.maxIterations = 25;
    config.convergenceTolerance = 1e-7f;
    config.initialGuess = std::clamp(Vf * 0.8f, 0.1f, 0.7f);
    
    float vDiode, iDiode;
    int iterations = m_solver.solve(std::abs(vApplied), config, vDiode, iDiode);
    
    if (iterations > 0 && iterations < config.maxIterations) {
        // Apply voltage divider correction for impedance effect
        // V_output = V_diode - I_diode * Rs (dropping across series resistance)
        // The impedance effects the current, which affects the voltage drop
        float totalCurrent = iDiode;
        float voltageDropAcrossRs = totalCurrent * m_diode.Rs;
        float correctedVDiode = vDiode - voltageDropAcrossRs;
        return vApplied < 0 ? -correctedVDiode : correctedVDiode;
    }
    
    // Fallback: use LUT-based approximation
    float absClamped = std::clamp(std::abs(vApplied), 0.0f, 0.7f);
    float currentLut = m_lut.evaluateCurrent(absClamped);
    float voltageDrop = currentLut * totalZ;
    float outputMag = absClamped - voltageDrop;
    return vApplied < 0 ? -outputMag : outputMag;
}

/**
 * solveParallelDiodeCircuit - Parallel configuration
 * 
 * Circuit equation:
 *   I_total = (V_applied - V_diode) / R_load
 *   I_total = I_diode = Is * (exp(V_diode/(n*Vt)) - 1)
 *   
 * Simplifies in most cases to just the diode forward voltage
 */
float DiodeClippingStage::solveParallelDiodeCircuit(float vApplied) {
    vApplied = std::clamp(vApplied, 0.0f, 1.0f);
    
    // For parallel diode, output is limited by diode forward voltage
    // Typically around 0.6-0.7V for silicon, ~0.3V for germanium
    
    if (vApplied < 0.1f) {
        // Sub-threshold: use LUT for accuracy
        return m_lut.evaluateCurrent(vApplied);
    }
    
    // Above threshold: clipping is limited to forward voltage
    float forwardVoltage = m_diode.n * m_diode.Vt * std::log(vApplied / m_diode.Is + 1.0f);
    return std::clamp(forwardVoltage, 0.0f, m_diode.n * m_diode.Vt * 20.0f);
}

/**
 * solveBackToBackDiodes - Back-to-back configuration
 * 
 * Two diodes: one conducting positive, one conducting negative
 * Creates symmetric soft clipping around zero
 * 
 * Circuit equations:
 *   If V > 0: V_out = V_diode_fwd (positive diode conducting)
 *   If V < 0: V_out = -V_diode_fwd (negative diode conducting)
 *   If -V_t < V < V_t: Linear region (both diodes off)
 */
float DiodeClippingStage::solveBackToBackDiodes(float vApplied) {
    if (std::abs(vApplied) < 0.0001f) {
        return vApplied; // Ultra-linear region
    }
    
    bool isPositive = vApplied > 0;
    float absVoltage = std::abs(vApplied);
    
    // Thermal voltage = n * Vt (typically ~26mV for Si @ 25°C)
    float Vt = m_diode.n * m_diode.Vt;
    
    // Calculate actual diode forward voltage (~0.65V for 1N4148)
    float Vf = Vt * std::log(1e-6f / m_diode.Is + 1.0f);
    
    // Define clipping regions based on diode physics
    float linear_limit = Vf * 0.3f;    // ~0.2V - pass through unchanged
    float soft_knee_start = Vf * 0.7f;  // ~0.45V - soft clipping begins
    float hard_clipping = Vf;           // ~0.65V - hard limit
    
    // Region 1: Linear region (both diodes reverse biased)
    if (absVoltage < linear_limit) {
        return vApplied;
    }
    
    // Region 2: Soft clipping region (smooth knee transition)
    if (absVoltage < soft_knee_start) {
        // Smooth tanh transition from linear to clipping
        float normalized = (absVoltage - linear_limit) / (soft_knee_start - linear_limit);
        float softClipFactor = std::tanh(normalized * 1.5f);
        float output = linear_limit + softClipFactor * (soft_knee_start - linear_limit);
        return isPositive ? output : -output;
    }
    
    // Region 3: Hard clipping region (diode forward voltage limiting)
    if (absVoltage < hard_clipping * 1.5f) {
        // Use Newton-Raphson for accurate forward voltage calculation
        DiodeNewtonRaphson::SolverConfig config;
        config.maxIterations = 20;
        config.convergenceTolerance = 1e-6f;
        config.initialGuess = Vf * 0.9f;
        
        float vDiode, iDiode;
        int iterations = m_solver.solve(absVoltage, config, vDiode, iDiode);
        
        if (iterations > 0 && iterations < config.maxIterations) {
            return isPositive ? vDiode : -vDiode;
        }
    }
    
    // Region 4: Final clipping (hard limit at forward voltage)
    return isPositive ? (Vf * 1.05f) : -(Vf * 1.05f);
}

} // namespace Nonlinear
