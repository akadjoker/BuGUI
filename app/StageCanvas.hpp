#pragma once

#include "StageCommon.hpp"
#include "Texture.hpp"

struct DragState {
    bool  active  = false;
    bool  grabSet = false;
    float grabX   = 0;
    float grabY   = 0;
};

inline void connectDrag(ImageView* img, DragState& drag)
{
    img->pressed.connect([&drag](int btn) {
        if (btn != 0) return;
        drag.active  = true;
        drag.grabSet = false;
    });
    img->moved.connect([&drag, img](float lx, float ly) {
        if (!drag.active) return;
        if (!drag.grabSet) { drag.grabX = lx; drag.grabY = ly; drag.grabSet = true; }
        const auto& r = img->rect();
        img->setPosition(r.x + (lx - drag.grabX), r.y + (ly - drag.grabY));
    });
    img->released.connect([&drag](int btn) {
        if (btn != 0) return;
        drag.active = false;
    });
}

inline void buildCanvasStage(WidgetApp& app, Texture* tex, DragState& drag)
{
    Widget* root = app.addStage("canvas");
    auto* page = root->createChild<BoxLayout>(LayoutDir::Vertical);
    page->setSpacing(6);
    page->setPadding(16, 16, 16, 16);

    // ── Header ───────────────────────────────────────────────────────────
    auto* header = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSpacing(8);
    header->setSize(0, 32);
    auto* back = header->createChild<Button>("<  Layout");
    back->clicked.connect([]{ WidgetApp::instance().setStage("layout", TransitionType::SlideRight, EaseType::OutBack); });
    header->createChild<Label>("Canvas")->setColor(Clr::accent);
    auto* toWidgets = header->createChild<Button>("Widgets  >");
    toWidgets->clicked.connect([]{ WidgetApp::instance().setStage("widgets", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── Content: panel + canvas side by side ─────────────────────────────
    auto* row = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    row->setSpacing(12);

    // Panel with overflow
    auto* panel = row->createChild<Panel>();
    panel->setSize(190, 260);
    panel->setBgColor(Clr::panelDark);

    auto* col = panel->createChild<BoxLayout>(LayoutDir::Vertical);
    col->setSpacing(5);
    col->setPadding(10);
    col->createChild<Label>("Controls");
    col->createChild<CheckBox>("Visible");
    col->createChild<CheckBox>("Wireframe");
    col->createChild<Label>("Opacity:")->setColor(Clr::dim);
    col->createChild<Slider>(0.0f, 1.0f, 1.0f);
    col->createChild<Label>("Scale:")->setColor(Clr::dim);
    col->createChild<Slider>(0.1f, 4.0f, 1.0f);
    col->createChild<Button>("Reset");
    col->createChild<Button>("Overflow 1");
    col->createChild<Button>("Overflow 2");

    // Canvas
    auto* canvas = row->createChild<Canvas>();
    canvas->setSize(500, 360);
    canvas->setBgColor(Clr::canvasBg);

    auto* img = canvas->createChild<ImageView>();
    img->setTexture(tex);
    img->setPosition(60, 60);
    img->setId("img");

    connectDrag(img, drag);

    // ── Rotation control ─────────────────────────────────────────────────
    auto* rotRow = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    rotRow->setSpacing(8);
    rotRow->setPadding(6, 0, 0, 0);
    rotRow->setSize(0, 28);

    rotRow->createChild<Label>("Rotation");
    auto* rotSlider = rotRow->createChild<Slider>(0.0f, 360.0f, 0.0f);
    rotSlider->setStretch(1.0f);
    auto* degLabel = rotRow->createChild<Label>("0");
    degLabel->setSize(40, 0);
    degLabel->setColor(Clr::dim);

    rotSlider->onValueChanged.connect([img, degLabel](float v) {
        img->setAngle(v);
        char b[16]; snprintf(b, sizeof(b), "%.0f", v);
        degLabel->setText(b);
    });

    // ── Footer ───────────────────────────────────────────────────────────
    dimLabel(page, "Drag the image inside the canvas — clipping is automatic");
}
