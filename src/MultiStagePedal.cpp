#include "MultiStagePedal.h"
#include <cmath>
#include <algorithm>

namespace LiveSpiceDSP {

// ============================================================================
// MultiStagePedal Implementation
// ============================================================================

MultiStagePedal::MultiStagePedal(float sampleRate, size_t numClipperStages)
    : m_sampleRate(sampleRate),
      m_inputGainLinear(1.0f),
      m_outputGainLinear(1.0f),
      m_inputLevelDb(-80.0f),
      m_clipperGainReductionDb(0.0f),
      m_outputLevelDb(-80.0f),
      m_toneStack(sampleRate),
      m_noiseGate(sampleRate),
      m_outputStage(sampleRate),
      m_inputBufferState{0.0f, 0.0f},
      m_outputBufferState{0.0f, 0.0f} {
    
    // Create clipper stages
    for (size_t i = 0; i < numClipperStages; ++i) {
        m_clipperStages.push_back(
            std::make_shared<Nonlinear::DiodeClippingStage>(Nonlinear::DiodeCharacteristics(), Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f)
        );
    }
}

float MultiStagePedal::process(float input) {
    float signal = input;
    
    // Measure input level
    m_inputLevelDb = calculateLevelDb(signal);
    
    // Stage 1: Input buffer (if enabled)
    if (!m_bypass.inputBuffer) {
        signal = processInputBuffer(signal);
    }
    
    // Stage 2: Input gain (drive)
    signal *= m_inputGainLinear;
    
    // Stage 3: Diode clippers (if enabled)
    if (!m_bypass.diodeClipper) {
        signal = processClippers(signal);
    }
    
    // Stage 4: Tone stack (if enabled)
    if (!m_bypass.toneStack) {
        signal = m_toneStack.process(signal);
    }
    
    // Stage 5: Noise gate (if enabled)
    if (!m_bypass.noiseGate) {
        signal = m_noiseGate.process(signal);
    }
    
    // Stage 6: Output compression & limiting (if enabled)
    if (!m_bypass.compressor || !m_bypass.limiter) {
        signal = m_outputStage.process(signal);
    }
    
    // Stage 7: Output buffer (if enabled)
    if (!m_bypass.outputBuffer) {
        signal = processOutputBuffer(signal);
    }
    
    // Stage 8: Output gain (volume)
    signal *= m_outputGainLinear;
    
    // Measure final output level
    m_outputLevelDb = calculateLevelDb(signal);
    
    return signal;
}

void MultiStagePedal::setDrive(float gainDb) {
    m_inputGainLinear = std::pow(10.0f, gainDb / 20.0f);
}

void MultiStagePedal::setClipperImpedance(float impedanceOhms) {
    for (auto& clipper : m_clipperStages) {
        clipper->setLoadImpedance(impedanceOhms);
    }
}

void MultiStagePedal::setVolume(float levelDb) {
    m_outputGainLinear = std::pow(10.0f, levelDb / 20.0f);
}

void MultiStagePedal::setBypass(const std::string& stageName, bool bypassed) {
    if (stageName == "input") m_bypass.inputBuffer = bypassed;
    else if (stageName == "clipper") m_bypass.diodeClipper = bypassed;
    else if (stageName == "tone") m_bypass.toneStack = bypassed;
    else if (stageName == "comp") m_bypass.compressor = bypassed;
    else if (stageName == "limiter") m_bypass.limiter = bypassed;
    else if (stageName == "gate") m_bypass.noiseGate = bypassed;
    else if (stageName == "output") m_bypass.outputBuffer = bypassed;
}

void MultiStagePedal::bypassAll() {
    m_bypass.inputBuffer = true;
    m_bypass.diodeClipper = true;
    m_bypass.toneStack = true;
    m_bypass.compressor = true;
    m_bypass.limiter = true;
    m_bypass.noiseGate = true;
    m_bypass.outputBuffer = true;
}

void MultiStagePedal::enableAll() {
    m_bypass.inputBuffer = false;
    m_bypass.diodeClipper = false;
    m_bypass.toneStack = false;
    m_bypass.compressor = false;
    m_bypass.limiter = false;
    m_bypass.noiseGate = false;
    m_bypass.outputBuffer = false;
}

void MultiStagePedal::reset() {
    m_toneStack.reset();
    m_noiseGate.reset();
    m_outputStage.reset();
    
    // Note: DiodeClippingStage doesn't have reset(); it doesn't maintain state
    
    m_inputBufferState = {0.0f, 0.0f};
    m_outputBufferState = {0.0f, 0.0f};
    
    m_inputLevelDb = -80.0f;
    m_clipperGainReductionDb = 0.0f;
    m_outputLevelDb = -80.0f;
}

float MultiStagePedal::processInputBuffer(float input) {
    // 1st-order high-pass filter at ~30Hz
    float omega = 2.0f * 3.14159265359f * 30.0f / m_sampleRate;
    float alpha = std::sin(omega) / 2.0f;
    
    float b0 = (1.0f + std::cos(omega)) / 2.0f;
    float b1 = -(1.0f + std::cos(omega));
    float a1 = -(1.0f - 2.0f * alpha);
    
    b0 /= (1.0f + alpha);
    b1 /= (1.0f + alpha);
    a1 /= (1.0f + alpha);
    
    float output = b0 * input + b1 * m_inputBufferState[0] + a1 * m_inputBufferState[1];
    m_inputBufferState[1] = m_inputBufferState[0];
    m_inputBufferState[0] = input;
    
    return output;
}

float MultiStagePedal::processClippers(float input) {
    float signal = input;
    
    // Cascade all clipper stages
    for (auto& clipper : m_clipperStages) {
        signal = clipper->processSample(signal);
    }
    
    // Measure approximate gain reduction from clipping
    float inputAbs = std::abs(input);
    float outputAbs = std::abs(signal);
    if (inputAbs > 0.01f && outputAbs > 1e-8f) {
        m_clipperGainReductionDb = 20.0f * std::log10(outputAbs / inputAbs);
    } else {
        m_clipperGainReductionDb = 0.0f;  // No clipping detected
    }
    
    return signal;
}

float MultiStagePedal::processOutputBuffer(float input) {
    // 1st-order low-pass filter at ~10kHz
    float omega = 2.0f * 3.14159265359f * 10000.0f / m_sampleRate;
    float alpha = std::sin(omega) / 2.0f;
    
    float b0 = alpha;
    float b1 = alpha;
    float a1 = -(1.0f - 2.0f * alpha);
    
    float output = b0 * input + b1 * m_outputBufferState[0] + a1 * m_outputBufferState[1];
    m_outputBufferState[1] = m_outputBufferState[0];
    m_outputBufferState[0] = input;
    
    return output;
}

float MultiStagePedal::calculateLevelDb(float sample) {
    float absVal = std::abs(sample);
    if (absVal < 1e-6f) return -80.0f;
    return 20.0f * std::log10(absVal);
}

// ============================================================================
// PresetManager Implementation
// ============================================================================

std::vector<PedalPreset> PresetManager::getDefaultPresets() {
    std::vector<PedalPreset> presets;
    
    // Preset 1: Clean Boost
    presets.push_back(PedalPreset("Clean Boost"));
    presets.back().drive = 3.0f;
    presets.back().volume = 3.0f;
    presets.back().bass = 0.0f;
    presets.back().mid = 0.0f;
    presets.back().treble = 0.0f;
    presets.back().compThreshold = -30.0f;
    presets.back().gateThreshold = -70.0f;
    
    // Preset 2: Crunch
    presets.push_back(PedalPreset("Crunch"));
    presets.back().drive = 9.0f;
    presets.back().volume = -3.0f;
    presets.back().bass = 2.0f;
    presets.back().mid = -1.0f;
    presets.back().treble = 1.0f;
    presets.back().compThreshold = -15.0f;
    presets.back().gateThreshold = -50.0f;
    
    // Preset 3: Lead
    presets.push_back(PedalPreset("Lead"));
    presets.back().drive = 15.0f;
    presets.back().volume = -6.0f;
    presets.back().bass = 3.0f;
    presets.back().mid = -3.0f;
    presets.back().treble = 3.0f;
    presets.back().presence = 3.0f;
    presets.back().compThreshold = -10.0f;
    presets.back().gateThreshold = -40.0f;
    
    // Preset 4: Fuzz
    presets.push_back(PedalPreset("Fuzz"));
    presets.back().drive = 18.0f;
    presets.back().volume = -9.0f;
    presets.back().bass = 2.0f;
    presets.back().mid = -6.0f;
    presets.back().treble = 2.0f;
    presets.back().compThreshold = -5.0f;
    presets.back().gateThreshold = -35.0f;
    
    // Preset 5: Warm Tone
    presets.push_back(PedalPreset("Warm Tone"));
    presets.back().drive = 6.0f;
    presets.back().volume = 0.0f;
    presets.back().bass = 6.0f;
    presets.back().mid = 0.0f;
    presets.back().treble = -6.0f;
    presets.back().compThreshold = -20.0f;
    presets.back().gateThreshold = -60.0f;
    
    // Preset 6: Bright Clean
    presets.push_back(PedalPreset("Bright Clean"));
    presets.back().drive = 0.0f;
    presets.back().volume = 0.0f;
    presets.back().bass = -3.0f;
    presets.back().mid = 0.0f;
    presets.back().treble = 6.0f;
    presets.back().presence = 3.0f;
    presets.back().compThreshold = -30.0f;
    presets.back().gateThreshold = -70.0f;
    
    return presets;
}

void PresetManager::applyPreset(MultiStagePedal& pedal, const PedalPreset& preset) {
    pedal.setDrive(preset.drive);
    pedal.setVolume(preset.volume);
    pedal.getToneStack().setBassGain(preset.bass);
    pedal.getToneStack().setMidGain(preset.mid);
    pedal.getToneStack().setTrebleGain(preset.treble);
    pedal.getToneStack().setPresenceGain(preset.presence);
    
    pedal.getCompressor().setThreshold(preset.compThreshold);
    pedal.getCompressor().setRatio(preset.compRatio);
    pedal.getNoiseGate().setThreshold(preset.gateThreshold);
}

} // namespace LiveSpiceDSP
