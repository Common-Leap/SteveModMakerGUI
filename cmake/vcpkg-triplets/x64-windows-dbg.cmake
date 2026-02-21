set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# NOTE:
# Keep this triplet full (debug+release) instead of debug-only.
# Some ports (notably abseil with dynamic linkage) currently fail in
# debug-only mode because they expect generated headers in include/.
# Leaving VCPKG_BUILD_TYPE unset avoids that failure path.
