project(bugui_stress)

# Reuse Raylib from demo_raylib if already found/fetched
if(NOT TARGET raylib)
    if(DEFINED BUGUI_RAYLIB_DIR)
        add_library(raylib STATIC IMPORTED)
        set_target_properties(raylib PROPERTIES
            IMPORTED_LOCATION "${BUGUI_RAYLIB_DIR}/lib/libraylib.a")
        target_include_directories(raylib INTERFACE "${BUGUI_RAYLIB_DIR}/include")
    else()
        include(FetchContent)
        message(STATUS "[stress] Fetching Raylib from GitHub...")
        FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG        5.5
            GIT_SHALLOW    TRUE)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(raylib)
    endif()
endif()

add_executable(bugui_stress
    main.cpp
    stress_stage.cpp
    ${CMAKE_SOURCE_DIR}/demos/backends/RaylibBackend.cpp
)

target_include_directories(bugui_stress PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/demos/backends
    ${CMAKE_SOURCE_DIR}/widgets/include
    ${CMAKE_SOURCE_DIR}/vendor/glad
)

target_link_libraries(bugui_stress PRIVATE
    raylib
    bugui_widgets
    bugui_stb
    $<$<PLATFORM_ID:Linux>:GL m pthread dl>
)

target_link_options(bugui_stress PRIVATE
    $<$<PLATFORM_ID:Linux>:-Wl,--allow-multiple-definition>
)

set_target_properties(bugui_stress PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
