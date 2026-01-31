#include "StateSpaceFilter.h"
#include <cmath>
#include <algorithm>
#include <complex>

namespace LiveSpiceDSP {

// ============================================================================
// BiquadFilter Implementation
// ============================================================================

BiquadFilter::BiquadFilter() : m_state{0.0f, 0.0f} {
    // Default: pass-through (unity gain)
    m_coeff.b0 = 1.0f;
    m_coeff.b1 = 0.0f;
    m_coeff.b2 = 0.0f;
    m_coeff.a1 = 0.0f;
    m_coeff.a2 = 0.0f;
}

float BiquadFilter::process(float input) {
    // Direct Form II implementation
    // y[n] = b0*w[n] + b1*w[n-1] + b2*w[n-2]
    // w[n] = x[n] - a1*w[n-1] - a2*w[n-2]
    
    float w = input - m_coeff.a1 * m_state[0] - m_coeff.a2 * m_state[1];
    float output = m_coeff.b0 * w + m_coeff.b1 * m_state[0] + m_coeff.b2 * m_state[1];
    
    // Update state
    m_state[1] = m_state[0];
    m_state[0] = w;
    
    return output;
}

void BiquadFilter::setCoefficients(const BiquadCoefficients& coeff) {
    m_coeff = coeff;
}

void BiquadFilter::reset() {
    m_state[0] = 0.0f;
    m_state[1] = 0.0f;
}

// ============================================================================
// Biquad Design Methods
// ============================================================================

BiquadCoefficients BiquadFilter::designLowPass(float sampleRate, float cutoffFreq) {
    // Butterworth 2nd-order low-pass
    float omega = 2.0f * 3.14159265359f * cutoffFreq / sampleRate;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * 0.7071067812f);  // Q = 0.707 (Butterworth)
    
    BiquadCoefficients coeff;
    coeff.b0 = (1.0f - cosOmega) / 2.0f;
    coeff.b1 = 1.0f - cosOmega;
    coeff.b2 = (1.0f - cosOmega) / 2.0f;
    
    float a0 = 1.0f + alpha;
    coeff.a1 = -2.0f * cosOmega / a0;
    coeff.a2 = (1.0f - alpha) / a0;
    
    coeff.b0 /= a0;
    coeff.b1 /= a0;
    coeff.b2 /= a0;
    
    return coeff;
}

BiquadCoefficients BiquadFilter::designHighPass(float sampleRate, float cutoffFreq) {
    // Butterworth 2nd-order high-pass
    float omega = 2.0f * 3.14159265359f * cutoffFreq / sampleRate;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * 0.7071067812f);  // Q = 0.707
    
    BiquadCoefficients coeff;
    coeff.b0 = (1.0f + cosOmega) / 2.0f;
    coeff.b1 = -(1.0f + cosOmega);
    coeff.b2 = (1.0f + cosOmega) / 2.0f;
    
    float a0 = 1.0f + alpha;
    coeff.a1 = -2.0f * cosOmega / a0;
    coeff.a2 = (1.0f - alpha) / a0;
    
    coeff.b0 /= a0;
    coeff.b1 /= a0;
    coeff.b2 /= a0;
    
    return coeff;
}

BiquadCoefficients BiquadFilter::designPeakFilter(float sampleRate, float centerFreq,
                                                   float qFactor, float gainDb) {
    // Peaking EQ filter (bell curve)
    float A = std::pow(10.0f, gainDb / 40.0f);  // Amplitude from dB
    float omega = 2.0f * 3.14159265359f * centerFreq / sampleRate;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * qFactor);
    
    BiquadCoefficients coeff;
    coeff.b0 = 1.0f + alpha * A;
    coeff.b1 = -2.0f * cosOmega;
    coeff.b2 = 1.0f - alpha * A;
    
    float a0 = 1.0f + alpha / A;
    coeff.a1 = -2.0f * cosOmega / a0;
    coeff.a2 = (1.0f - alpha / A) / a0;
    
    coeff.b0 /= a0;
    coeff.b1 /= a0;
    coeff.b2 /= a0;
    
    return coeff;
}

BiquadCoefficients BiquadFilter::designLowShelf(float sampleRate, float cutoffFreq,
                                                 float qFactor, float gainDb) {
    // Low-shelf filter (bass control)
    float A = std::pow(10.0f, gainDb / 40.0f);
    float omega = 2.0f * 3.14159265359f * cutoffFreq / sampleRate;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * qFactor);
    
    BiquadCoefficients coeff;
    float twoSqrtAAlpha = 2.0f * std::sqrt(A) * alpha;
    
    coeff.b0 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega + twoSqrtAAlpha);
    coeff.b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega);
    coeff.b2 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega - twoSqrtAAlpha);
    
    float a0 = (A + 1.0f) + (A - 1.0f) * cosOmega + twoSqrtAAlpha;
    coeff.a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega) / a0;
    coeff.a2 = ((A + 1.0f) + (A - 1.0f) * cosOmega - twoSqrtAAlpha) / a0;
    
    coeff.b0 /= a0;
    coeff.b1 /= a0;
    coeff.b2 /= a0;
    
    return coeff;
}

BiquadCoefficients BiquadFilter::designHighShelf(float sampleRate, float cutoffFreq,
                                                  float qFactor, float gainDb) {
    // High-shelf filter (treble control)
    float A = std::pow(10.0f, gainDb / 40.0f);
    float omega = 2.0f * 3.14159265359f * cutoffFreq / sampleRate;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * qFactor);
    
    BiquadCoefficients coeff;
    float twoSqrtAAlpha = 2.0f * std::sqrt(A) * alpha;
    
    coeff.b0 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + twoSqrtAAlpha);
    coeff.b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega);
    coeff.b2 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - twoSqrtAAlpha);
    
    float a0 = (A + 1.0f) - (A - 1.0f) * cosOmega + twoSqrtAAlpha;
    coeff.a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega) / a0;
    coeff.a2 = ((A + 1.0f) - (A - 1.0f) * cosOmega - twoSqrtAAlpha) / a0;
    
    coeff.b0 /= a0;
    coeff.b1 /= a0;
    coeff.b2 /= a0;
    
    return coeff;
}

// ============================================================================
// BiquadFilterBank Implementation
// ============================================================================

BiquadFilterBank::BiquadFilterBank(size_t numStages) {
    m_stages.resize(numStages);
}

float BiquadFilterBank::process(float input) {
    float output = input;
    for (auto& stage : m_stages) {
        output = stage.process(output);
    }
    return output;
}

void BiquadFilterBank::setStageCoefficients(size_t stageIndex, const BiquadCoefficients& coeff) {
    if (stageIndex < m_stages.size()) {
        m_stages[stageIndex].setCoefficients(coeff);
    }
}

void BiquadFilterBank::reset() {
    for (auto& stage : m_stages) {
        stage.reset();
    }
}

// ============================================================================
// ToneStackController Implementation
// ============================================================================

ToneStackController::ToneStackController(float sampleRate)
    : m_sampleRate(sampleRate),
      m_bassFilter(1),
      m_midFilter(1),
      m_trebleFilter(1),
      m_presenceFilter(1) {
    
    // Initialize with default tone stack settings
    // Standard 3-band EQ: Bass @ 120Hz, Mid @ 1kHz, Treble @ 4.5kHz
    
    auto bassCoeff = BiquadFilter::designLowShelf(sampleRate, 120.0f, 0.707f, 0.0f);
    m_bassFilter.setStageCoefficients(0, bassCoeff);
    
    auto midCoeff = BiquadFilter::designPeakFilter(sampleRate, 1000.0f, 0.707f, 0.0f);
    m_midFilter.setStageCoefficients(0, midCoeff);
    
    auto trebleCoeff = BiquadFilter::designHighShelf(sampleRate, 4500.0f, 0.707f, 0.0f);
    m_trebleFilter.setStageCoefficients(0, trebleCoeff);
    
    auto presenceCoeff = BiquadFilter::designPeakFilter(sampleRate, 4500.0f, 1.414f, 0.0f);
    m_presenceFilter.setStageCoefficients(0, presenceCoeff);
}

float ToneStackController::process(float input) {
    // Process through all three bands in parallel-like fashion
    // (actually cascade but with independent gains recombined)
    float output = input;
    
    // Apply each filter band (they're cascaded but each represents independent control)
    output = m_bassFilter.process(output);
    output = m_midFilter.process(output);
    output = m_trebleFilter.process(output);
    
    return output;
}

void ToneStackController::setBassGain(float bassGainDb) {
    // Clamp to ±12dB
    bassGainDb = std::max(-12.0f, std::min(12.0f, bassGainDb));
    auto coeff = BiquadFilter::designLowShelf(m_sampleRate, 120.0f, 0.707f, bassGainDb);
    m_bassFilter.setStageCoefficients(0, coeff);
}

void ToneStackController::setMidGain(float midGainDb) {
    midGainDb = std::max(-12.0f, std::min(12.0f, midGainDb));
    auto coeff = BiquadFilter::designPeakFilter(m_sampleRate, 1000.0f, 0.707f, midGainDb);
    m_midFilter.setStageCoefficients(0, coeff);
}

void ToneStackController::setTrebleGain(float trebleGainDb) {
    trebleGainDb = std::max(-12.0f, std::min(12.0f, trebleGainDb));
    auto coeff = BiquadFilter::designHighShelf(m_sampleRate, 4500.0f, 0.707f, trebleGainDb);
    m_trebleFilter.setStageCoefficients(0, coeff);
}

void ToneStackController::setPresenceGain(float presenceGainDb) {
    presenceGainDb = std::max(-12.0f, std::min(12.0f, presenceGainDb));
    auto coeff = BiquadFilter::designPeakFilter(m_sampleRate, 4500.0f, 1.414f, presenceGainDb);
    m_presenceFilter.setStageCoefficients(0, coeff);
}

void ToneStackController::reset() {
    m_bassFilter.reset();
    m_midFilter.reset();
    m_trebleFilter.reset();
    m_presenceFilter.reset();
}

// ============================================================================
// FrequencyResponseAnalyzer Implementation
// ============================================================================

std::vector<float> FrequencyResponseAnalyzer::getMagnitudeResponse(
    const BiquadCoefficients& coeff,
    const std::vector<float>& frequencies,
    float sampleRate) {
    
    std::vector<float> magnitude;
    magnitude.reserve(frequencies.size());
    
    for (float freq : frequencies) {
        float omega = 2.0f * 3.14159265359f * freq / sampleRate;
        
        // Numerator: b0 + b1*e^(-jw) + b2*e^(-2jw)
        float cosOmega = std::cos(omega);
        float sinOmega = std::sin(omega);
        float cos2Omega = std::cos(2.0f * omega);
        float sin2Omega = std::sin(2.0f * omega);
        
        float numReal = coeff.b0 + coeff.b1 * cosOmega + coeff.b2 * cos2Omega;
        float numImag = -coeff.b1 * sinOmega - coeff.b2 * sin2Omega;
        float numMag = std::sqrt(numReal * numReal + numImag * numImag);
        
        // Denominator: 1 + a1*e^(-jw) + a2*e^(-2jw)
        float denReal = 1.0f + coeff.a1 * cosOmega + coeff.a2 * cos2Omega;
        float denImag = -coeff.a1 * sinOmega - coeff.a2 * sin2Omega;
        float denMag = std::sqrt(denReal * denReal + denImag * denImag);
        
        magnitude.push_back(denMag > 1e-10f ? numMag / denMag : 0.0f);
    }
    
    return magnitude;
}

std::vector<float> FrequencyResponseAnalyzer::getPhaseResponse(
    const BiquadCoefficients& coeff,
    const std::vector<float>& frequencies,
    float sampleRate) {
    
    std::vector<float> phase;
    phase.reserve(frequencies.size());
    
    for (float freq : frequencies) {
        float omega = 2.0f * 3.14159265359f * freq / sampleRate;
        
        float cosOmega = std::cos(omega);
        float sinOmega = std::sin(omega);
        float cos2Omega = std::cos(2.0f * omega);
        float sin2Omega = std::sin(2.0f * omega);
        
        float numReal = coeff.b0 + coeff.b1 * cosOmega + coeff.b2 * cos2Omega;
        float numImag = -coeff.b1 * sinOmega - coeff.b2 * sin2Omega;
        
        float denReal = 1.0f + coeff.a1 * cosOmega + coeff.a2 * cos2Omega;
        float denImag = -coeff.a1 * sinOmega - coeff.a2 * sin2Omega;
        
        float phaseNum = std::atan2(numImag, numReal);
        float phaseDen = std::atan2(denImag, denReal);
        
        phase.push_back(phaseNum - phaseDen);
    }
    
    return phase;
}

std::vector<float> FrequencyResponseAnalyzer::generateLogSweep(
    float fLow, float fHigh, size_t numPoints) {
    
    std::vector<float> frequencies;
    frequencies.reserve(numPoints);
    
    float logFLow = std::log10(fLow);
    float logFHigh = std::log10(fHigh);
    
    for (size_t i = 0; i < numPoints; ++i) {
        float logFreq = logFLow + (logFHigh - logFLow) * i / (numPoints - 1);
        frequencies.push_back(std::pow(10.0f, logFreq));
    }
    
    return frequencies;
}

// ============================================================================
// DistortionPedalDSP Implementation
// ============================================================================

DistortionPedalDSP::DistortionPedalDSP(float sampleRate)
    : m_sampleRate(sampleRate),
      m_inputGain(1.0f),
      m_outputLevel(1.0f),
      m_toneStack(sampleRate),
      m_inputBufferState{0.0f, 0.0f},
      m_outputBufferState{0.0f, 0.0f} {
}

float DistortionPedalDSP::process(float input) {
    // Apply input gain (drive control)
    float driven = input * m_inputGain;
    
    // Input buffer (1st-order low-pass at ~10kHz)
    float omega = 2.0f * 3.14159265359f * 10000.0f / m_sampleRate;
    float alpha = std::sin(omega) / 2.0f;
    float b0 = alpha;
    float b1 = alpha;
    float a1 = -(1.0f - 2.0f * alpha);
    
    float buffered = b0 * driven + b1 * m_inputBufferState[0] + a1 * m_inputBufferState[1];
    m_inputBufferState[1] = m_inputBufferState[0];
    m_inputBufferState[0] = driven;
    
    // Diode clipping (tanh soft clipping approximates diode behavior)
    float clipped = std::tanh(buffered * 1.5f) * 0.67f;
    
    // Apply tone stack
    float toneShaped = m_toneStack.process(clipped);
    
    // Output buffer (1st-order low-pass at ~10kHz)
    float output = b0 * toneShaped + b1 * m_outputBufferState[0] + a1 * m_outputBufferState[1];
    m_outputBufferState[1] = m_outputBufferState[0];
    m_outputBufferState[0] = toneShaped;
    
    // Apply output level (volume control)
    return output * m_outputLevel;
}

void DistortionPedalDSP::setInputGain(float gainDb) {
    m_inputGain = std::pow(10.0f, gainDb / 20.0f);
}

void DistortionPedalDSP::setOutputLevel(float levelDb) {
    m_outputLevel = std::pow(10.0f, levelDb / 20.0f);
}

void DistortionPedalDSP::reset() {
    m_toneStack.reset();
    m_inputBufferState = {0.0f, 0.0f};
    m_outputBufferState = {0.0f, 0.0f};
}

} // namespace LiveSpiceDSP
