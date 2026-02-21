set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
set(VCPKG_ENV_PASSTHROUGH PATH)

# This project ships release artifacts; skip debug dependency builds.
set(VCPKG_BUILD_TYPE release)

# Prevent host Linux RenderDoc headers from leaking into MinGW builds.
set(VCPKG_CMAKE_CONFIGURE_OPTIONS
    -DCMAKE_DISABLE_FIND_PACKAGE_RenderDoc=ON
)
