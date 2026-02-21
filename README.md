# SteveModMaker

Generate Steve/Alex mods for Super Smash Bros. Ultimate from Minecraft skins.

This project provides two executables:

- `SteveModMaker`: CLI generator
- `SteveModMakerGUI`: desktop GUI that runs the CLI

Windows baseline target: Windows 11 x64.

## What Builds Where

- Linux native build on Linux: `SteveModMaker` + `SteveModMakerGUI`
- Windows native build on Windows: `SteveModMaker.exe` + `SteveModMakerGUI.exe`
- Windows cross-build on Linux: `SteveModMaker.exe` + `SteveModMakerGUI.exe`

## Quick Start (No Build)

Download prebuilt files from Releases:

- https://github.com/Common-Leap/SteveModMakerLinux/releases

### Windows 11 (Prebuilt)

1. Open the Releases page and download the latest Windows archive (`.zip`).
2. Extract it to a normal folder (your downloads folder or something).
3. Run `SteveModMakerGUI.exe`.
4. Keep all `.exe` and `.dll` files together in the same folder.

CLI usage example:

```powershell
.\SteveModMaker.exe Steve 1
```

### Linux (Prebuilt)

1. Open the Releases page and download the latest Linux artifact.
2. If it is an AppImage:

```bash
chmod +x SteveModMaker-*.AppImage
./SteveModMaker-*.AppImage
```

3. If it is a tar/zip archive, extract and run `SteveModMakerGUI`.

CLI usage example:

```bash
./SteveModMaker Steve 1
```

## CLI Usage

```text
SteveModMaker <minecraft_username> <slot_number> [arm_type]
SteveModMaker --skin-file <skin_png_path> <slot_number> [arm_type]
SteveModMaker -f <skin_png_path> <slot_number> [arm_type]
```

- `slot_number`: `1` through `8`
- `arm_type` (optional): `small` or `big`

Examples:

```bash
./SteveModMaker Steve 1
./SteveModMaker --skin-file ./my_skin.png 2
./SteveModMaker --skin-file ./my_skin.png 3 small
```

Output is written under `./patch/...` relative to the current working directory.

## Build From Fresh Install (Linux)

### 1) Install dependencies

Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build pkg-config \
  libopencv-dev libcurl4-openssl-dev rapidjson-dev qt6-base-dev git
```

Fedora:

```bash
sudo dnf install -y \
  gcc-c++ cmake ninja-build pkgconf-pkg-config \
  opencv-devel curl-devel rapidjson-devel qt6-qtbase-devel git
```

Arch:

```bash
sudo pacman -S --needed \
  base-devel cmake ninja pkgconf \
  opencv curl rapidjson qt6-base git
```

### 2) Build

```bash
git clone https://github.com/Common-Leap/SteveModMakerLinux.git
cd SteveModMakerLinux
cmake --preset release
cmake --build --preset release
```

Artifacts:

- `release/SteveModMaker`
- `release/SteveModMakerGUI`

On Linux x86_64, AppImage packaging is enabled by default in this preset.
Disable it if needed:

```bash
cmake --preset release -DSTEVE_MOD_MAKER_AUTO_APPIMAGE=OFF
cmake --build --preset release
```

## Build From Fresh Install (Windows 11, Native)

### 1) Install tools

- Visual Studio 2022 with `Desktop development with C++`
- CMake 3.21+
- Git
- vcpkg

### 2) Prepare vcpkg

```powershell
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg.exe install opencv4:x64-windows curl:x64-windows rapidjson:x64-windows qtbase:x64-windows
```

### 3) Build

```powershell
git clone https://github.com/Common-Leap/SteveModMakerLinux.git
cd SteveModMakerLinux
$env:VCPKG_ROOT="C:\vcpkg"
cmake --preset release-msvc
cmake --build --preset release-msvc
```

Artifacts:

- `release-msvc/Release/SteveModMaker.exe`
- `release-msvc/Release/SteveModMakerGUI.exe`

## Build Linux + Windows Artifacts From Linux (All Artifacts)

This path builds all four binaries in one run (Linux CLI+GUI and Windows CLI+GUI).

### 1) Install cross-toolchain prerequisites

Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build pkg-config git \
  mingw-w64 \
  autoconf autoconf-archive automake libtool
```

Fedora:

```bash
sudo dnf install -y \
  gcc-c++ cmake ninja-build pkgconf-pkg-config git \
  mingw64-gcc mingw64-gcc-c++ \
  autoconf autoconf-archive automake libtool
```

Arch:

```bash
sudo pacman -S --needed \
  mingw-w64-gcc cmake ninja git \
  autoconf autoconf-archive automake libtool
```

### 2) Prepare vcpkg

```bash
git clone https://github.com/microsoft/vcpkg.git "$HOME/vcpkg"
"$HOME/vcpkg/bootstrap-vcpkg.sh"
```

Verify it exists:

```bash
test -x "$HOME/vcpkg/vcpkg" && echo "vcpkg OK"
```

### 3) Build everything

```bash
cd SteveModMakerLinux
./scripts/build-linux-and-windows.sh --vcpkg-root "$HOME/vcpkg"
```

Artifacts are written to `release/`:

- `release/SteveModMaker`
- `release/SteveModMakerGUI`
- `release/SteveModMaker.exe`
- `release/SteveModMakerGUI.exe`

The script also bundles required MinGW runtime DLLs next to the Windows `.exe` files.

Quick artifact check:

```bash
ls -lh release/SteveModMaker release/SteveModMakerGUI release/SteveModMaker.exe release/SteveModMakerGUI.exe release/*.dll
```

## AppImage Packaging (Manual)

Automatic AppImage packaging is enabled for Linux x86_64 in normal preset builds.
You can also run packaging directly:

```bash
./scripts/build-appimage.sh
```

Optional runtime override:

```bash
APPIMAGE_RUNTIME_FILE=/path/to/runtime-x86_64 ./scripts/build-appimage.sh
```

## Optional: External `tegra_swizzle`

The project includes a built-in compatibility implementation and works without external `tegra_swizzle`.

If you want to use external `tegra_swizzle`, build it and point CMake at `TEGRA_SWIZZLE_ROOT`.

## Troubleshooting

### `Qt Widgets was not found` during Windows cross-configure

You are likely using a different vcpkg root than the one where `qtbase` was installed.
Use an explicit root:

```bash
./scripts/build-linux-and-windows.sh --vcpkg-root "$HOME/vcpkg"
```

If you changed vcpkg root/triplet recently, clear stale Windows cache and rerun:

```bash
rm -rf release/.build-windows
./scripts/build-linux-and-windows.sh --vcpkg-root "$HOME/vcpkg"
```

### `vcpkg executable not found at /absolute/path/to/vcpkg/vcpkg`

`/absolute/path/to/vcpkg` is a placeholder. Replace it with your real path.

Correct example:

```bash
export VCPKG_ROOT="$HOME/vcpkg"
./scripts/build-linux-and-windows.sh --vcpkg-root "$VCPKG_ROOT"
```

### `pwsh: Permission denied` in vcpkg

Fix execute permissions, then retry the vcpkg install/build:

```bash
chmod +x "$HOME"/vcpkg/downloads/tools/powershell-core-*/pwsh
```

If your vcpkg directory is on a `noexec` mount, move it to a normal executable location (for example `~/vcpkg`).

### `gperf ... requires autoconf/autoconf-archive/automake/libtool`

Install those host tools from your distro package manager, then rerun the build.

### Windows `.exe` does not start

Ensure the runtime DLLs are next to the executable:

- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

### GUI cannot find CLI

`SteveModMakerGUI` expects the CLI binary beside it (`SteveModMaker` or `SteveModMaker.exe`).

## Credits

- jam1garner: nutexb, bntx, and some code/models
- itsmeft24: original project and most of the code
- ScanMountGoat: tegra_swizzle
