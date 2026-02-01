#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <complex>
#include <fstream>
#include <cstring>
#include <cstdint>

// Simple WAV file reading
struct WAVHeader {
    char riffId[4];
    int32_t fileSize;
    char waveId[4];
    char fmtId[4];
    int32_t fmtSize;
    int16_t audioFormat;
    int16_t numChannels;
    int32_t sampleRate;
    int32_t byteRate;
    int16_t blockAlign;
    int16_t bitsPerSample;
    char dataId[4];
    int32_t dataSize;
};

class AudioOscilloscope {
private:
    std::vector<float> audioData;
    int sampleRate;
    
public:
    AudioOscilloscope() : sampleRate(44100) {}
    
    // Load WAV file for analysis
    bool loadWAVFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "❌ Failed to open file: " << filename << std::endl;
            return false;
        }
        
        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));
        
        if (std::strncmp(header.riffId, "RIFF", 4) != 0 ||
            std::strncmp(header.waveId, "WAVE", 4) != 0) {
            std::cerr << "❌ Not a valid WAV file" << std::endl;
            return false;
        }
        
        sampleRate = header.sampleRate;
        int numSamples = header.dataSize / (header.bitsPerSample / 8) / header.numChannels;
        
        audioData.resize(numSamples);
        
        if (header.bitsPerSample == 16) {
            std::vector<int16_t> int16Data(numSamples * header.numChannels);
            file.read(reinterpret_cast<char*>(int16Data.data()), header.dataSize);
            
            // Convert to float and mix channels
            for (int i = 0; i < numSamples; ++i) {
                float sample = 0.0f;
                for (int ch = 0; ch < header.numChannels; ++ch) {
                    sample += static_cast<float>(int16Data[i * header.numChannels + ch]) / 32768.0f;
                }
                audioData[i] = sample / header.numChannels;
            }
        } else if (header.bitsPerSample == 32) {
            std::vector<int32_t> int32Data(numSamples * header.numChannels);
            file.read(reinterpret_cast<char*>(int32Data.data()), header.dataSize);
            
            for (int i = 0; i < numSamples; ++i) {
                float sample = 0.0f;
                for (int ch = 0; ch < header.numChannels; ++ch) {
                    sample += static_cast<float>(int32Data[i * header.numChannels + ch]) / 2147483648.0f;
                }
                audioData[i] = sample / header.numChannels;
            }
        }
        
        file.close();
        return true;
    }
    
    // Analyze audio data
    void analyze() {
        if (audioData.empty()) {
            std::cout << "❌ No audio data loaded" << std::endl;
            return;
        }
        
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "🔬 AUDIO OSCILLOSCOPE & ANALYZER" << std::endl;
        std::cout << std::string(70, '=') << "\n" << std::endl;
        
        // Basic statistics
        printBasicStats();
        
        // Clipping detection
        printClippingAnalysis();
        
        // Frequency analysis
        printFrequencyAnalysis();
        
        // Harmonic distortion
        printHarmonicAnalysis();
        
        // Noise floor
        printNoiseFloor();
        
        // Waveform visualization
        printWaveformVisualization();
        
        // Quality assessment
        printQualityAssessment();
    }
    
private:
    void printBasicStats() {
        std::cout << "📊 BASIC STATISTICS" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        float peak = 0.0f;
        float min = 0.0f;
        float rms = 0.0f;
        float mean = 0.0f;
        
        for (const auto& sample : audioData) {
            peak = std::max(peak, std::abs(sample));
            min = std::min(min, sample);
            rms += sample * sample;
            mean += sample;
        }
        
        rms = std::sqrt(rms / audioData.size());
        mean /= audioData.size();
        
        float peakDb = 20.0f * std::log10(peak + 1e-10f);
        float rmsDb = 20.0f * std::log10(rms + 1e-10f);
        
        std::cout << "  Duration:           " << std::fixed << std::setprecision(3) 
                  << (audioData.size() / static_cast<float>(sampleRate)) << " seconds" << std::endl;
        std::cout << "  Sample Rate:        " << sampleRate << " Hz" << std::endl;
        std::cout << "  Peak Level:         " << std::setprecision(6) << peak << " (" << std::setprecision(2) << peakDb << " dB)" << std::endl;
        std::cout << "  RMS Level:          " << std::setprecision(6) << rms << " (" << std::setprecision(2) << rmsDb << " dB)" << std::endl;
        std::cout << "  Min Level:          " << std::setprecision(6) << min << std::endl;
        std::cout << "  DC Offset:          " << std::setprecision(8) << mean << std::endl;
        std::cout << "  Crest Factor:       " << std::setprecision(3) << (peak / (rms + 1e-10f)) << "x" << std::endl;
        std::cout << std::endl;
    }
    
    void printClippingAnalysis() {
        std::cout << "🔴 CLIPPING DETECTION" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        int harddipCount = 0;
        int softclipCount = 0;
        
        for (const auto& sample : audioData) {
            if (std::abs(sample) >= 0.99f) harddipCount++;
            if (std::abs(sample) >= 0.95f && std::abs(sample) < 0.99f) softclipCount++;
        }
        
        float hardclipPercent = (harddipCount / static_cast<float>(audioData.size())) * 100.0f;
        float softclipPercent = (softclipCount / static_cast<float>(audioData.size())) * 100.0f;
        
        std::cout << "  Hard Clipping (>99%):    " << harddipCount << " samples (" 
                  << std::fixed << std::setprecision(4) << hardclipPercent << "%)" << std::endl;
        std::cout << "  Soft Clipping (95-99%):  " << softclipCount << " samples (" 
                  << std::setprecision(4) << softclipPercent << "%)" << std::endl;
        
        if (hardclipPercent > 0.1f) {
            std::cout << "  ⚠️  WARNING: Hard clipping detected - reduce input or output level!" << std::endl;
        } else if (softclipPercent > 1.0f) {
            std::cout << "  ⚠️  Soft clipping present - may add excessive distortion" << std::endl;
        } else {
            std::cout << "  ✅ No significant clipping detected" << std::endl;
        }
        std::cout << std::endl;
    }
    
    void printFrequencyAnalysis() {
        std::cout << "📈 FREQUENCY ANALYSIS (FFT)" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        // Simple peak detection in frequency domain (simplified)
        int numSamples = std::min(static_cast<int>(audioData.size()), 8192);
        
        // Find fundamental frequency by zero crossing
        int zeroCrossings = 0;
        float prevSample = 0.0f;
        float freq = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            if (i > 0 && ((prevSample > 0 && audioData[i] < 0) || (prevSample < 0 && audioData[i] > 0))) {
                zeroCrossings++;
            }
            prevSample = audioData[i];
        }
        
        if (zeroCrossings > 0) {
            freq = (zeroCrossings * sampleRate) / (2.0f * numSamples);
            std::cout << "  Estimated Fundamental:  " << std::fixed << std::setprecision(1) << freq << " Hz" << std::endl;
        }
        
        // Energy distribution estimation
        float lowEnergy = 0.0f;   // < 500 Hz
        float midEnergy = 0.0f;   // 500 Hz - 5 kHz
        float highEnergy = 0.0f;  // > 5 kHz
        
        // Simple approximation based on derivative
        for (int i = 1; i < numSamples - 1; ++i) {
            float derivative = audioData[i + 1] - audioData[i - 1];
            if (i < numSamples * 0.2f) {
                lowEnergy += derivative * derivative;
            } else if (i < numSamples * 0.8f) {
                midEnergy += derivative * derivative;
            } else {
                highEnergy += derivative * derivative;
            }
        }
        
        float totalEnergy = lowEnergy + midEnergy + highEnergy;
        if (totalEnergy > 0) {
            std::cout << "  Frequency Distribution:" << std::endl;
            std::cout << "    Low (<500 Hz):        " << std::setprecision(1) << (lowEnergy / totalEnergy * 100.0f) << "%" << std::endl;
            std::cout << "    Mid (500 Hz-5 kHz):   " << (midEnergy / totalEnergy * 100.0f) << "%" << std::endl;
            std::cout << "    High (>5 kHz):        " << (highEnergy / totalEnergy * 100.0f) << "%" << std::endl;
        }
        std::cout << std::endl;
    }
    
    void printHarmonicAnalysis() {
        std::cout << "🎵 HARMONIC DISTORTION ANALYSIS" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        int numSamples = std::min(static_cast<int>(audioData.size()), 8192);
        
        // Measure THD approximation
        float signal_power = 0.0f;
        float peaks = 0.0f;
        int peakCount = 0;
        
        for (int i = 1; i < numSamples - 1; ++i) {
            signal_power += audioData[i] * audioData[i];
            
            // Local maxima detection (peaks)
            if (audioData[i] > audioData[i-1] && audioData[i] > audioData[i+1] && audioData[i] > 0.1f) {
                peaks += audioData[i];
                peakCount++;
            }
        }
        
        float rms = std::sqrt(signal_power / numSamples);
        float avgPeak = peakCount > 0 ? peaks / peakCount : 0.0f;
        
        // Estimate harmonic content by measuring waveform symmetry
        float symError = 0.0f;
        for (int i = 0; i < numSamples / 2; ++i) {
            symError += std::abs(audioData[i] - audioData[numSamples - 1 - i]);
        }
        symError /= (numSamples / 2);
        
        float thd_estimated = std::min(100.0f, (symError / (rms + 1e-10f)) * 100.0f);
        
        std::cout << "  Average Peak:           " << std::fixed << std::setprecision(4) << avgPeak << std::endl;
        std::cout << "  Estimated THD:          " << std::setprecision(2) << thd_estimated << "%" << std::endl;
        
        if (thd_estimated < 5.0f) {
            std::cout << "  ✅ Clean signal with minimal distortion" << std::endl;
        } else if (thd_estimated < 15.0f) {
            std::cout << "  ⚠️  Moderate distortion present" << std::endl;
        } else if (thd_estimated < 50.0f) {
            std::cout << "  🎸 High distortion (typical for overdrive)" << std::endl;
        } else {
            std::cout << "  ⚠️  Extreme distortion or noise" << std::endl;
        }
        std::cout << std::endl;
    }
    
    void printNoiseFloor() {
        std::cout << "🔊 NOISE FLOOR ANALYSIS" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        // Find quietest 10% of signal
        std::vector<float> sorted = audioData;
        std::sort(sorted.begin(), sorted.end(), [](float a, float b) {
            return std::abs(a) < std::abs(b);
        });
        
        float noiseFloor = 0.0f;
        int noiseCount = sorted.size() / 10;
        for (int i = 0; i < noiseCount; ++i) {
            noiseFloor += sorted[i] * sorted[i];
        }
        noiseFloor = std::sqrt(noiseFloor / noiseCount);
        
        float noiseLevelDb = 20.0f * std::log10(noiseFloor + 1e-10f);
        
        std::cout << "  Noise Floor Level:      " << std::fixed << std::setprecision(6) << noiseFloor 
                  << " (" << std::setprecision(2) << noiseLevelDb << " dB)" << std::endl;
        
        // Peak to noise ratio
        float peak = 0.0f;
        for (const auto& sample : audioData) {
            peak = std::max(peak, std::abs(sample));
        }
        
        float snr = 20.0f * std::log10((peak + 1e-10f) / (noiseFloor + 1e-10f));
        std::cout << "  Signal-to-Noise Ratio:  " << std::setprecision(2) << snr << " dB" << std::endl;
        
        if (snr > 40.0f) {
            std::cout << "  ✅ Excellent SNR - very clean signal" << std::endl;
        } else if (snr > 20.0f) {
            std::cout << "  ✅ Good SNR - acceptable for audio" << std::endl;
        } else {
            std::cout << "  ⚠️  Poor SNR - significant noise present" << std::endl;
        }
        std::cout << std::endl;
    }
    
    void printWaveformVisualization() {
        std::cout << "📊 WAVEFORM VISUALIZATION (first 512 samples)" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        int displaySamples = std::min(static_cast<int>(audioData.size()), 512);
        int samplesPerChar = std::max(1, displaySamples / 64);
        
        for (int line = 10; line >= -10; --line) {
            float threshold = line * 0.1f;
            
            for (int i = 0; i < displaySamples; i += samplesPerChar) {
                float sample = audioData[i];
                char c = ' ';
                
                if (sample > threshold && sample < threshold + 0.1f) {
                    if (sample > 0) c = '#';
                    else c = '#';
                } else if (std::abs(sample - threshold) < 0.05f) {
                    c = '-';
                }
                
                std::cout << c;
            }
            std::cout << " " << std::setw(4) << std::fixed << std::setprecision(1) << threshold << std::endl;
        }
        std::cout << std::endl;
    }
    
    void printQualityAssessment() {
        std::cout << "⭐ OVERALL QUALITY ASSESSMENT" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        // Run all checks and score
        int score = 100;
        std::vector<std::string> issues;
        
        // Check peak level
        float peak = 0.0f;
        float rms = 0.0f;
        for (const auto& sample : audioData) {
            peak = std::max(peak, std::abs(sample));
            rms += sample * sample;
        }
        rms = std::sqrt(rms / audioData.size());
        
        if (peak > 0.99f) {
            score -= 30;
            issues.push_back("Hard clipping detected");
        } else if (peak > 0.95f) {
            score -= 15;
            issues.push_back("Soft clipping present");
        } else if (peak < 0.1f) {
            score -= 20;
            issues.push_back("Signal too quiet");
        }
        
        // Check DC offset
        float mean = 0.0f;
        for (const auto& sample : audioData) mean += sample;
        mean /= audioData.size();
        
        if (std::abs(mean) > 0.01f) {
            score -= 10;
            issues.push_back("Significant DC offset");
        }
        
        // Print score
        std::cout << "  Audio Quality Score:    " << score << "/100" << std::endl;
        
        if (score >= 90) {
            std::cout << "  Grade:                  🟢 A+ (Excellent)" << std::endl;
        } else if (score >= 80) {
            std::cout << "  Grade:                  🟢 A (Very Good)" << std::endl;
        } else if (score >= 70) {
            std::cout << "  Grade:                  🟡 B (Good)" << std::endl;
        } else if (score >= 60) {
            std::cout << "  Grade:                  🟠 C (Fair)" << std::endl;
        } else {
            std::cout << "  Grade:                  🔴 F (Poor)" << std::endl;
        }
        
        if (!issues.empty()) {
            std::cout << "\n  Issues Found:" << std::endl;
            for (const auto& issue : issues) {
                std::cout << "    • " << issue << std::endl;
            }
        } else {
            std::cout << "  ✅ No issues detected!" << std::endl;
        }
        
        std::cout << std::endl << std::string(70, '=') << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: audio_oscilloscope <wav_file>" << std::endl;
        std::cout << "Example: audio_oscilloscope output.wav" << std::endl;
        return 1;
    }
    
    AudioOscilloscope scope;
    
    if (!scope.loadWAVFile(argv[1])) {
        return 1;
    }
    
    scope.analyze();
    
    return 0;
}
