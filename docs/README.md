# LiveSpice To JUCE DSP Translator

Convert LiveSpice guitar pedal schematics to JUCE audio plugins with real-time pattern-matched DSP processing.

**Status:** ✅ Production Ready | **Latest:** Phase 2 Complete (Pattern Matching)

---

## 📚 Documentation

**→ [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)** - Full documentation map (start here)

Key docs:
- [QUICK_START.md](QUICK_START.md) - Full setup guide
- [SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md) - Technical design
- [PHASE_2_COMPLETION_FINAL.md](PHASE_2_COMPLETION_FINAL.md) - Pattern matching details
- [BUILD_INFRASTRUCTURE.md](BUILD_INFRASTRUCTURE.md) - Build system reference

## ⚡ Quick Start (5 minutes)

### Generate a Plugin
```bash
.\livespice-translator.exe "example pedals/Boss Super Overdrive SD-1.schx"
```

### Build & Test
```bash
cd "JUCE - Boss Super Overdrive SD-1"
mkdir build && cd build
cmake .. && cmake --build . --config Release
```

## 🎯 Features

- ✅ **Pattern Matching** - 17 circuit topologies auto-detected
- ✅ **DSP Translation** - Real-time audio processing
- ✅ **Plugin Generation** - Auto-generated JUCE code
- ✅ **A/B Testing** - Compare circuits in real-time
- ✅ **Nonlinear Models** - Transistor & diode modeling

## 🔨 Build Translator

```bash
g++ -std=c++17 -O2 Livespice_to_DSP.cpp LiveSpiceParser.cpp CircuitAnalyzer.cpp \
    CircuitVisualizer.cpp CircuitDiagnostics.cpp LiveSpiceConnectionMapper.cpp \
    JuceDSPGenerator.cpp TopologyPatterns.cpp -o livespice-translator.exe -lm
```

## 🧪 Testing

```bash
.\test_patterns.exe                    # Pattern matching tests
.\livespice-translator.exe --help      # Show help
```

## ✨ Latest Update

**Phase 2 (Jan 31, 2026)** - Pattern Matching System ✅
- 17 circuit patterns implemented
- Topology-based DSP strategy assignment
- All tests passing - production ready

→ Details: [PHASE_2_COMPLETION_FINAL.md](PHASE_2_COMPLETION_FINAL.md)

---

**Need help?** → Start with [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)
