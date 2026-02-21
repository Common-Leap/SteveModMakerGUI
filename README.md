# SteveModMaker
A tool for generating Steve/Alex mods for Super Smash Bros. Ultimate. Tested on Arch Linux.

# Building 
## Prerequisites

Before building, install the required development dependencies:

- C++17 compiler (`g++`/`clang++`)
- CMake 3.21+ (required by `CMakePresets.json`)
- Make (`Unix Makefiles` generator)
- OpenCV development package
- libcurl development package
- RapidJSON development package
- Rust + Cargo (to build `tegra_swizzle`)
- Git (to clone `tegra_swizzle`)

### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    make \
    pkg-config \
    libopencv-dev \
    libcurl4-openssl-dev \
    rapidjson-dev \
    git \
    cargo \
    rustc
```

### Fedora/RHEL:
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    make \
    pkgconf-pkg-config \
    opencv-devel \
    curl-devel \
    rapidjson-devel \
    git \
    cargo \
    rust
```

### Arch:
```bash
sudo pacman -S \
    base-devel \
    cmake \
    pkgconf \
    opencv \
    curl \
    rapidjson \
    rust \
    cargo \
    git
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

If you installed `tegra_swizzle` to a local non-system path, point CMake at that library directory:
```bash
cmake --preset release -DCMAKE_LIBRARY_PATH="$TEGRA_SWIZZLE_HOME/lib"
```

3. Confirm CMake found `tegra_swizzle` in the configure output:
```text
-- Found tegra_swizzle: /path/to/libtegra_swizzle.so
```

4. Compile the project:
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
./release/SteveModMaker Steve 1
```

Optional arm override:
```bash
./release/SteveModMaker Steve 1 small
./release/SteveModMaker Steve 1 big
```

If `tegra_swizzle` is installed in a local non-system path, run with:
```bash
LD_LIBRARY_PATH="$TEGRA_SWIZZLE_HOME/lib:$LD_LIBRARY_PATH" ./release/SteveModMaker Steve 1
```

# Credits
jam1garner: nutexb, bntx, and some stolen code/models
itsmeft24: Original project and the majority of the code
ScanMountGoat: tegra_swizzle
OpenAI: GTP-5.3-Codex
Anthropic: Claude Haiku 4.5
