#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Grid layout demo stage
// ═════════════════════════════════════════════════════════════════════════════

inline void buildGridStage(WidgetApp& app)
{
    Widget* root = app.addStage("grid");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Navigation (outside scroll) ──────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    nav->setPadding(16, 16, 0, 16);
    auto* back = nav->createChild<Button>("<  Calc");
    back->clicked.connect([]{ WidgetApp::instance().setStage("calc", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("Grid Layout")->setColor(Clr::accent);
    nav->createChild<Spacer>()->setStretch(1.0f);
    auto* fwd = nav->createChild<Button>("Border  >");
    fwd->clicked.connect([]{ WidgetApp::instance().setStage("border", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── Scrollable body ──────────────────────────────────────────────────
    auto* sv = outer->createChild<ScrollView>();
    sv->setStretch(1.0f);

    auto* body = sv->setContent<BoxLayout>(LayoutDir::Vertical);
    body->setSpacing(8);
    body->setPadding(16, 16, 16, 16);

    // ═══════════════════════════════════════════════════════════════════════
    //  Demo 1: 3-column grid of buttons
    // ═══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "3-Column Grid (buttons)");

    auto* grid3 = body->createChild<GridLayout>(3);
    grid3->setSpacing(4);
    grid3->setPadding(4);
    grid3->setSize(0, 140);

    const Color btnBg(60, 62, 68, 255);
    for (int i = 1; i <= 9; ++i)
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", i);
        auto* btn = grid3->createChild<Button>(buf);
        btn->setBgColor(btnBg);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  Demo 2: 4-column color palette (panels)
    // ═══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "4-Column Color Palette");

    auto* grid4 = body->createChild<GridLayout>(4);
    grid4->setSpacing(4);
    grid4->setPadding(4);
    grid4->setSize(0, 120);

    const Color palette[] = {
        {220,  60,  60, 255}, {240, 140,  40, 255}, {240, 200,  40, 255}, { 80, 200,  80, 255},
        { 60, 160, 220, 255}, {140,  80, 220, 255}, {200,  80, 180, 255}, {160, 160, 165, 255},
    };
    const char* names[] = {
        "Red", "Orange", "Yellow", "Green",
        "Blue", "Purple", "Pink", "Gray",
    };

    for (int i = 0; i < 8; ++i)
    {
        auto* panel = grid4->createChild<Panel>();
        panel->setBgColor(palette[i]);
        auto* lbl = panel->createChild<Label>(names[i]);
        lbl->setColor(Color(255, 255, 255, 220));
        lbl->setAlign(TextAlign::CENTER);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  Demo 3: Mixed types — buttons, labels, spacers, checkboxes
    // ═══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "3-Column Mixed (Button + Label + Spacer + CheckBox)");

    auto* gridMix = body->createChild<GridLayout>(3);
    gridMix->setSpacing(4);
    gridMix->setPadding(4);
    gridMix->setSize(0, 140);

    gridMix->createChild<Button>("Btn 1")->setBgColor(Color(70, 70, 75, 255));
    gridMix->createChild<Label>("Label A")->setColor(Clr::accent);
    gridMix->createChild<CheckBox>("Check 1");

    gridMix->createChild<Spacer>();  // empty cell
    gridMix->createChild<Button>("Btn 2")->setBgColor(Color(55, 100, 55, 255));
    gridMix->createChild<Spacer>();  // empty cell

    gridMix->createChild<CheckBox>("Check 2");
    gridMix->createChild<Label>("Label B")->setColor(Color(220, 180, 80, 255));
    gridMix->createChild<Button>("Btn 3")->setBgColor(Color(100, 55, 55, 255));

    // ═══════════════════════════════════════════════════════════════════════
    //  Demo 4: 5-column grid (many cells, alternating rows)
    // ═══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "5-Column Grid (20 cells)");

    auto* grid5 = body->createChild<GridLayout>(5);
    grid5->setSpacing(3);
    grid5->setPadding(2);
    grid5->setSize(0, 160);

    for (int i = 0; i < 20; ++i)
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "%c%d", 'A' + (i / 5), (i % 5) + 1);
        auto* btn = grid5->createChild<Button>(buf);
        int row = i / 5;
        Color bg = (row % 2 == 0) ? Color(55, 58, 65, 255) : Color(45, 48, 55, 255);
        btn->setBgColor(bg);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  Demo 5: 2-column form layout
    // ═══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "2-Column Form Layout");

    auto* grid2 = body->createChild<GridLayout>(2);
    grid2->setSpacing(6, 4);
    grid2->setPadding(4);
    grid2->setSize(0, 120);

    const char* labels[] = {"Name:", "Email:", "Phone:", "City:"};
    const char* values[] = {"John Doe", "john@example", "555-0123", "Berlin"};

    for (int i = 0; i < 4; ++i)
    {
        auto* lbl = grid2->createChild<Label>(labels[i]);
        lbl->setColor(Clr::dim);
        lbl->setAlign(TextAlign::RIGHT);

        auto* val = grid2->createChild<Label>(values[i]);
        val->setColor(Color(220, 220, 225, 255));
    }
}
