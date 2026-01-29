#include "AudioRouter.h"
#include "Logging.h"

AudioRouter::AudioRouter()
{
}

void AudioRouter::prepareToPlay(double newSampleRate, int maximumExpectedSamplesPerBlock)
{
    sampleRate = newSampleRate;
    blockSize = maximumExpectedSamplesPerBlock;
    
    // Allocate temporary buffer for crossfading
    tempBuffer.setSize(2, maximumExpectedSamplesPerBlock);
    
    crossfadeSamples = (crossfadeDurationMs / 1000.0f) * static_cast<float>(sampleRate);
    
    logDebug("AudioRouter: Prepared - " + 
             juce::String(sampleRate) + " Hz, " + 
             juce::String(blockSize) + " samples", LogLevel::DEBUG);
}

void AudioRouter::releaseResources()
{
    tempBuffer.setSize(0, 0);
}

void AudioRouter::setSelection(ProcessorSelection selection)
{
    if (selection == currentSelection)
        return;
    
    targetSelection = selection;
    
    if (crossfadeEnabled)
    {
        // Start crossfade
        crossfadeProgress = 0.0f;
        logDebug("AudioRouter: Starting crossfade to " + 
                 juce::String(selection == ProcessorSelection::A ? "A" : "B"), LogLevel::DEBUG);
    }
    else
    {
        // Instant switch
        currentSelection = selection;
        crossfadeProgress = 1.0f;
        logDebug("AudioRouter: Switched to " + 
                 juce::String(selection == ProcessorSelection::A ? "A" : "B"), LogLevel::INFO);
    }
}

void AudioRouter::setCrossfadeDurationMs(float durationMs)
{
    crossfadeDurationMs = durationMs;
    crossfadeSamples = (crossfadeDurationMs / 1000.0f) * static_cast<float>(sampleRate);
}

void AudioRouter::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages,
    IAudioProcessor* processorA,
    IAudioProcessor* processorB)
{
    const int numSamples = buffer.getNumSamples();
    
    // Handle bypass
    if (bypassed)
    {
        // Pass through input unchanged
        return;
    }
    
    // Handle crossfading
    if (crossfadeEnabled && crossfadeProgress < 1.0f)
    {
        // Process both processors
        if (processorA && processorA->isLoaded())
        {
            // Copy input to temp buffer
            tempBuffer.makeCopyOf(buffer, true);
            
            // Process A
            processorA->processBlock(buffer, midiMessages);
            
            // Process B on temp buffer
            if (processorB && processorB->isLoaded())
            {
                processorB->processBlock(tempBuffer, midiMessages);
            }
            
            // Crossfade between A and B
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                auto* bufferData = buffer.getWritePointer(channel);
                auto* tempData = tempBuffer.getReadPointer(channel);
                
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    // Update crossfade for this sample
                    updateCrossfade(1);
                    
                    // Mix based on crossfade progress
                    float gainA = (targetSelection == ProcessorSelection::A) ? crossfadeProgress : (1.0f - crossfadeProgress);
                    float gainB = 1.0f - gainA;
                    
                    bufferData[sample] = bufferData[sample] * gainA + tempData[sample] * gainB;
                }
            }
            
            // Check if crossfade complete
            if (crossfadeProgress >= 1.0f)
            {
                currentSelection = targetSelection;
                logDebug("AudioRouter: Crossfade complete", LogLevel::DEBUG);
            }
        }
    }
    else
    {
        // No crossfade - direct routing
        if (currentSelection == ProcessorSelection::A)
        {
            if (processorA && processorA->isLoaded())
            {
                processorA->processBlock(buffer, midiMessages);
            }
            else
            {
                buffer.clear();
            }
        }
        else if (currentSelection == ProcessorSelection::B)
        {
            if (processorB && processorB->isLoaded())
            {
                processorB->processBlock(buffer, midiMessages);
            }
            else
            {
                buffer.clear();
            }
        }
        else if (currentSelection == ProcessorSelection::Bypass)
        {
            // Already handled above, but kept for clarity
        }
    }
}

void AudioRouter::updateCrossfade(int numSamples)
{
    if (crossfadeProgress >= 1.0f)
        return;
    
    float increment = static_cast<float>(numSamples) / crossfadeSamples;
    crossfadeProgress = juce::jmin(1.0f, crossfadeProgress + increment);
}
