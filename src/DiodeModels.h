#ifndef DIODE_MODELS_H
#define DIODE_MODELS_H

#include <cmath>
#include <array>
#include <algorithm>

namespace Nonlinear {

struct DiodeCharacteristics {
    float Is, n, Vt, Rs, CjZero, m;
    
    static DiodeCharacteristics Si1N4148() { return {1.4e-14f, 1.06f, 0.026f, 0.25f, 0.4e-12f, 0.4f}; }
    static DiodeCharacteristics Si1N914() { return {2.6e-15f, 1.04f, 0.026f, 0.1f, 0.95e-12f, 0.4f}; }
    static DiodeCharacteristics Ge_OA90() { return {5.0e-15f, 1.3f, 0.026f, 0.5f, 2.0e-12f, 0.5f}; }
    static DiodeCharacteristics Si1N4007() { return {1.0e-14f, 1.08f, 0.026f, 0.5f, 0.8e-12f, 0.4f}; }
};

class DiodeLUT {
public:
    static constexpr int LUT_SIZE = 512;
    static constexpr float VOLTAGE_MIN = -10.0f, VOLTAGE_MAX = 0.7f;
    
    DiodeLUT(const DiodeCharacteristics& diode) : m_diode(diode) { buildLookupTable(); }
    
    float evaluateCurrent(float voltage) const {
        voltage = std::clamp(voltage, VOLTAGE_MIN, VOLTAGE_MAX);
        float norm = (voltage - VOLTAGE_MIN) / (VOLTAGE_MAX - VOLTAGE_MIN);
        float idx = norm * (LUT_SIZE - 1);
        int i0 = int(idx), i1 = std::min(i0 + 1, LUT_SIZE - 1);
        float frac = idx - i0;
        return m_currentLUT[i0] + frac * (m_currentLUT[i1] - m_currentLUT[i0]);
    }
    
    float evaluateConductance(float voltage) const {
        float current = evaluateCurrent(voltage);
        float nVt = m_diode.n * m_diode.Vt;
        return (std::abs(current) + m_diode.Is) / nVt;
    }
    
private:
    DiodeCharacteristics m_diode;
    std::array<float, LUT_SIZE> m_currentLUT;
    
    void buildLookupTable() {
        float nVt = m_diode.n * m_diode.Vt;
        for (int i = 0; i < LUT_SIZE; ++i) {
            float voltage = VOLTAGE_MIN + float(i) / (LUT_SIZE - 1) * (VOLTAGE_MAX - VOLTAGE_MIN);
            float expArg = std::clamp(voltage / nVt, -20.0f, 50.0f);
            m_currentLUT[i] = m_diode.Is * (std::exp(expArg) - 1.0f);
        }
    }
};

class DiodeNewtonRaphson {
public:
    struct SolverConfig {
        int maxIterations = 20;
        float convergenceTolerance = 1e-6f;
        float initialGuess = 0.3f;
    };
    
    DiodeNewtonRaphson(const DiodeCharacteristics& diode) : m_diode(diode) {}
    
    int solve(float vApplied, const SolverConfig& config, float& outVoltage, float& outCurrent) const {
        float vDiode = config.initialGuess;
        float nVt = m_diode.n * m_diode.Vt;
        
        for (int iter = 0; iter < config.maxIterations; ++iter) {
            float expArg = std::clamp(vDiode / nVt, -100.0f, 50.0f);
            float current = m_diode.Is * (std::exp(expArg) - 1.0f);
            float residual = vDiode + current * m_diode.Rs - vApplied;
            
            if (std::abs(residual) < config.convergenceTolerance) {
                outVoltage = vDiode;
                outCurrent = current;
                return iter + 1;
            }
            
            float dI_dV = (m_diode.Is / nVt) * std::exp(expArg);
            float jacobian = 1.0f + dI_dV * m_diode.Rs;
            
            if (std::abs(jacobian) < 1e-12f) return 0;
            
            float damping = 0.5f;
            vDiode = vDiode - damping * residual / jacobian;
            vDiode = std::clamp(vDiode, -0.5f, 1.0f);
        }
        
        float expArg = std::clamp(vDiode / nVt, -100.0f, 50.0f);
        outCurrent = m_diode.Is * (std::exp(expArg) - 1.0f);
        outVoltage = vDiode;
        return config.maxIterations;
    }
    
private:
    DiodeCharacteristics m_diode;
};

class DiodeClippingStage {
public:
    enum class TopologyType { SeriesDiode, ParallelDiode, BackToBackDiodes, BridgeClipping };
    
    DiodeClippingStage(const DiodeCharacteristics& diode, TopologyType t = TopologyType::BackToBackDiodes, float r = 10000.0f)
        : m_topology(t), m_impedance(r), m_diode(diode), m_lut(diode) {}
    
    float processSample(float inputSample) {
        switch (m_topology) {
            case TopologyType::BackToBackDiodes:
                if (inputSample > 0.6f) return 0.6f;
                if (inputSample < -0.6f) return -0.6f;
                return inputSample;
            case TopologyType::SeriesDiode:
                return inputSample > 0.6f ? 0.6f : (inputSample < -5.0f ? -5.0f : inputSample);
            case TopologyType::ParallelDiode:
                return inputSample > 0.6f ? std::clamp(inputSample, 0.0f, 0.6f) : inputSample;
            case TopologyType::BridgeClipping: {
                float abs_in = std::abs(inputSample);
                float clipped = std::clamp(abs_in, 0.0f, 1.2f);
                return inputSample < 0 ? -clipped : clipped;
            }
            default:
                return inputSample;
        }
    }
    
private:
    TopologyType m_topology;
    float m_impedance;
    DiodeCharacteristics m_diode;
    DiodeLUT m_lut;
};

}  // namespace Nonlinear

#endif
