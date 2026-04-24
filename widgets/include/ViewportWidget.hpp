#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include "RenderTexture.hpp"
#include <functional>

// ═════════════════════════════════════════════════════════════════════════════
//  ViewportWidget — renders a scene into an FBO and displays it as a quad
//
//  Two modes:
//    Mode 1 — Managed (BuGUI creates/manages the FBO):
//      auto* vp = parent->createChild<ViewportWidget>();
//      vp->setRenderCallback([](int w, int h) {
//          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//          drawMyScene(w, h);
//      });
//
//    Mode 2 — External texture (user manages their own FBO):
//      auto* vp = parent->createChild<ViewportWidget>();
//      vp->setExternalTexture(myEngine.getTextureId(), 800, 600);
//
//  Signals:
//    viewportResized(int w, int h) — emitted when the viewport size changes
// ═════════════════════════════════════════════════════════════════════════════

class ViewportWidget : public Widget
{
public:
    ViewportWidget();
    ~ViewportWidget() override = default;

    // ── Mode 1: Managed FBO with render callback ─────────────────────────

    /// Callback called each frame with the FBO bound.
    /// Signature: void(int viewportWidth, int viewportHeight)
    using RenderCallback = std::function<void(int, int)>;
    void setRenderCallback(RenderCallback cb) { renderCb_ = std::move(cb); }

    /// Enable/disable depth buffer on the managed FBO (default: true).
    void setDepthBuffer(bool enabled) { hasDepth_ = enabled; }

    /// Enable/disable stencil buffer on the managed FBO (default: false).
    void setStencilBuffer(bool enabled) { hasStencil_ = enabled; }

    // ── Mode 2: External texture ─────────────────────────────────────────

    /// Display an externally-managed texture. Pass 0 to clear.
    void setExternalTexture(GLuint texId, int texW, int texH);

    // ── Common ───────────────────────────────────────────────────────────

    /// Background color shown when no callback/texture is set.
    void setBgColor(const Color& c) { bgColor_ = c; }
    const Color& bgColor() const    { return bgColor_; }

    /// Optional label drawn at the top-left corner (e.g., "Perspective", "Top").
    void setLabel(const std::string& s) { label_ = s; }
    const std::string& label() const    { return label_; }

    /// Flip the texture vertically when displaying (needed when FBO Y is inverted).
    void setFlipY(bool flip) { flipY_ = flip; }
    bool flipY() const       { return flipY_; }

    /// Current viewport size (equals widget rect, updated on layout).
    int viewportWidth()  const { return vpW_; }
    int viewportHeight() const { return vpH_; }

    /// Emitted when the viewport resizes.
    Signal<int, int> viewportResized;

    void layout() override;
    void paint(PaintContext& ctx) override;

private:
    RenderTexture   rt_;
    RenderCallback  renderCb_;

    // External texture mode
    GLuint extTexId_ = 0;
    int    extTexW_  = 0;
    int    extTexH_  = 0;

    // Config
    Color  bgColor_    = Color(30, 30, 35, 255);
    std::string label_;
    bool   hasDepth_   = true;
    bool   hasStencil_ = false;
    bool   flipY_      = true;  // FBOs are usually Y-flipped

    // Cached viewport size
    int vpW_ = 0;
    int vpH_ = 0;
};
