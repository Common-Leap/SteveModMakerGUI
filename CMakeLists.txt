cmake_minimum_required(VERSION 3.21)
project(SteveModMaker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STEVE_MOD_MAKER_BUILD_GUI "Build the Qt desktop GUI (SteveModMakerGUI)." ON)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(STEVE_MOD_MAKER_AUTO_APPIMAGE_DEFAULT ON)
else()
    set(STEVE_MOD_MAKER_AUTO_APPIMAGE_DEFAULT OFF)
endif()
option(
    STEVE_MOD_MAKER_AUTO_APPIMAGE
    "Automatically package an AppImage during regular builds (Linux x86_64 only)."
    ${STEVE_MOD_MAKER_AUTO_APPIMAGE_DEFAULT}
)

# Required dependencies for CLI generator
find_package(OpenCV REQUIRED COMPONENTS imgproc highgui core imgcodecs)
find_package(CURL REQUIRED)
find_path(RAPIDJSON_INCLUDE_DIR rapidjson/document.h)
if(NOT RAPIDJSON_INCLUDE_DIR)
    message(FATAL_ERROR "RapidJSON headers were not found. Install RapidJSON or set RAPIDJSON_INCLUDE_DIR.")
endif()

option(
    STEVE_MOD_MAKER_PREFER_SYSTEM_TEGRA_SWIZZLE
    "Use an installed tegra_swizzle library when available."
    ON
)
set(TEGRA_SWIZZLE_ROOT "" CACHE PATH "Optional tegra_swizzle install root (expects include/ and/or lib/).")

# Try to find tegra_swizzle, but don't fail if not found.
if(STEVE_MOD_MAKER_PREFER_SYSTEM_TEGRA_SWIZZLE)
    find_library(
        TEGRA_SWIZZLE_LIB
        NAMES tegra_swizzle libtegra_swizzle
        HINTS
            ${TEGRA_SWIZZLE_ROOT}
            ${TEGRA_SWIZZLE_ROOT}/lib
            /usr/local/lib
            /usr/lib
            /opt/tegra_swizzle/lib
    )
endif()

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${RAPIDJSON_INCLUDE_DIR})

set(CLI_SOURCES
    Base64.cpp
    BNTX.cpp
    EmbeddedAssets.cpp
    ImageUtils.cpp
    Main.cpp
    MinecraftSkinUtil.cpp
    Nutexb.cpp
    UiCharaDbTemplate.cpp
)

set(MSG_NAME_TEMPLATE_GENERATED "${CMAKE_BINARY_DIR}/generated/MsgNameTemplate.cpp")
add_custom_command(
    OUTPUT "${MSG_NAME_TEMPLATE_GENERATED}"
    COMMAND "${CMAKE_COMMAND}"
        -DSLOT_ONE_FILE=${CMAKE_SOURCE_DIR}/Resources/msg_name_slot1_template.xmsbt
        -DOTHER_SLOTS_FILE=${CMAKE_SOURCE_DIR}/Resources/msg_name_other_slots_template.xmsbt
        -DOUTPUT_FILE=${MSG_NAME_TEMPLATE_GENERATED}
        -P "${CMAKE_SOURCE_DIR}/cmake/GenerateMsgNameTemplate.cmake"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/Resources/msg_name_slot1_template.xmsbt"
        "${CMAKE_SOURCE_DIR}/Resources/msg_name_other_slots_template.xmsbt"
        "${CMAKE_SOURCE_DIR}/cmake/GenerateMsgNameTemplate.cmake"
    VERBATIM
)
list(APPEND CLI_SOURCES "${MSG_NAME_TEMPLATE_GENERATED}")

if(NOT TEGRA_SWIZZLE_LIB)
    list(APPEND CLI_SOURCES TegraSwizzleCompat.cpp)
endif()

set(CLI_HEADERS
    Base64.hpp
    BNTX.hpp
    Constants.hpp
    EmbeddedAssets.hpp
    ImageUtils.hpp
    MinecraftSkinUtil.hpp
    MsgNameTemplate.hpp
    Nutexb.hpp
    tegra_swizzle.hpp
    UiCharaDbTemplate.hpp
)

add_executable(SteveModMaker ${CLI_SOURCES} ${CLI_HEADERS})

target_link_libraries(SteveModMaker PRIVATE ${OpenCV_LIBRARIES})
if(TARGET CURL::libcurl)
    target_link_libraries(SteveModMaker PRIVATE CURL::libcurl)
else()
    target_link_libraries(SteveModMaker PRIVATE ${CURL_LIBRARIES})
endif()

if(TEGRA_SWIZZLE_LIB)
    target_link_libraries(SteveModMaker PRIVATE ${TEGRA_SWIZZLE_LIB})
    message(STATUS "Found tegra_swizzle: ${TEGRA_SWIZZLE_LIB}")
else()
    message(STATUS "tegra_swizzle library not found. Using built-in compatibility implementation.")
endif()

set(STEVE_MOD_MAKER_GUI_AVAILABLE OFF)
if(STEVE_MOD_MAKER_BUILD_GUI)
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if(Qt6_FOUND)
        set(STEVE_MOD_MAKER_QT_WIDGETS_TARGET Qt6::Widgets)
        set(STEVE_MOD_MAKER_GUI_AVAILABLE ON)
        message(STATUS "Qt6 Widgets detected. Building SteveModMakerGUI.")
    else()
        find_package(Qt5 COMPONENTS Widgets QUIET)
        if(Qt5_FOUND)
            set(STEVE_MOD_MAKER_QT_WIDGETS_TARGET Qt5::Widgets)
            set(STEVE_MOD_MAKER_GUI_AVAILABLE ON)
            message(STATUS "Qt5 Widgets detected. Building SteveModMakerGUI.")
        endif()
    endif()

    if(STEVE_MOD_MAKER_GUI_AVAILABLE)
        add_executable(
            SteveModMakerGUI
            GuiMain.cpp
            GuiWindow.cpp
            GuiWindow.hpp
        )
        target_link_libraries(SteveModMakerGUI PRIVATE ${STEVE_MOD_MAKER_QT_WIDGETS_TARGET})
        target_compile_definitions(SteveModMakerGUI PRIVATE SMM_CLI_BINARY_NAME=\"SteveModMaker\")
    else()
        message(
            FATAL_ERROR
            "STEVE_MOD_MAKER_BUILD_GUI=ON, but Qt Widgets was not found. "
            "Install Qt6/Qt5 Widgets dev packages (or vcpkg qtbase), "
            "or configure with -DSTEVE_MOD_MAKER_BUILD_GUI=OFF for CLI-only builds."
        )
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS SteveModMaker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(TARGET SteveModMakerGUI)
    install(TARGETS SteveModMakerGUI RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(STEVE_MOD_MAKER_AUTO_APPIMAGE)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(WARNING "STEVE_MOD_MAKER_AUTO_APPIMAGE is ON, but AppImage packaging is Linux-only. Disabling.")
    elseif(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
        message(WARNING "STEVE_MOD_MAKER_AUTO_APPIMAGE is ON, but AppImage packaging currently supports x86_64 only. Disabling.")
    elseif(NOT TARGET SteveModMakerGUI)
        message(WARNING "STEVE_MOD_MAKER_AUTO_APPIMAGE is ON, but SteveModMakerGUI target is unavailable. Disabling.")
    else()
        add_custom_target(
            SteveModMakerAppImage ALL
            COMMAND "${CMAKE_SOURCE_DIR}/scripts/build-appimage.sh"
                --build-dir "${CMAKE_BINARY_DIR}"
                --skip-configure
                --skip-build
            DEPENDS SteveModMaker SteveModMakerGUI
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Packaging AppImage bundle"
            USES_TERMINAL
        )
    endif()
endif()
