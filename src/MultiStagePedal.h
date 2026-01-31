#pragma once

#include "DiodeModels.h"
#include "StateSpaceFilter.h"
#include "CompressorDynamics.h"
#include <vector>
#include <memory>
#include <string>

namespace LiveSpiceDSP {

/**
 * @file MultiStagePedal.h
 * @brief Complete multi-stage distortion pedal with full signal chain
 * 
 * Phase 3: Integrates all components into a production-ready pedal:
 * 1. Input buffer + gain staging
 * 2. Diode clipper stage(s) - configurable topology
 * 3. Tone stack (3-band EQ)
 * 4. Output compression & limiting
 * 5. Noise gate
 * 6. Output buffer
 */

// ============================================================================
// Stage Bypass & Configuration
// ============================================================================

struct StageBypass {
    bool inputBuffer = false;
    bool diodeClipper = false;
    bool toneStack = false;
    bool compressor = false;
    bool limiter = false;
    bool noiseGate = false;
    bool outputBuffer = false;
};

// ============================================================================
// Multi-Stage Pedal
// ============================================================================

class MultiStagePedal {
public:
    /**
     * Initialize complete pedal system
     * @param sampleRate Sample rate (Hz)
     * @param numClipperStages Number of diode clipper stages (1-3 typical)
     */
    MultiStagePedal(float sampleRate = 44100.0f, size_t numClipperStages = 1);
    
    /**
     * Process audio sample through entire pedal chain
     * @param input Raw input sample
     * @return Processed output
     */
    float process(float input);
    
    // ========================================================================
    // Stage Configuration
    // ========================================================================
    
    /**
     * Set input gain (drive control)
     */
    void setDrive(float gainDb);
    
    /**
     * Set diode clipper impedance
     */
    void setClipperImpedance(float impedanceOhms);
    
    /**
     * Configure tone stack
     */
    ToneStackController& getToneStack() { return m_toneStack; }
    
    /**
     * Configure output compression
     */
    Compressor& getCompressor() { return m_outputStage.getCompressor(); }
    
    /**
     * Configure output limiter
     */
    Limiter& getLimiter() { return m_outputStage.getLimiter(); }
    
    /**
     * Configure noise gate
     */
    NoiseGate& getNoiseGate() { return m_noiseGate; }
    
    /**
     * Set output level (volume control)
     */
    void setVolume(float levelDb);
    
    // ========================================================================
    // Stage Bypass Control
    // ========================================================================
    
    /**
     * Get bypass configuration
     */
    StageBypass& getBypass() { return m_bypass; }
    
    /**
     * Bypass specific stage
     */
    void setBypass(const std::string& stageName, bool bypassed);
    
    /**
     * Bypass all stages (pass-through)
     */
    void bypassAll();
    
    /**
     * Enable all stages
     */
    void enableAll();
    
    // ========================================================================
    // Meter Functions
    // ========================================================================
    
    /**
     * Get input level (dB)
     */
    float getInputLevel() const { return m_inputLevelDb; }
    
    /**
     * Get clipper gain reduction (dB)
     */
    float getClipperGainReduction() const { return m_clipperGainReductionDb; }
    
    /**
     * Get compressor gain reduction (dB)
     */
    float getCompressorGainReduction() const {
        return m_outputStage.getCompressor().getGainReductionDb();
    }
    
    /**
     * Get output level (dB)
     */
    float getOutputLevel() const { return m_outputLevelDb; }
    
    // ========================================================================
    // System Control
    // ========================================================================
    
    /**
     * Reset all stages
     */
    void reset();
    
    /**
     * Get sample rate
     */
    float getSampleRate() const { return m_sampleRate; }
    
    /**
     * Get number of clipper stages
     */
    size_t getNumClipperStages() const { return m_clipperStages.size(); }

private:
    float m_sampleRate;
    
    // Signal chain stages
    float m_inputGainLinear;
    float m_outputGainLinear;
    
    // Metering
    float m_inputLevelDb;
    float m_clipperGainReductionDb;
    float m_outputLevelDb;
    
    // Bypass control
    StageBypass m_bypass;
    
    // Clipper stages (cascade multiple for more aggressive clipping)
    std::vector<std::shared_ptr<Nonlinear::DiodeClippingStage>> m_clipperStages;
    
    // Tone shaping
    ToneStackController m_toneStack;
    
    // Dynamics processing
    NoiseGate m_noiseGate;
    OutputStage m_outputStage;
    
    // Internal state buffers
    std::array<float, 2> m_inputBufferState;
    std::array<float, 2> m_outputBufferState;
    
    /**
     * Process through input buffer (high-pass @ 30Hz)
     */
    float processInputBuffer(float input);
    
    /**
     * Process through cascaded diode clippers
     */
    float processClippers(float input);
    
    /**
     * Process through output buffer (low-pass @ 10kHz)
     */
    float processOutputBuffer(float input);
    
    /**
     * Calculate signal level in dB
     */
    static float calculateLevelDb(float sample);
};

// ============================================================================
// Preset Manager
// ============================================================================

struct PedalPreset {
    std::string name;
    float drive = 6.0f;
    float volume = 0.0f;
    float bass = 0.0f;
    float mid = 0.0f;
    float treble = 0.0f;
    float presence = 0.0f;
    float compThreshold = -20.0f;
    float compRatio = 4.0f;
    float gateThreshold = -60.0f;
    
    PedalPreset(const std::string& n = "Default") : name(n) {}
};

class PresetManager {
public:
    /**
     * Create default presets
     */
    static std::vector<PedalPreset> getDefaultPresets();
    
    /**
     * Apply preset to pedal
     */
    static void applyPreset(MultiStagePedal& pedal, const PedalPreset& preset);
};

} // namespace LiveSpiceDSP
