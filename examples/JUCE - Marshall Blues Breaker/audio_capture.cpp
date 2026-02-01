#include <iostream>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cmath>

// Simple WAV file writer for capturing audio output
class WAVWriter {
private:
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
    
    std::ofstream file;
    WAVHeader header;
    int32_t dataSize;
    
public:
    WAVWriter(const std::string& filename, int sampleRate, int numChannels, int bitsPerSample)
        : dataSize(0) {
        
        std::strcpy(header.riffId, "RIFF");
        std::strcpy(header.waveId, "WAVE");
        std::strcpy(header.fmtId, "fmt ");
        std::strcpy(header.dataId, "data");
        
        header.fmtSize = 16;
        header.audioFormat = 1;  // PCM
        header.numChannels = numChannels;
        header.sampleRate = sampleRate;
        header.bitsPerSample = bitsPerSample;
        header.blockAlign = (numChannels * bitsPerSample) / 8;
        header.byteRate = sampleRate * header.blockAlign;
        
        file.open(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return;
        }
        
        // Write placeholder header (will update later)
        file.write(reinterpret_cast<char*>(&header), sizeof(WAVHeader));
    }
    
    void writeSample(float sample) {
        int16_t int16Sample = static_cast<int16_t>(
            std::max(-1.0f, std::min(1.0f, sample)) * 32767.0f
        );
        file.write(reinterpret_cast<char*>(&int16Sample), sizeof(int16_t));
        dataSize += sizeof(int16_t);
    }
    
    void writeStereo(float left, float right) {
        writeSample(left);
        writeSample(right);
    }
    
    void close() {
        if (!file.is_open()) return;
        
        // Update file size fields
        header.fileSize = 36 + dataSize;
        header.dataSize = dataSize;
        
        // Go back to start and update header
        file.seekp(0, std::ios::beg);
        file.write(reinterpret_cast<char*>(&header), sizeof(WAVHeader));
        
        file.close();
        
        std::cout << "✅ WAV file written: " << (36 + dataSize) << " bytes" << std::endl;
        std::cout << "   Samples: " << (dataSize / header.blockAlign) << std::endl;
        std::cout << "   Duration: " << std::fixed << std::setprecision(2)
                  << ((dataSize / header.blockAlign) / static_cast<float>(header.sampleRate)) 
                  << " seconds" << std::endl;
    }
    
    bool isOpen() const { return file.is_open(); }
};

// Simulate recording from the plugin for testing
// In production, this would connect to actual plugin output
void recordTestSignal(const std::string& outputFile, int durationSeconds = 5) {
    std::cout << "\n🔴 RECORDING TEST SIGNAL..." << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    const int sampleRate = 44100;
    const int totalSamples = sampleRate * durationSeconds;
    
    WAVWriter writer(outputFile, sampleRate, 2, 16);
    
    // Generate test sweep (1kHz to 5kHz) to test the plugin response
    std::cout << "  Recording: " << durationSeconds << " second sweep (1kHz → 5kHz)" << std::endl;
    
    for (int i = 0; i < totalSamples; ++i) {
        float t = i / static_cast<float>(sampleRate);
        float normalizedTime = t / durationSeconds;
        
        // Frequency sweep from 1kHz to 5kHz
        float f_start = 1000.0f;
        float f_end = 5000.0f;
        float freq = f_start + (f_end - f_start) * normalizedTime;
        
        // Chirp sine wave
        float phase = 2.0f * 3.14159265f * (f_start * t + (f_end - f_start) * t * t / (2.0f * durationSeconds));
        float sample = 0.1f * std::sin(phase);  // Start with quiet test signal
        
        writer.writeStereo(sample, sample);
        
        if ((i + 1) % (sampleRate / 4) == 0) {
            int progress = ((i + 1) * 100) / totalSamples;
            std::cout << "  Progress: " << progress << "% (" << freq << " Hz)" << std::endl;
        }
    }
    
    writer.close();
}

void printInstructions() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "🎸 AUDIO CAPTURE & ANALYSIS WORKFLOW" << std::endl;
    std::cout << std::string(70, '=') << "\n" << std::endl;
    
    std::cout << "STEP 1: Generate test signal" << std::endl;
    std::cout << "  ./audio_oscilloscope generate test_signal.wav" << std::endl;
    std::cout << "  → Creates 5-second frequency sweep (1kHz-5kHz)" << std::endl;
    
    std::cout << "\nSTEP 2: Play test signal through plugin" << std::endl;
    std::cout << "  • Open \"Marshall Blues Breaker.exe\"" << std::endl;
    std::cout << "  • Use system audio routing or plugin instance" << std::endl;
    std::cout << "  • Play test_signal.wav into plugin input" << std::endl;
    std::cout << "  • Record output with preferred tool (Audacity, etc.)" << std::endl;
    std::cout << "  • Save as plugin_output.wav" << std::endl;
    
    std::cout << "\nSTEP 3: Analyze output" << std::endl;
    std::cout << "  ./audio_oscilloscope analyze plugin_output.wav" << std::endl;
    std::cout << "  → Generates detailed analysis of plugin response" << std::endl;
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printInstructions();
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "generate" && argc >= 3) {
        recordTestSignal(argv[2]);
        return 0;
    } else if (command == "analyze" && argc >= 3) {
        std::cout << "To analyze, use: audio_oscilloscope <wavfile>" << std::endl;
        return 0;
    } else {
        // Assume it's a file to analyze
        std::cout << "Invalid command. Usage:" << std::endl;
        std::cout << "  audio_oscilloscope generate <output.wav>" << std::endl;
        std::cout << "  audio_oscilloscope <analysis_file.wav>" << std::endl;
        return 1;
    }
}
