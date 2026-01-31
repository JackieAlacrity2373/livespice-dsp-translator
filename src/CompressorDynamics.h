#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include <vector>

namespace LiveSpiceDSP {

/**
 * @file CompressorDynamics.h
 * @brief Dynamic range compression and limiting for pedal output stage
 * 
 * Phase 3: Implements real-time compressor with:
 * - Adjustable threshold, ratio, attack, release
 * - Look-ahead peak detection
 * - Smooth gain reduction envelope
 * - Optional soft knee
 */

// ============================================================================
// Compressor Configuration
// ============================================================================

struct CompressorConfig {
    float thresholdDb = -20.0f;      // Compression threshold (dB)
    float ratioDb = 4.0f;            // Compression ratio (4:1 = 4.0)
    float attackMs = 10.0f;          // Attack time (milliseconds)
    float releaseMs = 100.0f;        // Release time (milliseconds)
    float makeupGainDb = 0.0f;       // Output makeup gain (dB)
    bool useSoftKnee = true;         // Soft knee transition
    float kneeWidthDb = 6.0f;        // Soft knee width (dB)
    
    CompressorConfig() = default;
    CompressorConfig(float thresh, float ratio, float att, float rel)
        : thresholdDb(thresh), ratioDb(ratio), attackMs(att), releaseMs(rel) {}
};

// ============================================================================
// Peak Detector (Look-ahead)
// ============================================================================

class PeakDetector {
public:
    /**
     * Initialize peak detector
     * @param sampleRate Sample rate (Hz)
     * @param lookAheadMs Look-ahead window (milliseconds)
     */
    PeakDetector(float sampleRate = 44100.0f, float lookAheadMs = 5.0f);
    
    /**
     * Process sample and return peak level
     * @param sample Input sample
     * @return Peak level in dB
     */
    float processSample(float sample);
    
    /**
     * Get current peak
     */
    float getPeakDb() const { return m_peakDb; }
    
    /**
     * Reset detector
     */
    void reset();

private:
    float m_sampleRate;
    float m_peakDb;
    size_t m_lookAheadSamples;
    std::vector<float> m_buffer;
    size_t m_bufferIndex;
};

// ============================================================================
// Envelope Follower (Attack/Release)
// ============================================================================

class EnvelopeFollower {
public:
    /**
     * Initialize envelope follower
     * @param sampleRate Sample rate (Hz)
     */
    EnvelopeFollower(float sampleRate = 44100.0f);
    
    /**
     * Set attack and release times
     * @param attackMs Attack time (milliseconds)
     * @param releaseMs Release time (milliseconds)
     */
    void setTimes(float attackMs, float releaseMs);
    
    /**
     * Process envelope (smooth transitions)
     * @param targetDb Target level (dB)
     * @return Smoothed level
     */
    float process(float targetDb);
    
    /**
     * Reset to initial state
     */
    void reset();

private:
    float m_sampleRate;
    float m_currentDb;
    float m_attackCoeff;
    float m_releaseCoeff;
};

// ============================================================================
// Compressor Core
// ============================================================================

class Compressor {
public:
    /**
     * Initialize compressor
     * @param sampleRate Sample rate (Hz)
     */
    Compressor(float sampleRate = 44100.0f);
    
    /**
     * Configure compressor
     * @param config Compressor settings
     */
    void configure(const CompressorConfig& config);
    
    /**
     * Process single sample through compressor
     * @param input Input sample
     * @return Compressed output
     */
    float process(float input);
    
    /**
     * Get current gain reduction (dB)
     */
    float getGainReductionDb() const { return m_gainReductionDb; }
    
    /**
     * Reset compressor state
     */
    void reset();
    
    /**
     * Set threshold (dB)
     */
    void setThreshold(float thresholdDb);
    
    /**
     * Set compression ratio
     */
    void setRatio(float ratioDb);
    
    /**
     * Set makeup gain
     */
    void setMakeupGain(float gainDb);

private:
    float m_sampleRate;
    CompressorConfig m_config;
    PeakDetector m_peakDetector;
    EnvelopeFollower m_envelopeFollower;
    float m_gainReductionDb;
    
    /**
     * Calculate gain reduction from peak level
     */
    float calculateGainReduction(float peakDb);
    
    /**
     * Apply soft knee transition
     */
    float applySoftKnee(float gainReduction);
};

// ============================================================================
// Limiter (Hard Ceiling Compressor)
// ============================================================================

class Limiter {
public:
    /**
     * Initialize limiter
     * @param sampleRate Sample rate (Hz)
     * @param ceilingDb Hard ceiling level (typically 0dB or -0.5dB)
     */
    Limiter(float sampleRate = 44100.0f, float ceilingDb = -0.5f);
    
    /**
     * Process sample through limiter
     * @param input Input sample
     * @return Limited output (bounded)
     */
    float process(float input);
    
    /**
     * Set ceiling level
     * @param ceilingDb Maximum output level (dB)
     */
    void setCeiling(float ceilingDb);
    
    /**
     * Get current gain reduction
     */
    float getGainReductionDb() const;
    
    /**
     * Reset limiter
     */
    void reset();

private:
    float m_ceilingDb;
    float m_ceilingLinear;
    Compressor m_compressor;
};

// ============================================================================
// Noise Gate
// ============================================================================

class NoiseGate {
public:
    /**
     * Initialize noise gate
     * @param sampleRate Sample rate (Hz)
     */
    NoiseGate(float sampleRate = 44100.0f);
    
    /**
     * Set gate threshold
     * @param thresholdDb Gate opens above this level (dB)
     */
    void setThreshold(float thresholdDb);
    
    /**
     * Set attack and release times
     * @param attackMs Attack time (milliseconds)
     * @param releaseMs Release time (milliseconds)
     */
    void setTimes(float attackMs, float releaseMs);
    
    /**
     * Process sample through noise gate
     * @param input Input sample
     * @return Gated output (muted if below threshold)
     */
    float process(float input);
    
    /**
     * Get gate state (true = open, false = closed)
     */
    bool isOpen() const { return m_gateOpen; }
    
    /**
     * Reset gate state
     */
    void reset();

private:
    float m_sampleRate;
    float m_thresholdDb;
    float m_thresholdLinear;
    bool m_gateOpen;
    EnvelopeFollower m_envelopeFollower;
};

// ============================================================================
// Output Stage (Compressor + Limiter + Makeup Gain)
// ============================================================================

class OutputStage {
public:
    /**
     * Initialize output stage
     * @param sampleRate Sample rate (Hz)
     */
    OutputStage(float sampleRate = 44100.0f);
    
    /**
     * Process sample through complete output stage
     * @param input Input sample
     * @return Final output with dynamics processing
     */
    float process(float input);
    
    /**
     * Access compressor for configuration
     */
    Compressor& getCompressor() { return m_compressor; }
    const Compressor& getCompressor() const { return m_compressor; }
    
    /**
     * Access limiter for configuration
     */
    Limiter& getLimiter() { return m_limiter; }
    
    /**
     * Set output makeup gain
     * @param gainDb Makeup gain (dB)
     */
    void setMakeupGain(float gainDb);
    
    /**
     * Enable/disable compressor
     */
    void setCompressorEnabled(bool enabled) { m_compressorEnabled = enabled; }
    
    /**
     * Enable/disable limiter
     */
    void setLimiterEnabled(bool enabled) { m_limiterEnabled = enabled; }
    
    /**
     * Reset all stages
     */
    void reset();

private:
    float m_sampleRate;
    float m_makeupGainLinear;
    bool m_compressorEnabled;
    bool m_limiterEnabled;
    Compressor m_compressor;
    Limiter m_limiter;
};

} // namespace LiveSpiceDSP
