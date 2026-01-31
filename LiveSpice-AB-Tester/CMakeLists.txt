cmake_minimum_required(VERSION 3.16)
project(LiveSpice_AB_Tester VERSION 1.0.0 LANGUAGES CXX)

# JUCE library
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/JUCE")

# Add JUCE as subdirectory
add_subdirectory("${JUCE_PATH}" juce)

# Create standalone application
juce_add_gui_app(LiveSpice_AB_Tester
    PRODUCT_NAME "LiveSpice A/B Tester"
    COMPANY_NAME "LiveSpice DSP"
    BUNDLE_ID "com.livespicedsp.abtester"
    VERSION 1.0.0)

# Add source files
target_sources(LiveSpice_AB_Tester PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    Source/PluginHost.h
    Source/PluginHost.cpp
    Source/ControlPanel.h
    Source/ControlPanel.cpp
    Source/ABSwitch.h
    Source/ABSwitch.cpp)

# Link JUCE modules
target_link_libraries(LiveSpice_AB_Tester PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_dsp)

# Compiler definitions
target_compile_definitions(LiveSpice_AB_Tester PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:LiveSpice_AB_Tester,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:LiveSpice_AB_Tester,JUCE_VERSION>")

# Set C++ standard
set_target_properties(LiveSpice_AB_Tester PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
