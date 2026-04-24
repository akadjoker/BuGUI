#pragma once

#include "Opengl.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  RenderTexture — FBO wrapper for render-to-texture
//
//  Usage:
//    RenderTexture rt;
//    rt.create(800, 600);          // color + depth + stencil
//    rt.bind();                     // redirect GL rendering to FBO
//      glViewport(0, 0, 800, 600);
//      glClear(...);
//      drawScene();
//    rt.unbind();                   // restore default framebuffer
//    // Display: use rt.textureId() with TexturedQuad / Quad
//
//  Supports GL 3.3 Core (desktop) and GLES 3.0 (mobile).
// ═════════════════════════════════════════════════════════════════════════════

class RenderTexture
{
public:
    RenderTexture() = default;
    ~RenderTexture() { destroy(); }

    // Non-copyable, movable
    RenderTexture(const RenderTexture&)            = delete;
    RenderTexture& operator=(const RenderTexture&) = delete;
    RenderTexture(RenderTexture&& o) noexcept      { swap(o); }
    RenderTexture& operator=(RenderTexture&& o) noexcept { swap(o); return *this; }

    /// Create (or recreate) the FBO with the given size.
    /// @param hasDepth   attach a depth renderbuffer (default true)
    /// @param hasStencil attach a stencil renderbuffer (default false)
    /// @return true on success
    bool create(int width, int height, bool hasDepth = true, bool hasStencil = false);

    /// Destroy all GL resources.
    void destroy();

    /// Resize — only recreates if size actually changed.
    bool resize(int width, int height);

    /// Bind this FBO for rendering.  Saves the previous FBO binding.
    void bind();

    /// Unbind — restores the previously bound FBO.
    void unbind();

    /// GL texture handle (color attachment 0) — use with Quad / TexturedQuad.
    GLuint textureId() const { return colorTex_; }

    /// FBO handle.
    GLuint fboId() const { return fbo_; }

    int  width()  const { return width_; }
    int  height() const { return height_; }
    bool valid()  const { return fbo_ != 0; }

private:
    void swap(RenderTexture& o) noexcept;

    GLuint fbo_       = 0;
    GLuint colorTex_  = 0;
    GLuint depthRbo_  = 0;   // depth or depth+stencil renderbuffer
    int    width_     = 0;
    int    height_    = 0;
    bool   hasDepth_  = false;
    bool   hasStencil_= false;

    GLint  prevFbo_   = 0;   // saved during bind/unbind
};
