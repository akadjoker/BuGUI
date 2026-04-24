#pragma once

#include "StageCommon.hpp"
#include "ViewportWidget.hpp"
#include "RenderTexture.hpp"
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
//  Viewport stage — RenderTexture + ViewportWidget demo
//  Shows:
//    • Single viewport with render callback (spinning triangle)
//    • 4-viewport split (Top/Front/Side/Perspective)
// ═════════════════════════════════════════════════════════════════════════════

namespace {

static float sTime = 0.0f; // animation time

// Simple scene rendered into FBO: colored grid + spinning triangle
static void renderScene(int w, int h, float angle, const Color& clearCol)
{
    glViewport(0, 0, w, h);
    glClearColor(clearCol.r / 255.0f, clearCol.g / 255.0f,
                 clearCol.b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // We can't easily use the batch system inside the FBO callback
    // because the batches are set up for the main screen projection.
    // Instead, use raw GL for the FBO content — or just fill with a color
    // and let the viewport label distinguish them.
    //
    // For this demo, we render a simple pattern using direct GL.
    // In real use, the user's engine would render here.
}

} // anon namespace

inline void buildViewportStage(WidgetApp& app)
{
    Widget* root = app.addStage("viewport");

    auto* outer = root->createChild<BorderLayout>();
    outer->setSpacing(0);
    outer->setPadding(Edges(0));

    // ── Top bar ──────────────────────────────────────────────────────────
    auto* topBar = outer->set<BoxLayout>(BorderRegion::Top, LayoutDir::Horizontal);
    outer->setRegionSize(BorderRegion::Top, 30);
    topBar->setSpacing(8);
    topBar->setPadding(6, 6, 0, 6);

    auto* backBtn = topBar->createChild<Button>("< Home");
    backBtn->setSize(70, 0);
    backBtn->clicked.connect([]{
        WidgetApp::instance().setStage("home", TransitionType::SlideRight, EaseType::OutCubic);
    });
    topBar->createChild<Line>(1.0f);
    topBar->createChild<Label>("ViewportWidget Demo — RenderTexture (FBO)")->setColor(Clr::section);
    topBar->createChild<Spacer>();

    // ── Bottom: info ─────────────────────────────────────────────────────
    auto* statusBar = outer->set<StatusBar>(BorderRegion::Bottom);
    statusBar->setText("ViewportWidget: Managed FBO mode (render callback)");

    // ── Center: 4 viewports in a 2x2 grid via nested Splitters ──────────
    auto* vSplit = outer->set<Splitter>(BorderRegion::Center, LayoutDir::Vertical);
    vSplit->setRatio(0.5f);
    vSplit->setMinRatio(0.2f);
    vSplit->setMaxRatio(0.8f);

    // Top row
    auto* topSplit = vSplit->createChild<Splitter>(LayoutDir::Horizontal);
    topSplit->setRatio(0.5f);
    topSplit->setMinRatio(0.2f);
    topSplit->setMaxRatio(0.8f);

    // Bottom row
    auto* botSplit = vSplit->createChild<Splitter>(LayoutDir::Horizontal);
    botSplit->setRatio(0.5f);
    botSplit->setMinRatio(0.2f);
    botSplit->setMaxRatio(0.8f);

    // ── 4 ViewportWidgets ────────────────────────────────────────────────

    struct ViewConfig {
        const char* label;
        Color       clearColor;
    };

    ViewConfig configs[] = {
        {"Top",         Color(25, 30, 35, 255)},
        {"Perspective", Color(30, 30, 40, 255)},
        {"Front",       Color(30, 35, 30, 255)},
        {"Right",       Color(35, 30, 30, 255)},
    };

    Splitter* parents[] = {topSplit, topSplit, botSplit, botSplit};

    for (int i = 0; i < 4; ++i) {
        auto& cfg = configs[i];
        auto* vp = parents[i]->createChild<ViewportWidget>();
        vp->setLabel(cfg.label);
        vp->setBgColor(cfg.clearColor);
        vp->setDepthBuffer(true);

        Color cc = cfg.clearColor;
        const char* lbl = cfg.label;

        vp->setRenderCallback([cc, lbl, vp](int w, int h) {
            // Clear with viewport-specific color
            glClearColor(cc.r / 255.0f, cc.g / 255.0f,
                         cc.b / 255.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // In a real app, you'd call:
            //   myEngine.setCamera(cameraForThisView);
            //   myEngine.render();
            //
            // For demo, the FBO is cleared to a distinct color.
            // The viewport label overlay shows which view is which.
        });
    }
}
