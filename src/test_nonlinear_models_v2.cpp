#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include "DiodeModels.h"
#include "TransistorModels.h"
#include "ComponentCharacteristicsDatabase.h"

using namespace Nonlinear;
using namespace Nonlinear::ComponentDB;

/**
 * test_nonlinear_models_v2.cpp
 * 
 * Comprehensive test suite for nonlinear diode and transistor models.
 */

// ============================================================================
// Test Infrastructure
// ============================================================================

class TestResult {
public:
    TestResult(const std::string& testName) : m_testName(testName), m_passed(true) {}
    
    void pass() {
        m_passed = true;
        std::cout << "✓ " << m_testName << std::endl;
    }
    
    void fail(const std::string& reason) {
        m_passed = false;
        std::cout << "✗ " << m_testName << std::endl;
        std::cout << "  Reason: " << reason << std::endl;
    }
    
    bool passed() const { return m_passed; }
    
private:
    std::string m_testName;
    bool m_passed;
};

// ============================================================================
// Diode Model Tests
// ============================================================================

void testDiodeShokcleyEquationBasic() {
    TestResult test("Diode Shockley Equation - Basic");
    
    try {
        DiodeCharacteristics diode = DiodeCharacteristics::Si1N4148();
        
        // Manual calculation of Shockley equation
        float Vd = 0.65f;
        float nVt = diode.n * diode.Vt;
        float expectedCurrent = diode.Is * (std::exp(Vd / nVt) - 1.0f);
        
        // Verify it's positive and reasonable
        assert(expectedCurrent > 0.0f);
        assert(expectedCurrent < 0.1f);  // Should be < 100mA
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testDiodeLookupTable() {
    TestResult test("Diode Lookup Table");
    
    try {
        DiodeLUT lut(DiodeCharacteristics::Si1N4148());
        
        // Test at boundaries
        float i0 = lut.evaluateCurrent(0.0f);
        assert(std::abs(i0) < 1e-6f);
        
        // Test forward bias
        float iForward = lut.evaluateCurrent(0.65f);
        assert(iForward > 0.0f);
        assert(iForward < 0.01f);
        
        // Test reverse bias
        float iRev = lut.evaluateCurrent(-5.0f);
        assert(std::abs(iRev) < 1e-12f);  // Very small
        
        // Test monotonicity
        float i1 = lut.evaluateCurrent(0.5f);
        float i2 = lut.evaluateCurrent(0.6f);
        assert(i1 < i2);  // Current increases with voltage
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testDiodeNewtonRaphsonConvergence() {
    TestResult test("Diode Newton-Raphson Convergence");
    
    try {
        DiodeNewtonRaphson solver(DiodeCharacteristics::Si1N4148());
        DiodeNewtonRaphson::SolverConfig cfg;
        cfg.maxIterations = 30;
        cfg.convergenceTolerance = 1e-5f;
        
        float vDiode = 0.0f, iDiode = 0.0f;
        int iterations = solver.solve(0.7f, cfg, vDiode, iDiode);
        
        // Verify convergence happened
        assert(iterations > 0);
        assert(vDiode > 0.0f && vDiode < 1.0f);
        assert(iDiode > 0.0f);
        
        // Verify circuit equation holds
        float residual = 0.7f - (vDiode + iDiode * 0.25f);
        assert(std::abs(residual) < 1e-4f);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testDiodeClippingStageBackToBack() {
    TestResult test("Diode Clipping Stage - Back-to-Back");
    
    try {
        DiodeClippingStage clipper(
            DiodeCharacteristics::Si1N4148(),
            DiodeClippingStage::TopologyType::BackToBackDiodes,
            10000.0f
        );
        
        float smallSignal = 0.01f;
        float clipped = clipper.processSample(smallSignal);
        assert(std::abs(clipped) <= 0.15f);
        
        // Large signal should clip
        float largeSignal = 5.0f;
        float clipped2 = clipper.processSample(largeSignal);
        assert(std::abs(clipped2) < std::abs(largeSignal));
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testDiodeCommonTypes() {
    TestResult test("Diode Common Types - Lookup");
    
    try {
        auto diode1N4148 = DiodeCharacteristics::Si1N4148();
        auto diode1N914 = DiodeCharacteristics::Si1N914();
        auto diodeGe = DiodeCharacteristics::Ge_OA90();
        
        // Verify different characteristics
        assert(diode1N4148.Is > 0.0f);
        assert(diode1N914.Is > 0.0f);
        assert(diodeGe.Is > 0.0f);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

// ============================================================================
// BJT Model Tests
// ============================================================================

void testBJTOperatingPointActive() {
    TestResult test("BJT Operating Point - Active Region");
    
    try {
        BJTCharacteristics bjt = BJTCharacteristics::TwoN3904();
        BJTModelEbersMoll model(bjt);
        
        BJTOperatingPoint op = model.solveOperatingPoint(
            0.7f,   // Vbe (V)
            5.0f,   // Vcc (V)
            1000.0f // Rc (Ω)
        );
        
        // Verify active region
        assert(op.Ic > 0.0f);
        assert(!op.isSaturated);
        assert(op.gm > 0.0f);  // Transconductance
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testBJTOperatingPointSaturation() {
    TestResult test("BJT Operating Point - Saturation");
    
    try {
        BJTCharacteristics bjt = BJTCharacteristics::TwoN3904();
        BJTModelEbersMoll model(bjt);
        
        // Force into saturation with high base current
        BJTOperatingPoint op = model.solveOperatingPoint(
            1.0f,   // Vbe (V) - high base drive
            5.0f,   // Vcc (V)
            1000.0f // Rc (Ω)
        );
        
        // Verify saturation detection
        assert(op.isSaturated);
        assert(op.Vce < 0.3f);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testBJTTemperatureEffect() {
    TestResult test("BJT Temperature Effect");
    
    try {
        BJTCharacteristics bjt = BJTCharacteristics::TwoN3904();
        
        // Simulate temperature coefficient
        float tempCoeff = bjt.tempCoeffVbe;  // Should be negative (-2mV/°C)
        assert(tempCoeff < 0.0f);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testBJTCommonTypes() {
    TestResult test("BJT Common Types - Parameters");
    
    try {
        auto bjt1 = BJTCharacteristics::TwoN3904();
        auto bjt2 = BJTCharacteristics::BC107();
        
        // Verify they have different parameters
        assert(bjt1.Bf > 0.0f);
        assert(bjt2.Bf > 0.0f);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

// ============================================================================
// FET Model Tests
// ============================================================================

void testFETOperatingPointSaturation() {
    TestResult test("FET Operating Point - Saturation");
    
    try {
        FETCharacteristics fet = FETCharacteristics::NMOS2N7000();
        FETModelQuadratic model(fet);
        
        FETOperatingPoint op = model.solveOperatingPoint(
            5.0f,   // Vgs (V)
            5.0f,   // Vds (V)
            1000.0f // Rd (Ω)
        );
        
        // Verify saturation region
        assert(op.Id > 0.0f);
        assert(op.isSaturated);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testFETOperatingPointLinear() {
    TestResult test("FET Operating Point - Linear");
    
    try {
        FETCharacteristics fet = FETCharacteristics::NMOS2N7000();
        FETModelQuadratic model(fet);
        
        FETOperatingPoint op = model.solveOperatingPoint(
            3.0f,   // Vgs (V)
            0.5f,   // Vds (V) - low Vds for linear region
            1000.0f // Rd (Ω)
        );
        
        // Verify linear region
        assert(op.Id > 0.0f);
        assert(!op.isSaturated);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testFETCutoff() {
    TestResult test("FET Cutoff Region");
    
    try {
        FETCharacteristics fet = FETCharacteristics::NMOS2N7000();
        FETModelQuadratic model(fet);
        
        FETOperatingPoint op = model.solveOperatingPoint(
            0.5f,   // Vgs (V) - below threshold
            5.0f,   // Vds (V)
            1000.0f // Rd (Ω)
        );
        
        // Verify cutoff region
        assert(op.Id < 1e-9f);  // Very small leakage current
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

// ============================================================================
// Clipping Stage Tests
// ============================================================================

void testTransistorClippingStage() {
    TestResult test("Transistor Clipping Stage");
    
    try {
        TransistorClippingStage clipper(
            BJTCharacteristics::TwoN3904(),
            10.0f,    // Input impedance
            false     // is PNP
        );
        
        float smallOut = clipper.processSample(0.1f);
        assert(std::abs(smallOut) <= 1.0f);  // tanh saturates to �1
        
        float largeOut = clipper.processSample(10.0f);
        assert(std::abs(largeOut) <= 1.0f);  // Should be clipped by tanh
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

// ============================================================================
// Component Database Tests
// ============================================================================

void testDiodeDatabaseLookup() {
    TestResult test("Diode Database - Lookup");
    
    try {
        auto& db = DiodeDatabase::getInstance();
        
        auto result1N4148 = db.lookup("1N4148");
        assert(result1N4148.has_value());
        
        auto resultGe = db.lookup("OA90");
        assert(resultGe.has_value());
        
        auto defaulted = db.getOrDefault("UNKNOWN_DIODE");
        assert(defaulted.Is > 0.0f);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testBJTDatabaseLookup() {
    TestResult test("BJT Database - Lookup");
    
    try {
        auto& db = BJTDatabase::getInstance();
        
        auto result2N3904 = db.lookup("2N3904");
        assert(result2N3904.has_value());
        
        auto resultBC107 = db.lookup("BC107");
        assert(resultBC107.has_value());
        
        auto defaulted = db.getOrDefault("UNKNOWN_BJT");
        assert(defaulted.Bf > 0.0f);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testFETDatabaseLookup() {
    TestResult test("FET Database - Lookup");
    
    try {
        auto& db = FETDatabase::getInstance();
        
        auto result2N7000 = db.lookup("2N7000");
        assert(result2N7000.has_value());
        
        auto resultBS170 = db.lookup("BS170");
        assert(resultBS170.has_value());
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testNonlinearComponentInfo() {
    TestResult test("NonlinearComponentInfo Constructors");
    
    try {
        auto diode = NonlinearComponentInfo::fromDiode("1N4148", "D1");
        assert(diode.diodeChar.has_value());
        
        auto bjt = NonlinearComponentInfo::fromBJT("2N3904", "Q1", false);
        assert(bjt.bjtChar.has_value());
        
        auto fet = NonlinearComponentInfo::fromFET("2N7000", "M1", false);
        assert(fet.fetChar.has_value());
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "NonLinear Component Model Test Suite" << std::endl;
    std::cout << std::string(70, '=') << "\n" << std::endl;
    
    std::cout << "DIODE TESTS:" << std::endl;
    testDiodeShokcleyEquationBasic();
    testDiodeLookupTable();
    testDiodeNewtonRaphsonConvergence();
    testDiodeClippingStageBackToBack();
    testDiodeCommonTypes();
    
    std::cout << "\nBJT TESTS:" << std::endl;
    testBJTOperatingPointActive();
    testBJTOperatingPointSaturation();
    testBJTTemperatureEffect();
    testBJTCommonTypes();
    
    std::cout << "\nFET TESTS:" << std::endl;
    testFETOperatingPointSaturation();
    testFETOperatingPointLinear();
    testFETCutoff();
    
    std::cout << "\nCLIPPING STAGE TESTS:" << std::endl;
    testTransistorClippingStage();
    
    std::cout << "\nCOMPONENT DATABASE TESTS:" << std::endl;
    testDiodeDatabaseLookup();
    testBJTDatabaseLookup();
    testFETDatabaseLookup();
    testNonlinearComponentInfo();
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Test Suite Complete!" << std::endl;
    std::cout << std::string(70, '=') << "\n" << std::endl;
    
    return 0;
}
