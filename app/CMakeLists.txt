project(bugui_test)

add_executable(bugui_test main.cpp)

target_link_libraries(bugui_test PRIVATE bugui_widgets bugui_core)
