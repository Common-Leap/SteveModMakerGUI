# Linux Conversion Summary

This project has been successfully converted from a Windows-only Visual Studio project to a cross-platform Linux-compatible C++ application.

## Changes Made

### 1. Build System
- Replaced Visual Studio project files (`.vcxproj`, `.sln`) with CMake (`CMakeLists.txt`)
- Added support for GCC/Clang compilation on Linux
- Configured to use system package managers for dependencies

### 2. Code Changes

#### Removed Windows-Specific Code
- **`#define NOMINMAX`** - Windows macro to prevent min/max conflicts (not needed on Linux)
- **`#include <DirectXTex.h>`** - Windows-only texture library (removed from Main.cpp and BNTX.cpp)
- **`HRESULT` and `CoInitializeEx()`** - Windows COM initialization (removed from main())
- **`strcpy_s()`** - Windows-secure function replaced with standard `strncpy()`
- **`#pragma once` in .cpp files** - Removed from implementation files (only in headers)

#### Modified Files
- `Main.cpp` - Removed DirectXTex header and COM initialization
- `BNTX.cpp` - Removed DirectXTex header
- `Nutexb.cpp` - Replaced `strcpy_s()` with `strncpy()` for standard C++ compatibility
- `ImageUtils.cpp` - Removed pragma once from .cpp file
- `MinecraftSkinUtil.cpp` - Removed pragma once from .cpp file

### 3. New Files
- `CMakeLists.txt` - Cross-platform build configuration
- `BUILDING_ON_LINUX.md` - Comprehensive Linux build and installation guide
- `LINUX_CONVERSION.md` - This file

## Dependencies

All dependencies are now cross-platform and available on Linux:

| Dependency | Purpose | Platform |
|-----------|---------|----------|
| CMake 3.10+ | Build system | Cross-platform |
| OpenCV 4.0+ | Image processing, rendering | Linux/Windows/Mac |
| CURL | HTTP client for Minecraft API | Linux/Windows/Mac |
| RapidJSON | JSON parsing | Linux/Windows/Mac |
| tegra_swizzle | Texture swizzling (C FFI) | Cross-platform |
| GCC 7+ or Clang 5+ | C++17 compiler | Linux |

## Building and Running

See [BUILDING_ON_LINUX.md](BUILDING_ON_LINUX.md) for detailed build instructions.

Quick start:
```bash
# Install tegra_swizzle (one-time setup)
git clone https://github.com/ScanMountGoat/tegra_swizzle.git
cd tegra_swizzle
cargo build --release --lib --features=c_ffi
sudo cp target/release/libtegra_swizzle.so /usr/local/lib/
sudo ldconfig

# Build SteveModMaker
cd SteveModMakerLinux
mkdir build && cd build
cmake ..
make

# Run
./SteveModMaker <minecraft_username> <costume_number>
```

## Compatibility

The converted project is now compatible with:
- ✅ Linux (Ubuntu, Fedora, Arch, etc.)
- ✅ macOS (with appropriate dependencies)
- ✅ Other Unix-like systems

Windows users can still use the original Visual Studio project or use WSL 2 with this Linux version.

## Notes

- The conversion maintains full functionality of the original Windows version
- All image processing and texture handling works identically on Linux
- The tool still requires the `Resources/` directory with PNG shadow and lighting files
- Output directory structure (`./patch/`) is created automatically

## Original Project Information

**SteveModMaker** - A tool for generating Steve/Alex mods for Super Smash Bros. Ultimate from Minecraft: Java Edition usernames.

**Credits:**
- NUTEXB and BNTX file I/O based on Rust libraries by jam1garner
- Texture swizzling via tegra_swizzle C FFI by ScanMountGoat
- Image rendering using OpenCV perspective warping
- Minecraft skin data from Mojang web APIs via CURL

