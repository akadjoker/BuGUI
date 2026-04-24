project(bugui_core)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)

add_compile_options(
    -Wall -Wextra
    -Wno-sign-compare
    -Wno-missing-field-initializers
    -Wno-unused-parameter
    -funsigned-char
)

set(SOURCES
    src/Batch.cpp
    src/Color.cpp
    src/Device.cpp
    src/Font.cpp
    src/Input.cpp
    src/msf_gif_impl.cpp
    src/Pixmap.cpp
    src/RenderState.cpp
    src/Shader.cpp
    src/Texture.cpp
    src/RenderTexture.cpp
    src/FileSystem.cpp
    src/glad.cpp
)

add_library(bugui_core STATIC ${SOURCES})

target_include_directories(bugui_core PUBLIC include src)

target_include_directories(bugui_core PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/vendor/stb
    ${SDL2_INCLUDE_DIRS}
)

target_compile_definitions(bugui_core PRIVATE USE_STB_TRUETYPE)

target_link_libraries(bugui_core
    OpenGL::GL
    ${SDL2_LIBRARIES}
    glm::glm
)

target_sources(bugui_core PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/stb/stb_impl.cpp
)

target_precompile_headers(bugui_core PRIVATE src/pch.h)
