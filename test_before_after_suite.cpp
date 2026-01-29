#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

namespace {
    std::string readFile(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            return {};
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool contains(const std::string& haystack, const std::string& needle) {
        return haystack.find(needle) != std::string::npos;
    }

    int runCommand(const std::string& command) {
        return std::system(command.c_str());
    }
}

int main() {
    const std::string schematicPath = "example pedals/Boss Super Overdrive SD-1.schx";
    const std::string outputDir = "JUCE - Boss Super Overdrive SD-1";
    const std::string outputLog = "before_after_output.log";

    const std::string command = "livespice-translator \"" + schematicPath + "\" > \"" + outputLog + "\" 2>&1";
    int result = runCommand(command);
    if (result != 0) {
        std::cerr << "Test failed: translator command returned non-zero exit code: " << result << "\n";
        return 1;
    }

    const std::string logContent = readFile(outputLog);
    if (!contains(logContent, "Nonlinear Components")) {
        std::cerr << "Test failed: analysis output missing 'Nonlinear Components' section.\n";
        return 1;
    }

    const std::filesystem::path headerPath = std::filesystem::path(outputDir) / "CircuitProcessor.h";
    const std::filesystem::path sourcePath = std::filesystem::path(outputDir) / "CircuitProcessor.cpp";

    const std::string header = readFile(headerPath);
    const std::string source = readFile(sourcePath);

    if (header.empty() || source.empty()) {
        std::cerr << "Test failed: generated JUCE files not found or empty.\n";
        return 1;
    }

    if (!contains(header, "#include \"../../DiodeModels.h\"")) {
        std::cerr << "Test failed: missing DiodeModels include in header.\n";
        return 1;
    }

    if (!contains(header, "Nonlinear::DiodeClippingStage")) {
        std::cerr << "Test failed: missing DiodeClippingStage members in header.\n";
        return 1;
    }

    if (!contains(source, "processSample(signal)")) {
        std::cerr << "Test failed: missing nonlinear clipping usage in processBlock.\n";
        return 1;
    }

    // FET-focused test
    const std::string fetSchematic = "example pedals/Simple NMOS Amplifier.schx";
    const std::string fetOutputDir = "JUCE - Simple NMOS Amplifier";
    const std::string fetLog = "fet_output.log";

    const std::string fetCommand = "livespice-translator --beta \"" + fetSchematic + "\" > \"" + fetLog + "\" 2>&1";
    int fetResult = runCommand(fetCommand);
    if (fetResult != 0) {
        std::cerr << "Test failed: FET translator command returned non-zero exit code: " << fetResult << "\n";
        return 1;
    }

    const std::string fetLogContent = readFile(fetLog);
    if (!contains(fetLogContent, "Nonlinear Components")) {
        std::cerr << "Test failed: FET analysis output missing 'Nonlinear Components' section.\n";
        return 1;
    }
    if (!contains(fetLogContent, "FET:")) {
        std::cerr << "Test failed: FET DSP mapping label not found in analysis output.\n";
        return 1;
    }

    const std::filesystem::path fetHeaderPath = std::filesystem::path(fetOutputDir) / "CircuitProcessor.h";
    const std::filesystem::path fetSourcePath = std::filesystem::path(fetOutputDir) / "CircuitProcessor.cpp";

    const std::string fetHeader = readFile(fetHeaderPath);
    const std::string fetSource = readFile(fetSourcePath);

    if (fetHeader.empty() || fetSource.empty()) {
        std::cerr << "Test failed: generated FET JUCE files not found or empty.\n";
        return 1;
    }

    if (!contains(fetHeader, "Nonlinear::FETModelQuadratic")) {
        std::cerr << "Test failed: missing FETModelQuadratic members in FET header.\n";
        return 1;
    }

    if (!contains(fetSource, "processSample(signal)")) {
        std::cerr << "Test failed: missing FET nonlinear usage in processBlock.\n";
        return 1;
    }

    // BJT-focused test
    const std::string bjtSchematic = "example pedals/Common Emitter Transistor Amplifier.schx";
    const std::string bjtOutputDir = "JUCE - Common Emitter Transistor Amplifier";
    const std::string bjtLog = "bjt_output.log";

    const std::string bjtCommand = "livespice-translator --beta \"" + bjtSchematic + "\" > \"" + bjtLog + "\" 2>&1";
    int bjtResult = runCommand(bjtCommand);
    if (bjtResult != 0) {
        std::cerr << "Test failed: BJT translator command returned non-zero exit code: " << bjtResult << "\n";
        return 1;
    }

    const std::string bjtLogContent = readFile(bjtLog);
    if (!contains(bjtLogContent, "Nonlinear Components")) {
        std::cerr << "Test failed: BJT analysis output missing 'Nonlinear Components' section.\n";
        return 1;
    }
    if (!contains(bjtLogContent, "BJT:")) {
        std::cerr << "Test failed: BJT DSP mapping label not found in analysis output.\n";
        return 1;
    }

    const std::filesystem::path bjtHeaderPath = std::filesystem::path(bjtOutputDir) / "CircuitProcessor.h";
    const std::filesystem::path bjtSourcePath = std::filesystem::path(bjtOutputDir) / "CircuitProcessor.cpp";

    const std::string bjtHeader = readFile(bjtHeaderPath);
    const std::string bjtSource = readFile(bjtSourcePath);

    if (bjtHeader.empty() || bjtSource.empty()) {
        std::cerr << "Test failed: generated BJT JUCE files not found or empty.\n";
        return 1;
    }

    if (!contains(bjtHeader, "Nonlinear::BJTModelEbersMoll")) {
        std::cerr << "Test failed: missing BJTModelEbersMoll members in BJT header.\n";
        return 1;
    }

    if (!contains(bjtSource, "processSample(signal)")) {
        std::cerr << "Test failed: missing BJT nonlinear usage in processBlock.\n";
        return 1;
    }

    std::cout << "Before/After test passed.\n";
    return 0;
}
