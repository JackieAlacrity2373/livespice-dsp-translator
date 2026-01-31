#include "CompressorDynamics.h"
#include <cmath>
#include <algorithm>

namespace LiveSpiceDSP {

// ============================================================================
// PeakDetector Implementation
// ============================================================================

PeakDetector::PeakDetector(float sampleRate, float lookAheadMs)
    : m_sampleRate(sampleRate), m_peakDb(-80.0f), m_bufferIndex(0) {
    
    m_lookAheadSamples = static_cast<size_t>(std::max(1.0f, sampleRate * lookAheadMs / 1000.0f));
    m_buffer.resize(m_lookAheadSamples, 0.0f);
}

float PeakDetector::processSample(float sample) {
    // Add to circular buffer
    m_buffer[m_bufferIndex] = std::abs(sample);
    m_bufferIndex = (m_bufferIndex + 1) % m_lookAheadSamples;
    
    // Find peak in buffer
    float peakLinear = 0.0f;
    for (float val : m_buffer) {
        peakLinear = std::max(peakLinear, val);
    }
    
    // Convert to dB with floor at -80dB
    m_peakDb = peakLinear > 1e-5f ? 20.0f * std::log10(peakLinear) : -80.0f;
    return m_peakDb;
}

void PeakDetector::reset() {
    m_peakDb = -80.0f;
    m_bufferIndex = 0;
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
}

// ============================================================================
// EnvelopeFollower Implementation
// ============================================================================

EnvelopeFollower::EnvelopeFollower(float sampleRate)
    : m_sampleRate(sampleRate), m_currentDb(-80.0f), m_attackCoeff(0.0f), m_releaseCoeff(0.0f) {
    
    setTimes(10.0f, 100.0f);  // Default 10ms attack, 100ms release
}

void EnvelopeFollower::setTimes(float attackMs, float releaseMs) {
    // Time constant to coefficient conversion (exponential decay)
    // coeff = exp(-1 / (time_ms * sampleRate / 1000))
    float attackSamples = m_sampleRate * attackMs / 1000.0f;
    float releaseSamples = m_sampleRate * releaseMs / 1000.0f;
    
    m_attackCoeff = std::exp(-1.0f / std::max(1.0f, attackSamples));
    m_releaseCoeff = std::exp(-1.0f / std::max(1.0f, releaseSamples));
}

float EnvelopeFollower::process(float targetDb) {
    if (targetDb > m_currentDb) {
        // Attack phase: quick rise
        m_currentDb = m_attackCoeff * m_currentDb + (1.0f - m_attackCoeff) * targetDb;
    } else {
        // Release phase: slower fall
        m_currentDb = m_releaseCoeff * m_currentDb + (1.0f - m_releaseCoeff) * targetDb;
    }
    
    return m_currentDb;
}

void EnvelopeFollower::reset() {
    m_currentDb = -80.0f;
}

// ============================================================================
// Compressor Implementation
// ============================================================================

Compressor::Compressor(float sampleRate)
    : m_sampleRate(sampleRate), m_peakDetector(sampleRate), 
      m_envelopeFollower(sampleRate), m_gainReductionDb(0.0f) {
    
    configure(CompressorConfig());
}

void Compressor::configure(const CompressorConfig& config) {
    m_config = config;
    m_envelopeFollower.setTimes(config.attackMs, config.releaseMs);
}

float Compressor::process(float input) {
    // Get peak level from input
    float peakDb = m_peakDetector.processSample(input);
    
    // Calculate gain reduction needed
    float gainReduction = calculateGainReduction(peakDb);
    
    // Apply soft knee if enabled
    if (m_config.useSoftKnee) {
        gainReduction = applySoftKnee(gainReduction);
    }
    
    // Smooth gain reduction with envelope follower
    m_gainReductionDb = m_envelopeFollower.process(gainReduction);
    
    // Convert gain reduction to linear multiplier
    float gainReductionLinear = std::pow(10.0f, m_gainReductionDb / 20.0f);
    
    // Apply makeup gain
    float makeupGainLinear = std::pow(10.0f, m_config.makeupGainDb / 20.0f);
    
    // Apply compression and makeup
    return input * gainReductionLinear * makeupGainLinear;
}

void Compressor::reset() {
    m_peakDetector.reset();
    m_envelopeFollower.reset();
    m_gainReductionDb = 0.0f;
}

void Compressor::setThreshold(float thresholdDb) {
    m_config.thresholdDb = thresholdDb;
}

void Compressor::setRatio(float ratioDb) {
    m_config.ratioDb = ratioDb;
}

void Compressor::setMakeupGain(float gainDb) {
    m_config.makeupGainDb = gainDb;
}

float Compressor::calculateGainReduction(float peakDb) {
    // If below threshold, no compression
    if (peakDb < m_config.thresholdDb) {
        return 0.0f;
    }
    
    // Calculate overshoot above threshold
    float overDb = peakDb - m_config.thresholdDb;
    
    // Apply compression ratio
    // For 4:1 ratio: output rises 1dB for every 4dB input rise
    float compressedDb = overDb / m_config.ratioDb;
    
    // Gain reduction = how much to reduce to bring back to threshold
    return -(overDb - compressedDb);
}

float Compressor::applySoftKnee(float gainReduction) {
    // Soft knee: gradual transition around threshold
    float kneeStart = m_config.kneeWidthDb / 2.0f;
    
    // If gain reduction is small, apply soft knee
    if (std::abs(gainReduction) < kneeStart) {
        // Smooth transition using quadratic function
        float normalized = gainReduction / kneeStart;
        return gainReduction * (1.0f - 0.5f * normalized * normalized);
    }
    
    return gainReduction;
}

// ============================================================================
// Limiter Implementation
// ============================================================================

Limiter::Limiter(float sampleRate, float ceilingDb)
    : m_ceilingDb(ceilingDb), m_compressor(sampleRate) {
    
    setCeiling(ceilingDb);
    
    // Configure compressor as a brick-wall limiter
    CompressorConfig config(-12.0f, 100.0f, 0.5f, 50.0f);  // 100:1 ratio = limiter
    m_compressor.configure(config);
}

float Limiter::process(float input) {
    float output = m_compressor.process(input);
    
    // Ensure hard ceiling (safety check)
    output = std::max(-m_ceilingLinear, std::min(m_ceilingLinear, output));
    
    return output;
}

void Limiter::setCeiling(float ceilingDb) {
    m_ceilingDb = ceilingDb;
    m_ceilingLinear = std::pow(10.0f, ceilingDb / 20.0f);
}

float Limiter::getGainReductionDb() const {
    return m_compressor.getGainReductionDb();
}

void Limiter::reset() {
    m_compressor.reset();
}

// ============================================================================
// NoiseGate Implementation
// ============================================================================

NoiseGate::NoiseGate(float sampleRate)
    : m_sampleRate(sampleRate), m_thresholdDb(-60.0f), m_thresholdLinear(0.001f), 
      m_gateOpen(false), m_envelopeFollower(sampleRate) {
    
    setTimes(5.0f, 50.0f);  // Default fast attack, moderate release
}

void NoiseGate::setThreshold(float thresholdDb) {
    m_thresholdDb = thresholdDb;
    m_thresholdLinear = std::pow(10.0f, thresholdDb / 20.0f);
}

void NoiseGate::setTimes(float attackMs, float releaseMs) {
    m_envelopeFollower.setTimes(attackMs, releaseMs);
}

float NoiseGate::process(float input) {
    float levelDb = std::abs(input) > 1e-6f ? 
                     20.0f * std::log10(std::abs(input)) : -80.0f;
    
    // Check if signal exceeds threshold
    if (levelDb > m_thresholdDb) {
        m_gateOpen = true;
    } else {
        m_gateOpen = false;
    }
    
    // Use envelope follower for smooth transitions
    float smoothLevel = m_envelopeFollower.process(m_gateOpen ? 0.0f : -80.0f);
    float gateGain = std::pow(10.0f, smoothLevel / 20.0f);
    
    return input * gateGain;
}

void NoiseGate::reset() {
    m_gateOpen = false;
    m_envelopeFollower.reset();
}

// ============================================================================
// OutputStage Implementation
// ============================================================================

OutputStage::OutputStage(float sampleRate)
    : m_sampleRate(sampleRate), m_makeupGainLinear(1.0f),
      m_compressorEnabled(true), m_limiterEnabled(true),
      m_compressor(sampleRate), m_limiter(sampleRate) {
    
    // Default compressor: -20dB threshold, 4:1 ratio
    CompressorConfig compConfig(-20.0f, 4.0f, 10.0f, 100.0f);
    m_compressor.configure(compConfig);
}

float OutputStage::process(float input) {
    float output = input;
    
    // Apply compressor if enabled
    if (m_compressorEnabled) {
        output = m_compressor.process(output);
    }
    
    // Apply limiter if enabled (safety ceiling)
    if (m_limiterEnabled) {
        output = m_limiter.process(output);
    }
    
    // Apply makeup gain
    output *= m_makeupGainLinear;
    
    return output;
}

void OutputStage::setMakeupGain(float gainDb) {
    m_makeupGainLinear = std::pow(10.0f, gainDb / 20.0f);
}

void OutputStage::reset() {
    m_compressor.reset();
    m_limiter.reset();
}

} // namespace LiveSpiceDSP
