#!/usr/bin/env python3
"""
MXR Distortion+ JUCE Plugin Build Script
Attempts to build using available tools (CMake, MSBuild, or Visual Studio)
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path

def run_command(cmd, description=""):
    """Run a command and return success/failure"""
    if description:
        print(f"\n[*] {description}")
    print(f"    $ {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    try:
        result = subprocess.run(cmd, shell=isinstance(cmd, str), capture_output=False, text=True)
        return result.returncode == 0
    except Exception as e:
        print(f"    [-] Error: {e}")
        return False

def find_cmake():
    """Find CMake executable"""
    return shutil.which("cmake") is not None

def find_msbuild():
    """Find MSBuild executable"""
    return shutil.which("msbuild") is not None

def find_visual_studio():
    """Find Visual Studio installation"""
    vs_paths = [
        r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe",
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe",
        r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\devenv.exe",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe",
    ]
    for path in vs_paths:
        if Path(path).exists():
            return path
    return None

def main():
    print("=" * 60)
    print("  MXR Distortion+ JUCE Plugin Build")
    print("=" * 60)
    
    # Get to the right directory
    project_dir = Path(__file__).parent.absolute()
    os.chdir(project_dir)
    
    print(f"\n[*] Project directory: {project_dir}")
    print(f"[*] Files found:")
    print(f"    - CircuitProcessor.h: {Path('CircuitProcessor.h').exists()}")
    print(f"    - CircuitProcessor.cpp: {Path('CircuitProcessor.cpp').exists()}")
    print(f"    - CMakeLists.txt: {Path('CMakeLists.txt').exists()}")
    
    # Check available tools
    print(f"\n[*] Checking available build tools:")
    has_cmake = find_cmake()
    has_msbuild = find_msbuild()
    has_vs = find_visual_studio()
    
    print(f"    - CMake: {'✓' if has_cmake else '✗'}")
    print(f"    - MSBuild: {'✓' if has_msbuild else '✗'}")
    print(f"    - Visual Studio: {'✓' if has_vs else '✗'}")
    
    # Remove old build directory
    if Path("build").exists():
        print(f"\n[*] Removing old build directory...")
        shutil.rmtree("build")
    
    # Try CMake first
    if has_cmake:
        print(f"\n[+] CMake found - using CMake build system")
        
        Path("build").mkdir(exist_ok=True)
        os.chdir("build")
        
        # Configure
        if not run_command(["cmake", "..", "-G", "Visual Studio 17 2022", "-A", "x64"], 
                          "Configuring CMake (Visual Studio 2022 x64)"):
            print("\n[-] CMake configuration failed with VS2022, trying default generator...")
            if not run_command(["cmake", ".."], "Configuring CMake (default generator)"):
                print("[-] CMake configuration failed")
                return 1
        
        # Build
        if not run_command(["cmake", "--build", ".", "--config", "Release"], 
                          "Building project (Release configuration)"):
            print("[-] Build failed")
            return 1
        
        print("\n[+] Build successful!")
        print(f"\n[*] Plugin locations:")
        
        # Check for output files
        artefacts = Path("MXR_Distortion___artefacts/Release")
        if artefacts.exists():
            standalone = artefacts / "Standalone"
            vst3 = artefacts / "VST3"
            
            if standalone.exists():
                exes = list(standalone.glob("*.exe"))
                if exes:
                    print(f"    Standalone: {exes[0]}")
            
            if vst3.exists():
                dlls = list(vst3.glob("*.dll"))
                if dlls:
                    print(f"    VST3: {dlls[0]}")
        else:
            print(f"    [!] Build artefacts directory not found at: {artefacts}")
            print(f"    [*] Searching for build outputs...")
            for item in Path("build").rglob("MXR*"):
                print(f"        Found: {item}")
        
        return 0
    
    # Try MSBuild (if CMake generated project)
    elif has_msbuild and Path("build").exists():
        print(f"\n[+] MSBuild found - using existing project")
        os.chdir("build")
        
        sln_files = list(Path(".").glob("*.sln"))
        if sln_files:
            return run_command(["msbuild", str(sln_files[0]), "/p:Configuration=Release"], 
                              f"Building {sln_files[0]}")
        else:
            print("[-] No solution files found in build directory")
            return 1
    
    # Try Visual Studio IDE
    elif has_vs:
        print(f"\n[!] Visual Studio found but CMake not available")
        print(f"[*] Please install CMake from https://cmake.org/")
        return 1
    
    else:
        print(f"\n[-] No build tools found!")
        print(f"\n[*] To build this project, you need:")
        print(f"    1. CMake 3.16+ (https://cmake.org/)")
        print(f"    2. Visual Studio 2019+ OR")
        print(f"    3. MSVC compiler in PATH")
        print(f"\n[*] Installation steps:")
        print(f"    1. Download CMake from https://cmake.org/download/")
        print(f"    2. Install and add to PATH")
        print(f"    3. Rerun this script")
        return 1

if __name__ == "__main__":
    sys.exit(main())
