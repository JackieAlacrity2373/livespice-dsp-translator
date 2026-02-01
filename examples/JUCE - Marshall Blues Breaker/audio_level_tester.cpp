/*
  ==============================================================================
    Marshall Blues Breaker - Audio Level Testing & Diagnostics
    Purpose: Measure output levels without needing user testing
    Generates test signals and reports if gains are appropriate
  ==============================================================================
*/

#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>

// Simple test signal generator
class AudioTester {
public:
    // Generate 1kHz sine wave at specified dB level
    static std::vector<float> generateTestSignal(int sampleRate, float durationSeconds, float dBLevel)
    {
        std::vector<float> signal;
        float amplitude = std::pow(10.0f, dBLevel / 20.0f);  // dB to linear
        float frequency = 1000.0f;  // 1kHz test tone
        
        int numSamples = sampleRate * durationSeconds;
        for (int i = 0; i < numSamples; ++i)
        {
            float phase = 2.0f * 3.14159265f * frequency * i / sampleRate;
            signal.push_back(amplitude * std::sin(phase));
        }
        return signal;
    }
    
    // Measure peak level of audio buffer
    static float measurePeakLevel(const std::vector<float>& signal)
    {
        float peak = 0.0f;
        for (float sample : signal)
        {
            float absVal = std::abs(sample);
            if (absVal > peak) peak = absVal;
        }
        return peak;
    }
    
    // Convert linear to dB
    static float linearToDb(float linear)
    {
        if (linear <= 0.0f) return -120.0f;
        return 20.0f * std::log10(linear);
    }
    
    // Simulate the current gain structure
    static float simulateGainStage(float driveParam, float levelParam)
    {
        // Current implementation from CircuitProcessor.cpp
        float driveGain = 1.0f + (driveParam * driveParam) * 1.5f;  // 1x to 2.5x
        float levelGain = 0.4f + (levelParam * 0.4f);               // 0.4x to 0.8x
        
        // Other stage attenuations (estimated from circuit)
        float opAmpAttenuation = 1.0f;  // Unity gain buffer (identity)
        float diodeClipperAttenuation = 0.85f * 0.85f;  // Cascaded clippers with headroom
        float toneAttenuation = 0.9f;   // Tone control (average)
        float rcFilterAttenuation = 1.0f;  // Passive filters (roughly unity)
        
        // Total cascade gain
        float totalGain = driveGain * opAmpAttenuation * diodeClipperAttenuation * 
                         toneAttenuation * rcFilterAttenuation * levelGain;
        
        return totalGain;
    }
    
    // Test with various parameter combinations
    static void runDiagnostics()
    {
        std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  MARSHALL BLUES BREAKER - AUDIO LEVEL DIAGNOSTICS              ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
        
        // Test input levels
        float testInputDb = -18.0f;  // Typical guitar input level (-18dB = 0.126 linear)
        std::cout << "📊 TEST SIGNAL: 1kHz Sine @ " << testInputDb << " dB\n";
        std::cout << "   (Typical guitar input level)\n\n";
        
        // Generate test signal
        auto testSignal = generateTestSignal(48000, 1.0f, testInputDb);
        float inputPeak = measurePeakLevel(testSignal);
        
        std::cout << "🔧 TESTING CURRENT GAIN STRUCTURE:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << std::fixed << std::setprecision(2);
        
        // Test at different drive/level combinations
        struct TestCase {
            float drive;
            float level;
            const char* description;
        };
        
        TestCase tests[] = {
            {0.0f, 0.0f, "Drive: 0%, Level: 0% (Minimum)"},
            {0.0f, 0.5f, "Drive: 0%, Level: 50% (Clean mid-level)"},
            {0.0f, 1.0f, "Drive: 0%, Level: 100% (Clean max)"},
            {0.5f, 0.5f, "Drive: 50%, Level: 50% (Typical use)"},
            {0.5f, 1.0f, "Drive: 50%, Level: 100% (Pushed)"},
            {1.0f, 0.5f, "Drive: 100%, Level: 50% (Heavy drive)"},
            {1.0f, 1.0f, "Drive: 100%, Level: 100% (MAXIMUM)"}
        };
        
        float maxGain = 0.0f;
        float maxOutputDb = -120.0f;
        
        for (const auto& test : tests)
        {
            float totalGain = simulateGainStage(test.drive, test.level);
            float outputPeak = inputPeak * totalGain;
            float outputDb = linearToDb(outputPeak);
            
            // Determine if this would clip or be too loud
            std::string status = "✅ OK";
            if (outputDb > -0.5f) status = "🔴 CLIPPING!";
            else if (outputDb > -3.0f) status = "⚠️  HOT";
            else if (outputDb < -24.0f) status = "🔇 QUIET";
            
            std::cout << std::left << std::setw(40) << test.description 
                     << " | Gain: " << std::setw(5) << totalGain << "x"
                     << " | Output: " << std::setw(6) << outputDb << " dB"
                     << " | " << status << "\n";
            
            if (totalGain > maxGain) maxGain = totalGain;
            if (outputDb > maxOutputDb) maxOutputDb = outputDb;
        }
        
        std::cout << "\n📈 SUMMARY:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "Maximum Cascade Gain: " << maxGain << "x (" << linearToDb(maxGain) << " dB)\n";
        std::cout << "Maximum Output Level: " << maxOutputDb << " dB\n";
        
        if (maxOutputDb > -0.5f) {
            std::cout << "\n❌ PROBLEM: Output will CLIP at maximum settings!\n";
            std::cout << "   Need to reduce gains by at least " << (-0.5f - maxOutputDb) << " dB\n";
        } else if (maxOutputDb > -3.0f) {
            std::cout << "\n⚠️  WARNING: Output is very HOT, may clip with some audio interfaces\n";
            std::cout << "   Consider reducing gains by " << (-6.0f - maxOutputDb) << " dB\n";
        } else if (maxOutputDb > -12.0f) {
            std::cout << "\n✅ ACCEPTABLE: Output level is reasonable\n";
        } else {
            std::cout << "\n🔇 TOO QUIET: Output is too attenuated\n";
            std::cout << "   Consider increasing gains by " << (-12.0f - maxOutputDb) << " dB\n";
        }
        
        std::cout << "\n🎯 RECOMMENDED ADJUSTMENTS:\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // Calculate required scale factor
        float targetOutputDb = -6.0f;  // Target: -6dB at maximum settings (safe headroom)
        float requiredReduction = maxOutputDb - targetOutputDb;
        float scaleFactor = std::pow(10.0f, -requiredReduction / 20.0f);
        
        std::cout << "Target Output Level: " << targetOutputDb << " dB (safe headroom)\n";
        std::cout << "Required Reduction: " << requiredReduction << " dB (" 
                 << std::setprecision(3) << scaleFactor << "x scale factor)\n\n";
        
        std::cout << "Suggested Code Changes:\n";
        std::cout << "  Drive: 1x - 2.5x  →  1x - " << (2.5f * scaleFactor) << "x\n";
        std::cout << "  Level: 0.4x - 0.8x  →  0.4x - " << (0.8f * scaleFactor) << "x\n";
        
        std::cout << "\n";
    }
};

int main()
{
    AudioTester::runDiagnostics();
    return 0;
}
