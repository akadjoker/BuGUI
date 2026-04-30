#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "GizmoWidgets.hpp"
#include <cstdio>
#include <cmath>
#include <cstring>

// ─────────────────────────────────────────────────────────────────────────────
//  Stage 12 — Gizmo2D + Gizmo3D
// ─────────────────────────────────────────────────────────────────────────────

// ── Camera helpers (column-major, matches BuGUI Mat4f) ───────────────────────

static void makeLookAt(float* m, float ex, float ey, float ez,
                                 float cx, float cy, float cz)
{
    float fx = cx-ex, fy = cy-ey, fz = cz-ez;
    float fl = std::sqrt(fx*fx + fy*fy + fz*fz);
    fx/=fl; fy/=fl; fz/=fl;

    float rx = fy*1-fz*0, ry = fz*0-fx*1, rz = fx*0-fy*0;  // cross(fwd, up=Y)
    // redo properly
    float ux=0, uy=1, uz=0;
    rx = fy*uz - fz*uy; ry = fz*ux - fx*uz; rz = fx*uy - fy*ux;
    float rl = std::sqrt(rx*rx + ry*ry + rz*rz);
    rx/=rl; ry/=rl; rz/=rl;
    ux = ry*fz - rz*fy; uy = rz*fx - rx*fz; uz = rx*fy - ry*fx;

    m[ 0]=rx; m[ 1]=ux; m[ 2]=-fx; m[ 3]=0;
    m[ 4]=ry; m[ 5]=uy; m[ 6]=-fy; m[ 7]=0;
    m[ 8]=rz; m[ 9]=uz; m[10]=-fz; m[11]=0;
    m[12]=-(rx*ex+ry*ey+rz*ez);
    m[13]=-(ux*ex+uy*ey+uz*ez);
    m[14]= (fx*ex+fy*ey+fz*ez);
    m[15]=1;
}

static void makePerspective(float* m, float fovDeg, float aspect, float zn, float zf)
{
    float f = 1.0f / std::tan(fovDeg * 3.14159265f / 360.0f);
    float q = zf / (zn - zf);
    for (int i = 0; i < 16; ++i) m[i] = 0;
    m[ 0] = f / aspect;
    m[ 5] = f;
    m[10] = q;
    m[11] = -1.0f;
    m[14] = zn * q;
}

// ── Wireframe cube overlay ────────────────────────────────────────────────────
// Drawn as a sibling before the Gizmo3D so the gizmo renders on top.

class WireCubeOverlay : public Widget
{
public:
    Gizmo3D* gizmo = nullptr;
    Mat4f    view;
    Mat4f    proj;
    int      vpW = 640, vpH = 400;

    void setCamera(const float* v, const float* p, int w, int h)
    {
        std::memcpy(view.data, v, 64);
        std::memcpy(proj.data, p, 64);
        vpW = w; vpH = h;
        markDirty();
    }

    void layout() override
    {
        if (parent_)
        {
            rect_.x = 0; rect_.y = 0;
            rect_.w = parent_->rect().w;
            rect_.h = parent_->rect().h;
        }
    }

    Vec2f sizeHint() const override { return {0, 0}; }

private:
    // Project a world-space point to screen pixels using our view/proj
    ::Vec2f project(const Vec3f& world) const
    {
        Rect pa = parent_ ? parent_->absoluteRect() : Rect{0,0,(float)vpW,(float)vpH};
        Vec4f clip = proj * (view * Vec4f(world, 1.0f));
        if (std::fabs(clip.w) < 1e-6f) return {-9999.f, -9999.f};
        float iw = 1.0f / clip.w;
        float nx = clip.x * iw, ny = clip.y * iw;
        return { pa.x + (nx * 0.5f + 0.5f) * pa.w,
                 pa.y + (1.0f - (ny * 0.5f + 0.5f)) * pa.h };
    }

    void drawLine3D(PaintContext& ctx, const Vec3f& a, const Vec3f& b,
                    const Color& c, float thick = 1.5f) const
    {
        ::Vec2f sa = project(a), sb = project(b);
        float dx = sb.x - sa.x, dy = sb.y - sa.y;
        float len = std::sqrt(dx*dx + dy*dy);
        if (len < 0.5f) return;
        float nx = -dy/len * thick * 0.5f, ny = dx/len * thick * 0.5f;
        ctx.fill.SetColor(c.r, c.g, c.b, c.a);
        ctx.fillTriangle(sa.x+nx, sa.y+ny, sa.x-nx, sa.y-ny, sb.x-nx, sb.y-ny);
        ctx.fillTriangle(sa.x+nx, sa.y+ny, sb.x-nx, sb.y-ny, sb.x+nx, sb.y+ny);
    }

    void paint(PaintContext& ctx) override
    {
        if (!visible_) return;

        // ── Ground grid ──────────────────────────────────────────────────
        Color gridC(55, 55, 60, 70);
        for (int i = -5; i <= 5; ++i)
        {
            float f = static_cast<float>(i);
            drawLine3D(ctx, {f, 0, -5}, {f, 0, 5}, gridC, 1.0f);
            drawLine3D(ctx, {-5, 0, f}, {5, 0, f}, gridC, 1.0f);
        }
        drawLine3D(ctx, {0,0,0}, {3,0,0}, Color(120, 50, 50, 140), 1.5f);
        drawLine3D(ctx, {0,0,0}, {0,3,0}, Color(50, 120, 50, 140), 1.5f);
        drawLine3D(ctx, {0,0,0}, {0,0,3}, Color(50, 50, 120, 140), 1.5f);

        if (!gizmo) return;

        // ── Wireframe cube transformed by gizmo target ───────────────────
        const Vec3f& pos = gizmo->targetPosition();
        const Vec3f& rot = gizmo->targetRotation();
        const Vec3f& scl = gizmo->targetScale();

        // Build model matrix via our Mat4f (TRS)
        auto makeRotY = [](float deg) -> Mat4f {
            float r = deg * 3.14159265f / 180.0f;
            float c = std::cos(r), s = std::sin(r);
            Mat4f m; m.setIdentity();
            m(0,0)= c; m(0,2)= s;
            m(2,0)=-s; m(2,2)= c;
            return m;
        };
        auto makeRotX = [](float deg) -> Mat4f {
            float r = deg * 3.14159265f / 180.0f;
            float c = std::cos(r), s = std::sin(r);
            Mat4f m; m.setIdentity();
            m(1,1)= c; m(1,2)=-s;
            m(2,1)= s; m(2,2)= c;
            return m;
        };
        auto makeRotZ = [](float deg) -> Mat4f {
            float r = deg * 3.14159265f / 180.0f;
            float c = std::cos(r), s = std::sin(r);
            Mat4f m; m.setIdentity();
            m(0,0)= c; m(0,1)=-s;
            m(1,0)= s; m(1,1)= c;
            return m;
        };

        Mat4f T; T.setIdentity();
        T(0,3)=pos.x; T(1,3)=pos.y; T(2,3)=pos.z;

        Mat4f S; S.setIdentity();
        S(0,0)=scl.x; S(1,1)=scl.y; S(2,2)=scl.z;

        Mat4f model = T * makeRotY(rot.y) * makeRotX(rot.x) * makeRotZ(rot.z) * S;

        static const Vec3f corners[8] = {
            {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f},
            { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
            {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f},
            { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        };
        static const int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0},
            {4,5},{5,6},{6,7},{7,4},
            {0,4},{1,5},{2,6},{3,7},
        };

        Vec3f xc[8];
        for (int i = 0; i < 8; ++i)
        {
            Vec4f p = model * Vec4f(corners[i], 1.0f);
            xc[i] = {p.x, p.y, p.z};
        }

        // Faces (semi-transparent fill)
        static const int faces[6][4] = {
            {0,1,2,3},{4,5,6,7},{0,4,7,3},{1,5,6,2},{0,1,5,4},{3,2,6,7}
        };
        Color faceC(90, 150, 210, 35);
        for (auto& f : faces)
        {
            ::Vec2f s0=project(xc[f[0]]),s1=project(xc[f[1]]),
                    s2=project(xc[f[2]]),s3=project(xc[f[3]]);
            ctx.fill.SetColor(faceC.r, faceC.g, faceC.b, faceC.a);
            ctx.fillTriangle(s0.x,s0.y, s1.x,s1.y, s2.x,s2.y);
            ctx.fillTriangle(s0.x,s0.y, s2.x,s2.y, s3.x,s3.y);
        }

        // Edges
        Color edgeC(100, 180, 255, 220);
        for (auto& e : edges)
            drawLine3D(ctx, xc[e[0]], xc[e[1]], edgeC, 1.5f);
    }
};

// ─────────────────────────────────────────────────────────────────────────────

static Gizmo3D*       s_gizmo3d      = nullptr;
static WireCubeOverlay* s_cubeOverlay = nullptr;
static StatusBar*     s_statusBar    = nullptr;

static void updateCamera(int w, int h)
{
    if (!s_gizmo3d) return;
    float view[16], proj[16];
    makeLookAt(view, 5.f, 4.f, 6.f, 0,0,0);
    makePerspective(proj, 45.f, (float)w / std::max(h, 1), 0.1f, 100.f);
    s_gizmo3d->setViewProjection(view, proj, w, h);
    if (s_cubeOverlay) s_cubeOverlay->setCamera(view, proj, w, h);
}

// ─────────────────────────────────────────────────────────────────────────────

void registerGizmosStage(WidgetApp& app)
{
    s_gizmo3d     = nullptr;
    s_cubeOverlay = nullptr;
    s_statusBar   = nullptr;

    auto* root   = app.addStage("gizmos");
    auto* layout = root->createChild<BorderLayout>();
    layout->setSpacing(0.0f);
    layout->setPadding(0.0f);

    // ── Top bar ────────────────────────────────────────────────────────────
    auto* topBar = layout->set<BoxLayout>(BorderRegion::Top, LayoutDir::Horizontal);
    layout->setRegionSize(BorderRegion::Top, 32.0f);
    topBar->setSpacing(8.0f);
    topBar->setPadding(Edges(6.0f, 10.0f));

    auto* back = topBar->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]()
    {
        app.setStage("menu", TransitionType::CoverRight);
    });
    topBar->createChild<Line>(1.0f);
    topBar->createChild<Label>("12 \u2014 Gizmo2D + Gizmo3D");

    // ── Status bar ─────────────────────────────────────────────────────────
    s_statusBar = layout->set<StatusBar>(BorderRegion::Bottom);
    layout->setRegionSize(BorderRegion::Bottom, 22.0f);
    s_statusBar->setText("Drag gizmo handles to transform.  Switch mode with the buttons.");

    // ── Center: splitter — left=2D, right=3D ──────────────────────────────
    auto* split = layout->set<Splitter>(BorderRegion::Center, LayoutDir::Horizontal);
    split->setRatio(0.5f);
    split->setMinRatio(0.2f);
    split->setMaxRatio(0.8f);

    // ═══════════════════════════════════════════════════════════════════════
    //  LEFT — Gizmo2D
    // ═══════════════════════════════════════════════════════════════════════
    auto* leftBox = split->createChild<BoxLayout>(LayoutDir::Vertical);
    leftBox->setSpacing(0.0f);
    leftBox->setPadding(0.0f);

    // Mode bar
    auto* modeBar2D = leftBox->createChild<BoxLayout>(LayoutDir::Horizontal);
    modeBar2D->setSpacing(4.0f);
    modeBar2D->setPadding(Edges(6.0f, 8.0f));
    modeBar2D->createChild<Label>("2D:");
    modeBar2D->createChild<Spacer>(0.0f)->setStretch(1.0f);

    // Canvas area
    auto* canvas2D = leftBox->createChild<BoxLayout>(LayoutDir::Vertical);
    canvas2D->setStretch(1.0f);
    canvas2D->setPadding(0.0f);

    auto* gizmo2d = canvas2D->createChild<Gizmo2D>();
    gizmo2d->setPosition(200.0f, 160.0f);
    gizmo2d->setMode(GizmoMode::Translate);

    gizmo2d->onTranslate.connect([](float, float)
    {
        if (s_statusBar) s_statusBar->setText("Translate");
    });
    gizmo2d->onRotate.connect([](float deg)
    {
        if (s_statusBar)
        {
            char buf[48];
            std::snprintf(buf, sizeof(buf), "Rotate  %.1f\xc2\xb0", deg);
            s_statusBar->setText(buf);
        }
    });
    gizmo2d->onScale.connect([](float sx, float sy)
    {
        if (s_statusBar)
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "Scale  %.2f, %.2f", sx, sy);
            s_statusBar->setText(buf);
        }
    });

    // Mode buttons (declared after gizmo2d)
    auto* btnT2 = modeBar2D->createChild<Button>("Translate");
    auto* btnR2 = modeBar2D->createChild<Button>("Rotate");
    auto* btnS2 = modeBar2D->createChild<Button>("Scale");
    btnT2->clicked.connect([gizmo2d]() { gizmo2d->setMode(GizmoMode::Translate); });
    btnR2->clicked.connect([gizmo2d]() { gizmo2d->setMode(GizmoMode::Rotate);    });
    btnS2->clicked.connect([gizmo2d]() { gizmo2d->setMode(GizmoMode::Scale);     });

    // ═══════════════════════════════════════════════════════════════════════
    //  RIGHT — Gizmo3D + wireframe cube
    // ═══════════════════════════════════════════════════════════════════════
    auto* rightBox = split->createChild<BoxLayout>(LayoutDir::Vertical);
    rightBox->setSpacing(0.0f);
    rightBox->setPadding(0.0f);

    // Mode bar
    auto* modeBar3D = rightBox->createChild<BoxLayout>(LayoutDir::Horizontal);
    modeBar3D->setSpacing(4.0f);
    modeBar3D->setPadding(Edges(6.0f, 8.0f));
    modeBar3D->createChild<Label>("3D:");
    modeBar3D->createChild<Spacer>(0.0f)->setStretch(1.0f);

    // Viewport canvas (plain BoxLayout — no FBO needed, pure 2D overlay)
    auto* viewport3D = rightBox->createChild<BoxLayout>(LayoutDir::Vertical);
    viewport3D->setStretch(1.0f);
    viewport3D->setPadding(0.0f);

    // Cube overlay drawn first (behind the gizmo handles)
    auto* cubeOverlay = viewport3D->createChild<WireCubeOverlay>();
    s_cubeOverlay = cubeOverlay;

    auto* gizmo3d = viewport3D->createChild<Gizmo3D>();
    s_gizmo3d = gizmo3d;
    cubeOverlay->gizmo = gizmo3d;

    float pos3[3]   = { 0.0f, 0.5f, 0.0f };
    float rot3[3]   = { 0.0f, 0.0f, 0.0f };
    float scale3[3] = { 1.0f, 1.0f, 1.0f };
    gizmo3d->setTarget(pos3, rot3, scale3);
    updateCamera(640, 400);

    gizmo3d->onTranslate3D.connect([](Vec3f d)
    {
        if (s_statusBar)
        {
            char buf[80];
            std::snprintf(buf, sizeof(buf), "Translate  %.2f, %.2f, %.2f", d.x, d.y, d.z);
            s_statusBar->setText(buf);
        }
    });
    gizmo3d->onRotate3D.connect([](Vec3f r)
    {
        if (s_statusBar)
        {
            char buf[80];
            std::snprintf(buf, sizeof(buf),
                "Rotate  %.1f\xc2\xb0 %.1f\xc2\xb0 %.1f\xc2\xb0", r.x, r.y, r.z);
            s_statusBar->setText(buf);
        }
    });
    gizmo3d->onScale3D.connect([](Vec3f s)
    {
        if (s_statusBar)
        {
            char buf[80];
            std::snprintf(buf, sizeof(buf), "Scale  %.2f, %.2f, %.2f", s.x, s.y, s.z);
            s_statusBar->setText(buf);
        }
    });

    // Mode buttons
    auto* btnT3 = modeBar3D->createChild<Button>("Translate");
    auto* btnR3 = modeBar3D->createChild<Button>("Rotate");
    auto* btnS3 = modeBar3D->createChild<Button>("Scale");
    btnT3->clicked.connect([gizmo3d]() { gizmo3d->setMode(GizmoMode3D::Translate); });
    btnR3->clicked.connect([gizmo3d]() { gizmo3d->setMode(GizmoMode3D::Rotate);    });
    btnS3->clicked.connect([gizmo3d]() { gizmo3d->setMode(GizmoMode3D::Scale);     });
}
