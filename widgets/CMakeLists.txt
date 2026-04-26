project(bugui_widgets)

option(BUGUI_BUILD_EXTRA_WIDGETS "Build experimental/heavy widgets" OFF)

set(BUGUI_WIDGET_CORE_SOURCES
    src/BuGUI.cpp
)

set(BUGUI_WIDGET_EXTRA_SOURCES
    src/ViewportWidget.cpp
    src/Gizmo2D.cpp
    src/Gizmo3D.cpp
    src/CurveEditor.cpp
    src/NodeEditor.cpp
    src/Timeline.cpp
    src/PlotWidget.cpp
    src/HistogramWidget.cpp
    src/VUMeter.cpp
    src/WaveformView.cpp
    src/SpectrumAnalyzer.cpp
    src/ADSRWidget.cpp
    src/PianoRoll.cpp
    src/AssetBrowser.cpp
    src/DockPanel.cpp
)

set(SOURCES ${BUGUI_WIDGET_CORE_SOURCES})

if(BUGUI_BUILD_EXTRA_WIDGETS)
    list(APPEND SOURCES ${BUGUI_WIDGET_EXTRA_SOURCES})
endif()

add_library(bugui_widgets STATIC ${SOURCES})

target_include_directories(bugui_widgets PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/vendor/stb
)

target_link_libraries(bugui_widgets PUBLIC bugui_base)
target_link_libraries(bugui_widgets PRIVATE bugui_stb)
