# LiveSpice A/B Tester - Build & Run Script (PowerShell)
# Usage: 
#   .\build.ps1              - Configure, build, and launch
#   .\build.ps1 -Clean       - Remove build directory and rebuild
#   .\build.ps1 -NoRun       - Configure and build but don't launch
#   .\build.ps1 -Clean -NoRun - Clean rebuild without launching

param(
    [switch]$Clean = $false,
    [switch]$NoRun = $false
)

Write-Host "╔════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║   LiveSpice A/B Testing Suite - Build & Run Script    ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "Stop"
$buildDir = "build"
$cmakePath = "C:\Program Files\CMake\bin\cmake.exe"

# Verify CMake exists
if (-not (Test-Path $cmakePath)) {
    Write-Host "❌ CMake not found at: $cmakePath" -ForegroundColor Red
    Write-Host "Please install CMake or update the path in this script." -ForegroundColor Yellow
    exit 1
}

# Clean if requested
if ($Clean) {
    Write-Host "🧹 Cleaning previous build..." -ForegroundColor Yellow
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
        Write-Host "   Build directory removed." -ForegroundColor Green
    }
}

# Create build directory if it doesn't exist
if (-not (Test-Path $buildDir)) {
    Write-Host "📁 Creating build directory..." -ForegroundColor Yellow
    mkdir $buildDir | Out-Null
}

# Change to build directory
Push-Location $buildDir

try {
    # Configure with CMake
    Write-Host "`n⚙️  Configuring with CMake..." -ForegroundColor Cyan
    & $cmakePath .. -G "Visual Studio 17 2022" -A x64

    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n❌ CMake configuration failed!" -ForegroundColor Red
        exit 1
    }

    # Build Release
    Write-Host "`n🔨 Building Release configuration..." -ForegroundColor Cyan
    & $cmakePath --build . --config Release

    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n❌ Build failed!" -ForegroundColor Red
        exit 1
    }

    # Check for executable
    $exePath = ".\LiveSpice_AB_Tester_artefacts\Release\B Tester.exe"
    if (Test-Path $exePath) {
        $exeSize = [math]::Round((Get-Item $exePath).Length / 1MB, 2)
        Write-Host "`n╔════════════════════════════════════════════════════════╗" -ForegroundColor Green
        Write-Host "║        ✅ Build Successful!                           ║" -ForegroundColor Green
        Write-Host "╚════════════════════════════════════════════════════════╝" -ForegroundColor Green
        Write-Host ""
        Write-Host "📦 Executable Size: $exeSize MB" -ForegroundColor Green
        Write-Host "📍 Location: $exePath" -ForegroundColor Cyan
        Write-Host ""
        
        # Launch if not NoRun
        if (-not $NoRun) {
            Write-Host "🚀 Launching application..." -ForegroundColor Yellow
            Start-Process $exePath
            Write-Host "   Application launched in background" -ForegroundColor Green
        }
    } else {
        Write-Host "`n⚠️  Build completed but executable not found!" -ForegroundColor Yellow
        Write-Host "Check the build output above for errors." -ForegroundColor Yellow
    }

} finally {
    Pop-Location
}

Write-Host ""
Write-Host "✨ Build script complete!" -ForegroundColor Cyan
