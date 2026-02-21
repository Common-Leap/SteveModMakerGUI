cmake_minimum_required(VERSION 3.10)
project(SteveModMaker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED imgproc highgui core imgcodecs)
find_package(CURL REQUIRED)

# Try to find tegra_swizzle, but don't fail if not found
find_library(TEGRA_SWIZZLE_LIB tegra_swizzle HINTS /usr/local/lib /usr/lib /opt/tegra_swizzle/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${OPENCV_INCLUDE_DIRS})

# Add source files
set(SOURCES
    Base64.cpp
    BNTX.cpp
    EmbeddedAssets.cpp
    ImageUtils.cpp
    Main.cpp
    MinecraftSkinUtil.cpp
    Nutexb.cpp
)

# Add headers
set(HEADERS
    Base64.hpp
    BNTX.hpp
    Constants.hpp
    EmbeddedAssets.hpp
    ImageUtils.hpp
    MinecraftSkinUtil.hpp
    Nutexb.hpp
    tegra_swizzle.hpp
)

# Create executable
add_executable(SteveModMaker ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(SteveModMaker PRIVATE 
    ${OpenCV_LIBRARIES}
    ${CURL_LIBRARIES}
)

# Link against tegra_swizzle if found
if(TEGRA_SWIZZLE_LIB)
    target_link_libraries(SteveModMaker PRIVATE ${TEGRA_SWIZZLE_LIB})
    message(STATUS "Found tegra_swizzle: ${TEGRA_SWIZZLE_LIB}")
else()
    message(WARNING "tegra_swizzle library not found. Make sure it is installed.")
    message(WARNING "See README.md for instructions on building tegra_swizzle.")
endif()
