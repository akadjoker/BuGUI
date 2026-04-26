project(bugui_demo)

add_executable(bugui_demo
    main.cpp
    backends/SdlOpenGLBackend.cpp
)

target_include_directories(bugui_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

target_link_libraries(bugui_demo PRIVATE bugui_widgets bugui_core)
