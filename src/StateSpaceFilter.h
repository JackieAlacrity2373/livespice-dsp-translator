#pragma once

#include <cmath>
#include <array>
#include <vector>

namespace LiveSpiceDSP {

/**
 * @file StateSpaceFilter.h
 * @brief State-space filtering for frequency-dependent tone shaping
 * 
 * Phase 2: Implements biquadratic IIR filters in state-space form for:
 * - Bass/Mid/Treble tone control (3-band EQ)
 * - Frequency-dependent impedance effects
 * - Transformer coupling saturation
 * - Output low-pass filtering
 * 
 * Uses direct form II topology with cascade capability
 */

// ============================================================================
// Biquadratic Filter Coefficients
// ============================================================================

struct BiquadCoefficients {
    float b0, b1, b2;  // Numerator coefficients
    float a1, a2;      // Denominator coefficients (a0 normalized to 1.0)
    
    BiquadCoefficients() : b0(1.0f), b1(0.0f), b2(0.0f), a1(0.0f), a2(0.0f) {}
    
    BiquadCoefficients(float b0_, float b1_, float b2_, float a1_, float a2_)
        : b0(b0_), b1(b1_), b2(b2_), a1(a1_), a2(a2_) {}
};

// ============================================================================
// Biquadratic IIR Filter (Direct Form II)
// ============================================================================

class BiquadFilter {
public:
    BiquadFilter();
    
    /**
     * Process single sample through biquad
     * @param input Input sample
     * @return Filtered output
     */
    float process(float input);
    
    /**
     * Set filter coefficients
     * @param coeff New biquad coefficients
     */
    void setCoefficients(const BiquadCoefficients& coeff);
    
    /**
     * Reset internal state
     */
    void reset();
    
    /**
     * Get current state for analysis
     */
    std::array<float, 2> getState() const { return m_state; }
    
    // ========================================================================
    // Factory methods for common filter types
    // ========================================================================
    
    /**
     * Design low-pass Butterworth filter
     * @param sampleRate Sample rate (Hz)
     * @param cutoffFreq Cutoff frequency (-3dB point)
     * @return Filter coefficients
     */
    static BiquadCoefficients designLowPass(float sampleRate, float cutoffFreq);
    
    /**
     * Design high-pass Butterworth filter
     * @param sampleRate Sample rate (Hz)
     * @param cutoffFreq Cutoff frequency (-3dB point)
     * @return Filter coefficients
     */
    static BiquadCoefficients designHighPass(float sampleRate, float cutoffFreq);
    
    /**
     * Design peak (bell) filter
     * @param sampleRate Sample rate (Hz)
     * @param centerFreq Center frequency (Hz)
     * @param qFactor Q factor (bandwidth = centerFreq / Q)
     * @param gainDb Gain at center frequency (dB)
     * @return Filter coefficients
     */
    static BiquadCoefficients designPeakFilter(float sampleRate, float centerFreq, 
                                               float qFactor, float gainDb);
    
    /**
     * Design low-shelf filter (bass control)
     * @param sampleRate Sample rate (Hz)
     * @param cutoffFreq Transition frequency (Hz)
     * @param qFactor Q factor
     * @param gainDb Gain amount (dB)
     * @return Filter coefficients
     */
    static BiquadCoefficients designLowShelf(float sampleRate, float cutoffFreq,
                                             float qFactor, float gainDb);
    
    /**
     * Design high-shelf filter (treble control)
     * @param sampleRate Sample rate (Hz)
     * @param cutoffFreq Transition frequency (Hz)
     * @param qFactor Q factor
     * @param gainDb Gain amount (dB)
     * @return Filter coefficients
     */
    static BiquadCoefficients designHighShelf(float sampleRate, float cutoffFreq,
                                              float qFactor, float gainDb);

private:
    std::array<float, 2> m_state;      // State variables: [w[n-1], w[n-2]]
    BiquadCoefficients m_coeff;         // Current coefficients
};

// ============================================================================
// Cascaded Biquad Filter Bank
// ============================================================================

class BiquadFilterBank {
public:
    /**
     * Create filter bank with specified cascade stages
     * @param numStages Number of biquad stages to cascade
     */
    BiquadFilterBank(size_t numStages = 1);
    
    /**
     * Process sample through all cascaded stages
     * @param input Input sample
     * @return Filtered output
     */
    float process(float input);
    
    /**
     * Set coefficients for specific stage
     * @param stageIndex Stage index (0-based)
     * @param coeff Biquad coefficients
     */
    void setStageCoefficients(size_t stageIndex, const BiquadCoefficients& coeff);
    
    /**
     * Reset all stages
     */
    void reset();
    
    /**
     * Get number of stages
     */
    size_t getNumStages() const { return m_stages.size(); }

private:
    std::vector<BiquadFilter> m_stages;
};

// ============================================================================
// 3-Band Tone Stack Controller
// ============================================================================

class ToneStackController {
public:
    /**
     * Initialize tone stack with default cutoff frequencies
     * @param sampleRate Sample rate (Hz)
     */
    ToneStackController(float sampleRate = 44100.0f);
    
    /**
     * Process audio through tone stack
     * @param input Input sample
     * @return Tone-shaped output
     */
    float process(float input);
    
    /**
     * Set bass control (-12dB to +12dB)
     * @param bassGainDb Bass gain in dB
     */
    void setBassGain(float bassGainDb);
    
    /**
     * Set mid control (-12dB to +12dB)
     * @param midGainDb Mid gain in dB
     */
    void setMidGain(float midGainDb);
    
    /**
     * Set treble control (-12dB to +12dB)
     * @param trebleGainDb Treble gain in dB
     */
    void setTrebleGain(float trebleGainDb);
    
    /**
     * Set presence control (emphasis around 4-5kHz)
     * @param presenceGainDb Presence boost in dB
     */
    void setPresenceGain(float presenceGainDb);
    
    /**
     * Reset all filters and state
     */
    void reset();
    
    /**
     * Get bass filter bank
     */
    BiquadFilterBank& getBassFilterBank() { return m_bassFilter; }
    
    /**
     * Get mid filter bank
     */
    BiquadFilterBank& getMidFilterBank() { return m_midFilter; }
    
    /**
     * Get treble filter bank
     */
    BiquadFilterBank& getTrebleFilterBank() { return m_trebleFilter; }

private:
    float m_sampleRate;
    BiquadFilterBank m_bassFilter;      // Low-shelf @ 120Hz
    BiquadFilterBank m_midFilter;       // Peak filter @ 1kHz
    BiquadFilterBank m_trebleFilter;    // High-shelf @ 4.5kHz
    BiquadFilterBank m_presenceFilter;  // Peak filter @ 4.5kHz
};

// ============================================================================
// Frequency Response Analyzer
// ============================================================================

class FrequencyResponseAnalyzer {
public:
    /**
     * Analyze magnitude response of biquad filter
     * @param coeff Filter coefficients
     * @param frequencies Array of test frequencies (Hz)
     * @param sampleRate Sample rate (Hz)
     * @return Magnitude response (linear, not dB)
     */
    static std::vector<float> getMagnitudeResponse(
        const BiquadCoefficients& coeff,
        const std::vector<float>& frequencies,
        float sampleRate);
    
    /**
     * Analyze phase response of biquad filter
     * @param coeff Filter coefficients
     * @param frequencies Array of test frequencies (Hz)
     * @param sampleRate Sample rate (Hz)
     * @return Phase response (radians)
     */
    static std::vector<float> getPhaseResponse(
        const BiquadCoefficients& coeff,
        const std::vector<float>& frequencies,
        float sampleRate);
    
    /**
     * Generate logarithmic frequency sweep from fLow to fHigh
     * @param fLow Lowest frequency (Hz)
     * @param fHigh Highest frequency (Hz)
     * @param numPoints Number of points in sweep
     * @return Array of frequency points
     */
    static std::vector<float> generateLogSweep(float fLow, float fHigh, size_t numPoints);
};

// ============================================================================
// Combined Pedal DSP Chain (Phase 2 Integrated)
// ============================================================================

class DistortionPedalDSP {
public:
    /**
     * Initialize complete distortion pedal with clipping and tone shaping
     * @param sampleRate Sample rate (Hz)
     */
    DistortionPedalDSP(float sampleRate = 44100.0f);
    
    /**
     * Process audio sample through complete chain:
     * Input → Input Buffer → Diode Clipper → Tone Stack → Output Buffer
     * 
     * @param input Raw input sample
     * @return Processed and tone-shaped output
     */
    float process(float input);
    
    /**
     * Set input gain (drive control)
     * @param gainDb Gain in dB
     */
    void setInputGain(float gainDb);
    
    /**
     * Set output level (volume control)
     * @param levelDb Level in dB
     */
    void setOutputLevel(float levelDb);
    
    /**
     * Access tone stack controller
     */
    ToneStackController& getToneStack() { return m_toneStack; }
    
    /**
     * Reset entire DSP chain
     */
    void reset();

private:
    float m_sampleRate;
    float m_inputGain;
    float m_outputLevel;
    ToneStackController m_toneStack;
    
    // State for input/output buffers
    std::array<float, 2> m_inputBufferState;
    std::array<float, 2> m_outputBufferState;
};

} // namespace LiveSpiceDSP
