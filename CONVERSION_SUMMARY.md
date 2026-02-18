# SteveModMaker - Linux Conversion Complete ✓

Your Windows C++ project has been successfully converted to compile and run on Linux!

## What Was Done

### Removed Windows Dependencies
- ❌ Removed `#define NOMINMAX` (Windows macro)
- ❌ Removed `#include <DirectXTex.h>` (Windows-only texture library)
- ❌ Removed `HRESULT` and `CoInitializeEx()` (Windows COM API)
- ❌ Removed `#pragma once` from `.cpp` files
- ❌ Replaced `strcpy_s()` with standard `strncpy()`

### Added Cross-Platform Support
- ✅ Created `CMakeLists.txt` for CMake-based building
- ✅ Configured for GCC/Clang compilers
- ✅ Set C++17 standard requirement
- ✅ Proper dependency detection for OpenCV, CURL, tegra_swizzle

### Modified Source Files
| File | Changes |
|------|---------|
| Main.cpp | Removed DirectXTex header and COM initialization |
| BNTX.cpp | Removed DirectXTex header |
| ImageUtils.cpp | Removed pragma once from implementation file |
| MinecraftSkinUtil.cpp | Removed pragma once from implementation file |
| Nutexb.cpp | Replaced Windows `strcpy_s()` with standard `strncpy()` |

### Documentation Created
- `BUILDING_ON_LINUX.md` - Complete build and installation guide
- `LINUX_CONVERSION.md` - Detailed conversion documentation
- This file - Quick start guide

## Project Requirements

All dependencies are now cross-platform:
- **CMake** 3.10+ - Build system
- **OpenCV** 4.0+ - Image processing
- **CURL** - HTTP downloads
- **RapidJSON** - JSON parsing
- **tegra_swizzle** - Texture swizzling (requires build from source)
- **GCC 7+** or **Clang 5+** - C++17 compiler
- **Rust & Cargo** - To build tegra_swizzle

## Quick Start

### 1. Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libopencv-dev \
    libcurl4-openssl-dev rapidjson-dev git cargo rustc
```

**Fedora:**
```bash
sudo dnf install -y gcc-c++ cmake opencv-devel curl-devel \
    rapidjson-devel git cargo rust
```

**Arch:**
```bash
sudo pacman -S base-devel cmake opencv curl rapidjson rust cargo
```

### 2. Build and Install tegra_swizzle

```bash
# Clone and build
git clone https://github.com/ScanMountGoat/tegra_swizzle.git
cd tegra_swizzle
cargo build --release --lib --features=c_ffi

# Install to system
sudo cp target/release/libtegra_swizzle.so /usr/local/lib/
sudo ldconfig
```

### 3. Build SteveModMaker

```bash
cd SteveModMakerLinux
mkdir -p build && cd build
cmake ..
make
```

### 4. Run

```bash
./SteveModMaker <minecraft_username> <costume_number>
# Example:
./SteveModMaker Steve 00
```

## Files Changed

### Source Files Modified
- `Main.cpp` - Removed Windows headers and COM initialization
- `BNTX.cpp` - Removed Windows headers
- `ImageUtils.cpp` - Removed pragma once
- `MinecraftSkinUtil.cpp` - Removed pragma once
- `Nutexb.cpp` - Fixed unsafe string function

### New Files Added
- `CMakeLists.txt` - CMake build configuration
- `BUILDING_ON_LINUX.md` - Comprehensive build guide
- `LINUX_CONVERSION.md` - Detailed conversion documentation
- `CONVERSION_SUMMARY.md` - This file

## Directory Structure

```
SteveModMakerLinux/
├── CMakeLists.txt              # Build configuration
├── BUILDING_ON_LINUX.md        # Build instructions
├── LINUX_CONVERSION.md         # Detailed documentation
├── CONVERSION_SUMMARY.md       # This file
├── *.cpp / *.hpp               # Source files (modified)
├── Resources/                  # PNG assets
├── merge_patch/                # Game mod structure
└── Chara_Masks/                # Character mask files
```

## Build Output

After compilation:
- Executable: `build/SteveModMaker`
- Created mod files: `patch/` directory (when run)

## Troubleshooting

### "libtegra_swizzle.so not found"
Build and install tegra_swizzle (see step 2 above), then:
```bash
sudo ldconfig
```

### "OpenCV not found"
Install OpenCV development packages:
```bash
sudo apt-get install libopencv-dev      # Ubuntu/Debian
sudo dnf install opencv-devel           # Fedora
sudo pacman -S opencv                   # Arch
```

### "CURL not found"
Install CURL:
```bash
sudo apt-get install libcurl4-openssl-dev   # Ubuntu/Debian
sudo dnf install curl-devel                 # Fedora
sudo pacman -S curl                         # Arch
```

### CMake version too old
Update CMake:
```bash
sudo apt-get install cmake          # Ubuntu/Debian
sudo dnf install cmake              # Fedora
sudo pacman -S cmake                # Arch
```

## Platform Support

Now Compatible With:
- ✅ Linux (Ubuntu, Debian, Fedora, Arch, etc.)
- ✅ macOS
- ✅ Other Unix-like systems

The original Windows version still works with Visual Studio.

## Summary

Your SteveModMaker project is now:
- ✅ Cross-platform compatible
- ✅ Using CMake for portable building
- ✅ Free from Windows-specific dependencies
- ✅ Ready to deploy on Linux servers
- ✅ Properly documented for Linux users

See `BUILDING_ON_LINUX.md` for detailed build instructions.

