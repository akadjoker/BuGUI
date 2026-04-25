cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

project(BuGUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Address Sanitizer for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT CMAKE_BUILD_TYPE)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(vendor/poly2tri)
#add_subdirectory(vendor/SDL)
add_subdirectory(core)
add_subdirectory(widgets)
add_subdirectory(app)


option(BUGUI_BUILD_TUTORIALS "Build tutorial executables" OFF)
if(BUGUI_BUILD_TUTORIALS)
    add_subdirectory(tutorials)
endif()
