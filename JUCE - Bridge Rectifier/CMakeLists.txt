cmake_minimum_required(VERSION 3.16)
project(Bridge_Rectifier VERSION 1.0.0 LANGUAGES CXX)

# JUCE library
set(JUCE_PATH "H:/Live Spice DSP translation layer/third_party/JUCE")

# Add JUCE as subdirectory
add_subdirectory("${JUCE_PATH}" juce)

# Create plugin target
juce_add_plugin(Bridge_Rectifier
    PRODUCT_NAME "Bridge Rectifier"
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
target_sources(Bridge_Rectifier PRIVATE
    CircuitProcessor.h
    CircuitProcessor.cpp)

# Link JUCE
target_link_libraries(Bridge_Rectifier PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics)

# Set C++ standard
set_target_properties(Bridge_Rectifier PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
