#include "ViewportWidget.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  ViewportWidget
// ═════════════════════════════════════════════════════════════════════════════

ViewportWidget::ViewportWidget()
{
    cursor_ = CursorType::Crosshair;
}

void ViewportWidget::setExternalTexture(GLuint texId, int texW, int texH)
{
    extTexId_ = texId;
    extTexW_  = texW;
    extTexH_  = texH;
    // Destroy managed FBO if we switch to external mode
    rt_.destroy();
}

void ViewportWidget::layout()
{
    // Update cached viewport size
    Rect r = absoluteRect();
    int newW = static_cast<int>(r.w);
    int newH = static_cast<int>(r.h);

    if (newW != vpW_ || newH != vpH_) {
        vpW_ = newW;
        vpH_ = newH;
        viewportResized.emit(vpW_, vpH_);
    }

    Widget::layout();
}

void ViewportWidget::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    int x = static_cast<int>(abs.x);
    int y = static_cast<int>(abs.y);
    int w = static_cast<int>(abs.w);
    int h = static_cast<int>(abs.h);

    if (w <= 0 || h <= 0) return;

    // ── Determine which texture to display ───────────────────────────────
    GLuint displayTex = 0;

    if (renderCb_) {
        // Mode 1: Managed FBO - ensure it exists and is the right size
        if (!rt_.valid() || rt_.width() != w || rt_.height() != h) {
            rt_.create(w, h, hasDepth_, hasStencil_);
        }

        if (rt_.valid()) {
            // Flush any pending GUI batches before switching FBOs
            ctx.fill.Render();
            ctx.line.Render();
            ctx.text.Render();

            // Save current viewport
            GLint prevVP[4];
            glGetIntegerv(GL_VIEWPORT, prevVP);

            // Render user scene into FBO
            rt_.bind();
            glViewport(0, 0, w, h);
            renderCb_(w, h);
            rt_.unbind();

            // Restore GUI viewport
            glViewport(prevVP[0], prevVP[1], prevVP[2], prevVP[3]);

            displayTex = rt_.textureId();
        }
    } else if (extTexId_ != 0) {
        // Mode 2: External texture
        displayTex = extTexId_;
    }

    // ── Draw background ──────────────────────────────────────────────────
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    ctx.fill.Rectangle(x, y, w, h, true);

    // ── Draw the viewport texture ────────────────────────────────────────
    if (displayTex != 0) {
        ctx.fill.SetColor(255, 255, 255, 255);
        ctx.fill.SetTexture(displayTex);

        float x1 = abs.x, y1 = abs.y;
        float x2 = abs.x + abs.w, y2 = abs.y + abs.h;

        if (flipY_) {
            // FBO has Y-flipped content - flip UVs vertically
            ctx.fill.DrawQuad(x1, y1, x2, y2,
                              0.0f, 1.0f, 1.0f, 0.0f);
        } else {
            ctx.fill.DrawQuad(x1, y1, x2, y2,
                              0.0f, 0.0f, 1.0f, 1.0f);
        }
    }

    // ── Border ───────────────────────────────────────────────────────────
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
    ctx.line.Rectangle(x, y, w, h, false);

    // ── Label overlay ────────────────────────────────────────────────────
    if (!label_.empty()) {
        ctx.font.SetFontSize(12.0f);
        ctx.font.SetColor(Color(180, 180, 185, 200));
        ctx.font.SetBatch(&ctx.text);
        ctx.font.Print(label_.c_str(), abs.x + 6, abs.y + 4);
    }

    // Paint children (overlays, gizmos, etc.)
    ctx.pushClip(abs);
    for (auto& c : children_)
        c->paint(ctx);
    ctx.popClip();
}
