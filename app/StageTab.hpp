#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  TabLayout demo — full stage, with position switcher and closable tabs
// ═════════════════════════════════════════════════════════════════════════════

inline void buildTabStage(WidgetApp& app)
{
    Widget* root = app.addStage("tabs");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Navigation ───────────────────────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    nav->setPadding(16, 16, 0, 16);
    auto* back = nav->createChild<Button>("<  Layouts2");
    back->clicked.connect([]{ WidgetApp::instance().setStage("layouts2", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("TabLayout")->setColor(Clr::accent);
    auto* fwd = nav->createChild<Button>("Anchor  >");
    fwd->clicked.connect([]{ WidgetApp::instance().setStage("anchor", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── Toolbar: position buttons + add tab ──────────────────────────────
    auto* toolbar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    toolbar->setSpacing(6);
    toolbar->setPadding(8, 4, 4, 4);
    toolbar->setSize(0, 30);
    toolbar->createChild<Label>("Position:")->setColor(Clr::dim);

    // ── TabLayout fills the stage ────────────────────────────────────────
    auto* tabs = outer->createChild<TabLayout>();
    tabs->setStretch(1.0f);
    tabs->setTabsClosable(true);
    tabs->setTabBarSize(30);

    // Position buttons
    auto* bTop = toolbar->createChild<Button>("Top");
    auto* bBot = toolbar->createChild<Button>("Bottom");
    auto* bLft = toolbar->createChild<Button>("Left");
    auto* bRgt = toolbar->createChild<Button>("Right");
    bTop->clicked.connect([tabs]{ tabs->setTabPosition(TabPosition::Top); });
    bBot->clicked.connect([tabs]{ tabs->setTabPosition(TabPosition::Bottom); });
    bLft->clicked.connect([tabs]{ tabs->setTabPosition(TabPosition::Left); });
    bRgt->clicked.connect([tabs]{ tabs->setTabPosition(TabPosition::Right); });

    toolbar->createChild<Spacer>()->setStretch(1);

    // Add tab button
    static int tabCounter = 4; // safe: only included in one TU (main.cpp)
    auto* addBtn = toolbar->createChild<Button>("+ Add Tab");
    addBtn->setBgColor(Color(60, 130, 80, 255));
    addBtn->clicked.connect([tabs]{
        char name[32];
        snprintf(name, sizeof(name), "Tab %d", tabCounter++);
        auto* p = tabs->addTab<Panel>(name);
        p->setBgColor(Color(40 + (tabCounter * 7) % 20, 42 + (tabCounter * 11) % 20, 50, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(12);
        box->setSpacing(6);
        box->createChild<Label>(name)->setColor(Clr::accent);
        box->createChild<Label>("Dynamically added tab")->setColor(Clr::dim);
        box->createChild<Slider>();
        box->createChild<CheckBox>("Option");
        tabs->setCurrentIndex(tabs->tabCount() - 1);
    });

    // ── Pre-built tabs ───────────────────────────────────────────────────

    // Tab 0: Scene
    {
        auto* p = tabs->addTab<Panel>("Scene");
        p->setBgColor(Color(38, 42, 50, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(12);
        box->setSpacing(6);
        box->createChild<Label>("Scene View")->setColor(Clr::accent);
        box->createChild<Label>("3D viewport with wireframe overlay")->setColor(Clr::dim);

        auto* grid = box->createChild<GridLayout>(3);
        grid->setSpacing(4);
        grid->setSize(0, 160);
        for (int i = 0; i < 6; ++i)
        {
            auto* cell = grid->createChild<Panel>();
            cell->setBgColor(Color(30 + i * 8, 32 + i * 4, 38 + i * 6, 255));
            cell->createChild<Label>(i == 3 ? "Selected" : "")->setColor(
                i == 3 ? Color(100, 220, 100, 255) : Clr::dim);
        }

        auto* row = box->createChild<BoxLayout>(LayoutDir::Horizontal);
        row->setSpacing(6);
        row->createChild<Button>("Wireframe")->setBgColor(Color(60, 60, 65, 255));
        row->createChild<Button>("Textured")->setBgColor(Color(60, 60, 65, 255));
        row->createChild<Button>("Shaded")->setBgColor(Color(60, 60, 65, 255));
    }

    // Tab 1: Assets
    {
        auto* p = tabs->addTab<Panel>("Assets");
        p->setBgColor(Color(40, 44, 52, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(12);
        box->setSpacing(6);
        box->createChild<Label>("Asset Browser")->setColor(Clr::accent);

        auto* grid = box->createChild<GridLayout>(4);
        grid->setSpacing(4);
        grid->setStretch(1);
        const char* assets[] = {"brick.png", "wood.png", "metal.png", "grass.png",
                                 "stone.png", "sand.png", "water.png", "lava.png"};
        for (int i = 0; i < 8; ++i)
        {
            auto* cell = grid->createChild<Panel>();
            cell->setBgColor(Color(50 + i * 5, 52, 58, 255));
            cell->createChild<Label>(assets[i])->setColor(Clr::dim);
        }
    }

    // Tab 2: Console
    {
        auto* p = tabs->addTab<Panel>("Console");
        p->setBgColor(Color(25, 28, 32, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(12);
        box->setSpacing(4);
        box->createChild<Label>("Console")->setColor(Clr::accent);
        box->createChild<Label>("> Loading scene...")->setColor(Color(180, 180, 185, 255));
        box->createChild<Label>("> Compiling shaders (3/3)")->setColor(Color(180, 180, 185, 255));
        box->createChild<Label>("> Build succeeded (0.42s)")->setColor(Color(80, 200, 80, 255));
        box->createChild<Label>("> Warning: unused texture 'old_brick'")->setColor(Color(220, 180, 60, 255));
        box->createChild<Label>("> Ready.")->setColor(Color(180, 180, 185, 255));
    }

    // Tab 3: Properties
    {
        auto* p = tabs->addTab<Panel>("Properties");
        p->setBgColor(Color(38, 40, 48, 255));
        auto* form = p->createChild<FormLayout>();
        form->setLabelWidth(100);
        form->setSpacing(8, 6);
        form->setPadding(12);
        form->addRow<Slider>("Position X");
        form->addRow<Slider>("Position Y");
        form->addRow<Slider>("Position Z");
        form->addRow<Slider>("Rotation");
        form->addRow<Slider>("Scale");
        form->addRow<CheckBox>("Visible");
        form->addRow<CheckBox>("Static");
    }

    // ── StatusBar ────────────────────────────────────────────────────────
    auto* status = outer->createChild<StatusBar>();
    status->setText("TabLayout demo | closable | 4 positions | dynamic add");
}
