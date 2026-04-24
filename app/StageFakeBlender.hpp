#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Fake Blender — 3D editor layout showcase
//  MenuBar + Toolbar | 3D Viewport + Outliner | Properties | Timeline
// ═════════════════════════════════════════════════════════════════════════════

inline void buildFakeBlenderStage(WidgetApp& app)
{
    Widget* root = app.addStage("fakeblender");

    auto* outer = root->createChild<BorderLayout>();
    outer->setSpacing(0);
    outer->setPadding(Edges(0));

    // ═══════ TOP: MenuBar + Toolbar ══════════════════════════════════════
    auto* topBar = outer->set<BoxLayout>(BorderRegion::Top, LayoutDir::Vertical);
    topBar->setSpacing(0);
    outer->setRegionSize(BorderRegion::Top, 52);

    auto* menuBar = topBar->createChild<MenuBar>();
    auto* fileMenu = menuBar->addMenu("File");
    fileMenu->addAction("New", []{}, "Ctrl+N");
    fileMenu->addAction("Open...", []{}, "Ctrl+O");
    fileMenu->addAction("Save", []{}, "Ctrl+S");
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", []{
        WidgetApp::instance().setStage("home", TransitionType::SlideRight, EaseType::OutCubic);
    }, "Ctrl+Q");

    auto* editMenu = menuBar->addMenu("Edit");
    editMenu->addAction("Undo", []{}, "Ctrl+Z");
    editMenu->addAction("Redo", []{}, "Ctrl+Shift+Z");
    editMenu->addSeparator();
    editMenu->addAction("Preferences...", []{});

    auto* viewMenu = menuBar->addMenu("View");
    viewMenu->addCheckable("Show Grid", true, [](bool){});
    viewMenu->addCheckable("Show Overlays", true, [](bool){});

    auto* windowMenu = menuBar->addMenu("Window");
    windowMenu->addAction("Toggle Fullscreen", []{}, "F11");

    auto* helpMenu = menuBar->addMenu("Help");
    helpMenu->addAction("About Blender (fake)", []{
        Toast::show("This is a BuGUI demo, not real Blender!", Toast::Info);
    });

    // Toolbar
    auto* toolbar = topBar->createChild<BoxLayout>(LayoutDir::Horizontal);
    toolbar->setSize(0, 26);
    toolbar->setSpacing(4);
    toolbar->setPadding(4, 4, 0, 4);

    auto* homeBtn = toolbar->createChild<Button>("< Home");
    homeBtn->setSize(70, 0);
    homeBtn->clicked.connect([]{
        WidgetApp::instance().setStage("home", TransitionType::SlideRight, EaseType::OutCubic);
    });
    toolbar->createChild<Line>(1.0f);

    const char* tools[] = {"Select", "Move", "Rotate", "Scale"};
    for (auto t : tools) {
        auto* b = toolbar->createChild<Button>(t);
        b->setSize(60, 0);
    }
    toolbar->createChild<Spacer>();
    toolbar->createChild<Label>("Object Mode")->setColor(Clr::dim);

    // ═══════ BOTTOM: Timeline ════════════════════════════════════════════
    auto* bottom = outer->set<BoxLayout>(BorderRegion::Bottom, LayoutDir::Vertical);
    outer->setRegionSize(BorderRegion::Bottom, 140);

    bottom->createChild<Line>();

    auto* timeHeader = bottom->createChild<BoxLayout>(LayoutDir::Horizontal);
    timeHeader->setSize(0, 24);
    timeHeader->setPadding(8, 8, 0, 8);
    timeHeader->setSpacing(8);
    timeHeader->createChild<Label>("Timeline")->setColor(Clr::accent);
    timeHeader->createChild<Spacer>();
    timeHeader->createChild<Button>("|<")->setSize(28, 0);
    timeHeader->createChild<Button>("<<")->setSize(28, 0);
    timeHeader->createChild<Button>(">")->setSize(28, 0);
    timeHeader->createChild<Button>(">>")->setSize(28, 0);
    timeHeader->createChild<Button>(">|")->setSize(28, 0);

    auto* frameRow = bottom->createChild<BoxLayout>(LayoutDir::Horizontal);
    frameRow->setPadding(8, 8, 4, 8);
    frameRow->setSpacing(8);
    frameRow->createChild<Label>("Frame:")->setColor(Clr::dim);
    auto* frameSlider = frameRow->createChild<Slider>(1, 250, 1);
    frameSlider->setStretch(1);
    auto* frameLabel = frameRow->createChild<Label>("1");
    frameLabel->setSize(40, 0);
    frameSlider->onValueChanged.connect([frameLabel](float v){
        frameLabel->setText(std::to_string(static_cast<int>(v)));
    });

    // Keyframe timeline area (canvas)
    auto* timeCanvas = bottom->createChild<Canvas>();
    timeCanvas->setStretch(1);
    timeCanvas->setBgColor(Color(28, 28, 32, 255));
    timeCanvas->setOnPaint([](PaintContext& ctx, const Rect& b){
        const auto& t = Theme::instance();
        // Draw frame markers
        for (int i = 0; i <= 250; i += 10) {
            float x = b.x + (static_cast<float>(i) / 250.0f) * b.w;
            ctx.line.SetColor(60, 60, 65, 255);
            ctx.line.Line2D(x, b.y, x, b.y + b.h);
            if (i % 50 == 0) {
                ctx.font.SetFontSize(10.0f);
                ctx.font.SetColor(Color(90, 90, 95, 255));
                ctx.font.SetBatch(&ctx.text);
                char buf[8]; snprintf(buf, 8, "%d", i);
                ctx.font.Print(buf, x + 2, b.y + 2);
            }
        }
        // Playhead
        ctx.line.SetColor(200, 60, 60, 255);
        float px = b.x + (1.0f / 250.0f) * b.w;
        ctx.line.Line2D(px, b.y, px, b.y + b.h);
    });

    // ═══════ RIGHT: Properties ═══════════════════════════════════════════
    auto* rightPanel = outer->set<BoxLayout>(BorderRegion::Right, LayoutDir::Vertical);
    outer->setRegionSize(BorderRegion::Right, 260);

    rightPanel->createChild<Line>();

    auto* propTabs = rightPanel->createChild<TabLayout>();
    propTabs->setStretch(1);

    // Properties tab
    auto* propGrid = propTabs->addTab<PropertyGrid>("Properties");
    propGrid->addSection("Transform");
    propGrid->addVec3("Location", 0, 0, 0, -100, 100);
    propGrid->addVec3("Rotation", 0, 0, 0, -360, 360);
    propGrid->addVec3("Scale", 1, 1, 1, 0.01f, 100);
    propGrid->addSection("Object");
    propGrid->addString("Name", "Cube");
    propGrid->addBool("Visible", true);
    propGrid->addColor("Color", Color(200, 200, 200, 255));
    propGrid->addCombo("Display As", {"Solid", "Wire", "Bounds"}, 0);

    // Modifiers tab
    auto* modLayout = propTabs->addTab<BoxLayout>("Modifiers", LayoutDir::Vertical);
    modLayout->setPadding(8);
    modLayout->setSpacing(4);
    auto* modCollapse = modLayout->createChild<Collapsible>("Subdivision Surface", true);
    auto* modContent = modCollapse->createChild<BoxLayout>(LayoutDir::Vertical);
    modContent->setSpacing(4);
    auto* modForm = modContent->createChild<FormLayout>();
    modForm->setLabelWidth(80);
    modForm->addRow<Slider>("Levels", 0, 6, 2);
    modForm->addRow<Slider>("Render", 0, 6, 3);
    modForm->addRow<CheckBox>("Optimal Display");

    // Constraints tab (empty placeholder)
    auto* constLayout = propTabs->addTab<BoxLayout>("Constraints", LayoutDir::Vertical);
    constLayout->setPadding(12);
    constLayout->createChild<Label>("No constraints")->setColor(Clr::dim);

    // ═══════ CENTER: Splitter (3D Viewport | Outliner) ═══════════════════
    auto* center = outer->set<Splitter>(BorderRegion::Center, LayoutDir::Horizontal);
    center->setRatio(0.72f);
    center->setMinRatio(0.3f);
    center->setMaxRatio(0.9f);

    // 3D Viewport
    auto* viewport = center->createChild<BoxLayout>(LayoutDir::Vertical);
    viewport->setSpacing(0);

    auto* vpHeader = viewport->createChild<BoxLayout>(LayoutDir::Horizontal);
    vpHeader->setSize(0, 22);
    vpHeader->setPadding(8, 4, 0, 4);
    vpHeader->createChild<Label>("3D Viewport")->setColor(Clr::accent);
    vpHeader->createChild<Spacer>();
    vpHeader->createChild<Label>("Perspective")->setColor(Clr::dim);

    auto* vpCanvas = viewport->createChild<Canvas>();
    vpCanvas->setStretch(1);
    vpCanvas->setBgColor(Color(35, 35, 40, 255));
    vpCanvas->setOnPaint([](PaintContext& ctx, const Rect& b){
        // Grid
        float cx = b.x + b.w * 0.5f, cy = b.y + b.h * 0.5f;
        for (int i = -10; i <= 10; ++i) {
            float off = i * 20.0f;
            ctx.line.SetColor(50, 50, 55, i == 0 ? 200 : 100);
            ctx.line.Line2D(cx + off, b.y + 40, cx + off, b.y + b.h - 20);
            ctx.line.Line2D(b.x + 20, cy + off, b.x + b.w - 20, cy + off);
        }
        // X/Y axes
        ctx.line.SetColor(180, 50, 50, 200);
        ctx.line.Line2D(cx, cy, cx + 120, cy); // X
        ctx.line.SetColor(50, 180, 50, 200);
        ctx.line.Line2D(cx, cy, cx, cy - 120); // Y
        ctx.line.SetColor(50, 50, 180, 200);
        ctx.line.Line2D(cx, cy, cx - 50, cy + 50); // Z (fake perspective)

        // Cube wireframe (centered)
        float s = 40;
        float x0 = cx - s, x1 = cx + s, y0 = cy - s, y1 = cy + s;
        float zx = -15, zy = 15; // fake depth offset
        ctx.line.SetColor(220, 220, 225, 200);
        // Front face
        ctx.line.Line2D(x0, y0, x1, y0);
        ctx.line.Line2D(x1, y0, x1, y1);
        ctx.line.Line2D(x1, y1, x0, y1);
        ctx.line.Line2D(x0, y1, x0, y0);
        // Back face
        ctx.line.Line2D(x0+zx, y0+zy, x1+zx, y0+zy);
        ctx.line.Line2D(x1+zx, y0+zy, x1+zx, y1+zy);
        ctx.line.Line2D(x1+zx, y1+zy, x0+zx, y1+zy);
        ctx.line.Line2D(x0+zx, y1+zy, x0+zx, y0+zy);
        // Edges connecting front to back
        ctx.line.Line2D(x0, y0, x0+zx, y0+zy);
        ctx.line.Line2D(x1, y0, x1+zx, y0+zy);
        ctx.line.Line2D(x1, y1, x1+zx, y1+zy);
        ctx.line.Line2D(x0, y1, x0+zx, y1+zy);

        // Origin dot
        ctx.fill.SetColor(255, 120, 40, 255);
        ctx.fill.Circle(static_cast<int>(cx), static_cast<int>(cy), 4, true);

        // 3D cursor
        ctx.font.SetFontSize(11.0f);
        ctx.font.SetColor(Color(80, 80, 85, 255));
        ctx.font.SetBatch(&ctx.text);
        ctx.font.Print("Cube selected | Verts:8 Edges:12 Faces:6", b.x + 10, b.y + b.h - 20);
    });

    // Outliner
    auto* outlinerBox = center->createChild<BoxLayout>(LayoutDir::Vertical);
    outlinerBox->setSpacing(0);

    auto* outHeader = outlinerBox->createChild<BoxLayout>(LayoutDir::Horizontal);
    outHeader->setSize(0, 22);
    outHeader->setPadding(8, 4, 0, 4);
    outHeader->createChild<Label>("Outliner")->setColor(Clr::accent);

    auto* tree = outlinerBox->createChild<TreeView>();
    tree->setStretch(1);
    auto* scene = tree->addRoot("Scene Collection");
    scene->setExpanded(true);
    scene->setIcon(IconId::Folder);
    auto* cam = scene->addChild("Camera");
    cam->setIcon(IconId::File);
    auto* light = scene->addChild("Light");
    light->setIcon(IconId::File);
    auto* cube = scene->addChild("Cube");
    cube->setIcon(IconId::File);
    tree->setSelectedNode(cube);

    auto* empty = scene->addChild("Empty");
    empty->setIcon(IconId::File);
    auto* plane = scene->addChild("Plane");
    plane->setIcon(IconId::File);
}
