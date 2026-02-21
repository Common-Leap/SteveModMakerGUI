#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/release"
TOOLS_DIR="${PROJECT_ROOT}/tools/appimage"
ARCH="$(uname -m)"
RUN_CONFIGURE=1
RUN_BUILD=1

usage() {
	echo "Usage: $0 [--build-dir <dir>] [--skip-configure] [--skip-build]"
	echo "Default build dir: release"
	echo "Env: APPIMAGE_RUNTIME_FILE=/path/to/runtime-x86_64 (optional)"
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--build-dir)
			shift
			if [[ $# -eq 0 ]]; then
				echo "Missing value for --build-dir"
				usage
				exit 1
			fi
			if [[ "$1" = /* ]]; then
				BUILD_DIR="$1"
			else
				BUILD_DIR="${PROJECT_ROOT}/$1"
			fi
			;;
		--skip-configure)
			RUN_CONFIGURE=0
			;;
		--skip-build)
			RUN_BUILD=0
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown argument: $1"
			usage
			exit 1
			;;
	esac
	shift
done

if [[ "${ARCH}" != "x86_64" ]]; then
	echo "This script currently supports x86_64 AppImage builds only."
	exit 1
fi

mkdir -p "${BUILD_DIR}"
BUILD_DIR="$(cd "${BUILD_DIR}" && pwd)"
APPDIR="${BUILD_DIR}/AppDir"

download_tool() {
	local url="$1"
	local destination="$2"
	if [[ -x "${destination}" ]]; then
		return
	fi
	echo "Downloading $(basename "${destination}")..."
	curl -fL "${url}" -o "${destination}"
	chmod +x "${destination}"
}

download_file() {
	local url="$1"
	local destination="$2"
	if [[ -s "${destination}" ]]; then
		return
	fi
	echo "Downloading $(basename "${destination}")..."
	curl -fL "${url}" -o "${destination}"
}

pick_qmake() {
	local qt_major="$1"
	local candidate
	local candidates=()

	if [[ "${qt_major}" == "6" ]]; then
		candidates=(qmake6 qmake-qt6 qmake)
	else
		candidates=(qmake-qt5 qmake5 qmake)
	fi

	for candidate in "${candidates[@]}"; do
		if command -v "${candidate}" >/dev/null 2>&1; then
			command -v "${candidate}"
			return 0
		fi
	done

	return 1
}

mkdir -p "${TOOLS_DIR}"

LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT_PLUGIN="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
APPIMAGETOOL="${TOOLS_DIR}/appimagetool-x86_64.AppImage"
APPIMAGE_RUNTIME_DEFAULT="${TOOLS_DIR}/runtime-x86_64"
APPIMAGE_RUNTIME="${APPIMAGE_RUNTIME_FILE:-${APPIMAGE_RUNTIME_DEFAULT}}"

download_tool "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" "${LINUXDEPLOY}"
download_tool "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" "${LINUXDEPLOY_QT_PLUGIN}"
download_tool "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" "${APPIMAGETOOL}"
if [[ -z "${APPIMAGE_RUNTIME_FILE:-}" ]]; then
	if ! download_file "https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64" "${APPIMAGE_RUNTIME}"; then
		echo "Failed to download AppImage runtime."
		echo "Re-run with APPIMAGE_RUNTIME_FILE=/path/to/runtime-x86_64 to use a local runtime file."
		exit 1
	fi
fi

if [[ ! -s "${APPIMAGE_RUNTIME}" ]]; then
	echo "AppImage runtime file not found: ${APPIMAGE_RUNTIME}"
	echo "Set APPIMAGE_RUNTIME_FILE to a valid runtime file or allow runtime download."
	exit 1
fi

ln -sf "$(basename "${LINUXDEPLOY_QT_PLUGIN}")" "${TOOLS_DIR}/linuxdeploy-plugin-qt"
ln -sf "$(basename "${APPIMAGETOOL}")" "${TOOLS_DIR}/appimagetool"

if (( RUN_CONFIGURE )); then
	echo "Configuring project..."
	cmake \
		-S "${PROJECT_ROOT}" \
		-B "${BUILD_DIR}" \
		-DCMAKE_BUILD_TYPE=Release \
		-DSTEVE_MOD_MAKER_BUILD_GUI=ON \
		-DSTEVE_MOD_MAKER_AUTO_APPIMAGE=OFF
fi

if (( RUN_BUILD )); then
	echo "Building project..."
	cmake --build "${BUILD_DIR}" --parallel
fi

if [[ ! -x "${BUILD_DIR}/SteveModMakerGUI" ]]; then
	echo "GUI build artifact not found at ${BUILD_DIR}/SteveModMakerGUI"
	echo "Install Qt5/Qt6 Widgets development packages and retry."
	exit 1
fi

if [[ ! -x "${BUILD_DIR}/SteveModMaker" ]]; then
	echo "CLI build artifact not found at ${BUILD_DIR}/SteveModMaker"
	exit 1
fi

echo "Staging AppDir..."
rm -rf "${APPDIR}"
cmake --install "${BUILD_DIR}" --prefix "${APPDIR}/usr"

mkdir -p "${APPDIR}/usr/share/applications"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/scalable/apps"
cp "${PROJECT_ROOT}/packaging/SteveModMaker.desktop" "${APPDIR}/usr/share/applications/stevemodmaker.desktop"
cp "${PROJECT_ROOT}/packaging/stevemodmaker.svg" "${APPDIR}/usr/share/icons/hicolor/scalable/apps/stevemodmaker.svg"

if git -C "${PROJECT_ROOT}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
	VERSION="$(git -C "${PROJECT_ROOT}" describe --tags --always --dirty 2>/dev/null || git -C "${PROJECT_ROOT}" rev-parse --short HEAD)"
else
	VERSION="dev"
fi
export VERSION

echo "Packaging AppImage..."
export APPIMAGE_EXTRACT_AND_RUN=1
export NO_STRIP=1
export PATH="${TOOLS_DIR}:${PATH}"

# Some distro Qt imageformat plugins depend on optional libraries that may not be
# installed (e.g. kimg_jxr -> libjxrglue). Exclude those from deployment.
if [[ -n "${LINUXDEPLOY_EXCLUDED_LIBRARIES:-}" ]]; then
	export LINUXDEPLOY_EXCLUDED_LIBRARIES="${LINUXDEPLOY_EXCLUDED_LIBRARIES};*kimg_jxr.so*;*libjxrglue.so*"
else
	export LINUXDEPLOY_EXCLUDED_LIBRARIES="*kimg_jxr.so*;*libjxrglue.so*"
fi

if ldd "${BUILD_DIR}/SteveModMakerGUI" | grep -q "libQt6"; then
	if qmake_path="$(pick_qmake 6)"; then
		export QMAKE="${qmake_path}"
		echo "Using Qt6 qmake for packaging: ${QMAKE}"
	fi
elif ldd "${BUILD_DIR}/SteveModMakerGUI" | grep -q "libQt5"; then
	if qmake_path="$(pick_qmake 5)"; then
		export QMAKE="${qmake_path}"
		echo "Using Qt5 qmake for packaging: ${QMAKE}"
	fi
fi

SAFE_VERSION="$(echo "${VERSION}" | tr '/ ' '__')"
APPIMAGE_OUTPUT="${BUILD_DIR}/SteveModMaker-${SAFE_VERSION}-${ARCH}.AppImage"

pushd "${BUILD_DIR}" >/dev/null
"${LINUXDEPLOY}" \
	--appdir "${APPDIR}" \
	--desktop-file "${APPDIR}/usr/share/applications/stevemodmaker.desktop" \
	--icon-file "${APPDIR}/usr/share/icons/hicolor/scalable/apps/stevemodmaker.svg" \
	--executable "${APPDIR}/usr/bin/SteveModMakerGUI" \
	--executable "${APPDIR}/usr/bin/SteveModMaker" \
	--plugin qt

rm -f "${APPIMAGE_OUTPUT}"
"${APPIMAGETOOL}" --runtime-file "${APPIMAGE_RUNTIME}" "${APPDIR}" "${APPIMAGE_OUTPUT}"
popd >/dev/null

echo "AppImage build complete. Output files:"
ls -1 "${BUILD_DIR}"/*.AppImage
