project(bugui_stb)

add_library(bugui_stb STATIC
    stb_impl.cpp
)

target_include_directories(bugui_stb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(bugui_stb PRIVATE
    -Wno-unused-function
    -Wno-unused-but-set-variable
    -Wno-array-bounds
)
