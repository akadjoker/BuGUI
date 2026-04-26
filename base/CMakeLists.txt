project(bugui_base)

find_package(glm REQUIRED)

add_library(bugui_base STATIC
    ${CMAKE_SOURCE_DIR}/core/src/Color.cpp
    ${CMAKE_SOURCE_DIR}/core/src/FileSystem.cpp
)

target_include_directories(bugui_base PUBLIC
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(bugui_base PUBLIC
    glm::glm
)
