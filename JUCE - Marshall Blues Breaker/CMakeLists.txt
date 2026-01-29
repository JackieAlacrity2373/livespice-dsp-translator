cmake_minimum_required(VERSION 3.16)
project(Marshall_Blues_Breaker VERSION 1.0.0 LANGUAGES CXX)

# Disable VST2 to avoid conflicts
add_definitions(-DJUCE_VST3_CAN_REPLACE_VST2=0)

# JUCE library
set(JUCE_PATH "H:/Live Spice DSP translation layer/third_party/JUCE")

# Add JUCE as subdirectory
add_subdirectory("${JUCE_PATH}" juce)

# Create plugin target
juce_add_plugin(Marshall_Blues_Breaker
    PRODUCT_NAME "Marshall Blues Breaker"
    COMPANY_NAME "LiveSpice DSP"
    VERSION 1.0.0
    DESCRIPTION "Auto-generated from LiveSpice circuit schematic"
    PLUGIN_MANUFACTURER_CODE LSDP
    PLUGIN_CODE MBbk
    FORMATS VST3
    VST3_COPY_DIR "H:/Live Spice DSP translation layer/JUCE - Marshall Blues Breaker/_ The VST is in here"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Add source files
target_sources(Marshall_Blues_Breaker PRIVATE
    CircuitProcessor.h
    CircuitProcessor.cpp)

# Link JUCE
target_link_libraries(Marshall_Blues_Breaker PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics)

# Set C++ standard
set_target_properties(Marshall_Blues_Breaker PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
