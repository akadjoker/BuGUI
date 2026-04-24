#pragma once

#include "StageCommon.hpp"
#include "ViewportWidget.hpp"
#include "Gizmo2D.hpp"
#include "Gizmo3D.hpp"
#include "RenderTexture.hpp"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

// ═════════════════════════════════════════════════════════════════════════════
//  Gizmo stage — Gizmo2D + Gizmo3D demo
//  Left: 2D gizmo on a canvas (move/rotate/scale a rectangle)
//  Right: 3D gizmo in a viewport (translate/rotate/scale a wireframe cube)
// ═════════════════════════════════════════════════════════════════════════════

// ── Wireframe cube overlay (draws projected 3D cube as 2D lines) ─────────

struct WireCubeOverlay : public Widget
{
    Gizmo3D* gizmo_ = nullptr;
    glm::mat4 view_ = glm::mat4(1.0f);
    glm::mat4 proj_ = glm::mat4(1.0f);

    void setCamera(const glm::mat4& v, const glm::mat4& p) { view_ = v; proj_ = p; }

    glm::vec2 project3D(const glm::vec3& world) const
    {
        Rect pAbs = parent_ ? parent_->absoluteRect() : Rect{0,0,800,600};
        glm::vec4 clip = proj_ * view_ * glm::vec4(world, 1.0f);
        if (std::fabs(clip.w) < 1e-6f) return {-9999, -9999};
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        float sx = pAbs.x + (ndc.x * 0.5f + 0.5f) * pAbs.w;
        float sy = pAbs.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * pAbs.h;
        return {sx, sy};
    }

    void drawLine3D(PaintContext& ctx, const glm::vec3& a, const glm::vec3& b,
                    const Color& c, float thickness = 1.5f) const
    {
        glm::vec2 sa = project3D(a), sb = project3D(b);
        // Thick line as ribbon
        float dx = sb.x - sa.x, dy = sb.y - sa.y;
        float len = std::sqrt(dx*dx + dy*dy);
        if (len < 0.5f) return;
        float nx = -dy/len * thickness * 0.5f;
        float ny =  dx/len * thickness * 0.5f;
        ctx.fill.SetColor(c.r, c.g, c.b, c.a);
        ctx.fill.Triangle(sa.x+nx, sa.y+ny, sa.x-nx, sa.y-ny, sb.x-nx, sb.y-ny, true);
        ctx.fill.Triangle(sa.x+nx, sa.y+ny, sb.x-nx, sb.y-ny, sb.x+nx, sb.y+ny, true);
    }

    void paint(PaintContext& ctx) override
    {
        if (!gizmo_ || !visible_) return;

        // ── Ground grid ──────────────────────────────────────────────────
        Color gridC(60, 60, 65, 80);
        for (int i = -5; i <= 5; ++i) {
            float f = static_cast<float>(i);
            drawLine3D(ctx, {f, 0, -5}, {f, 0, 5}, gridC, 1.0f);
            drawLine3D(ctx, {-5, 0, f}, {5, 0, f}, gridC, 1.0f);
        }
        // Axis indicators on grid
        drawLine3D(ctx, {0,0,0}, {5,0,0}, Color(100,50,50,120), 1.5f);
        drawLine3D(ctx, {0,0,0}, {0,0,5}, Color(50,50,100,120), 1.5f);

        // ── Wireframe cube transformed by gizmo ─────────────────────────
        glm::vec3 pos = gizmo_->targetPosition();
        glm::vec3 rot = gizmo_->targetRotation();
        glm::vec3 scl = gizmo_->targetScale();

        // Build model matrix: translate * rotateY * rotateX * rotateZ * scale
        glm::mat4 model(1.0f);
        model = glm::translate(model, pos);
        model = glm::rotate(model, glm::radians(rot.y), {0,1,0});
        model = glm::rotate(model, glm::radians(rot.x), {1,0,0});
        model = glm::rotate(model, glm::radians(rot.z), {0,0,1});
        model = glm::scale(model, scl);

        // Unit cube corners (centered at origin, half-extent 0.5)
        static const glm::vec3 corners[8] = {
            {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
        };
        // 12 edges
        static const int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0}, // back face
            {4,5},{5,6},{6,7},{7,4}, // front face
            {0,4},{1,5},{2,6},{3,7}, // connecting
        };

        // Transform corners
        glm::vec3 xformed[8];
        for (int i = 0; i < 8; ++i) {
            glm::vec4 p = model * glm::vec4(corners[i], 1.0f);
            xformed[i] = glm::vec3(p);
        }

        // Draw solid faces (semi-transparent)
        Color faceColor(100, 160, 220, 40);
        static const int faces[6][4] = {
            {0,1,2,3}, {4,5,6,7}, {0,4,7,3}, {1,5,6,2}, {0,1,5,4}, {3,2,6,7}
        };
        for (auto& f : faces) {
            glm::vec2 s0 = project3D(xformed[f[0]]);
            glm::vec2 s1 = project3D(xformed[f[1]]);
            glm::vec2 s2 = project3D(xformed[f[2]]);
            glm::vec2 s3 = project3D(xformed[f[3]]);
            ctx.fill.SetColor(faceColor.r, faceColor.g, faceColor.b, faceColor.a);
            ctx.fill.Triangle(s0.x, s0.y, s1.x, s1.y, s2.x, s2.y, true);
            ctx.fill.Triangle(s0.x, s0.y, s2.x, s2.y, s3.x, s3.y, true);
        }

        // Draw edges
        Color edgeColor(100, 175, 240, 220);
        for (auto& e : edges)
            drawLine3D(ctx, xformed[e[0]], xformed[e[1]], edgeColor, 1.5f);
    }
};

// ═════════════════════════════════════════════════════════════════════════════

inline void buildGizmoStage(WidgetApp& app)
{
    Widget* root = app.addStage("gizmo");

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
    topBar->createChild<Label>("Gizmo Demo — 2D (left) + 3D (right)")->setColor(Clr::section);
    topBar->createChild<Spacer>();

    // ── Bottom: status ───────────────────────────────────────────────────
    auto* statusBar = outer->set<StatusBar>(BorderRegion::Bottom);
    statusBar->setText("Drag gizmo handles to transform. Switch mode with buttons.");

    // ── Center: splitter with 2D left, 3D right ─────────────────────────
    auto* split = outer->set<Splitter>(BorderRegion::Center, LayoutDir::Horizontal);
    split->setRatio(0.5f);
    split->setMinRatio(0.25f);
    split->setMaxRatio(0.75f);

    // ═══════════════════════════════════════════════════════════════════════
    //  LEFT: Gizmo2D demo
    // ═══════════════════════════════════════════════════════════════════════

    auto* leftBox = split->createChild<BoxLayout>(LayoutDir::Vertical);
    leftBox->setSpacing(0);

    // Mode buttons
    auto* modeBar2D = leftBox->createChild<BoxLayout>(LayoutDir::Horizontal);
    modeBar2D->setSize(0, 28);
    modeBar2D->setSpacing(4);
    modeBar2D->setPadding(8, 8, 4, 8);
    modeBar2D->createChild<Label>("2D Gizmo")->setColor(Clr::accent);
    modeBar2D->createChild<Spacer>();

    // Canvas with gizmo overlay
    auto* canvas2D = leftBox->createChild<Canvas>();
    canvas2D->setStretch(1);
    canvas2D->setBgColor(Color(30, 30, 35, 255));

    // The gizmo is a child of the canvas — draws on top
    auto* gizmo2D = canvas2D->createChild<Gizmo2D>();
    gizmo2D->setPosition(200, 200);
    gizmo2D->setMode(GizmoMode::Translate);

    // Mode buttons (need gizmo pointer)
    auto* btnMove2D = modeBar2D->createChild<Button>("Move");
    btnMove2D->setSize(60, 0);
    btnMove2D->clicked.connect([gizmo2D]{ gizmo2D->setMode(GizmoMode::Translate); });

    auto* btnRot2D = modeBar2D->createChild<Button>("Rotate");
    btnRot2D->setSize(60, 0);
    btnRot2D->clicked.connect([gizmo2D]{ gizmo2D->setMode(GizmoMode::Rotate); });

    auto* btnScl2D = modeBar2D->createChild<Button>("Scale");
    btnScl2D->setSize(60, 0);
    btnScl2D->clicked.connect([gizmo2D]{ gizmo2D->setMode(GizmoMode::Scale); });

    // Draw a rectangle that the gizmo controls
    canvas2D->setOnPaint([gizmo2D](PaintContext& ctx, const Rect& b){
        // Draw grid
        ctx.line.SetColor(40, 40, 45, 255);
        for (float x = b.x; x < b.x + b.w; x += 40)
            ctx.line.Line2D(x, b.y, x, b.y + b.h);
        for (float y = b.y; y < b.y + b.h; y += 40)
            ctx.line.Line2D(b.x, y, b.x + b.w, y);

        // The "object" — a rectangle at the gizmo position
        float ox = b.x + gizmo2D->positionX();
        float oy = b.y + gizmo2D->positionY();
        float hw = 30 * gizmo2D->scaleX();
        float hh = 20 * gizmo2D->scaleY();

        // Rotation
        float rad = gizmo2D->rotation() * 3.14159265f / 180.0f;
        float cs = std::cos(rad), sn = std::sin(rad);

        // Rotated rectangle corners
        auto rot = [&](float lx, float ly, float& rx, float& ry) {
            rx = ox + lx * cs - ly * sn;
            ry = oy + lx * sn + ly * cs;
        };

        float x0, y0, x1, y1, x2, y2, x3, y3;
        rot(-hw, -hh, x0, y0);
        rot( hw, -hh, x1, y1);
        rot( hw,  hh, x2, y2);
        rot(-hw,  hh, x3, y3);

        // Fill (two triangles)
        ctx.fill.SetColor(100, 160, 220, 120);
        ctx.fill.Triangle(x0, y0, x1, y1, x2, y2, true);
        ctx.fill.Triangle(x0, y0, x2, y2, x3, y3, true);

        // Outline
        ctx.line.SetColor(100, 160, 220, 255);
        ctx.line.Line2D(x0, y0, x1, y1);
        ctx.line.Line2D(x1, y1, x2, y2);
        ctx.line.Line2D(x2, y2, x3, y3);
        ctx.line.Line2D(x3, y3, x0, y0);
    });

    // ═══════════════════════════════════════════════════════════════════════
    //  RIGHT: Gizmo3D demo
    // ═══════════════════════════════════════════════════════════════════════

    auto* rightBox = split->createChild<BoxLayout>(LayoutDir::Vertical);
    rightBox->setSpacing(0);

    // Mode buttons
    auto* modeBar3D = rightBox->createChild<BoxLayout>(LayoutDir::Horizontal);
    modeBar3D->setSize(0, 28);
    modeBar3D->setSpacing(4);
    modeBar3D->setPadding(8, 8, 4, 8);
    modeBar3D->createChild<Label>("3D Gizmo")->setColor(Clr::accent);
    modeBar3D->createChild<Spacer>();

    // Viewport with 3D gizmo
    auto* viewport3D = rightBox->createChild<ViewportWidget>();
    viewport3D->setStretch(1);
    viewport3D->setBgColor(Color(25, 28, 32, 255));
    viewport3D->setLabel("3D View");

    // Wireframe cube overlay — drawn BEFORE gizmo so gizmo is on top
    auto* cubeOverlay = viewport3D->createChild<WireCubeOverlay>();

    auto* gizmo3D = viewport3D->createChild<Gizmo3D>();
    gizmo3D->setTarget(glm::vec3(0, 0.5f, 0), glm::vec3(0), glm::vec3(1));
    gizmo3D->setMode(GizmoMode3D::Translate);

    cubeOverlay->gizmo_ = gizmo3D;

    // Mode buttons
    auto* btnMove3D = modeBar3D->createChild<Button>("Move");
    btnMove3D->setSize(60, 0);
    btnMove3D->clicked.connect([gizmo3D]{ gizmo3D->setMode(GizmoMode3D::Translate); });

    auto* btnRot3D = modeBar3D->createChild<Button>("Rotate");
    btnRot3D->setSize(60, 0);
    btnRot3D->clicked.connect([gizmo3D]{ gizmo3D->setMode(GizmoMode3D::Rotate); });

    auto* btnScl3D = modeBar3D->createChild<Button>("Scale");
    btnScl3D->setSize(60, 0);
    btnScl3D->clicked.connect([gizmo3D]{ gizmo3D->setMode(GizmoMode3D::Scale); });

    // Set up a simple perspective camera looking at origin
    auto setupCamera = [gizmo3D, cubeOverlay, viewport3D]() {
        glm::vec3 eye(5, 4, 6);
        glm::vec3 target(0, 0, 0);
        glm::vec3 up(0, 1, 0);

        int w = viewport3D->viewportWidth();
        int h = viewport3D->viewportHeight();
        if (w <= 0) w = 400;
        if (h <= 0) h = 300;

        glm::mat4 view = glm::lookAt(eye, target, up);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                           static_cast<float>(w) / static_cast<float>(h),
                                           0.1f, 100.0f);

        gizmo3D->setViewProjection(view, proj, w, h);
        cubeOverlay->setCamera(view, proj);
    };

    // Set up camera initially and on resize
    setupCamera();
    viewport3D->viewportResized.connect([setupCamera](int, int) {
        setupCamera();
    });

    // FBO render — just clear to dark background
    viewport3D->setRenderCallback([setupCamera](int w, int h) {
        setupCamera();
        glClearColor(0.10f, 0.11f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    });
}
