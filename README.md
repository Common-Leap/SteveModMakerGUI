# SteveModMaker
A tool for generating Steve/Alex mods for Super Smash Bros. Ultimate.

# Building 
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

## Building tegra_swizzle

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

2. Create a build directory:
```bash
mkdir -p build
cd build
```

3. Configure the build with CMake:
```bash
cmake ..
```

4. Compile the project:
```bash
make
```

## Running

After successful compilation, the executable will be located at `build/SteveModMaker`

Run the tool with:
```bash
./SteveModMaker <minecraft_username> <costume_number> <"big" or "small" for the arms">
```

For example:
```bash
./SteveModMaker Steve 00 big
```

# Credits
The code that handles NUTEXB and BNTX file IO was based off of the existing Rust libraries [nutexb](https://github.com/jam1garner/nutexb) and [bntx](https://github.com/jam1garner/bntx). Texture swizzling is handled by [tegra_swizzle](https://github.com/ScanMountGoat/tegra_swizzle)'s C FFI API. Renders and UI are generated using OpenCV perspective warping. Skin data is grabbed from the Minecraft/Mojang web APIs using curl and rapidjson.
