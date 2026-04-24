#pragma once

#include "StageCommon.hpp"

inline void buildScrollStage(WidgetApp& app)
{
    Widget* root = app.addStage("scroll");
    auto* page = root->createChild<BoxLayout>(LayoutDir::Vertical);
    page->setSpacing(6);
    page->setPadding(16, 16, 16, 16);

    // ── Header ───────────────────────────────────────────────────────────
    auto* header = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSpacing(8);
    header->setSize(0, 32);
    auto* back = header->createChild<Button>("<  Widgets");
    back->clicked.connect([]{ WidgetApp::instance().setStage("widgets", TransitionType::SlideRight, EaseType::OutBack); });
    header->createChild<Label>("ScrollBar & ScrollView")->setColor(Clr::accent);
    auto* toCalc = header->createChild<Button>("Calc  >");
    toCalc->clicked.connect([]{ WidgetApp::instance().setStage("calc", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── Standalone ScrollBars ────────────────────────────────────────────
    sectionLabel(page, "ScrollBar  (standalone)");

    auto* sbRow = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    sbRow->setSpacing(12);
    sbRow->setPadding(6);
    sbRow->setSize(0, 80);

    // Vertical scrollbar
    auto* vsb = sbRow->createChild<ScrollBar>(LayoutDir::Vertical);
    vsb->setSize(14, 70);
    vsb->setContentSize(500.0f);
    vsb->setVisibleSize(70.0f);
    auto* vLabel = sbRow->createChild<Label>("V: 0");
    vLabel->setSize(60, 20);
    vsb->onValueChanged.connect([vLabel](float v) {
        char b[16]; snprintf(b, sizeof(b), "V: %.0f", v);
        vLabel->setText(b);
    });

    // Horizontal scrollbar
    auto* hsbCol = sbRow->createChild<BoxLayout>(LayoutDir::Vertical);
    hsbCol->setSpacing(4);
    hsbCol->setStretch(1.0f);
    auto* hsb = hsbCol->createChild<ScrollBar>(LayoutDir::Horizontal);
    hsb->setSize(0, 14);
    hsb->setStretch(1.0f);
    hsb->setContentSize(800.0f);
    hsb->setVisibleSize(200.0f);
    auto* hLabel = hsbCol->createChild<Label>("H: 0");
    hLabel->setSize(100, 20);
    hsb->onValueChanged.connect([hLabel](float v) {
        char b[16]; snprintf(b, sizeof(b), "H: %.0f", v);
        hLabel->setText(b);
    });
    dimLabel(hsbCol, "Drag thumb or click track");

    page->createChild<Line>();

    // ── H+V ScrollView (wide grid) ──────────────────────────────────────
    sectionLabel(page, "ScrollView  H+V  (wide grid — scroll both axes)");

    auto* svHV = page->createChild<ScrollView>();
    svHV->setSize(0, 200);

    // Content wider and taller than the viewport
    auto* grid = svHV->setContent<BoxLayout>(LayoutDir::Vertical);
    grid->setSpacing(4);
    grid->setPadding(4, 4, 4, 4);

    for (int row = 0; row < 15; ++row)
    {
        auto* r = grid->createChild<BoxLayout>(LayoutDir::Horizontal);
        r->setSpacing(4);
        r->setSize(0, 28);

        for (int col = 0; col < 16; ++col)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "[%d,%d]", row, col);
            auto* btn = r->createChild<Button>(buf);
            btn->setSize(90, 26);
        }
    }

    page->createChild<Line>();

    // ── Vertical ScrollView (long list) ──────────────────────────────────
    sectionLabel(page, "ScrollView  V  (long list with buttons)");

    auto* sv = page->createChild<ScrollView>();
    sv->setStretch(1.0f);
    sv->setSize(0, 0);

    auto* content = sv->setContent<BoxLayout>(LayoutDir::Vertical);
    content->setSpacing(4);
    content->setPadding(8, 8, 8, 8);

    for (int i = 0; i < 30; ++i)
    {
        char buf[64];
        if (i % 6 == 0)
        {
            snprintf(buf, sizeof(buf), "── Section %d ──", i / 6 + 1);
            auto* lbl = content->createChild<Label>(buf);
            lbl->setColor(Clr::accent);
            lbl->setSize(0, 24);
        }
        else if (i % 6 == 3)
        {
            auto* row = content->createChild<BoxLayout>(LayoutDir::Horizontal);
            row->setSpacing(4);
            row->setSize(0, 28);
            snprintf(buf, sizeof(buf), "Action %d", i + 1);
            auto* btn = row->createChild<Button>(buf);
            btn->clicked.connect([i]{ printf("Clicked item %d\n", i + 1); });
            row->createChild<Label>("(click me)")->setColor(Clr::dim);
        }
        else
        {
            snprintf(buf, sizeof(buf), "Item %d", i + 1);
            content->createChild<Label>(buf)->setSize(0, 22);
        }
    }

    dimLabel(page, "Mouse wheel: V scroll.  Shift+wheel or trackpad: H scroll.");
}
