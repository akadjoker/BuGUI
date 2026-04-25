#include "WidgetApp.hpp"
#include "Widgets.hpp"
#include "Batch.hpp"

#include <cmath>
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
struct WireCubeOverlay : public Widget
{
    Gizmo3D* gizmo = nullptr;
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);

    void setCamera(const glm::mat4& v, const glm::mat4& p)
    {
        view = v;
        proj = p;
    }

    glm::vec2 project3D(const glm::vec3& world) const
    {
        Rect abs = parent_ ? parent_->absoluteRect() : Rect{0, 0, 800, 600};
        glm::vec4 clip = proj * view * glm::vec4(world, 1.0f);
        if (std::fabs(clip.w) < 1e-6f) return {-9999.0f, -9999.0f};
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        float sx = abs.x + (ndc.x * 0.5f + 0.5f) * abs.w;
        float sy = abs.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * abs.h;
        return {sx, sy};
    }

    void drawLine3D(PaintContext& ctx, const glm::vec3& a, const glm::vec3& b,
                    const Color& c) const
    {
        glm::vec2 sa = project3D(a);
        glm::vec2 sb = project3D(b);
        ctx.line.SetColor(c.r, c.g, c.b, c.a);
        ctx.drawLine(sa.x, sa.y, sb.x, sb.y);
    }

    void paint(PaintContext& ctx) override
    {
        if (!visible_ || !gizmo) return;

        for (int i = -6; i <= 6; ++i) {
            float f = static_cast<float>(i);
            drawLine3D(ctx, {f, 0.0f, -6.0f}, {f, 0.0f, 6.0f}, Color(55, 58, 64, 110));
            drawLine3D(ctx, {-6.0f, 0.0f, f}, {6.0f, 0.0f, f}, Color(55, 58, 64, 110));
        }

        glm::vec3 pos = gizmo->targetPosition();
        glm::vec3 rot = gizmo->targetRotation();
        glm::vec3 scl = gizmo->targetScale();

        glm::mat4 model(1.0f);
        model = glm::translate(model, pos);
        model = glm::rotate(model, glm::radians(rot.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(rot.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(rot.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, scl);

        static const glm::vec3 corners[8] = {
            {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
        };
        static const int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0},
            {4,5},{5,6},{6,7},{7,4},
            {0,4},{1,5},{2,6},{3,7},
        };

        glm::vec3 worldCorners[8];
        for (int i = 0; i < 8; ++i) {
            glm::vec4 p = model * glm::vec4(corners[i], 1.0f);
            worldCorners[i] = glm::vec3(p);
        }

        for (auto& e : edges) {
            drawLine3D(ctx, worldCorners[e[0]], worldCorners[e[1]], Color(120, 185, 245, 230));
        }
    }
};
} // namespace

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 12 - Gizmo3D + Property Panel", 1340, 860))
        return 1;

    Widget* root = app.addStage("main");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(12, 12, 12, 12);
    outer->setSpacing(10);

    auto* title = outer->createChild<Label>("Tutorial 12: Gizmo3D + Property Panel");
    title->setColor(Color(125, 196, 255, 255));
    auto* status = outer->createChild<Label>("Drag 3D gizmo handles or edit transform values.");

    auto* body = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    body->setSpacing(10);
    body->setStretch(1);

    auto* viewportPanel = body->createChild<Panel>();
    viewportPanel->setStretch(3);
    auto* vpCol = viewportPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    vpCol->setPadding(8, 8, 8, 8);
    vpCol->setSpacing(6);
    vpCol->createChild<Label>("3D View");

    auto* viewport = vpCol->createChild<ViewportWidget>();
    viewport->setStretch(1);
    viewport->setBgColor(Color(22, 24, 29, 255));
    viewport->setLabel("Perspective");

    auto* overlay = viewport->createChild<WireCubeOverlay>();
    auto* gizmo = viewport->createChild<Gizmo3D>();
    gizmo->setTarget(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    gizmo->setMode(GizmoMode3D::Translate);
    gizmo->setSnapTranslate(0.0f);
    gizmo->setSnapRotate(0.0f);
    gizmo->setSnapScale(0.0f);
    gizmo->setScreenScale(90.0f);
    overlay->gizmo = gizmo;

    auto* propsPanel = body->createChild<Panel>();
    propsPanel->setStretch(2);
    auto* ppCol = propsPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    ppCol->setPadding(8, 8, 8, 8);
    ppCol->setSpacing(6);
    ppCol->createChild<Label>("Property Panel");
    auto* props = ppCol->createChild<PropertyGrid>();
    props->setStretch(1);

    float cameraYaw = 42.0f;
    float cameraPitch = 24.0f;
    float cameraDist = 7.0f;
    glm::vec3 cameraTarget(0.0f, 0.5f, 0.0f);

    auto updateCamera = [=]() mutable {
        int w = viewport->viewportWidth();
        int h = viewport->viewportHeight();
        if (w <= 0) w = 640;
        if (h <= 0) h = 420;

        float yawRad = glm::radians(cameraYaw);
        float pitchRad = glm::radians(cameraPitch);
        glm::vec3 eye = cameraTarget + glm::vec3(
            std::cos(pitchRad) * std::cos(yawRad),
            std::sin(pitchRad),
            std::cos(pitchRad) * std::sin(yawRad)
        ) * cameraDist;

        glm::mat4 view = glm::lookAt(eye, cameraTarget, glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                          static_cast<float>(w) / static_cast<float>(h),
                                          0.1f, 100.0f);
        gizmo->setViewProjection(view, proj, w, h);
        overlay->setCamera(view, proj);
    };

    props->addSection("Transform");
    int rowMode = props->addCombo("Mode", {"Translate", "Rotate", "Scale"}, 0,
        [gizmo](int idx) {
            if (idx == 0) gizmo->setMode(GizmoMode3D::Translate);
            else if (idx == 1) gizmo->setMode(GizmoMode3D::Rotate);
            else gizmo->setMode(GizmoMode3D::Scale);
        });
    int rowPosition = props->addVec3("Position", 0.0f, 0.5f, 0.0f, -10.0f, 10.0f,
        [gizmo](float x, float y, float z) {
            gizmo->setTarget(glm::vec3(x, y, z), gizmo->targetRotation(), gizmo->targetScale());
        });
    int rowRotation = props->addVec3("Rotation", 0.0f, 0.0f, 0.0f, -180.0f, 180.0f,
        [gizmo](float x, float y, float z) {
            gizmo->setTarget(gizmo->targetPosition(), glm::vec3(x, y, z), gizmo->targetScale());
        });
    int rowScale = props->addVec3("Scale", 1.0f, 1.0f, 1.0f, 0.05f, 8.0f,
        [gizmo](float x, float y, float z) {
            gizmo->setTarget(gizmo->targetPosition(), gizmo->targetRotation(), glm::vec3(x, y, z));
        });

    props->addSeparator();
    props->addSection("Snap");
    props->addFloat("Move", 0.0f, 0.0f, 2.0f, [gizmo](float v) { gizmo->setSnapTranslate(v); });
    props->addFloat("Rotate", 0.0f, 0.0f, 90.0f, [gizmo](float v) { gizmo->setSnapRotate(v); });
    props->addFloat("Scale", 0.0f, 0.0f, 1.0f, [gizmo](float v) { gizmo->setSnapScale(v); });

    props->addSeparator();
    props->addSection("Camera");
    props->addFloat("Yaw", cameraYaw, -180.0f, 180.0f, [&](float v) { cameraYaw = v; updateCamera(); });
    props->addFloat("Pitch", cameraPitch, -80.0f, 80.0f, [&](float v) { cameraPitch = v; updateCamera(); });
    props->addFloat("Distance", cameraDist, 2.0f, 20.0f, [&](float v) { cameraDist = v; updateCamera(); });
    props->addFloat("Gizmo Size", gizmo->screenScale(), 40.0f, 180.0f, [gizmo](float v) { gizmo->setScreenScale(v); });
    props->addButton("Reset Transform", [=]() {
        gizmo->setTarget(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
        gizmo->setMode(GizmoMode3D::Translate);
        props->setCombo(rowMode, 0);
        props->setVec3(rowPosition, 0.0f, 0.5f, 0.0f);
        props->setVec3(rowRotation, 0.0f, 0.0f, 0.0f);
        props->setVec3(rowScale, 1.0f, 1.0f, 1.0f);
        status->setText("Transform reset.");
    });

    gizmo->onTranslate3D.connect([=](const glm::vec3& delta) {
        glm::vec3 p = gizmo->targetPosition();
        props->setVec3(rowPosition, p.x, p.y, p.z);
        char buf[160];
        std::snprintf(buf, sizeof(buf), "Translate d(%.3f, %.3f, %.3f)", delta.x, delta.y, delta.z);
        status->setText(buf);
    });
    gizmo->onRotate3D.connect([=](const glm::vec3& rot) {
        props->setVec3(rowRotation, rot.x, rot.y, rot.z);
        char buf[160];
        std::snprintf(buf, sizeof(buf), "Rotate (%.1f, %.1f, %.1f) deg", rot.x, rot.y, rot.z);
        status->setText(buf);
    });
    gizmo->onScale3D.connect([=](const glm::vec3& scl) {
        props->setVec3(rowScale, scl.x, scl.y, scl.z);
        char buf[160];
        std::snprintf(buf, sizeof(buf), "Scale (%.2f, %.2f, %.2f)", scl.x, scl.y, scl.z);
        status->setText(buf);
    });

    updateCamera();
    viewport->viewportResized.connect([&](int, int) { updateCamera(); });

    app.setStage("main");
    return app.run();
}
