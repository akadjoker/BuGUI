#pragma once

#include "StageCommon.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include "Texture.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Atlas stage — visualize the font atlas texture
// ═════════════════════════════════════════════════════════════════════════════

inline void buildAtlasStage(WidgetApp& app)
{
    Widget* root = app.addStage("atlas");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Top bar ──────────────────────────────────────────────────────────
    auto* topBar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSize(0, 32);
    topBar->setSpacing(4);
    topBar->setPadding(8, 4, 0, 0);

    auto* backBtn = topBar->createChild<Button>("< Demo");
    backBtn->clicked.connect([]{ WidgetApp::instance().setStage("demo", TransitionType::SlideRight); });

    topBar->createChild<Spacer>();
    topBar->createChild<Label>("Font Atlas Viewer");
    topBar->createChild<Spacer>();

    // ── Canvas showing the atlas texture ─────────────────────────────────
    auto* canvas = outer->createChild<Canvas>();
    canvas->setStretch(1);
    canvas->setBgColor(Color(20, 20, 25, 255));

    canvas->setOnPaint([&app](PaintContext& ctx, const Rect& bounds) {
        Texture* atlas = app.font().GetTexture();
        if (!atlas) return;

        float texW = static_cast<float>(atlas->width);
        float texH = static_cast<float>(atlas->height);

        // Scale atlas to fill the canvas (fit to bounds)
        float padW = 16.0f, padH = 16.0f;
        float maxW = bounds.w - padW;
        float maxH = bounds.h - padH;
        float scale = std::min(maxW / texW, maxH / texH);

        float drawW = texW * scale;
        float drawH = texH * scale;
        float dx = bounds.x + (bounds.w - drawW) * 0.5f;
        float dy = bounds.y + (bounds.h - drawH) * 0.5f;

        // Set color to white so the font texture (RG swizzled) is visible
        ctx.fill.SetColor(255, 255, 255, 255);

        // Draw atlas texture (full UV 0-1)
        float clip[4] = {bounds.x, bounds.y, bounds.w, bounds.h};
        ctx.fill.DrawImageEx(atlas, dx, dy, drawW, drawH, 0.0f, 0.0f, 0.0f, clip);

        // Border around the atlas
        ctx.line.SetColor(80, 80, 90, 255);
        ctx.line.Line2D(dx, dy, dx + drawW, dy);
        ctx.line.Line2D(dx + drawW, dy, dx + drawW, dy + drawH);
        ctx.line.Line2D(dx + drawW, dy + drawH, dx, dy + drawH);
        ctx.line.Line2D(dx, dy + drawH, dx, dy);

        // Info text
        char info[128];
        snprintf(info, sizeof(info), "Atlas: %dx%d  |  Glyphs: %d  |  Scale: %.0f%%",
                 atlas->width, atlas->height, app.font().GetGlyphCount(), scale * 100.0f);
        ctx.font.SetFontSize(14.0f);
        ctx.font.SetColor(Color(180, 180, 190, 255));
        ctx.font.SetBatch(&ctx.text);
        ctx.font.Print(info, bounds.x + 12.0f, bounds.y + bounds.h - 24.0f);
    });

    // ── Status bar ───────────────────────────────────────────────────────
    auto* status = outer->createChild<StatusBar>();
    status->setText("Font Atlas — Noto Sans TTF");
}
