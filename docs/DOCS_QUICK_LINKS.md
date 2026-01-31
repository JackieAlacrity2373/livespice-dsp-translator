# Documentation Quick Links

**For quick access to all documentation → [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)**

---

## 📍 Find What You Need

### I'm New - Start Here
→ [README.md](README.md) → [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)

### I Want to Setup Quickly (5 min)
→ [QUICK_START.md](QUICK_START.md)

### I Want to Understand the Architecture
→ [SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md)

### I'm Having Build Issues
→ [BUILD_INFRASTRUCTURE.md](BUILD_INFRASTRUCTURE.md)

### I Want to Build a Plugin
→ [JUCE_PLUGIN_BUILD_GUIDE.md](JUCE_PLUGIN_BUILD_GUIDE.md)

### I Need Component Information
→ [COMPONENT_REFERENCE_LOOKUP.md](COMPONENT_REFERENCE_LOOKUP.md)

### I Want a Cheat Sheet
→ [QUICK_REFERENCE.md](QUICK_REFERENCE.md)

### I'm Using the A/B Tester
→ [LiveSpice-AB-Tester/README.md](LiveSpice-AB-Tester/README.md)

### I Need Details on Phase 2 (Pattern Matching)
→ [PHASE_2_COMPLETION_FINAL.md](PHASE_2_COMPLETION_FINAL.md)

### I Want to Know the Project Status
→ [PROJECT_STATUS.md](PROJECT_STATUS.md)

### I'm Looking for Consolidated Info
→ [CONSOLIDATION_SUMMARY.md](CONSOLIDATION_SUMMARY.md)

### I Need to Find an Archived Document
→ [DOCS_ARCHIVE_NOTICE.md](DOCS_ARCHIVE_NOTICE.md)

---

## 🚀 Essential Commands

```bash
# Generate a plugin
.\livespice-translator.exe "example pedals/Boss Super Overdrive SD-1.schx"

# Build plugin
cd "JUCE - Boss Super Overdrive SD-1"
mkdir build && cd build
cmake .. && cmake --build . --config Release

# Test pattern matching
.\test_patterns.exe

# Build translator from source
g++ -std=c++17 -O2 *.cpp -o livespice-translator.exe -lm
```

---

**All documentation accessible from [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)**
