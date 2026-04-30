project(bugui_demo)

add_executable(bugui_demo
    main.cpp
    backends/SdlOpenGLBackend.cpp
)

find_package(SDL2 REQUIRED)

target_include_directories(bugui_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(bugui_demo PRIVATE bugui_widgets bugui_glad ${SDL2_LIBRARIES})
