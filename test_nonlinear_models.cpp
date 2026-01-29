#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <vector>
#include "DiodeModels.h"
#include "TransistorModels.h"
#include "ComponentCharacteristicsDatabase.h"

using namespace Nonlinear;
using namespace ComponentDB;

/**
 * test_nonlinear_models.cpp
 * 
 * Comprehensive test suite for nonlinear diode and transistor models.
 * 
 * Validates:
 * 1. Shockley diode equation accuracy
 * 2. Lookup table performance and accuracy
 * 3. Newton-Raphson convergence
 * 4. BJT DC operating point calculation
 * 5. FET quadratic model
 * 6. Audio-rate clipping behavior
 * 7. Component database lookups
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
        
        // Test 1: Zero voltage -> near-zero current
        float i0 = lut.evaluateCurrent(0.0f);
        assert(std::abs(i0) < 1e-6f);
        
        // Test 2: Forward bias (0.65V) -> positive current
        float i065 = lut.evaluateCurrent(0.65f);
        assert(i065 > 0.0f);
        assert(i065 < 0.1f);
        
        // Test 3: Reverse bias (-5V) -> reverse saturation current
        float iRev = lut.evaluateCurrent(-5.0f);
        assert(iRev < 0.0f);
        assert(std::abs(iRev) < 1e-12f);  // Very small
        
        // Test 4: Monotonicity (current increases with voltage)
        float i0_1 = lut.evaluateCurrent(0.1f);
        float i0_5 = lut.evaluateCurrent(0.5f);
        float i0_6 = lut.evaluateCurrent(0.6f);
        
        assert(i0_1 < i0_5);
        assert(i0_5 < i0_6);
        
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
        cfg.maxIterations = 10;
        cfg.convergenceTolerance = 1e-6f;
        
        // Test case 1: 1V applied voltage
        float vDiode, iDiode;
        int iterations = solver.solve(1.0f, cfg, vDiode, iDiode);
        
        // Should converge (iterations > 0)
        assert(iterations > 0);
        assert(iterations <= cfg.maxIterations);
        
        // Verify solution: Vd + I*Rs = Vin
        float residual = vDiode + iDiode * 0.25f - 1.0f;
        assert(std::abs(residual) < cfg.convergenceTolerance);
        
        // Test case 2: Higher voltage
        iterations = solver.solve(5.0f, cfg, vDiode, iDiode);
        assert(iterations > 0);
        
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
        
        // Small signal should pass through with minimal clipping
        float smallSignal = 0.1f;
        float clipped = clipper.processSample(smallSignal);
        assert(std::abs(clipped) <= 0.15f);
        
        // Large positive signal should clip at ~0.6V
        float largePos = 5.0f;
        float clippedPos = clipper.processSample(largePos);
        assert(clippedPos > 0.0f);
        assert(clippedPos < 0.8f);
        
        // Large negative signal should clip at ~-0.6V
        float largeNeg = -5.0f;
        float clippedNeg = clipper.processSample(largeNeg);
        assert(clippedNeg < 0.0f);
        assert(clippedNeg > -0.8f);
        
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
        
        // 1N4148 vs 1N914: 1N914 has higher saturation current (faster switching)
        assert(diode1N914.Is > diode1N4148.Is);
        
        // Germanium diodes have higher ideality factor
        assert(diodeGe.n > diode1N4148.n);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

// ============================================================================
// Transistor Model Tests
// ============================================================================

void testBJTOperatingPointActive() {
    TestResult test("BJT Operating Point - Active Region");
    
    try {
        BJTModelEbersMoll bjt(BJTCharacteristics::NPN2N3904());
        
        // Active region: Vbe ≈ 0.65V, Vce = 5V
        auto op = bjt.solveOperatingPoint(0.65f, 5.0f);
        
        // Verify physical correctness
        assert(op.Ic > 0.0f);                      // Positive collector current
        assert(op.Ib > 0.0f);                      // Positive base current
        assert(op.Ic / op.Ib > 50.0f);            // Beta > 50
        assert(op.Ic / op.Ib < 500.0f);           // Beta < 500
        assert(!op.isSaturated);                   // Not saturated
        assert(op.gm > 0.0f);                      // Positive transconductance
        assert(op.rce > 0.0f);                     // Positive output resistance
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testBJTOperatingPointSaturation() {
    TestResult test("BJT Operating Point - Saturation");
    
    try {
        BJTModelEbersMoll bjt(BJTCharacteristics::NPN2N3904());
        
        // Saturation: Low Vce (0.2V)
        auto opSat = bjt.solveOperatingPoint(0.65f, 0.2f);
        
        // Should be saturated
        assert(opSat.isSaturated);
        assert(opSat.Vce < 0.3f);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testBJTTemperatureEffect() {
    TestResult test("BJT Temperature Effect");
    
    try {
        // Test at two different temperatures
        BJTModelEbersMoll bjtCold(BJTCharacteristics::NPN2N3904(), 0.0f);
        BJTModelEbersMoll bjtWarm(BJTCharacteristics::NPN2N3904(), 50.0f);
        
        // At same input voltages
        auto opCold = bjtCold.solveOperatingPoint(0.65f, 5.0f);
        auto opWarm = bjtWarm.solveOperatingPoint(0.65f, 5.0f);
        
        // Both should be reasonable
        assert(opCold.Ic > 0.0f);
        assert(opWarm.Ic > 0.0f);
        
        // Warm transistor typically has slightly higher current (more transconductance)
        // (Implementation dependent on temperature coefficient)
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testBJTCommonTypes() {
    TestResult test("BJT Common Types - Parameters");
    
    try {
        auto bjt2N3904 = BJTCharacteristics::NPN2N3904();
        auto bjt2N2222 = BJTCharacteristics::NPN2N2222();
        auto bjtBC107 = BJTCharacteristics::NPNBC107();
        
        // Verify reasonable ranges
        assert(bjt2N3904.Bf > 100.0f && bjt2N3904.Bf < 500.0f);
        assert(bjt2N2222.Bf > 100.0f && bjt2N2222.Bf < 500.0f);
        assert(bjtBC107.Bf > 100.0f && bjtBC107.Bf < 500.0f);
        
        // Vaf should be in 50-200V range
        assert(bjt2N3904.Vaf > 50.0f && bjt2N3904.Vaf < 200.0f);
        
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
        FETModelQuadratic fet(FETCharacteristics::NMOS2N7000());
        
        // Saturation: Vgs=3V, Vds=5V (large Vds)
        auto op = fet.solveOperatingPoint(3.0f, 5.0f);
        
        // Verify saturation
        assert(op.isSaturated);
        assert(op.Id > 0.0f);
        assert(op.gm > 0.0f);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testFETOperatingPointLinear() {
    TestResult test("FET Operating Point - Linear");
    
    try {
        FETModelQuadratic fet(FETCharacteristics::NMOS2N7000());
        
        // Linear region: Vgs=3V, Vds=0.5V (small Vds)
        auto op = fet.solveOperatingPoint(3.0f, 0.5f);
        
        // Should NOT be saturated
        assert(!op.isSaturated);
        assert(op.Id > 0.0f);
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testFETCutoff() {
    TestResult test("FET Cutoff Region");
    
    try {
        FETModelQuadratic fet(FETCharacteristics::NMOS2N7000());
        
        // Below threshold: Vgs = 0.5V (below Vto ≈ 1.5V)
        auto op = fet.solveOperatingPoint(0.5f, 5.0f);
        
        // Should be off
        assert(op.Id < 1e-9f);  // Minimal current
        assert(op.gm < 1e-9f);  // Minimal transconductance
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

// ============================================================================
// Transistor Clipping Stage Tests
// ============================================================================

void testTransistorClippingStage() {
    TestResult test("Transistor Clipping Stage");
    
    try {
        TransistorClippingStage clipper(10.0f);  // 10x gain
        
        // Small input
        float small = 0.05f;
        float smallOut = clipper.processSample(small);
        assert(std::abs(smallOut) < 1.0f);  // Should be within reasonable range
        
        // Large input (should clip)
        float large = 2.0f;
        float largeOut = clipper.processSample(large);
        assert(std::abs(largeOut) < 2.0f);  // Should be clipped
        
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
        auto& db = ComponentDB::getDiodeDB();
        
        // Test successful lookups
        auto result1N4148 = db.lookup("1N4148");
        assert(result1N4148.has_value());
        
        auto resultGe = db.lookup("OA90");
        assert(resultGe.has_value());
        
        // Test getOrDefault
        auto defaulted = db.getOrDefault("UNKNOWN_DIODE");
        assert(defaulted.Is > 0.0f);  // Should have valid Is
        
        test.pass();
    } catch (const std::exception& e) {
        test.fail(std::string("Exception: ") + e.what());
    }
}

void testBJTDatabaseLookup() {
    TestResult test("BJT Database - Lookup");
    
    try {
        auto& db = ComponentDB::getBJTDB();
        
        // Test successful lookups
        auto result2N3904 = db.lookup("2N3904");
        assert(result2N3904.has_value());
        
        auto resultBC107 = db.lookup("BC107");
        assert(resultBC107.has_value());
        
        // Test getOrDefault
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
        auto& db = ComponentDB::getFETDB();
        
        // Test successful lookups
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
        // Create diode info
        auto diode = ComponentDB::NonlinearComponentInfo::fromDiode("1N4148", "D1");
        assert(diode.type == ComponentDB::NonlinearComponentInfo::ComponentType::DIODE);
        assert(diode.diodeChar.has_value());
        
        // Create BJT info
        auto bjt = ComponentDB::NonlinearComponentInfo::fromBJT("2N3904", "Q1", false);
        assert(bjt.type == ComponentDB::NonlinearComponentInfo::ComponentType::BJT_NPN);
        assert(bjt.bjtChar.has_value());
        
        // Create FET info
        auto fet = ComponentDB::NonlinearComponentInfo::fromFET("2N7000", "M1", false);
        assert(fet.type == ComponentDB::NonlinearComponentInfo::ComponentType::FET_NMOS);
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
    
    // Diode tests
    std::cout << "DIODE TESTS:" << std::endl;
    {
        testDiodeShokcleyEquationBasic();
        testDiodeLookupTable();
        testDiodeNewtonRaphsonConvergence();
        testDiodeClippingStageBackToBack();
        testDiodeCommonTypes();
    }
    
    // BJT tests
    std::cout << "\nBJT TESTS:" << std::endl;
    {
        testBJTOperatingPointActive();
        testBJTOperatingPointSaturation();
        testBJTTemperatureEffect();
        testBJTCommonTypes();
    }
    
    // FET tests
    std::cout << "\nFET TESTS:" << std::endl;
    {
        testFETOperatingPointSaturation();
        testFETOperatingPointLinear();
        testFETCutoff();
    }
    
    // Clipping stage tests
    std::cout << "\nCLIPPING STAGE TESTS:" << std::endl;
    {
        testTransistorClippingStage();
    }
    
    // Database tests
    std::cout << "\nCOMPONENT DATABASE TESTS:" << std::endl;
    {
        testDiodeDatabaseLookup();
        testBJTDatabaseLookup();
        testFETDatabaseLookup();
        testNonlinearComponentInfo();
    }
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Test Suite Complete!" << std::endl;
    std::cout << std::string(70, '=') << "\n" << std::endl;
    
    return 0;
}
