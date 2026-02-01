/*
  ==============================================================================
    Auto-generated JUCE Audio Processor Implementation
    Phase 6: Real-time Parameter Controls
  ==============================================================================
*/

#include "CircuitProcessor.h"

CircuitProcessor::CircuitProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
, apvts(*this, nullptr, "Parameters", createParameterLayout()), D1_clipper(Nonlinear::ComponentDB::getDiodeDB().getOrDefault("1N4148"), Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f), D2_clipper(Nonlinear::ComponentDB::getDiodeDB().getOrDefault("1N4148"), Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f), D3_clipper(Nonlinear::ComponentDB::getDiodeDB().getOrDefault("1N4148"), Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f), D4_clipper(Nonlinear::ComponentDB::getDiodeDB().getOrDefault("1N4148"), Nonlinear::DiodeClippingStage::TopologyType::BackToBackDiodes, 10000.0f)
{
    // Initialize parameter pointers
    driveParam = apvts.getRawParameterValue("drive");
    levelParam = apvts.getRawParameterValue("level");
    toneParam = apvts.getRawParameterValue("tone");
    bypassParam = apvts.getRawParameterValue("bypass");
}

CircuitProcessor::~CircuitProcessor()
{
}

const juce::String CircuitProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CircuitProcessor::acceptsMidi() const
{
    return false;
}

bool CircuitProcessor::producesMidi() const
{
    return false;
}

bool CircuitProcessor::isMidiEffect() const
{
    return false;
}

double CircuitProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CircuitProcessor::getNumPrograms()
{
    return 1;
}

int CircuitProcessor::getCurrentProgram()
{
    return 0;
}

void CircuitProcessor::setCurrentProgram (int)
{
}

const juce::String CircuitProcessor::getProgramName (int)
{
    return {};
}

void CircuitProcessor::changeProgramName (int, const juce::String&)
{
}

void CircuitProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 2;

    // ========================================================================
    // Initialize LiveSPICE Component Processors
    // ========================================================================

    // Stage 0: Input Buffer
    // RC High-Pass Filter: f = 15.9155 Hz
    stage0_resistor.prepare(1e+06);
    stage0_capacitor.prepare(1e-08, 0.1); // 1e-08 F with 0.1Ω ESR

    // Stage 1: Op-Amp Clipping Stage
    // Diode clipping with Shockley equation
    stage1_diode1.prepare("1N4148", 25.0); // Silicon diode, 25°C
    stage1_diode2.prepare("1N4148", 25.0);
    stage1_opamp.prepare("TL072", sampleRate); // Dual op-amp

    // Stage 2: Tone Control
    // TODO: Initialize LiveSPICE processor

    // Stage 3: RC Low-Pass Filter
    // RC Low-Pass Filter: fc = 15.9155 Hz
    stage3_resistor.prepare(10000);
    stage3_capacitor.prepare(1e-08, 0.1);
    
    // Initialize tone control filter coefficients
    updateToneControlCoefficients(sampleRate);
    
    // Reset accumulators
    dcOffsetAccumulator = 0.0f;
    prevParameterSmooth_drive = 1.0f;
    prevParameterSmooth_level = 0.3f;
    prevParameterSmooth_tone = 0.5f;

}

void CircuitProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Get current parameter values
    float driveValue = driveParam->load();
    float levelValue = levelParam->load();
    float toneValue = toneParam->load();
    bool bypassValue = bypassParam->load() > 0.5f;

    // Convert normalized parameters to useful ranges with SMOOTHING
    // to prevent clicks/pops from rapid parameter changes
    // ULTRA-CONSERVATIVE: Prevent any clipping whatsoever
    // Drive: 0.0 - 1.0 → 1.0x - 1.02x gain (0dB to +0.17dB) - MINIMAL (reduced from 0.05)
    float targetDriveGain = 1.0f + (driveValue * 0.02f);
    float driveGain = prevParameterSmooth_drive * 0.98f + targetDriveGain * 0.02f;
    prevParameterSmooth_drive = driveGain;
    
    // Level: 0.0 - 1.0 → 0.08x - 0.20x gain (-21.9dB to -13.98dB) - ULTRA-HEAVY ATTENUATION
    // Default at 0.5 = 0.14x = -17.1dB - extreme headroom for clipping
    float targetLevelGain = 0.08f + (levelValue * 0.12f);
    float levelGain = prevParameterSmooth_level * 0.97f + targetLevelGain * 0.03f;
    prevParameterSmooth_level = levelGain;
    
    // Tone smoothing: prevent filter coefficient jumps
    float targetToneValue = toneValue;
    float smoothedToneValue = prevParameterSmooth_tone * 0.90f + targetToneValue * 0.10f;
    prevParameterSmooth_tone = smoothedToneValue;

    // ========================================================================
    // Audio Processing Path Selection
    // ========================================================================
    
    if (bypassValue)
    {
        // BYPASS MODE: Pass signal through cleanly with no processing
        // This avoids any potential polling mismatches or uninitialized data
        
        float prevSample = 0.0f;
        audioStats.averageLevelInBypass = 0.0f;
        audioStats.crackleSuspectSamples = 0;
        
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                // Clean pass-through with output level control only
                float output = channelData[sample] * levelGain;
                
                // DEBUG: Detect crackle artifacts
                if (enableDebugLogging && isCrackleArtifact(prevSample, output))
                {
                    audioStats.crackleSuspectSamples++;
                    if (debugSampleCounter % 1000 == 0)
                    {
                        DBG("⚠️ BYPASS CRACKLE DETECTED: Jump from " << prevSample << " to " << output);
                    }
                }
                
                audioStats.averageLevelInBypass += std::abs(output);
                prevSample = output;
                channelData[sample] = output;
            }
        }
        
        if (enableDebugLogging && audioStats.crackleSuspectSamples > 0)
        {
            DBG("🔧 Bypass Frame Stats: " << audioStats.crackleSuspectSamples << " crackle suspects / " 
                << buffer.getNumSamples() << " samples, Avg Level: " 
                << (audioStats.averageLevelInBypass / buffer.getNumSamples()));
        }
        
        return;  // Exit early - bypass complete
    }

    // ========================================================================
    // ACTIVE PROCESSING MODE
    // LiveSPICE Component-Based DSP Processing
    // Sample-by-sample processing for accurate component modeling
    // ========================================================================

    // Reset audio statistics
    audioStats.peakLevel = 0.0f;
    audioStats.minLevel = 0.0f;
    audioStats.maxLevel = 0.0f;
    audioStats.zeroCrossingCount = 0;
    audioStats.crackleSuspectSamples = 0;
    
    float prevSample = 0.0f;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float signal = channelData[sample];
            
            // Anti-denormalization: clamp very small values to zero
            // Prevents CPU thrashing from subnormal floating point numbers
            if (std::abs(signal) < 1e-20f) signal = 0.0f;
            
            // Apply input drive gain
            signal *= driveGain;
            
            // Stage 0: Input Buffer
            // RC filter using LiveSPICE components
            stage0_resistor.process(signal);
            stage0_capacitor.process(signal, currentSampleRate);
            signal = (float)stage0_capacitor.getVoltage();
<<<<<<< HEAD

            // Stage 1: Op-Amp Clipping Stage
            // TL072 op-amp with diode feedback clipping (unity gain buffer configuration)
            // Signal path: signal → op-amp non-inverting input
            // Feedback: diodes → op-amp inverting input (soft clipping feedback)
            
            // Process through diodes for feedback clipping characteristics
            stage1_diode1.process(signal);
            stage1_diode2.process(-signal);
            
            // Diode feedback voltage (both diodes provide symmetric soft clipping)
            double diodeFeedback = (stage1_diode1.getVoltage() + stage1_diode2.getVoltage()) / 2.0;
            
            // Op-amp in unity-gain buffer configuration:
            // Non-inverting input = signal (direct input)
            // Inverting input = diode feedback (soft clipping limit)
            stage1_opamp.process(signal, diodeFeedback);
            signal = (float)stage1_opamp.getOutputVoltage();
            
            // Clamp tiny values to prevent noise accumulation
            if (std::abs(signal) < 1e-20f) signal = 0.0f;

            // Nonlinear component processing - cascaded diode clippers
            // Process signal through clippers without pre-multiplication
            // Let the clippers' own characteristics handle the dynamics
            signal = D1_clipper.processSample(signal);
            signal = D2_clipper.processSample(signal);
            signal = D3_clipper.processSample(signal);
            signal = D4_clipper.processSample(signal);

            // Stage 2: Tone Control - Proper Low-Pass Filtering
            // Tone = 0.0 = Bright/Treble-heavy
            // Tone = 1.0 = Dark/Bass-heavy with treble rolled off
            // Uses proper single-pole low-pass filter with variable cutoff
            // Use SMOOTHED tone value to prevent coefficient jumping
            int toneIndex = static_cast<int>(smoothedToneValue * 99.0f);
            toneIndex = std::max(0, std::min(99, toneIndex));  // Clamp to valid range [0, 99]
            float toneCoeff = toneCoefficients[toneIndex];
            toneFilterState = toneFilterState * (1.0f - toneCoeff) + signal * toneCoeff;
            signal = toneFilterState;  // Filtered output
            
            // Stage 3: RC Low-Pass Filter
            // RC filter using LiveSPICE components
            stage3_resistor.process(signal);
            stage3_capacitor.process(signal, currentSampleRate);
            signal = (float)stage3_capacitor.getVoltage();
            
            // Clamp tiny values to prevent noise accumulation
            if (std::abs(signal) < 1e-20f) signal = 0.0f;

            // Apply output level control
            signal *= levelGain;
            
            // DC offset removal (high-pass filter accumulation)
            dcOffsetAccumulator = dcOffsetAccumulator * 0.9999f + signal * 0.0001f;
            signal -= dcOffsetAccumulator;

            // Soft limiter with aggressive knee: prevent hard clipping with soft compression
            // Threshold: -6dB (0.5), soft knee starts at -12dB (0.25), hard limit at -1dB (0.9)
            float absSignal = std::abs(signal);
            
            if (absSignal > 0.25f) {
                // Begin soft knee compression at -12dB
                // Maps smoothly from 0.25 (-12dB) → 0.5 (-6dB) → 0.9 (-1dB) → 1.0 (0dB)
                
                if (absSignal > 0.9f) {
                    // Hard limiter above -1dB to prevent ANY clipping
                    signal = signal * (0.9f / (absSignal + 1e-10f));
                } else if (absSignal > 0.5f) {
                    // Soft knee compression: -6dB to -1dB with quadratic curve
                    float excess = absSignal - 0.5f;
                    float knee_reduction = excess * excess * 0.4f;  // Aggressive quadratic (increased from 0.5)
                    signal = signal * (1.0f - knee_reduction / (absSignal + 1e-10f));
                } else {
                    // Soft compression starting at -12dB with gentler curve
                    float excess = absSignal - 0.25f;
                    float knee_reduction = excess * excess * 0.2f;  // Gentle start
                    signal = signal * (1.0f - knee_reduction / (absSignal + 1e-10f));
                }
            }

            // Clip to prevent digital overflow - use conservative threshold to prevent any possibility of clipping
            signal = juce::jlimit(-0.90f, 0.90f, signal);
            
            // Final anti-denormalization
            if (std::abs(signal) < 1e-20f) signal = 0.0f;

            // DEBUG: Audio statistics collection
            if (enableDebugLogging)
            {
                float absSignal = std::abs(signal);
                audioStats.peakLevel = std::max(audioStats.peakLevel, absSignal);
                if (sample == 0 || absSignal < audioStats.minLevel) audioStats.minLevel = absSignal;
                audioStats.maxLevel = std::max(audioStats.maxLevel, absSignal);
                
                // Zero crossing detection (can indicate timing issues)
                if (sample > 0 && ((prevSample > 0 && signal < 0) || (prevSample < 0 && signal > 0)))
                {
                    audioStats.zeroCrossingCount++;
                }
                
                // Crackle detection: sudden amplitude jumps
                if (isCrackleArtifact(prevSample, signal, 0.3f))
                {
                    audioStats.crackleSuspectSamples++;
                }
                
                prevSample = signal;
            }

            channelData[sample] = signal;
        }
    }
    
    // DEBUG: Log frame statistics
    if (enableDebugLogging && debugSampleCounter % 100 == 0)
    {
        DBG("📊 ACTIVE PROCESSING FRAME #" << debugSampleCounter 
            << " | Peak: " << audioStats.peakLevel
            << " | Range: [" << audioStats.minLevel << ", " << audioStats.maxLevel << "]"
            << " | Zero Crossings: " << audioStats.zeroCrossingCount
            << " | Crackle Suspects: " << audioStats.crackleSuspectSamples);
    }
    
    debugSampleCounter++;
}

void CircuitProcessor::releaseResources()
{
    // Resources released automatically
}

juce::AudioProcessorEditor* CircuitProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

bool CircuitProcessor::hasEditor() const
{
    return true;
}

void CircuitProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CircuitProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CircuitProcessor();
}
