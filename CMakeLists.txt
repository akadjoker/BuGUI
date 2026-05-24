cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

project(BuGUI)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUGUI_BUILD_SHARED "Build bugui_widgets as a shared library (default: static)" OFF)
if(BUGUI_BUILD_SHARED)
    set(BUGUI_LIB_TYPE SHARED)
else()
    set(BUGUI_LIB_TYPE STATIC)
endif()

# Address Sanitizer for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT CMAKE_BUILD_TYPE)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

option(BUGUI_BUILD_APP "Build BuGUI demo app executable" ON)
option(BUGUI_BUILD_DEMO "Build BuGUI backend demo executable" ON)
option(BUGUI_BUILD_EDITOR "Build BuGUI editor executable" ON)
option(BUGUI_BUILD_TUTORIALS "Build tutorial executables" OFF)


add_subdirectory(vendor/stb)
add_subdirectory(vendor/glad)
add_subdirectory(widgets)



if(EXISTS "${CMAKE_SOURCE_DIR}/demos/CMakeLists.txt")
    add_subdirectory(demos)
endif()


if(EXISTS "${CMAKE_SOURCE_DIR}/demo_raylib/CMakeLists.txt")
    add_subdirectory(demo_raylib)
endif()




# ── Install rules ─────────────────────────────────────────────────────────────
# cmake --install <build_dir> [--prefix /usr/local]
include(GNUInstallDirs)

install(TARGETS bugui_widgets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY widgets/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/BuGUI
    FILES_MATCHING PATTERN "*.hpp"
)
