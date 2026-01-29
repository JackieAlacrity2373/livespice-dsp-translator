cmake_minimum_required(VERSION 3.15)
project(LiveSpice_DSP_Translator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable
add_executable(livespice-translator
    Livespice_to_DSP.cpp
    LiveSpiceParser.cpp
    CircuitAnalyzer.cpp
    CircuitVisualizer.cpp
    CircuitDiagnostics.cpp
    LiveSpiceConnectionMapper.cpp
    JuceDSPGenerator.cpp
)

# Compiler flags
if(MSVC)
    target_compile_options(livespice-translator PRIVATE /W4)
else()
    target_compile_options(livespice-translator PRIVATE -Wall -Wextra -Wpedantic)
    if(NOT APPLE)
        target_link_libraries(livespice-translator m)
    endif()
endif()

# Set output directory
set_target_properties(livespice-translator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Platform-specific settings
if(WIN32)
    set_target_properties(livespice-translator PROPERTIES SUFFIX ".exe")
endif()
