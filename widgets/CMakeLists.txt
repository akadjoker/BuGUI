project(bugui_widgets)

set(SOURCES
    src/Widget.cpp
    src/BasicWidgets.cpp
    src/InputWidgets.cpp
    src/TextEdit.cpp
    src/ScrollWidgets.cpp
    src/ViewWidgets.cpp
    src/LayoutWidgets.cpp
    src/MenuWidgets.cpp
    src/TreePropertyColor.cpp
    src/IconAtlas.cpp
    src/DataGrid.cpp
    src/TreeGrid.cpp
    src/FileDialog.cpp
    src/Dialogs.cpp
    src/ViewportWidget.cpp
    src/Gizmo2D.cpp
    src/Gizmo3D.cpp
    src/CurveEditor.cpp
    src/NodeEditor.cpp
    src/Timeline.cpp
    src/ConsoleWidget.cpp
    src/UIApp.cpp
    src/WidgetApp.cpp
    src/WidgetSerializer.cpp
)

add_library(bugui_widgets STATIC ${SOURCES})

target_include_directories(bugui_widgets PUBLIC include ${CMAKE_SOURCE_DIR}/vendor)

target_link_libraries(bugui_widgets PUBLIC bugui_core)
