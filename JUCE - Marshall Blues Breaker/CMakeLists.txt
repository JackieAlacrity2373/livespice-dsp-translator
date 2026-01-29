cmake_minimum_required(VERSION 3.16)
project(Marshall_Blues_Breaker VERSION 1.0.0 LANGUAGES CXX)

# JUCE library
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/JUCE")

# Add JUCE as subdirectory
add_subdirectory("${JUCE_PATH}" juce)

# Create plugin target
juce_add_plugin(Marshall_Blues_Breaker
    PRODUCT_NAME "Marshall Blues Breaker"
    COMPANY_NAME "LiveSpice DSP"
    BUNDLE_ID "com.livespicedsp.marshallbb"
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
target_sources(Marshall_Blues_Breaker PRIVATE
    CircuitProcessor.h
    CircuitProcessor.cpp)

# Link JUCE
target_link_libraries(Marshall_Blues_Breaker PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_audio_utils
    juce::juce_audio_devices)

# Set C++ standard
set_target_properties(Marshall_Blues_Breaker PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
