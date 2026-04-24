project(bugui_test)

# Glob all Stage*.hpp + main.cpp into one target
file(GLOB APP_SOURCES "*.cpp")
add_executable(bugui_test ${APP_SOURCES})

target_link_libraries(bugui_test PRIVATE bugui_widgets bugui_core)

# PCH — amortises Widgets.hpp parse (saves ~0.5s per TU in Debug+ASan)
target_precompile_headers(bugui_test PRIVATE pch.hpp)
