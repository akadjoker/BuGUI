#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  TreeGrid demo stage  (hierarchical file browser)
// ═════════════════════════════════════════════════════════════════════════════

inline void buildTreeGridStage(WidgetApp& app)
{
    Widget* root = app.addStage("treegrid");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Navigation ───────────────────────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    nav->setPadding(16, 16, 0, 16);
    auto* back = nav->createChild<Button>("<  DataGrid");
    back->clicked.connect([]{ WidgetApp::instance().setStage("datagrid", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("TreeGrid")->setColor(Clr::accent);
    auto* navFD = nav->createChild<Button>("FileDialog >");
    navFD->clicked.connect([]{ WidgetApp::instance().setStage("filedialog", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── Main layout ──────────────────────────────────────────────────────
    auto* body = outer->createChild<BoxLayout>(LayoutDir::Vertical);
    body->setStretch(1);
    body->setSpacing(8);
    body->setPadding(8);

    auto* info = body->createChild<Label>("Arrows to navigate, Left/Right collapse/expand, Enter to edit, double-click to edit cell");
    info->setColor(Clr::dim);

    // ── TreeGrid ─────────────────────────────────────────────────────────
    auto* tg = body->createChild<TreeGrid>();
    tg->setStretch(1);
    tg->setZebraStripes(true);

    tg->addColumn("Name", 260, true);
    tg->addColumn("Type", 100, true);
    tg->addColumn("Size", 90, true);
    tg->addColumn("Modified", 150, true);

    // ── Sample file tree ─────────────────────────────────────────────────
    auto* src = tg->addRoot({"src", "Folder", "48 KB", "2025-01-15"});
    src->setIcon(IconId::Folder);
    {
        auto* core = src->addChild({"core", "Folder", "20 KB", "2025-01-14"});
        core->setIcon(IconId::Folder);
        auto* f1 = core->addChild({"Batch.cpp", "C++ Source", "5.2 KB", "2025-01-14"});
        f1->setIcon(IconId::File);
        auto* f2 = core->addChild({"Device.cpp", "C++ Source", "3.8 KB", "2025-01-12"});
        f2->setIcon(IconId::File);
        auto* f3 = core->addChild({"Font.cpp", "C++ Source", "8.1 KB", "2025-01-10"});
        f3->setIcon(IconId::File);
        auto* f4 = core->addChild({"Shader.cpp", "C++ Source", "2.9 KB", "2025-01-13"});
        f4->setIcon(IconId::File);

        auto* widgets = src->addChild({"widgets", "Folder", "28 KB", "2025-01-15"});
        widgets->setIcon(IconId::Folder);
        auto* w1 = widgets->addChild({"Widget.cpp", "C++ Source", "12 KB", "2025-01-15"});
        w1->setIcon(IconId::File);
        auto* w2 = widgets->addChild({"DataGrid.cpp", "C++ Source", "9.4 KB", "2025-01-14"});
        w2->setIcon(IconId::File);
        auto* w3 = widgets->addChild({"TreeGrid.cpp", "C++ Source", "6.6 KB", "2025-01-15"});
        w3->setIcon(IconId::File);
    }

    auto* inc = tg->addRoot({"include", "Folder", "12 KB", "2025-01-14"});
    inc->setIcon(IconId::Folder);
    {
        auto* h1 = inc->addChild({"Widget.hpp", "C++ Header", "4.5 KB", "2025-01-14"});
        h1->setIcon(IconId::File);
        auto* h2 = inc->addChild({"Widgets.hpp", "C++ Header", "6.2 KB", "2025-01-14"});
        h2->setIcon(IconId::File);
        auto* h3 = inc->addChild({"Types.hpp", "C++ Header", "1.3 KB", "2025-01-10"});
        h3->setIcon(IconId::File);
    }

    auto* vendor = tg->addRoot({"vendor", "Folder", "320 KB", "2025-01-08"});
    vendor->setIcon(IconId::Folder);
    {
        auto* sdl = vendor->addChild({"SDL", "Folder", "280 KB", "2025-01-05"});
        sdl->setIcon(IconId::Folder);
        sdl->addChild({"SDL.h", "C Header", "12 KB", "2025-01-05"})->setIcon(IconId::File);
        sdl->addChild({"SDL_video.h", "C Header", "8.4 KB", "2025-01-05"})->setIcon(IconId::File);

        auto* stb = vendor->addChild({"stb", "Folder", "40 KB", "2025-01-08"});
        stb->setIcon(IconId::Folder);
        stb->addChild({"stb_image.h", "C Header", "24 KB", "2025-01-08"})->setIcon(IconId::File);
        stb->addChild({"stb_truetype.h", "C Header", "16 KB", "2025-01-08"})->setIcon(IconId::File);
    }

    auto* build = tg->addRoot({"CMakeLists.txt", "CMake", "1.2 KB", "2025-01-15"});
    build->setIcon(IconId::File);

    auto* readme = tg->addRoot({"README.md", "Markdown", "3.4 KB", "2025-01-12"});
    readme->setIcon(IconId::File);

    // ── Status bar ───────────────────────────────────────────────────────
    auto* statusBar = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    statusBar->setSize(0, 24);
    statusBar->setSpacing(16);
    statusBar->setPadding(4, 4, 0, 4);

    auto* selLabel = statusBar->createChild<Label>("Selected: none");
    selLabel->setColor(Clr::dim);

    tg->selectionChanged.connect([selLabel](TreeGrid::Node* node) {
        if (node && !node->cells.empty()) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Selected: %s", node->cells[0].c_str());
            selLabel->setText(buf);
        }
    });

    tg->nodeExpanded.connect([](TreeGrid::Node* node) {
        (void)node;
    });

    tg->nodeCollapsed.connect([](TreeGrid::Node* node) {
        (void)node;
    });
}
