# Marshall Blues Breaker JUCE Plugin - Rebuild Script
# Run this from the "JUCE - Marshall Blues Breaker" directory to recompile with updated code

Write-Host "=== Marshall Blues Breaker - Rebuild Script ===" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "Stop"
$buildDir = "build"

# Clean build directory
if (Test-Path $buildDir) {
    Write-Host "Cleaning previous build..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

# Create build directory
Write-Host "Creating build directory..." -ForegroundColor Green
mkdir $buildDir | Out-Null
cd $buildDir

# Configure with CMake
Write-Host "`nConfiguring with CMake..." -ForegroundColor Green
& "C:\Program Files\CMake\bin\cmake.exe" .. -G "Visual Studio 17 2022" -A x64

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nCMake configuration failed!" -ForegroundColor Red
    exit 1
}

# Build Release
Write-Host "`nBuilding Release configuration..." -ForegroundColor Green
& "C:\Program Files\CMake\bin\cmake.exe" --build . --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nBuild failed!" -ForegroundColor Red
    exit 1
}

# Check for executable
$exePath = ".\Marshall_Blues_Breaker_artefacts\Release\Standalone\Marshall Blues Breaker.exe"
if (Test-Path $exePath) {
    $exeSize = [math]::Round((Get-Item $exePath).Length / 1MB, 2)
    Write-Host "`n=== Build Successful ===" -ForegroundColor Green
    Write-Host "Standalone Executable: $exeSize MB" -ForegroundColor Green
    Write-Host "Location: $exePath" -ForegroundColor Cyan
    
    Write-Host "`nVST3 Plugin built to: .\Marshall_Blues_Breaker_artefacts\Release\VST3\Marshall Blues Breaker.vst3" -ForegroundColor Cyan
    
    # Ask to run
    $run = Read-Host "`nRun the standalone app now? (y/n)"
    if ($run -eq 'y') {
        Start-Process $exePath
    }
} else {
    Write-Host "`nBuild completed but executable not found!" -ForegroundColor Yellow
    Write-Host "Expected path: $exePath" -ForegroundColor Yellow
    Write-Host "Check the build output for errors." -ForegroundColor Yellow
}

cd ..
