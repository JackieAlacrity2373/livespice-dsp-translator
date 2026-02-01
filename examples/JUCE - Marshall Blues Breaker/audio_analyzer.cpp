#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdint>

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

class SimpleAudioAnalyzer {
private:
    std::vector<float> audioData;
    int sampleRate;
    
public:
    SimpleAudioAnalyzer() : sampleRate(44100) {}
    
    bool loadWAV(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file: " << filename << std::endl;
            return false;
        }
        
        std::cout << "Reading file: " << filename << std::endl;
        
        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));
        
        if (!file.good()) {
            std::cerr << "Error: Cannot read WAV header" << std::endl;
            return false;
        }
        
        std::cout << "WAV Header Read:" << std::endl;
        std::cout << "  Channels: " << header.numChannels << std::endl;
        std::cout << "  Sample Rate: " << header.sampleRate << std::endl;
        std::cout << "  Bits Per Sample: " << header.bitsPerSample << std::endl;
        std::cout << "  Data Size: " << header.dataSize << " bytes" << std::endl;
        
        sampleRate = header.sampleRate;
        int numSamples = header.dataSize / (header.bitsPerSample / 8) / header.numChannels;
        audioData.resize(numSamples);
        
        std::cout << "Converting " << numSamples << " samples..." << std::endl;
        
        if (header.bitsPerSample == 16) {
            std::vector<int16_t> buffer(numSamples * header.numChannels);
            file.read(reinterpret_cast<char*>(buffer.data()), header.dataSize);
            
            for (int i = 0; i < numSamples; ++i) {
                float sample = 0.0f;
                for (int ch = 0; ch < header.numChannels; ++ch) {
                    sample += static_cast<float>(buffer[i * header.numChannels + ch]) / 32768.0f;
                }
                audioData[i] = sample / header.numChannels;
            }
        } else if (header.bitsPerSample == 32) {
            std::vector<int32_t> buffer(numSamples * header.numChannels);
            file.read(reinterpret_cast<char*>(buffer.data()), header.dataSize);
            
            for (int i = 0; i < numSamples; ++i) {
                float sample = 0.0f;
                for (int ch = 0; ch < header.numChannels; ++ch) {
                    sample += static_cast<float>(buffer[i * header.numChannels + ch]) / 2147483648.0f;
                }
                audioData[i] = sample / header.numChannels;
            }
        }
        
        file.close();
        return true;
    }
    
    void analyze() {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "AUDIO ANALYSIS REPORT" << std::endl;
        std::cout << std::string(80, '=') << "\n" << std::endl;
        
        if (audioData.empty()) {
            std::cout << "ERROR: No audio data" << std::endl;
            return;
        }
        
        // Basic stats
        float peak = 0.0f;
        float rms = 0.0f;
        float mean = 0.0f;
        int hardClipCount = 0;
        int softClipCount = 0;
        
        for (const auto& sample : audioData) {
            float abs_sample = std::abs(sample);
            peak = std::max(peak, abs_sample);
            rms += sample * sample;
            mean += sample;
            
            if (abs_sample >= 0.99f) hardClipCount++;
            else if (abs_sample >= 0.95f) softClipCount++;
        }
        
        rms = std::sqrt(rms / audioData.size());
        mean /= audioData.size();
        
        float peakDb = 20.0f * std::log10(peak + 1e-10f);
        float rmsDb = 20.0f * std::log10(rms + 1e-10f);
        
        std::cout << "[BASIC METRICS]" << std::endl;
        std::cout << "Duration:            " << std::fixed << std::setprecision(2) 
                  << (audioData.size() / static_cast<float>(sampleRate)) << " seconds" << std::endl;
        std::cout << "Sample Rate:         " << sampleRate << " Hz" << std::endl;
        std::cout << "Total Samples:       " << audioData.size() << std::endl;
        std::cout << "Peak Level:          " << std::setprecision(4) << peak << " (" 
                  << std::setprecision(2) << peakDb << " dB)" << std::endl;
        std::cout << "RMS Level:           " << std::setprecision(4) << rms << " (" 
                  << std::setprecision(2) << rmsDb << " dB)" << std::endl;
        std::cout << "Mean (DC Offset):    " << std::setprecision(6) << mean << std::endl;
        std::cout << "Crest Factor:        " << std::setprecision(3) << (peak / (rms + 1e-10f)) << "x" << std::endl;
        
        std::cout << "\n[CLIPPING ANALYSIS]" << std::endl;
        float hardClipPercent = (hardClipCount / static_cast<float>(audioData.size())) * 100.0f;
        float softClipPercent = (softClipCount / static_cast<float>(audioData.size())) * 100.0f;
        std::cout << "Hard Clipping (>99%):   " << hardClipCount << " samples (" 
                  << std::setprecision(4) << hardClipPercent << "%)" << std::endl;
        std::cout << "Soft Clipping (95-99%): " << softClipCount << " samples (" 
                  << std::setprecision(4) << softClipPercent << "%)" << std::endl;
        
        if (hardClipPercent > 0.1f) {
            std::cout << "WARNING: Hard clipping detected - reduce input or output!" << std::endl;
        } else if (softClipPercent > 1.0f) {
            std::cout << "NOTE: Soft clipping present" << std::endl;
        } else {
            std::cout << "OK: No significant clipping" << std::endl;
        }
        
        // THD estimate
        std::cout << "\n[DISTORTION ANALYSIS]" << std::endl;
        float symError = 0.0f;
        int halfSize = audioData.size() / 2;
        for (int i = 0; i < halfSize; ++i) {
            symError += std::abs(audioData[i] - audioData[audioData.size() - 1 - i]);
        }
        symError /= halfSize;
        float thd = std::min(100.0f, (symError / (rms + 1e-10f)) * 100.0f);
        
        std::cout << "Estimated THD:       " << std::setprecision(2) << thd << "%" << std::endl;
        if (thd < 5.0f) {
            std::cout << "Quality: CLEAN (minimal distortion)" << std::endl;
        } else if (thd < 15.0f) {
            std::cout << "Quality: MODERATE DISTORTION" << std::endl;
        } else if (thd < 50.0f) {
            std::cout << "Quality: HIGH DISTORTION (typical for overdrive)" << std::endl;
        } else {
            std::cout << "Quality: EXTREME DISTORTION" << std::endl;
        }
        
        // Noise floor
        std::cout << "\n[NOISE ANALYSIS]" << std::endl;
        std::vector<float> sortedByMagnitude = audioData;
        std::sort(sortedByMagnitude.begin(), sortedByMagnitude.end(), 
                  [](float a, float b) { return std::abs(a) < std::abs(b); });
        
        float noiseFloor = 0.0f;
        int noiseCount = sortedByMagnitude.size() / 10;
        for (int i = 0; i < noiseCount; ++i) {
            noiseFloor += sortedByMagnitude[i] * sortedByMagnitude[i];
        }
        noiseFloor = std::sqrt(noiseFloor / noiseCount);
        float noiseDb = 20.0f * std::log10(noiseFloor + 1e-10f);
        float snr = 20.0f * std::log10((peak + 1e-10f) / (noiseFloor + 1e-10f));
        
        std::cout << "Noise Floor:         " << std::setprecision(6) << noiseFloor 
                  << " (" << std::setprecision(2) << noiseDb << " dB)" << std::endl;
        std::cout << "Signal-to-Noise:     " << std::setprecision(2) << snr << " dB" << std::endl;
        if (snr > 40.0f) {
            std::cout << "SNR Quality: EXCELLENT" << std::endl;
        } else if (snr > 20.0f) {
            std::cout << "SNR Quality: GOOD" << std::endl;
        } else {
            std::cout << "SNR Quality: POOR - significant noise" << std::endl;
        }
        
        // Overall assessment
        std::cout << "\n[OVERALL ASSESSMENT]" << std::endl;
        int score = 100;
        if (hardClipPercent > 0.1f) score -= 30;
        else if (softClipPercent > 1.0f) score -= 15;
        else if (peak < 0.1f) score -= 20;
        
        if (std::abs(mean) > 0.01f) score -= 10;
        
        std::cout << "Quality Score:       " << score << "/100" << std::endl;
        if (score >= 90) {
            std::cout << "Grade: A+ (EXCELLENT)" << std::endl;
        } else if (score >= 80) {
            std::cout << "Grade: A (VERY GOOD)" << std::endl;
        } else if (score >= 70) {
            std::cout << "Grade: B (GOOD)" << std::endl;
        } else if (score >= 60) {
            std::cout << "Grade: C (FAIR)" << std::endl;
        } else {
            std::cout << "Grade: F (POOR)" << std::endl;
        }
        
        std::cout << "\n" << std::string(80, '=') << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: audio_analyzer <wav_file>" << std::endl;
        return 1;
    }
    
    SimpleAudioAnalyzer analyzer;
    if (!analyzer.loadWAV(argv[1])) {
        return 1;
    }
    
    analyzer.analyze();
    return 0;
}
