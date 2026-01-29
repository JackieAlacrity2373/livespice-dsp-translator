cmake_minimum_required(VERSION 3.16)
project(Simple_NMOS_Amplifier VERSION 1.0.0 LANGUAGES CXX)

# JUCE library
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/JUCE")

# Add JUCE as subdirectory
add_subdirectory("${JUCE_PATH}" juce)

# Create plugin target
juce_add_plugin(Simple_NMOS_Amplifier
    PRODUCT_NAME "Simple NMOS Amplifier"
    COMPANY_NAME "LiveSpice DSP"
    VERSION 1.0.0
    DESCRIPTION "Auto-generated from LiveSpice circuit schematic"
    PLUGIN_MANUFACTURER_CODE LSDP
    PLUGIN_CODE LSDP
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Add source files
target_sources(Simple_NMOS_Amplifier PRIVATE
    CircuitProcessor.h
    CircuitProcessor.cpp)

# Link JUCE
target_link_libraries(Simple_NMOS_Amplifier PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics)

# Set C++ standard
set_target_properties(Simple_NMOS_Amplifier PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
