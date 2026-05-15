project(bugui_demo_raylib)

# ── Find or fetch Raylib ──────────────────────────────────────────────────────
# Option A (preferred): point cmake to a pre-built raylib.a via:
#   cmake -DBUGUI_RAYLIB_DIR=/path/to/raylib/install ..
# Option B: FetchContent — downloads and builds Raylib automatically.

if(DEFINED BUGUI_RAYLIB_DIR)
    # Pre-built raylib.a supplied by the user.
    add_library(raylib STATIC IMPORTED)
    set_target_properties(raylib PROPERTIES
        IMPORTED_LOCATION "${BUGUI_RAYLIB_DIR}/lib/libraylib.a"
    )
    target_include_directories(raylib INTERFACE "${BUGUI_RAYLIB_DIR}/include")
else()
    include(FetchContent)
    message(STATUS "[demo_raylib] Fetching Raylib from GitHub...")
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5
        GIT_SHALLOW    TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

# ── Demo executable ───────────────────────────────────────────────────────────
add_executable(bugui_demo_raylib
    main.cpp
    stage_demo.cpp
    stage_menu.cpp
    stage_basic.cpp
    stage_controls.cpp
    stage_scroll.cpp
    stage_inputs.cpp
    stage_menus.cpp
    stage_dialogs.cpp
    stage_dock.cpp
    stage_properties.cpp
    ${CMAKE_SOURCE_DIR}/demos/backends/RaylibBackend.cpp
)

target_include_directories(bugui_demo_raylib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/demos/backends
    ${CMAKE_SOURCE_DIR}/demos/stages
    ${CMAKE_SOURCE_DIR}/widgets/include
    # GLAD headers — used for type declarations only;
    # function pointer definitions come from raylib.a (loaded by InitWindow).
    ${CMAKE_SOURCE_DIR}/vendor/glad
)

# Link raylib (provides OpenGL + window + GLAD symbols).
# Do NOT link bugui_glad — it would duplicate glad_glFoo symbols already
# provided by raylib.a, causing multiple-definition linker errors.
target_link_libraries(bugui_demo_raylib PRIVATE
    raylib
    bugui_widgets
    bugui_stb

    # Platform libs required by Raylib on Linux
    $<$<PLATFORM_ID:Linux>:GL m pthread dl>
)

# Both bugui_stb and raylib bundle stb_image with identical sources.
# Allow duplicate weak symbols so the linker picks either (both are the same code).
target_link_options(bugui_demo_raylib PRIVATE
    $<$<PLATFORM_ID:Linux>:-Wl,--allow-multiple-definition>
)
