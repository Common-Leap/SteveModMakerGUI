#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${PROJECT_ROOT}/release"
WINDOWS_TRIPLET="x64-mingw-static"
VCPKG_OVERLAY_TRIPLETS="${PROJECT_ROOT}/cmake/vcpkg-triplets"
LINUX_BUILD_DIR=""
WINDOWS_BUILD_DIR=""
CUSTOM_LINUX_BUILD_DIR=0
CUSTOM_WINDOWS_BUILD_DIR=0
RUN_LINUX=1
RUN_WINDOWS=1
INSTALL_WINDOWS_DEPS=1
VCPKG_ROOT_VALUE="${VCPKG_ROOT:-}"
VCPKG_EXE=""

usage() {
    cat <<'EOF'
Usage: ./scripts/build-linux-and-windows.sh [options]

Options:
  --output-dir <dir>          Artifact output directory (default: release)
  --linux-build-dir <dir>     Linux build directory (default: release/.build-linux)
  --windows-build-dir <dir>   Windows build directory (default: release/.build-windows)
  --vcpkg-root <path>         Path to vcpkg root or vcpkg executable (or set VCPKG_ROOT)
  --windows-triplet <name>    vcpkg Windows triplet (default: x64-mingw-static)
  --skip-linux                Skip native Linux build stage
  --skip-windows              Skip Windows cross-build stage
  --skip-windows-deps         Skip vcpkg install for Windows dependencies
  -h, --help                  Show this help

Notes:
  - Windows cross-build requires MinGW-w64 toolchain and vcpkg.
  - Default triplet is a repo overlay that forces release deps and disables RenderDoc.
  - This script builds both CLI + GUI for Linux and Windows.
EOF
}

require_cmd() {
    local cmd="$1"
    if ! command -v "${cmd}" >/dev/null 2>&1; then
        echo "Required command not found: ${cmd}" >&2
        exit 1
    fi
}

copy_runtime_dll_if_found() {
    local dll_name="$1"
    local output_dir="$2"
    local dll_path=""
    local search_dirs=(
        "/usr/x86_64-w64-mingw32/bin"
        "/usr/lib/gcc/x86_64-w64-mingw32"
    )

    for dir in "${search_dirs[@]}"; do
        if [[ -d "${dir}" ]]; then
            dll_path="$(find "${dir}" -type f -name "${dll_name}" 2>/dev/null | head -n1 || true)"
            if [[ -n "${dll_path}" ]]; then
                cp -f "${dll_path}" "${output_dir}/${dll_name}"
                echo "[build] Bundled runtime DLL: ${dll_name}"
                return 0
            fi
        fi
    done

    echo "[build] Warning: Unable to locate runtime DLL ${dll_name}. The Windows executables may not launch without it." >&2
    return 1
}

resolve_vcpkg() {
    local candidate_root=""
    local candidate_exe=""

    if [[ -n "${VCPKG_ROOT_VALUE}" ]]; then
        candidate_root="${VCPKG_ROOT_VALUE}"
        if [[ "${candidate_root}" != /* ]]; then
            candidate_root="${PROJECT_ROOT}/${candidate_root}"
        fi

        if [[ -x "${candidate_root}/vcpkg" ]]; then
            VCPKG_ROOT_VALUE="${candidate_root}"
            VCPKG_EXE="${candidate_root}/vcpkg"
            return 0
        fi

        if [[ -x "${candidate_root}" ]]; then
            VCPKG_EXE="${candidate_root}"
            VCPKG_ROOT_VALUE="$(cd "$(dirname "${candidate_root}")" && pwd)"
            return 0
        fi

        echo "[build] Warning: VCPKG_ROOT points to a non-existent location: ${candidate_root}" >&2
        echo "[build] Warning: Attempting auto-detection of vcpkg instead." >&2
    fi

    candidate_exe="$(command -v vcpkg || true)"
    if [[ -n "${candidate_exe}" ]]; then
        VCPKG_EXE="${candidate_exe}"
        VCPKG_ROOT_VALUE="$(cd "$(dirname "${candidate_exe}")" && pwd)"
        return 0
    fi

    if [[ -x "${HOME}/vcpkg/vcpkg" ]]; then
        VCPKG_EXE="${HOME}/vcpkg/vcpkg"
        VCPKG_ROOT_VALUE="${HOME}/vcpkg"
        return 0
    fi

    if [[ -x "/tmp/vcpkg/vcpkg" ]]; then
        VCPKG_EXE="/tmp/vcpkg/vcpkg"
        VCPKG_ROOT_VALUE="/tmp/vcpkg"
        return 0
    fi

    if [[ -x "${PROJECT_ROOT}/vcpkg/vcpkg" ]]; then
        VCPKG_EXE="${PROJECT_ROOT}/vcpkg/vcpkg"
        VCPKG_ROOT_VALUE="${PROJECT_ROOT}/vcpkg"
        return 0
    fi

    echo "Unable to find vcpkg. Set VCPKG_ROOT to your vcpkg root or executable path." >&2
    echo "Example: export VCPKG_ROOT=\"\$HOME/vcpkg\"" >&2
    return 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output-dir)
            shift
            [[ $# -gt 0 ]] || { echo "Missing value for --output-dir" >&2; exit 1; }
            OUTPUT_DIR="$1"
            ;;
        --linux-build-dir)
            shift
            [[ $# -gt 0 ]] || { echo "Missing value for --linux-build-dir" >&2; exit 1; }
            LINUX_BUILD_DIR="$1"
            CUSTOM_LINUX_BUILD_DIR=1
            ;;
        --windows-build-dir)
            shift
            [[ $# -gt 0 ]] || { echo "Missing value for --windows-build-dir" >&2; exit 1; }
            WINDOWS_BUILD_DIR="$1"
            CUSTOM_WINDOWS_BUILD_DIR=1
            ;;
        --vcpkg-root)
            shift
            [[ $# -gt 0 ]] || { echo "Missing value for --vcpkg-root" >&2; exit 1; }
            VCPKG_ROOT_VALUE="$1"
            ;;
        --windows-triplet)
            shift
            [[ $# -gt 0 ]] || { echo "Missing value for --windows-triplet" >&2; exit 1; }
            WINDOWS_TRIPLET="$1"
            ;;
        --skip-linux)
            RUN_LINUX=0
            ;;
        --skip-windows)
            RUN_WINDOWS=0
            ;;
        --skip-windows-deps)
            INSTALL_WINDOWS_DEPS=0
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage
            exit 1
            ;;
    esac
    shift
done

if (( RUN_LINUX == 0 && RUN_WINDOWS == 0 )); then
    echo "Nothing to do: both Linux and Windows stages are disabled." >&2
    exit 1
fi

require_cmd cmake
require_cmd ninja

if [[ "${OUTPUT_DIR}" != /* ]]; then
    OUTPUT_DIR="${PROJECT_ROOT}/${OUTPUT_DIR}"
fi
if (( CUSTOM_LINUX_BUILD_DIR == 0 )); then
    LINUX_BUILD_DIR="${OUTPUT_DIR}/.build-linux"
fi
if (( CUSTOM_WINDOWS_BUILD_DIR == 0 )); then
    WINDOWS_BUILD_DIR="${OUTPUT_DIR}/.build-windows"
fi
if [[ "${LINUX_BUILD_DIR}" != /* ]]; then
    LINUX_BUILD_DIR="${PROJECT_ROOT}/${LINUX_BUILD_DIR}"
fi
if [[ "${WINDOWS_BUILD_DIR}" != /* ]]; then
    WINDOWS_BUILD_DIR="${PROJECT_ROOT}/${WINDOWS_BUILD_DIR}"
fi
mkdir -p "${OUTPUT_DIR}"

if (( RUN_LINUX )); then
    echo "[build] Configuring Linux build..."
    cmake \
        -S "${PROJECT_ROOT}" \
        -B "${LINUX_BUILD_DIR}" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DSTEVE_MOD_MAKER_BUILD_GUI=ON \
        -DSTEVE_MOD_MAKER_AUTO_APPIMAGE=OFF

    echo "[build] Building Linux CLI + GUI targets..."
    cmake --build "${LINUX_BUILD_DIR}" --parallel --target SteveModMaker SteveModMakerGUI
    cp -f "${LINUX_BUILD_DIR}/SteveModMaker" "${OUTPUT_DIR}/SteveModMaker"
    cp -f "${LINUX_BUILD_DIR}/SteveModMakerGUI" "${OUTPUT_DIR}/SteveModMakerGUI"
fi

if (( RUN_WINDOWS )); then
    require_cmd x86_64-w64-mingw32-g++
    require_cmd x86_64-w64-mingw32-gcc
    require_cmd x86_64-w64-mingw32-windres
    if [[ ! -d "${VCPKG_OVERLAY_TRIPLETS}" ]]; then
        echo "Missing vcpkg overlay triplet directory: ${VCPKG_OVERLAY_TRIPLETS}" >&2
        exit 1
    fi

    if ! resolve_vcpkg; then
        exit 1
    fi
    echo "[build] Using vcpkg: ${VCPKG_EXE}"
    echo "[build] Using vcpkg root: ${VCPKG_ROOT_VALUE}"
    echo "[build] Using Windows triplet: ${WINDOWS_TRIPLET}"
    if [[ -n "${VCPKG_ROOT:-}" && "${VCPKG_ROOT}" != "${VCPKG_ROOT_VALUE}" ]]; then
        echo "[build] Warning: Environment VCPKG_ROOT ('${VCPKG_ROOT}') differs from selected vcpkg root ('${VCPKG_ROOT_VALUE}')." >&2
        echo "[build] Warning: Forcing VCPKG_ROOT='${VCPKG_ROOT_VALUE}' for this run." >&2
    fi

    if [[ -f "${WINDOWS_BUILD_DIR}/CMakeCache.txt" ]]; then
        cached_vcpkg_root="$(sed -n 's/^Z_VCPKG_ROOT_DIR:INTERNAL=//p' "${WINDOWS_BUILD_DIR}/CMakeCache.txt" | head -n1)"
        cached_triplet="$(sed -n 's/^VCPKG_TARGET_TRIPLET:STRING=//p' "${WINDOWS_BUILD_DIR}/CMakeCache.txt" | head -n1)"
        if [[ -n "${cached_vcpkg_root}" && "${cached_vcpkg_root}" != "${VCPKG_ROOT_VALUE}" ]] || [[ -n "${cached_triplet}" && "${cached_triplet}" != "${WINDOWS_TRIPLET}" ]]; then
            echo "[build] Detected stale Windows CMake cache (root/triplet changed). Resetting ${WINDOWS_BUILD_DIR} cache..."
            rm -f "${WINDOWS_BUILD_DIR}/CMakeCache.txt"
            rm -rf "${WINDOWS_BUILD_DIR}/CMakeFiles"
        fi
    fi

    if (( INSTALL_WINDOWS_DEPS )); then
        echo "[build] Installing Windows cross dependencies with vcpkg..."
        VCPKG_ROOT="${VCPKG_ROOT_VALUE}" VCPKG_DISABLE_METRICS=1 "${VCPKG_EXE}" install \
            --overlay-triplets="${VCPKG_OVERLAY_TRIPLETS}" \
            "opencv4[core,highgui,jpeg,png,webp,win32ui]:${WINDOWS_TRIPLET}" \
            "curl:${WINDOWS_TRIPLET}" \
            "qtbase:${WINDOWS_TRIPLET}" \
            "rapidjson:${WINDOWS_TRIPLET}"
    fi

    echo "[build] Configuring Windows cross-build..."
    VCPKG_ROOT="${VCPKG_ROOT_VALUE}" cmake \
        -S "${PROJECT_ROOT}" \
        -B "${WINDOWS_BUILD_DIR}" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT_VALUE}/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_OVERLAY_TRIPLETS="${VCPKG_OVERLAY_TRIPLETS}" \
        -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="${PROJECT_ROOT}/cmake/toolchains/mingw-w64.cmake" \
        -DVCPKG_TARGET_TRIPLET="${WINDOWS_TRIPLET}" \
        -DVCPKG_APPLOCAL_DEPS=OFF \
        -DSTEVE_MOD_MAKER_BUILD_GUI=ON \
        -DSTEVE_MOD_MAKER_AUTO_APPIMAGE=OFF

    echo "[build] Building Windows CLI + GUI targets..."
    cmake --build "${WINDOWS_BUILD_DIR}" --parallel --target SteveModMaker SteveModMakerGUI
    cp -f "${WINDOWS_BUILD_DIR}/SteveModMaker.exe" "${OUTPUT_DIR}/SteveModMaker.exe"
    cp -f "${WINDOWS_BUILD_DIR}/SteveModMakerGUI.exe" "${OUTPUT_DIR}/SteveModMakerGUI.exe"
    copy_runtime_dll_if_found "libgcc_s_seh-1.dll" "${OUTPUT_DIR}" || true
    copy_runtime_dll_if_found "libstdc++-6.dll" "${OUTPUT_DIR}" || true
    copy_runtime_dll_if_found "libwinpthread-1.dll" "${OUTPUT_DIR}" || true
fi

echo "[build] Done."
echo "  Artifacts: ${OUTPUT_DIR}"
if (( RUN_LINUX )); then
    echo "  Linux CLI: ${OUTPUT_DIR}/SteveModMaker"
    if [[ -x "${OUTPUT_DIR}/SteveModMakerGUI" ]]; then
        echo "  Linux GUI: ${OUTPUT_DIR}/SteveModMakerGUI"
    fi
fi
if (( RUN_WINDOWS )); then
    echo "  Windows CLI: ${OUTPUT_DIR}/SteveModMaker.exe"
    echo "  Windows GUI: ${OUTPUT_DIR}/SteveModMakerGUI.exe"
fi
