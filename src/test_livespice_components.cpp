#include <iostream>
#include <cmath>

// Include LiveSpice components only (no JUCE)
#include "third_party/livespice-components/ComponentModels.h"
#include "third_party/livespice-components/DSPImplementations.h"

using namespace LiveSpiceDSP;
using namespace LiveSpiceComponents;

int main() {
    std::cout << "=== Testing LiveSpice Component Availability ===" << std::endl;
    
    try {
        // Test 1: ResistorProcessor
        std::cout << "\n[1] ResistorProcessor... ";
        ResistorProcessor resistor;
        resistor.prepare(10000.0);
        resistor.process(5.0);
        std::cout << "✓ OK (V=" << resistor.getVoltage() << ", R=" << resistor.getResistance() << ")" << std::endl;
        
        // Test 2: CapacitorProcessor
        std::cout << "[2] CapacitorProcessor... ";
        CapacitorProcessor capacitor;
        capacitor.prepare(1e-9, 0.1);
        capacitor.process(5.0, 44100.0);
        std::cout << "✓ OK (V=" << capacitor.getVoltage() << ")" << std::endl;
        
        // Test 3: InductorProcessor
        std::cout << "[3] InductorProcessor... ";
        InductorProcessor inductor;
        inductor.prepare(0.001, 1.0);
        inductor.process(5.0, 44100.0);
        std::cout << "✓ OK" << std::endl;
        
        // Test 4: DiodeProcessor
        std::cout << "[4] DiodeProcessor... ";
        DiodeProcessor diode;
        diode.prepare("1N4148", 25.0);
        diode.process(0.5);
        std::cout << "✓ OK (I=" << diode.getCurrent() << "A)" << std::endl;
        
        // Test 5: BJTProcessor
        std::cout << "[5] BJTProcessor... ";
        BJTProcessor bjt;
        bjt.prepare("2N3904", 25.0);
        bjt.process(0.6, 0.1, 0.0);
        std::cout << "✓ OK" << std::endl;
        
        // Test 6: JFETProcessor
        std::cout << "[6] JFETProcessor... ";
        JFETProcessor jfet;
        jfet.prepare("2N5457");
        jfet.process(1.0, 0.0, 0.0);  // Vgate, Vsource, Vdrain
        std::cout << "✓ OK (Id=" << jfet.getDrainCurrent() << "A)" << std::endl;
        
        // Test 7: OpAmpProcessor
        std::cout << "[7] OpAmpProcessor... ";
        OpAmpProcessor opamp;
        opamp.prepare("TL072", 44100.0);
        opamp.process(1.0, 0.1);
        std::cout << "✓ OK (Vout=" << opamp.getOutputVoltage() << "V)" << std::endl;
        
        // Test 8: TriodeProcessor
        std::cout << "[8] TriodeProcessor... ";
        TriodeProcessor triode;
        triode.prepare("12AX7");
        triode.process(0.0, 2.0, 250.0);  // Vcathode, Vgrid, Vplate
        std::cout << "✓ OK (Ip=" << triode.getPlateCurrent() << "A)" << std::endl;
        
        // Test 9: SoftClipperProcessor
        std::cout << "[9] SoftClipperProcessor... ";
        SoftClipperProcessor softclip;
        softclip.prepare(SoftClipperProcessor::TANH, 1.0, 1.0);
        double output = softclip.process(0.5);
        std::cout << "✓ OK (out=" << output << ")" << std::endl;
        
        std::cout << "\n=== ALL 9 LIVESPICE PROCESSORS AVAILABLE ===" << std::endl;
        std::cout << "Status: ✓ READY FOR JUCE INTEGRATION" << std::endl;
        
        std::cout << "\n=== COMPONENT COVERAGE ===" << std::endl;
        std::cout << "MXR Distortion+ uses:" << std::endl;
        std::cout << "  ✓ ResistorProcessor" << std::endl;
        std::cout << "  ✓ CapacitorProcessor" << std::endl;
        std::cout << "  ✓ DiodeProcessor" << std::endl;
        std::cout << "  ✓ OpAmpProcessor" << std::endl;
        std::cout << "\nAll 4 required components: IMPLEMENTED" << std::endl;
        std::cout << "Additional components available:" << std::endl;
        std::cout << "  • InductorProcessor (for tone stacks)" << std::endl;
        std::cout << "  • BJTProcessor (for transistor stages)" << std::endl;
        std::cout << "  • JFETProcessor (for FET buffers)" << std::endl;
        std::cout << "  • TriodeProcessor (for tube stages)" << std::endl;
        std::cout << "  • SoftClipperProcessor (for audio-friendly clipping)" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
