#ifndef DIODE_MODELS_H
#define DIODE_MODELS_H

#include <cmath>
#include <array>
#include <algorithm>

namespace Nonlinear {

/**
 * DiodeCharacteristics - Physical and electrical parameters for a diode
 */
struct DiodeCharacteristics {
    float Is;           // Saturation current
    float n;            // Ideality factor
    float Vt;           // Thermal voltage (26mV @ 25°C)
    float Rs;           // Series resistance
    float CjZero;       // Junction capacitance
    float m;            // Grading coefficient
    
    static DiodeCharacteristics Si1N4148() {
        return {1.4e-14f, 1.06f, 0.026f, 0.25f, 0.4e-12f, 0.4f};
    }
    
    static DiodeCharacteristics Si1N914() {
        return {2.6e-15f, 1.04f, 0.026f, 0.1f, 0.95e-12f, 0.4f};
    }
    
    static DiodeCharacteristics Ge_OA90() {
        return {5.0e-15f, 1.3f, 0.026f, 0.5f, 2.0e-12f, 0.5f};
    }
    
    static DiodeCharacteristics Si1N4007() {
        return {1.0e-14f, 1.08f, 0.026f, 0.5f, 0.8e-12f, 0.4f};
    }
};

/**
 * DiodeLUT - Pre-computed lookup table for Shockley equation
 */
class DiodeLUT {
public:
    static constexpr int LUT_SIZE = 512;
    static constexpr float VOLTAGE_MIN = -10.0f;
    static constexpr float VOLTAGE_MAX = 0.7f;
    
    DiodeLUT(const DiodeCharacteristics& diode) 
        : m_diode(diode) {
        buildLookupTable();
    }
    
    float evaluateCurrent(float voltage) const {
        voltage = std::clamp(voltage, VOLTAGE_MIN, VOLTAGE_MAX);
        float normalized = (voltage - VOLTAGE_MIN) / (VOLTAGE_MAX - VOLTAGE_MIN);
        float indexFloat = normalized * (LUT_SIZE - 1);
        int index0 = static_cast<int>(indexFloat);
        int index1 = std::min(index0 + 1, LUT_SIZE - 1);
        float frac = indexFloat - index0;
        
        float i0 = m_currentLUT[index0];
        float i1 = m_currentLUT[index1];
        return i0 + frac * (i1 - i0);
    }
    
    float evaluateConductance(float voltage) const {
        voltage = std::clamp(voltage, VOLTAGE_MIN, VOLTAGE_MAX);
        float nVt = m_diode.n * m_diode.Vt;
        float expArg = std::clamp(voltage / nVt, -20.0f, 20.0f);
        float gShock = (m_diode.Is / nVt) * std::exp(expArg);
        return std::clamp(gShock, 1e-12f, 1.0f);
    }
    
private:
    void buildLookupTable() {
        float nVt = m_diode.n * m_diode.Vt;
        for (int i = 0; i < LUT_SIZE; ++i) {
            float normalized = static_cast<float>(i) / (LUT_SIZE - 1);
            float voltage = VOLTAGE_MIN + normalized * (VOLTAGE_MAX - VOLTAGE_MIN);
            float expArg = std::clamp(voltage / nVt, -50.0f, 20.0f);
            m_currentLUT[i] = m_diode.Is * (std::exp(expArg) - 1.0f);
        }
    }
    
    DiodeCharacteristics m_diode;
    std::array<float, LUT_SIZE> m_currentLUT;
};

/**
 * DiodeNewtonRaphson - Iterative solver for nonlinear diode equation
 */
class DiodeNewtonRaphson {
public:
    struct SolverConfig {
        int maxIterations = 10;
        float convergenceTolerance = 1e-6f;
        float initialGuess = 0.3f;
    };
    
    DiodeNewtonRaphson(const DiodeCharacteristics& diode)
        : m_diode(diode) {}
    
    int solve(float vApplied, const SolverConfig& config, 
              float& outVoltage, float& outCurrent) const {
        float vDiode = config.initialGuess;
        float nVt = m_diode.n * m_diode.Vt;
        
        for (int iter = 0; iter < config.maxIterations; ++iter) {
            float expArg = std::clamp(vDiode / nVt, -50.0f, 20.0f);
            float iDiode = m_diode.Is * (std::exp(expArg) - 1.0f);
            float gDiode = (m_diode.Is / nVt) * std::exp(expArg);
            
            float residual = vApplied - (vDiode + iDiode * 10000.0f);
            
            if (std::abs(residual) < config.convergenceTolerance) {
                outVoltage = vDiode;
                outCurrent = iDiode;
                return iter + 1;
            }
            
            float jac = 1.0f + 10000.0f * gDiode;
            float step = residual / jac;
            float dampedStep = 0.5f * step;
            vDiode += dampedStep;
            vDiode = std::clamp(vDiode, -0.5f, 1.0f);
        }
        
        float expArg = std::clamp(vDiode / nVt, -50.0f, 20.0f);
        outVoltage = vDiode;
        outCurrent = m_diode.Is * (std::exp(expArg) - 1.0f);
        return config.maxIterations;
    }
    
private:
    DiodeCharacteristics m_diode;
};

/**
 * DiodeClippingStage - Audio-rate nonlinear clipping processor
 */
class DiodeClippingStage {
public:
    enum class TopologyType {
        SeriesDiode,
        ParallelDiode,
        BackToBackDiodes,
        BridgeClipping
    };
    
    DiodeClippingStage(const DiodeCharacteristics& diode, 
                       TopologyType topology = TopologyType::BackToBackDiodes,
                       float loadResistance = 10000.0f)
        : m_diode(diode), m_topology(topology), m_loadR(loadResistance),
          m_solver(diode) {}
    
    float processSample(float vIn) {
        DiodeNewtonRaphson::SolverConfig cfg;
        cfg.maxIterations = 5;
        cfg.convergenceTolerance = 1e-5f;
        
        float vDiode = 0.0f, iDiode = 0.0f;
        m_solver.solve(vIn, cfg, vDiode, iDiode);
        
        switch (m_topology) {
            case TopologyType::BackToBackDiodes:
                return std::clamp(vDiode, -0.6f, 0.6f);
            case TopologyType::SeriesDiode:
                return vDiode;
            case TopologyType::ParallelDiode:
                return vIn * 0.5f;
            case TopologyType::BridgeClipping:
                return std::clamp(vIn, -0.7f, 0.7f);
            default:
                return vIn;
        }
    }
    
private:
    DiodeCharacteristics m_diode;
    TopologyType m_topology;
    float m_loadR;
    DiodeNewtonRaphson m_solver;
};

}  // namespace Nonlinear

#endif  // DIODE_MODELS_H
