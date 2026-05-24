project(bugui_glad)

add_library(bugui_glad STATIC
    glad.cpp
)

target_include_directories(bugui_glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..       # vendor/      → #include <glad/glad.h>
    ${CMAKE_CURRENT_SOURCE_DIR}          # vendor/glad/ → #include <Opengl.hpp>
)

target_link_libraries(bugui_glad PUBLIC dl)
