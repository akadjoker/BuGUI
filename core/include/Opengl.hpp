#pragma once

// Detectar plataforma WEB (Emscripten)
#if defined(__EMSCRIPTEN__)
    #include <GLES3/gl3.h>
    #include <GLES3/gl2ext.h>
    #define BUGUI_GLSL_VERSION "#version 300 es\n"
    #define BUGUI_GLSL_PRECISION "precision mediump float;\n"

#elif defined(__ANDROID__)
    #include <GLES3/gl3.h>
    #include <GLES3/gl2ext.h>
    #define BUGUI_GLSL_VERSION "#version 300 es\n"
    #define BUGUI_GLSL_PRECISION "precision mediump float;\n"

// Desktop (Windows, Linux, macOS)
#else
 
    #include <glad/glad.h>
    #define BUGUI_GLSL_VERSION "#version 130\n"
    #define BUGUI_GLSL_PRECISION ""
#endif
