cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

project(BuGUI)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

add_subdirectory(vendor/poly2tri)
add_subdirectory(vendor/stb)
#add_subdirectory(vendor/SDL)
add_subdirectory(base)
add_subdirectory(core)
add_subdirectory(widgets)

if(BUGUI_BUILD_APP AND EXISTS "${CMAKE_SOURCE_DIR}/app/CMakeLists.txt")
    add_subdirectory(app)
endif()

if(BUGUI_BUILD_DEMO AND EXISTS "${CMAKE_SOURCE_DIR}/demo/CMakeLists.txt")
    add_subdirectory(demo)
endif()

if(BUILD_TESTING AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

if(BUGUI_BUILD_EDITOR AND EXISTS "${CMAKE_SOURCE_DIR}/editor/CMakeLists.txt")
    add_subdirectory(editor)
endif()

if(BUGUI_BUILD_TUTORIALS)
    add_subdirectory(tutorials)
endif()
