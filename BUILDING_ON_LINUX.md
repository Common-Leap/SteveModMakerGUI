# Building SteveModMaker on Linux

## Prerequisites

Before building, install the required development dependencies:

### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libopencv-dev \
    libcurl4-openssl-dev \
    rapidjson-dev \
    git \
    curl \
    cargo \
    rustc
```

### Fedora/RHEL:
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    opencv-devel \
    curl-devel \
    rapidjson-devel \
    git \
    curl \
    cargo \
    rust
```

### Arch:
```bash
sudo pacman -S \
    base-devel \
    cmake \
    opencv \
    curl \
    rapidjson \
    rust \
    cargo
```

## Building tegra_swizzle (Required Dependency)

The SteveModMaker requires the `tegra_swizzle` library which provides C FFI bindings for texture swizzling. You'll need to build it from source:

```bash
# Clone the tegra_swizzle repository
git clone https://github.com/ScanMountGoat/tegra_swizzle.git
cd tegra_swizzle

# Build the C FFI library
cargo build --release --lib --features=c_ffi

# Install the library to system location
sudo cp target/release/libtegra_swizzle.so /usr/local/lib/
sudo ldconfig
```

Alternatively, if you don't have sudo access:

```bash
# Build and use a local installation
TEGRA_SWIZZLE_HOME=$(pwd)/tegra_swizzle
mkdir -p $TEGRA_SWIZZLE_HOME/lib
git clone https://github.com/ScanMountGoat/tegra_swizzle.git $TEGRA_SWIZZLE_HOME/src
cd $TEGRA_SWIZZLE_HOME/src
cargo build --release --lib --features=c_ffi
cp target/release/libtegra_swizzle.so $TEGRA_SWIZZLE_HOME/lib/

# Then when building SteveModMaker:
export LD_LIBRARY_PATH=$TEGRA_SWIZZLE_HOME/lib:$LD_LIBRARY_PATH
```

## Building SteveModMaker

1. Navigate to the project directory:
```bash
cd SteveModMakerLinux
```

2. Configure a release build from the source directory:
```bash
cmake --preset release
```

3. Compile the project:
```bash
cmake --build --preset release
```

## Running

After successful compilation, the executable will be located at `release/SteveModMaker`

Run the tool with:
```bash
./release/SteveModMaker <minecraft_username> <costume_number> [arm_type]
```

For example:
```bash
./release/SteveModMaker Steve 00
```

Optional arm override:
```bash
./release/SteveModMaker Steve 00 small
./release/SteveModMaker Steve 00 big
```

The tool will download the Minecraft skin for the specified username and generate mod files for Super Smash Bros. Ultimate.

## Cleaning the Build

To clean up the build directory:
```bash
rm -rf release/
```

Or if you want to just rebuild:
```bash
cmake --build --preset release --clean-first
```

## Troubleshooting

### CMake not found
Make sure CMake 3.10 or higher is installed. Check with:
```bash
cmake --version
```

### OpenCV not found
If CMake can't find OpenCV, install the development headers:
```bash
# Ubuntu/Debian
sudo apt-get install libopencv-dev

# Fedora
sudo dnf install opencv-devel

# Arch
sudo pacman -S opencv
```

### CURL not found
Install CURL development libraries:
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

# Fedora
sudo dnf install curl-devel

# Arch
sudo pacman -S curl
```

### tegra_swizzle library not found at runtime
If you get "libtegra_swizzle.so not found" when running:

1. Make sure you built and installed tegra_swizzle (see "Building tegra_swizzle" section)
2. If installed locally, set the library path before running:
```bash
export LD_LIBRARY_PATH=/path/to/tegra_swizzle/lib:$LD_LIBRARY_PATH
./SteveModMaker <username> <costume>
```
3. If installed to /usr/local/lib, rebuild the cache:
```bash
sudo ldconfig
```

### Build errors with C++ standard
This project requires C++17. Make sure your compiler supports it (GCC 7+, Clang 5+).

### Missing libraries at runtime
If you get library not found errors when running, you may need to install runtime libraries:
```bash
# Ubuntu/Debian
sudo apt-get install libopencv-core4.5 libcurl4

# Fedora
sudo dnf install opencv-libs
```

## Project Structure

The converted project now uses:
- **CMake** for cross-platform building instead of Visual Studio projects
- **OpenCV** cross-platform image processing (no DirectXTex)
- **CURL** for downloading Minecraft skins (no Windows dependencies)
- **RapidJSON** for parsing JSON responses
- **tegra_swizzle** C FFI for texture swizzling (replaces Windows-specific DirectXTex)

## Notes

- The project no longer depends on DirectXTex (Windows-only library)
- All dependencies are cross-platform and freely available on Linux
- The NOMINMAX Windows macro has been removed
- Windows COM initialization code (CoInitializeEx) has been removed
- `strcpy_s` has been replaced with standard `strncpy`
- Texture swizzling is handled by the tegra_swizzle C FFI API which works on Linux
- The Resources directory with PNG files must be in the working directory when running the tool
