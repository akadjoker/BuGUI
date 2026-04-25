#include "WidgetApp.hpp"
#include "Widgets.hpp"
#include "Batch.hpp"

#include <cmath>

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 11 - Gizmo2D + Property Panel", 1280, 820))
        return 1;

    Widget* root = app.addStage("main");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(12, 12, 12, 12);
    outer->setSpacing(10);

    auto* title = outer->createChild<Label>("Tutorial 11: Gizmo2D + Property Panel");
    title->setColor(Color(120, 190, 255, 255));
    auto* status = outer->createChild<Label>("Drag handles or edit values in PropertyGrid.");

    auto* body = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    body->setSpacing(10);
    body->setStretch(1);

    auto* viewportPanel = body->createChild<Panel>();
    viewportPanel->setStretch(3);
    auto* vpCol = viewportPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    vpCol->setPadding(8, 8, 8, 8);
    vpCol->setSpacing(6);
    vpCol->createChild<Label>("2D View");

    auto* canvas = vpCol->createChild<Canvas>();
    canvas->setStretch(1);
    canvas->setBgColor(Color(28, 30, 36, 255));

    auto* propsPanel = body->createChild<Panel>();
    propsPanel->setStretch(2);
    auto* ppCol = propsPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    ppCol->setPadding(8, 8, 8, 8);
    ppCol->setSpacing(6);
    ppCol->createChild<Label>("Property Panel");
    auto* props = ppCol->createChild<PropertyGrid>();
    props->setStretch(1);

    auto* gizmo = canvas->createChild<Gizmo2D>();
    gizmo->setPosition(260.0f, 220.0f);
    gizmo->setRotation(0.0f);
    gizmo->setScale(1.0f, 1.0f);
    gizmo->setMode(GizmoMode::Translate);
    gizmo->setSnapTranslate(0.0f);
    gizmo->setSnapRotate(0.0f);
    gizmo->setSnapScale(0.0f);

    int rowMode = props->addCombo("Mode", {"Translate", "Rotate", "Scale"}, 0,
        [gizmo](int idx) {
            if (idx == 0) gizmo->setMode(GizmoMode::Translate);
            else if (idx == 1) gizmo->setMode(GizmoMode::Rotate);
            else gizmo->setMode(GizmoMode::Scale);
        });
    (void)rowMode;

    int rowPosX = props->addFloat("Position X", gizmo->positionX(), -2000.0f, 2000.0f,
        [gizmo](float v) { gizmo->setPosition(v, gizmo->positionY()); });
    int rowPosY = props->addFloat("Position Y", gizmo->positionY(), -2000.0f, 2000.0f,
        [gizmo](float v) { gizmo->setPosition(gizmo->positionX(), v); });
    int rowRot = props->addFloat("Rotation", gizmo->rotation(), -720.0f, 720.0f,
        [gizmo](float v) { gizmo->setRotation(v); });
    int rowScaleX = props->addFloat("Scale X", gizmo->scaleX(), 0.05f, 8.0f,
        [gizmo](float v) { gizmo->setScale(v, gizmo->scaleY()); });
    int rowScaleY = props->addFloat("Scale Y", gizmo->scaleY(), 0.05f, 8.0f,
        [gizmo](float v) { gizmo->setScale(gizmo->scaleX(), v); });
    props->addSeparator();
    props->addFloat("Snap Move", 0.0f, 0.0f, 64.0f, [gizmo](float v) { gizmo->setSnapTranslate(v); });
    props->addFloat("Snap Rot",  0.0f, 0.0f, 90.0f, [gizmo](float v) { gizmo->setSnapRotate(v); });
    props->addFloat("Snap Scale", 0.0f, 0.0f, 1.0f, [gizmo](float v) { gizmo->setSnapScale(v); });

    gizmo->onTranslate.connect([props, status, gizmo, rowPosX, rowPosY](float dx, float dy) {
        props->setFloat(rowPosX, gizmo->positionX());
        props->setFloat(rowPosY, gizmo->positionY());
        status->setText("Translate d(" + std::to_string(dx) + ", " + std::to_string(dy) + ")");
    });
    gizmo->onRotate.connect([props, status, rowRot](float deg) {
        props->setFloat(rowRot, deg);
        status->setText("Rotate " + std::to_string(deg) + " deg");
    });
    gizmo->onScale.connect([props, status, rowScaleX, rowScaleY](float sx, float sy) {
        props->setFloat(rowScaleX, sx);
        props->setFloat(rowScaleY, sy);
        status->setText("Scale (" + std::to_string(sx) + ", " + std::to_string(sy) + ")");
    });

    canvas->setOnPaint([gizmo](PaintContext& ctx, const Rect& b) {
        ctx.fill.SetColor(24, 26, 30, 255);
        ctx.fillRect(b.x, b.y, b.w, b.h);

         ctx.fill.SetColor(45, 48, 54, 255);

        
 

         for (float x = b.x; x <= b.x + b.w; x += 32.0f)  ctx.fill.Line2D(x, b.y, x, b.y + b.h);
         for (float y = b.y; y <= b.y + b.h; y += 32.0f)  ctx.fill.Line2D(b.x, y, b.x + b.w, y);


 
        float cx = b.x + gizmo->positionX();
        float cy = b.y + gizmo->positionY();
        float sx = 80.0f * gizmo->scaleX();
        float sy = 50.0f * gizmo->scaleY();
        float rad = gizmo->rotation() * 3.14159265f / 180.0f;
        float cs = std::cos(rad), sn = std::sin(rad);

        auto rotatePt = [&](float lx, float ly, float& ox, float& oy) 
        {
            ox = cx + lx * cs - ly * sn;
            oy = cy + lx * sn + ly * cs;
        };

        float x0, y0, x1, y1, x2, y2, x3, y3;
        rotatePt(-sx, -sy, x0, y0);
        rotatePt( sx, -sy, x1, y1);
        rotatePt( sx,  sy, x2, y2);
        rotatePt(-sx,  sy, x3, y3);

        ctx.fill.SetColor(100, 170, 240, 120);
        ctx.fill.Triangle(x0, y0, x1, y1, x2, y2, true);
        ctx.fill.Triangle(x0, y0, x2, y2, x3, y3, true);

        ctx.fill.SetColor(130, 205, 255, 255);
        ctx.fill.Line2D(x0, y0, x1, y1);
        ctx.fill.Line2D(x1, y1, x2, y2);
        ctx.fill.Line2D(x2, y2, x3, y3);
        ctx.fill.Line2D(x3, y3, x0, y0);
    });

    app.setStage("main");
    return app.run();
}
