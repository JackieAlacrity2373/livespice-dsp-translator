cmake_minimum_required(VERSION 3.16)
project(Common_Emitter_Transistor_Amplifier VERSION 1.0.0 LANGUAGES CXX)

# JUCE library
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/JUCE")

# Add JUCE as subdirectory
add_subdirectory("${JUCE_PATH}" juce)

# Create plugin target
juce_add_plugin(Common_Emitter_Transistor_Amplifier
    PRODUCT_NAME "Common Emitter Transistor Amplifier"
    COMPANY_NAME "LiveSpice DSP"
    BUNDLE_ID "com.livespicedsp.emitteramp"
    VERSION 1.0.0
    DESCRIPTION "Auto-generated from LiveSpice circuit schematic"
    PLUGIN_MANUFACTURER_CODE LSDP
    PLUGIN_CODE LSDP
    FORMATS Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Add source files
target_sources(Common_Emitter_Transistor_Amplifier PRIVATE
    CircuitProcessor.h
    CircuitProcessor.cpp)

# Link JUCE
target_link_libraries(Common_Emitter_Transistor_Amplifier PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_audio_utils
    juce::juce_audio_devices)

# Set C++ standard
set_target_properties(Common_Emitter_Transistor_Amplifier PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
