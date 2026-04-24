#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  BorderLayout demo stage
// ═════════════════════════════════════════════════════════════════════════════

inline void buildBorderStage(WidgetApp& app)
{
    Widget* root = app.addStage("border");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Navigation ───────────────────────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    nav->setPadding(16, 16, 0, 16);
    auto* back = nav->createChild<Button>("<  Grid");
    back->clicked.connect([]{ WidgetApp::instance().setStage("grid", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("Border Layout")->setColor(Clr::accent);
    auto* fwd = nav->createChild<Button>("Layouts2  >");
    fwd->clicked.connect([]{ WidgetApp::instance().setStage("layouts2", TransitionType::SlideLeft, EaseType::OutBack); });

    auto* body = outer->createChild<BoxLayout>(LayoutDir::Vertical);
    body->setStretch(1.0f);
    body->setSpacing(6);
    body->setPadding(8, 4, 8, 4);

    // ═══════════════════════════════════════════════════════════════════════
    //  Demo 1: Level Editor — Menu + Scene + Viewport + Inspector + Assets
    // ═══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "Level Editor: Menu + Scene + Viewport + Inspector + Assets");

    auto* ed = body->createChild<BorderLayout>();
    ed->setStretch(1.0f);
    ed->setSpacing(2);
    ed->setRegionSize(BorderRegion::Top,    26);     // menu bar
    ed->setRegionSize(BorderRegion::Bottom, -0.22f); // assets ~22%
    ed->setRegionSize(BorderRegion::Left,   -0.18f); // scene panel
    ed->setRegionSize(BorderRegion::Right,  -0.20f); // inspector

    // ── Menu bar (Top) ───────────────────────────────────────────────────
    auto* menu = ed->set<BoxLayout>(BorderRegion::Top, LayoutDir::Horizontal);
    menu->setSpacing(2);
    menu->setPadding(4, 0, 4, 0);
    const char* menuItems[] = {"File", "Edit", "Record", "View", "Theme"};
    const char* menuTips[]  = {"Open, Save, Export", "Undo, Redo, Copy, Paste",
                               "Record macro", "Toggle panels", "Change color theme"};
    for (int i = 0; i < 5; ++i)
    {
        auto* btn = menu->createChild<Button>(menuItems[i]);
        btn->setBgColor(Color(50, 52, 58, 255));
        btn->setTooltip(menuTips[i]);
    }

    // ── Scene panel (Left) — with Collapsibles ────────────────────────
    auto* scene = ed->set<BoxLayout>(BorderRegion::Left, LayoutDir::Vertical);
    scene->setSpacing(1);
    scene->setPadding(0);
    scene->createChild<Label>("Scene")->setColor(Clr::accent);

    auto* cTools = scene->createChild<Collapsible>("Tools");
    cTools->setHeaderColor(Color(40, 42, 50, 255));
    auto* toolGrid = cTools->createChild<GridLayout>(2);
    toolGrid->setSpacing(2);
    const char* tools[]    = {"Select", "Move", "Scale", "Rotate"};
    const char* toolTips[] = {"Select objects (Q)", "Move selection (W)",
                              "Scale selection (S)", "Rotate selection (R)"};
    for (int i = 0; i < 4; ++i)
    {
        auto* btn = toolGrid->createChild<Button>(tools[i]);
        btn->setBgColor(Color(50, 52, 60, 255));
        btn->setTooltip(toolTips[i]);
    }

    auto* cMesh = scene->createChild<Collapsible>("Mesh Objects");
    cMesh->setHeaderColor(Color(40, 42, 50, 255));
    cMesh->createChild<Button>("Create Empty")->setBgColor(Color(50, 52, 60, 255));

    auto* cEntities = scene->createChild<Collapsible>("Entities", false);
    cEntities->setHeaderColor(Color(40, 42, 50, 255));
    cEntities->createChild<Label>("(none)")->setColor(Clr::dim);

    auto* cEdit = scene->createChild<Collapsible>("Mesh Edit");
    cEdit->setHeaderColor(Color(40, 42, 50, 255));
    cEdit->createChild<Label>("Vertices: 24  Faces: 6")->setColor(Clr::dim);
    cEdit->createChild<Button>("Recalc Normals")->setBgColor(Color(50, 52, 60, 255));
    cEdit->createChild<Button>("Flat Normals")->setBgColor(Color(50, 52, 60, 255));

    auto* cCSG = scene->createChild<Collapsible>("CSG", false);
    cCSG->setHeaderColor(Color(40, 42, 50, 255));
    cCSG->createChild<Label>("(empty)")->setColor(Clr::dim);

    // ── Inspector (Right) — with Collapsibles ────────────────────────────
    auto* insp = ed->set<BoxLayout>(BorderRegion::Right, LayoutDir::Vertical);
    insp->setSpacing(1);
    insp->setPadding(0);
    insp->createChild<Label>("Inspector")->setColor(Clr::accent);

    auto* cStats = insp->createChild<Collapsible>("Level Stats", false);
    cStats->setHeaderColor(Color(40, 42, 50, 255));
    cStats->createChild<Label>("Objects: 1")->setColor(Clr::dim);

    auto* cPivot = insp->createChild<Collapsible>("Creation Pivot");
    cPivot->setHeaderColor(Color(40, 42, 50, 255));
    cPivot->createChild<Label>("Position: 0 0 0")->setColor(Clr::dim);
    cPivot->createChild<Label>("Rotation: 0 0 0")->setColor(Clr::dim);

    auto* cLight = insp->createChild<Collapsible>("Lightmap", false);
    cLight->setHeaderColor(Color(40, 42, 50, 255));
    cLight->createChild<Label>("(not baked)")->setColor(Clr::dim);

    auto* cView = insp->createChild<Collapsible>("View Settings", false);
    cView->setHeaderColor(Color(40, 42, 50, 255));
    cView->createChild<CheckBox>("Wireframe");
    cView->createChild<CheckBox>("Grid");

    auto* cTex = insp->createChild<Collapsible>("Texture", false);
    cTex->setHeaderColor(Color(40, 42, 50, 255));
    cTex->createChild<Label>("(none selected)")->setColor(Clr::dim);

    // ── Assets (Bottom) ──────────────────────────────────────────────────
    auto* assets = ed->set<BoxLayout>(BorderRegion::Bottom, LayoutDir::Vertical);
    assets->setSpacing(2);
    assets->setPadding(4);
    auto* assetsHdr = assets->createChild<BoxLayout>(LayoutDir::Horizontal);
    assetsHdr->setSpacing(4);
    assetsHdr->createChild<Label>("Assets")->setColor(Clr::accent);
    assetsHdr->createChild<Button>("Folder...")->setBgColor(Color(50, 52, 58, 255));
    assetsHdr->createChild<Button>("Rescan")->setBgColor(Color(50, 52, 58, 255));
    // Asset thumbnails row
    auto* thumbs = assets->createChild<GridLayout>(6);
    thumbs->setStretch(1.0f);
    thumbs->setSpacing(3);
    for (int i = 0; i < 6; ++i)
    {
        auto* p = thumbs->createChild<Panel>();
        p->setBgColor(Color(55, 58, 65, 255));
    }

    // ── Viewport (Center) ────────────────────────────────────────────────
    auto* viewport = ed->set<GridLayout>(BorderRegion::Center, 2);
    viewport->setSpacing(2);
    viewport->setPadding(2);
    const char* views[] = {"Top [Wire]", "Front [Wire]", "Right [Wire]", "3D [Tex]"};
    for (int i = 0; i < 4; ++i)
    {
        auto* p = viewport->createChild<Panel>();
        p->setBgColor(Color(30, 32, 38, 255));
        p->createChild<Label>(views[i])->setColor(Color(120, 180, 100, 255));
    }

    // ── Status Bar ───────────────────────────────────────────────────────
    auto* status = outer->createChild<StatusBar>();
    status->setText("Ready | Objects: 1 | Verts: 24 | Faces: 6");
}
