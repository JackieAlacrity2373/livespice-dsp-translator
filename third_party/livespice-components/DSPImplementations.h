#pragma once
#include <cmath>
#include <array>
#include "../livespice-components/ComponentModels.h"

namespace LiveSpiceDSP {

using namespace LiveSpiceComponents;

// ============================================================================
// DSP RESISTOR PROCESSOR
// ============================================================================

class ResistorProcessor {
private:
    double resistance = 1000.0;  // 1k Ohm default
    double voltage = 0.0;
    double current = 0.0;

public:
    void prepare(double resistance) {
        this->resistance = resistance;
    }

    void process(double inputVoltage) {
        voltage = inputVoltage;
        current = ResistorModel::calculateCurrent(voltage, resistance);
    }

    double getVoltage() const { return voltage; }
    double getCurrent() const { return current; }
    double getResistance() const { return resistance; }
};

// ============================================================================
// DSP CAPACITOR PROCESSOR
// ============================================================================

class CapacitorProcessor {
private:
    double capacitance = 1e-6;  // 1µF default
    double voltage = 0.0;
    double current = 0.0;
    double previousVoltage = 0.0;
    double esr = 0.1;

public:
    void prepare(double cap, double seriesResistance = 0.1) {
        capacitance = cap;
        esr = seriesResistance;
        voltage = 0.0;
        previousVoltage = 0.0;
    }

    void process(double inputVoltage, double sampleRate) {
        double dt = 1.0 / sampleRate;
        
        // dV/dt estimation
        double dv_dt = (inputVoltage - previousVoltage) / dt;
        
        // Capacitive current: i = C * dV/dt
        double capacitiveImpedance = 1.0 / (capacitance * dv_dt + 1e-10);
        current = (inputVoltage - voltage) / (esr + capacitiveImpedance);
        
        // Update voltage
        voltage = inputVoltage - (current * esr);
        previousVoltage = voltage;
    }

    double getVoltage() const { return voltage; }
    double getCurrent() const { return current; }
    double getCapacitance() const { return capacitance; }
};

// ============================================================================
// DSP INDUCTOR PROCESSOR
// ============================================================================

class InductorProcessor {
private:
    double inductance = 0.1;  // 100mH default
    double dcResistance = 1.0;
    double current = 0.0;
    double previousCurrent = 0.0;
    double voltage = 0.0;

public:
    void prepare(double inductance, double dcR = 1.0) {
        this->inductance = inductance;
        this->dcResistance = dcR;
        current = 0.0;
        previousCurrent = 0.0;
    }

    void process(double appliedVoltage, double sampleRate) {
        double dt = 1.0 / sampleRate;
        
        // V = L * dI/dt + I*R
        // Solve for current change: dI = (V - I*R) / L * dt
        if (inductance > 0.0) {
            double di_dt = (appliedVoltage - (current * dcResistance)) / inductance;
            current += di_dt * dt;
        }
        
        // Voltage across inductor: V = L * dI/dt
        if (dt > 0.0) {
            voltage = inductance * ((current - previousCurrent) / dt) + (current * dcResistance);
        }
        
        previousCurrent = current;
    }

    double getVoltage() const { return voltage; }
    double getCurrent() const { return current; }
    double getInductance() const { return inductance; }
};

// ============================================================================
// DSP DIODE PROCESSOR - SHOCKLEY EQUATION
// ============================================================================

class DiodeProcessor {
private:
    DiodeModel model;
    double voltage = 0.0;
    double current = 0.0;
    double temperature = 298.15;  // 25°C Kelvin

public:
    void prepare(const std::string& partNumber = "1N4148", double temp = 298.15) {
        model = DiodeModel::getModel(partNumber);
        temperature = temp;
        voltage = 0.0;
        current = 0.0;
    }

    void prepare(const DiodeModel& diodeModel, double temp = 298.15) {
        model = diodeModel;
        temperature = temp;
    }

    void process(double appliedVoltage) {
        voltage = appliedVoltage;
        
        // Shockley equation with series resistance iteration
        // I = IS * (exp(V/(n*Vt)) - 1), considering series resistance
        current = DiodeModel::calculateCurrent(voltage, model.IS, model.n, temperature);
        
        // Apply series resistance feedback correction
        for (int i = 0; i < 3; ++i) {  // 3 iterations for convergence
            double irsDrop = current * model.Rs;
            double correctedVoltage = appliedVoltage - irsDrop;
            current = DiodeModel::calculateCurrent(correctedVoltage, model.IS, model.n, temperature);
        }
    }

    double getVoltage() const { return voltage; }
    double getCurrent() const { return current; }
    double getDifferentialResistance() const {
        // dI/dV for operating point
        const double q = 1.602176634e-19;
        const double k = 1.380649e-23;
        double Vt = k * temperature / q;
        double rd = (model.n * Vt) / (current + 1e-12);  // Avoid division by zero
        return rd + model.Rs;
    }
};

// ============================================================================
// DSP BJT PROCESSOR - EBERS-MOLL MODEL
// ============================================================================

class BJTProcessor {
private:
    BJTModel model;
    double vbe = 0.0;     // Base-emitter voltage
    double vce = 0.0;     // Collector-emitter voltage
    double ic = 0.0;      // Collector current
    double ib = 0.0;      // Base current
    double ie = 0.0;      // Emitter current
    double temperature = 298.15;
    
    // Thermal voltage
    const double q = 1.602176634e-19;
    const double k = 1.380649e-23;

public:
    void prepare(const std::string& partNumber = "2N3904", double temp = 298.15) {
        model = BJTModel::getModel(partNumber);
        temperature = temp;
    }

    void process(double baseVoltage, double collectorVoltage, double emitterVoltage) {
        vbe = baseVoltage - emitterVoltage;
        vce = collectorVoltage - emitterVoltage;
        
        // Calculate collector current
        ic = BJTModel::calculateCollectorCurrent(vbe, vce, model, temperature);
        
        // Beta relationship: Ic = Beta * Ib
        if (vbe > 0.4) {  // Forward bias threshold
            ib = ic / model.Bf;
        } else {
            ic = 0.0;
            ib = 0.0;
        }
        
        // Kirchhoff's current law: Ie = Ic + Ib
        ie = ic + ib;
    }

    double getCollectorCurrent() const { return ic; }
    double getBaseCurrent() const { return ib; }
    double getEmitterCurrent() const { return ie; }
    double getVbe() const { return vbe; }
    double getVce() const { return vce; }
    double getBeta() const { return model.Bf; }
};

// ============================================================================
// DSP JFET PROCESSOR - QUADRATIC MODEL
// ============================================================================

class JFETProcessor {
private:
    JFETModel model;
    double vgs = 0.0;     // Gate-source voltage
    double vds = 0.0;     // Drain-source voltage
    double id = 0.0;      // Drain current
    double gm = 0.0;      // Transconductance

public:
    void prepare(const std::string& partNumber = "2N5457") {
        model = JFETModel::getModel(partNumber);
    }

    void process(double gateVoltage, double sourceVoltage, double drainVoltage) {
        vgs = gateVoltage - sourceVoltage;
        vds = drainVoltage - sourceVoltage;
        
        // Calculate drain current (Shichman-Hodges model)
        id = JFETProcessor::calculateDrainCurrent(vgs, vds, model);
        
        // Transconductance: gm = dId/dVgs = 2*Kp*(Vgs - Vto)*(1 + lambda*Vds)
        double vov = vgs - model.Vto;
        if (vov > 0) {
            gm = 2.0 * model.Kp * vov * (1.0 + model.lambda * vds);
        } else {
            gm = 0.0;
        }
    }

    static double calculateDrainCurrent(double vgs, double vds, const JFETModel& model) {
        double vov = vgs - model.Vto;
        if (vov <= 0) return 0.0;
        
        double id = model.Kp * vov * vov * (1.0 + model.lambda * vds);
        return id;
    }

    double getDrainCurrent() const { return id; }
    double getTransconductance() const { return gm; }
    double getVgs() const { return vgs; }
    double getVds() const { return vds; }
};

// ============================================================================
// DSP OP-AMP PROCESSOR - BEHAVIORAL MODEL
// ============================================================================

class OpAmpProcessor {
private:
    OpAmpModel model;
    double outputVoltage = 0.0;
    double voltage1 = 0.0;
    double voltage2 = 0.0;
    
    // State for frequency response
    std::array<double, 4> filterState = {0.0};

public:
    void prepare(const std::string& partNumber = "UA741", double sampleRate = 48000.0) {
        model = OpAmpModel::getModel(partNumber);
        // Initialize low-pass filter for bandwidth limiting
        initializeLowPass(sampleRate, model.gainBW);
    }

    void process(double nonInvertingInput, double invertingInput) {
        voltage1 = nonInvertingInput;
        voltage2 = invertingInput;
        
        // Differential input
        double vin = voltage1 - voltage2;
        
        // Open-loop gain applied with saturation
        double output = vin * model.gain;
        
        // Apply slew rate limiting
        double maxChange = model.slewRate * 1e-6;  // Per microsecond
        
        // Clamp to supply rails
        outputVoltage = std::max(model.minOutput, std::min(model.maxOutput, output));
    }

    void initializeLowPass(double sampleRate, double cutoff) {
        // 1st order low-pass filter for frequency response
        double wc = 2.0 * 3.14159265359 * cutoff;
        double dt = 1.0 / sampleRate;
        double a = wc * dt / (1.0 + wc * dt);
        filterState[0] = a;
    }

    double getOutputVoltage() const { return outputVoltage; }
    double getGain() const { return model.gain; }
    double getGainBW() const { return model.gainBW; }
    double getSlewRate() const { return model.slewRate; }
};

// ============================================================================
// DSP TRIODE PROCESSOR - KOREN MODEL
// ============================================================================

class TriodeProcessor {
private:
    TriodeModel model;
    double gridVoltage = 0.0;
    double plateVoltage = 0.0;
    double plateCurrent = 0.0;
    double gridCurrent = 0.0;

public:
    void prepare(const std::string& partNumber = "12AX7") {
        model = TriodeModel::getModel(partNumber);
    }

    void process(double cathodeVoltage, double gridVoltageApplied, double plateVoltageApplied) {
        // Voltages relative to cathode
        gridVoltage = gridVoltageApplied - cathodeVoltage;
        plateVoltage = plateVoltageApplied - cathodeVoltage;
        
        // Calculate plate current using Koren model
        plateCurrent = TriodeModel::calculatePlateCurrentKoren(gridVoltage, plateVoltage, model);
        
        // Grid current (simplified - non-linear below cathode potential)
        if (gridVoltage > 0) {
            // Grid is positive relative to cathode - space charge limited
            gridCurrent = plateCurrent * 0.01;  // Approx 1% of plate current
        } else {
            gridCurrent = 0.0;  // Negative grid - no grid current
        }
    }

    double getPlateCurrent() const { return plateCurrent; }
    double getGridCurrent() const { return gridCurrent; }
    double getGridVoltage() const { return gridVoltage; }
    double getPlateVoltage() const { return plateVoltage; }
    double getAmplificationFactor() const { return model.mu; }
};

// ============================================================================
// DSP SOFT CLIPPER - COMMON IN GUITAR PEDALS
// ============================================================================

class SoftClipperProcessor {
public:
    enum ClipType {
        TANH,           // Hyperbolic tangent
        SINE_SHAPED,    // Sine wave shaping
        DIODE_BRIDGE,   // Diode bridge clipper
        TUBE_SATURATE,  // Tube-style saturation
        HARD_CLIP       // Hard limiting
    };

private:
    ClipType clipType = TANH;
    double gainBefore = 1.0;
    double gainAfter = 1.0;
    DiodeModel diodeModel;

public:
    void prepare(ClipType type, double preGain = 1.0, double postGain = 1.0) {
        clipType = type;
        gainBefore = preGain;
        gainAfter = postGain;
    }

    double process(double input) {
        double boosted = input * gainBefore;
        double clipped = 0.0;
        
        switch (clipType) {
            case TANH:
                clipped = std::tanh(boosted);
                break;
                
            case SINE_SHAPED:
                if (boosted > 1.5) {
                    clipped = 1.0;
                } else if (boosted < -1.5) {
                    clipped = -1.0;
                } else {
                    clipped = std::sin(boosted * 3.14159265359 / 3.0);
                }
                break;
                
            case DIODE_BRIDGE: {
                // Diode bridge clipping (two diodes in series)
                DiodeProcessor d1, d2;
                d1.prepare(diodeModel);
                d1.process(boosted);
                double v1 = d1.getVoltage();
                
                d2.prepare(diodeModel);
                d2.process(-boosted);
                double v2 = d2.getVoltage();
                
                clipped = v1 - v2;
                break;
            }
                
            case TUBE_SATURATE: {
                // Tube soft saturation characteristic
                double absInput = std::abs(boosted);
                clipped = std::tanh(absInput) * (boosted / (absInput + 1e-10));
                break;
            }
                
            case HARD_CLIP:
                clipped = std::max(-1.0, std::min(1.0, boosted));
                break;
        }
        
        return clipped * gainAfter;
    }
};

} // namespace LiveSpiceDSP
