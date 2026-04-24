#include "RenderTexture.hpp"
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────

bool RenderTexture::create(int width, int height, bool hasDepth, bool hasStencil)
{
    if (width <= 0 || height <= 0) return false;

    // Clean up any previous resources
    destroy();

    width_      = width;
    height_     = height;
    hasDepth_   = hasDepth;
    hasStencil_ = hasStencil;

    // ── Create FBO ───────────────────────────────────────────────────────
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    // ── Color texture (RGBA8) ────────────────────────────────────────────
    glGenTextures(1, &colorTex_);
    glBindTexture(GL_TEXTURE_2D, colorTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, colorTex_, 0);

    // ── Depth / Stencil renderbuffer ─────────────────────────────────────
    if (hasDepth || hasStencil) {
        glGenRenderbuffers(1, &depthRbo_);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRbo_);

        if (hasDepth && hasStencil) {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                                  width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                      GL_RENDERBUFFER, depthRbo_);
        } else if (hasDepth) {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                                  width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER, depthRbo_);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    // ── Verify completeness ──────────────────────────────────────────────
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "ERROR: [RenderTexture] FBO incomplete (0x%X)\n", status);
        destroy();
        return false;
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

void RenderTexture::destroy()
{
    if (depthRbo_)  { glDeleteRenderbuffers(1, &depthRbo_);  depthRbo_  = 0; }
    if (colorTex_)  { glDeleteTextures(1, &colorTex_);       colorTex_  = 0; }
    if (fbo_)       { glDeleteFramebuffers(1, &fbo_);        fbo_       = 0; }
    width_ = height_ = 0;
}

// ─────────────────────────────────────────────────────────────────────────────

bool RenderTexture::resize(int width, int height)
{
    if (width == width_ && height == height_) return true;
    return create(width, height, hasDepth_, hasStencil_);
}

// ─────────────────────────────────────────────────────────────────────────────

void RenderTexture::bind()
{
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void RenderTexture::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, prevFbo_);
}

// ─────────────────────────────────────────────────────────────────────────────

void RenderTexture::swap(RenderTexture& o) noexcept
{
    std::swap(fbo_,        o.fbo_);
    std::swap(colorTex_,   o.colorTex_);
    std::swap(depthRbo_,   o.depthRbo_);
    std::swap(width_,      o.width_);
    std::swap(height_,     o.height_);
    std::swap(hasDepth_,   o.hasDepth_);
    std::swap(hasStencil_, o.hasStencil_);
    std::swap(prevFbo_,    o.prevFbo_);
}
