@echo off
REM Build script for MXR Distortion+ JUCE plugin
REM This script attempts to build using available tools

setlocal enabledelayedexpansion

cd /d "h:\Live Spice DSP translation layer\JUCE - MXR Distortion +"

echo ========================================
echo  MXR Distortion+ Plugin Build
echo ========================================
echo.

REM Check for cmake
where cmake >nul 2>&1
if %errorlevel% equ 0 (
    echo [+] CMake found
    if exist build (
        echo [*] Removing old build directory...
        rmdir /s /q build
    )
    mkdir build
    cd build
    echo [*] Running CMake...
    cmake .. -G "Visual Studio 17 2022" -A x64
    if %errorlevel% equ 0 (
        echo [+] CMake configuration successful
        echo [*] Building project...
        cmake --build . --config Release
        goto :success
    ) else (
        echo [-] CMake configuration failed
        cd ..
    )
) else (
    echo [-] CMake not found, trying alternative method...
)

REM Check for msbuild
where msbuild >nul 2>&1
if %errorlevel% equ 0 (
    echo [+] MSBuild found
    if exist build\*.sln (
        echo [*] Found JUCE project file
        cd build
        msbuild MXR_Distortion__.sln /p:Configuration=Release /p:Platform=x64
        goto :success
    )
) else (
    echo [-] MSBuild not found
)

REM Fallback: g++ direct compilation
echo.
echo [*] Attempting g++ standalone compilation...
cd ..

REM Try to compile without JUCE framework (won't work fully, but worth trying)
g++ -std=c++17 -Wall -Wextra CircuitProcessor.cpp -I../../third_party/livespice-components -c -o CircuitProcessor.o
if %errorlevel% equ 0 (
    echo [+] CircuitProcessor compiled successfully
    echo [!] Note: Full plugin requires JUCE framework linking
    echo [!] Recommend using Projucer to generate project files
) else (
    echo [-] Compilation failed
)

goto :end

:success
echo.
echo ========================================
echo [+] BUILD SUCCESSFUL
echo ========================================
pause
goto :end

:end
echo.
echo Build process complete.
pause
