project(bugui_widget_tutorials)

function(bugui_add_widget_tutorial target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE bugui_widgets bugui_core)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/tutorials/widgets"
    )
endfunction()

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_01_basic
    01-basic/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_02_inputs
    02-inputs/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_03_layouts
    03-layouts/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_04_scroll_list
    04-scroll-list/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_05_tree_datagrid
    05-tree-datagrid/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_06_menus_dialogs
    06-menus-dialogs/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_07_tabs_dock
    07-tabs-dock/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_08_node_curve_timeline
    08-node-curve-timeline/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_09_audio_widgets
    09-audio-widgets/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_10_serialization
    10-serialization/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_11_gizmo2d_property_panel
    11-gizmo2d-property-panel/main.cpp
)

bugui_add_widget_tutorial(
    bugui_tutorial_widgets_12_gizmo3d_property_panel
    12-gizmo3d-property-panel/main.cpp
)
