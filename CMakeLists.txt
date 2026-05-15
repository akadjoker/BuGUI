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
#add_subdirectory(vendor/SDL)
#add_subdirectory(base)    # removed: Color/FileSystem from core/ no longer needed
#add_subdirectory(core)    # removed: replaced by bugui_glad + BuGUI_base.hpp
add_subdirectory(widgets)

if(BUGUI_BUILD_APP AND EXISTS "${CMAKE_SOURCE_DIR}/app/CMakeLists.txt")
    add_subdirectory(app)
endif()

if(BUGUI_BUILD_DEMO AND EXISTS "${CMAKE_SOURCE_DIR}/demo/CMakeLists.txt")
    add_subdirectory(demo)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/demos/CMakeLists.txt")
    add_subdirectory(demos)
endif()

if(BUILD_TESTING AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

if(BUGUI_BUILD_EDITOR AND EXISTS "${CMAKE_SOURCE_DIR}/editor/CMakeLists.txt")
    add_subdirectory(editor)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/demo_raylib/CMakeLists.txt")
    add_subdirectory(demo_raylib)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/stress/CMakeLists.txt")
    add_subdirectory(stress)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/atlas/CMakeLists.txt")
    add_subdirectory(atlas)
endif()

if(BUGUI_BUILD_TUTORIALS)
    add_subdirectory(tutorials)
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
