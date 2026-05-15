project(bugui_demos)

add_executable(bugui_demos
    main.cpp
    stages/stage_menu.cpp
    stages/stage_basic.cpp
    stages/stage_inputs.cpp
    stages/stage_layouts.cpp
    stages/stage_scroll.cpp
    stages/stage_controls.cpp
    stages/stage_inputs2.cpp
    stages/stage_menus.cpp
    stages/stage_dialogs.cpp
    stages/stage_treepropcolor.cpp
    stages/stage_datagrid.cpp
    stages/stage_floatwindow.cpp
    stages/stage_gizmos.cpp
    stages/stage_anchorlayouts.cpp
    stages/stage_carousel.cpp
    stages/stage_slidepanel.cpp
    stages/stage_charts.cpp
    stages/stage_nodeeditor.cpp
    stages/stage_timeline.cpp
    stages/stage_audio.cpp
    stages/stage_thumbnail.cpp
    stages/stage_filedialog.cpp
    stages/stage_console.cpp
    stages/stage_dock.cpp
    stages/stage_newfeatures.cpp
    stages/stage_datetime.cpp
    stages/stage_cluster.cpp
    stages/stage_animation_model.cpp
    stages/stage_fake_blender.cpp
    stages/stage_crm.cpp
    stages/stage_fake_ide.cpp
    stages/stage_fake_unity.cpp
    stages/stage_toolbar.cpp
    stages/stage_codeeditor.cpp
    stages/stage_appwidgets.cpp
    stages/stage_focus.cpp
    backends/SdlOpenGLBackend.cpp
)

find_package(SDL2 REQUIRED)

target_include_directories(bugui_demos PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
    ${CMAKE_SOURCE_DIR}/widgets/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(bugui_demos PRIVATE bugui_widgets bugui_glad ${SDL2_LIBRARIES})
