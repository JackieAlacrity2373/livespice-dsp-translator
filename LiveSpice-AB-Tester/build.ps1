# LiveSpice A/B Tester - Quick Build Script
# Run this from the LiveSpice-AB-Tester directory

Write-Host "=== LiveSpice A/B Testing Suite - Build Script ===" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "Stop"
$buildDir = "build"

# Clean previous build
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
$exePath = ".\LiveSpice_AB_Tester_artefacts\Release\LiveSpiceABTester.exe"
if (Test-Path $exePath) {
    $exeSize = [math]::Round((Get-Item $exePath).Length / 1MB, 2)
    Write-Host "`n=== Build Successful ===" -ForegroundColor Green
    Write-Host "Executable: $exeSize MB" -ForegroundColor Green
    Write-Host "Location: $exePath" -ForegroundColor Cyan
    
    # Ask to run
    $run = Read-Host "`nRun the application now? (y/n)"
    if ($run -eq 'y') {
        Start-Process $exePath
    }
} else {
    Write-Host "`nBuild completed but executable not found!" -ForegroundColor Yellow
    Write-Host "Expected path: $exePath" -ForegroundColor Yellow
    Write-Host "Check the build output for errors." -ForegroundColor Yellow
}

cd ..
