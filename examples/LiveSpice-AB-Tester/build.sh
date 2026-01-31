#!/bin/bash
# LiveSpice A/B Tester - Build & Run Script (Bash)
# Usage:
#   ./build.sh              - Configure, build, and launch
#   ./build.sh clean        - Remove build directory and rebuild
#   ./build.sh no-run       - Configure and build but don't launch
#   ./build.sh clean no-run - Clean rebuild without launching

set -e

echo "╔════════════════════════════════════════════════════════╗"
echo "║   LiveSpice A/B Testing Suite - Build & Run Script    ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

BUILD_DIR="build"
CLEAN_BUILD=false
NO_RUN=false

# Parse arguments
for arg in "$@"; do
    if [ "$arg" = "clean" ] || [ "$arg" = "-clean" ]; then
        CLEAN_BUILD=true
    elif [ "$arg" = "no-run" ] || [ "$arg" = "-no-run" ]; then
        NO_RUN=true
    fi
done

# Clean if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "🧹 Cleaning previous build..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf $BUILD_DIR
        echo "   Build directory removed."
    fi
fi

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "📁 Creating build directory..."
    mkdir -p $BUILD_DIR
fi

# Change to build directory
cd $BUILD_DIR

# Configure with CMake
echo ""
echo "⚙️  Configuring with CMake..."
if ! cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release; then
    echo ""
    echo "❌ CMake configuration failed!"
    exit 1
fi

# Build
echo ""
echo "🔨 Building Release configuration..."
if ! cmake --build . --config Release --parallel; then
    echo ""
    echo "❌ Build failed!"
    exit 1
fi

# Check for executable
EXE_PATH="./LiveSpice_AB_Tester_artefacts/Release/B Tester"
if [ -f "$EXE_PATH" ]; then
    EXE_SIZE=$(du -h "$EXE_PATH" | cut -f1)
    
    echo ""
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║        ✅ Build Successful!                           ║"
    echo "╚════════════════════════════════════════════════════════╝"
    echo ""
    echo "📦 Executable Size: $EXE_SIZE"
    echo "📍 Location: $EXE_PATH"
    echo ""
    
    if [ "$NO_RUN" = false ]; then
        echo "🚀 Launching application..."
        "$EXE_PATH" &
        echo "   Application launched in background"
    fi
else
    echo ""
    echo "⚠️  Build completed but executable not found!"
    echo "Check the build output above for errors."
    exit 1
fi

cd ..
echo ""
echo "✨ Build script complete!"
