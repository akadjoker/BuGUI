project(bugui_editor)

add_executable(bugui_editor
    main.cpp
)

target_link_libraries(bugui_editor PRIVATE bugui_widgets bugui_core)

set_target_properties(bugui_editor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
