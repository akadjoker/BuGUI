project(bugui_tests)

 

 
 
add_test(NAME batch_clip_test COMMAND batch_clip_test)

add_executable(batch_clip_gl_test
    batch_clip_gl_test.cpp
)

target_link_libraries(batch_clip_gl_test PRIVATE bugui_core)

target_include_directories(batch_clip_gl_test PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
)

add_test(NAME batch_clip_gl_test COMMAND batch_clip_gl_test)

