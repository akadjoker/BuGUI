#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Home — navigation hub for all demos and app showcases
// ═════════════════════════════════════════════════════════════════════════════

namespace {

struct DemoEntry {
    const char* name;
    const char* stage;
    const char* desc;
    Color       color;
};

static const DemoEntry kApps[] = {
    {"Blender",       "fakeblender",  "3D editor layout",         Color(230, 120,  40, 255)},
    {"Tiled",         "faketiled",    "Tile map editor",          Color( 80, 180,  80, 255)},
};

static const DemoEntry kDemos[] = {
    {"Widgets",       "widgets",      "Buttons, sliders, toggles", Color(100, 160, 220, 255)},
    {"Layouts",       "layout",       "Box, grid, flow layouts",   Color(160, 100, 220, 255)},
    {"Layouts 2",     "layouts2",     "Form, stack, anchor",       Color(140, 110, 200, 255)},
    {"Scroll",        "scroll",       "ScrollView, ListBox",       Color(100, 200, 160, 255)},
    {"Tabs",          "tabs",         "TabLayout + Carousel",      Color(200, 160, 100, 255)},
    {"Inputs",        "inputs",       "TextInput, TextEdit",       Color(180, 130, 180, 255)},
    {"Canvas",        "canvas",       "Custom paint + ImageView",  Color(130, 180, 130, 255)},
    {"Border",        "border",       "BorderLayout regions",      Color(180, 180, 100, 255)},
    {"Grid",          "grid",         "GridLayout demo",           Color(100, 180, 180, 255)},
    {"Anchor",        "anchor",       "AnchorLayout rules",        Color(180, 100, 140, 255)},
    {"Tree/Prop",     "treepropcolor","TreeView, PropertyGrid",    Color(130, 160, 200, 255)},
    {"DataGrid",      "datagrid",     "Spreadsheet table",         Color(200, 140, 100, 255)},
    {"TreeGrid",      "treegrid",     "Hierarchical table",        Color(140, 200, 140, 255)},
    {"FileDialog",    "filedialog",   "File picker + MessageBox",  Color(200, 120, 160, 255)},
    {"Calculator",    "calc",         "Calculator app",            Color(160, 160, 160, 255)},
    {"Demo",          "demo",         "Kitchen sink (MenuBar)",    Color(120, 140, 180, 255)},
    {"Viewport",      "viewport",     "RenderTexture (FBO)",       Color(220, 180,  60, 255)},
    {"Gizmo",          "gizmo",         "2D + 3D transform gizmos",  Color(180, 220,  60, 255)},
    {"Editors",        "editorwidgets", "Curve, Node, Timeline",    Color( 60, 180, 200, 255)},
    {"Charts",         "charts",        "Gradient, Plot, Histogram", Color(200, 120, 220, 255)},
    {"Sound",          "sound",         "VUMeter, Waveform, Spectrum, ADSR, PianoRoll", Color(80, 180, 200, 255)},
    {"Assets",         "assetinspector","AssetBrowser + InspectorPanel",              Color(180, 200,  80, 255)},
};

// Draw a card button for a demo entry
static Button* makeCard(Widget* parent, const DemoEntry& e)
{
    auto* card = parent->createChild<BoxLayout>(LayoutDir::Vertical);
    card->setSize(140, 80);
    card->setSpacing(2);
    card->setPadding(0);

    auto* btn = card->createChild<Button>(e.name);
    btn->setStretch(1);
    btn->setBgColor(Color(e.color.r, e.color.g, e.color.b, 40));

    auto* desc = card->createChild<Label>(e.desc);
    desc->setColor(Clr::dim);

    const char* stage = e.stage;
    btn->clicked.connect([stage]{
        WidgetApp::instance().setStage(stage, TransitionType::SlideLeft, EaseType::OutCubic);
    });

    return btn;
}

} // anon namespace

inline void buildHomeStage(WidgetApp& app)
{
    Widget* root = app.addStage("home");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Header ───────────────────────────────────────────────────────────
    auto* header = outer->createChild<BoxLayout>(LayoutDir::Vertical);
    header->setPadding(24, 24, 12, 24);
    header->setSpacing(4);

    auto* title = header->createChild<Label>("BuGUI");
    title->setColor(Clr::accent);

    header->createChild<Label>("Lightweight C++17 Widget Library — SDL2 + OpenGL")->setColor(Clr::dim);

    // ── Apps section ─────────────────────────────────────────────────────
    auto* body = outer->createChild<BoxLayout>(LayoutDir::Vertical);
    body->setStretch(1);
    body->setPadding(24, 24, 16, 24);
    body->setSpacing(12);

    body->createChild<Label>("App Showcases")->setColor(Clr::section);

    auto* appFlow = body->createChild<FlowLayout>();
    appFlow->setSpacing(10, 10);
    appFlow->setSize(0, 100);
    for (auto& e : kApps)
        makeCard(appFlow, e);

    // ── Demos section ────────────────────────────────────────────────────
    body->createChild<Line>();
    body->createChild<Label>("Widget Demos")->setColor(Clr::section);

    auto* demoFlow = body->createChild<FlowLayout>();
    demoFlow->setSpacing(10, 10);
    demoFlow->setStretch(1);
    for (auto& e : kDemos)
        makeCard(demoFlow, e);
}
