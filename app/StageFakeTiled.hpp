#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Fake Tiled — tile map editor layout showcase
//  MenuBar + Toolbar | Tile Canvas + Tileset + Layers | Properties
// ═════════════════════════════════════════════════════════════════════════════

inline void buildFakeTiledStage(WidgetApp& app)
{
    Widget* root = app.addStage("faketiled");

    auto* outer = root->createChild<BorderLayout>();
    outer->setSpacing(0);
    outer->setPadding(Edges(0));

    // ═══════ TOP: Menu + Toolbar ═════════════════════════════════════════
    auto* topBar = outer->set<BoxLayout>(BorderRegion::Top, LayoutDir::Vertical);
    topBar->setSpacing(0);
    outer->setRegionSize(BorderRegion::Top, 52);

    auto* menuBar = topBar->createChild<MenuBar>();
    auto* fileMenu = menuBar->addMenu("File");
    fileMenu->addAction("New Map...", []{}, "Ctrl+N");
    fileMenu->addAction("Open Map...", []{}, "Ctrl+O");
    fileMenu->addAction("Save", []{}, "Ctrl+S");
    fileMenu->addAction("Export As Image...", []{});
    fileMenu->addSeparator();
    fileMenu->addAction("Close", []{
        WidgetApp::instance().setStage("home", TransitionType::SlideRight, EaseType::OutCubic);
    });

    auto* editMenu = menuBar->addMenu("Edit");
    editMenu->addAction("Undo", []{}, "Ctrl+Z");
    editMenu->addAction("Redo", []{}, "Ctrl+Y");
    editMenu->addSeparator();
    editMenu->addAction("Select All", []{}, "Ctrl+A");

    auto* mapMenu = menuBar->addMenu("Map");
    mapMenu->addAction("Resize Map...", []{});
    mapMenu->addAction("Map Properties...", []{});

    auto* viewMenu = menuBar->addMenu("View");
    viewMenu->addCheckable("Show Grid", true, [](bool){});
    viewMenu->addCheckable("Show Tile Animations", false, [](bool){});
    viewMenu->addCheckable("Highlight Current Layer", true, [](bool){});

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

    const char* tileTools[] = {"Stamp", "Terrain", "Bucket", "Eraser", "Select"};
    for (auto t : tileTools) {
        auto* b = toolbar->createChild<Button>(t);
        b->setSize(60, 0);
    }
    toolbar->createChild<Spacer>();

    auto* zoomLabel = toolbar->createChild<Label>("100%");
    zoomLabel->setColor(Clr::dim);

    // ═══════ BOTTOM: StatusBar ═══════════════════════════════════════════
    auto* statusBar = outer->set<StatusBar>(BorderRegion::Bottom);
    statusBar->setText("Tile: 5, 3");
    auto* statusInfo = statusBar->createChild<Label>("Map: 20x15 (Orthogonal)  |  Tile size: 32x32  |  Stamp Brush");
    statusInfo->setColor(Clr::dim);

    // ═══════ LEFT: Layers + Tileset ══════════════════════════════════════
    auto* leftPanel = outer->set<BoxLayout>(BorderRegion::Left, LayoutDir::Vertical);
    outer->setRegionSize(BorderRegion::Left, 220);
    leftPanel->setSpacing(0);

    // Layers section
    auto* layersCollapse = leftPanel->createChild<Collapsible>("Layers", true);
    auto* layersBox = layersCollapse->createChild<BoxLayout>(LayoutDir::Vertical);
    layersBox->setSpacing(4);
    layersBox->setPadding(4);
    layersBox->setSize(0, 120);

    auto* layerList = layersBox->createChild<ListWidget>();
    layerList->setStretch(1);
    layerList->addRow<Label>("Objects");
    layerList->addRow<Label>("Foreground");
    layerList->addRow<Label>("Buildings");
    layerList->addRow<Label>("Ground");
    layerList->addRow<Label>("Background");
    layerList->setSelectedIndex(3); // Ground

    auto* layerBtns = layersBox->createChild<BoxLayout>(LayoutDir::Horizontal);
    layerBtns->setSpacing(4);
    layerBtns->createChild<Button>("+")->setSize(28, 0);
    layerBtns->createChild<Button>("-")->setSize(28, 0);
    layerBtns->createChild<Button>("^")->setSize(28, 0);
    layerBtns->createChild<Button>("v")->setSize(28, 0);
    layerBtns->createChild<Spacer>();

    leftPanel->createChild<Line>();

    // Tileset section
    auto* tilesetCollapse = leftPanel->createChild<Collapsible>("Tileset: terrain", true);
    auto* tilesetArea = tilesetCollapse->createChild<Canvas>();
    tilesetArea->setStretch(1);
    tilesetArea->setBgColor(Color(45, 45, 50, 255));
    tilesetArea->setOnPaint([](PaintContext& ctx, const Rect& b){
        // Draw a fake tileset grid
        int tileSize = 24;
        int cols = static_cast<int>(b.w) / tileSize;
        int rows = static_cast<int>(b.h) / tileSize;
        Color colors[] = {
            Color(60, 120, 60, 255),  // grass
            Color(100, 80, 50, 255),  // dirt
            Color(70, 70, 180, 255),  // water
            Color(130, 130, 130, 255),// stone
            Color(50, 90, 50, 255),   // dark grass
            Color(180, 160, 120, 255),// sand
        };
        int numColors = 6;
        int idx = 0;
        for (int r = 0; r < rows && r < 8; ++r) {
            for (int c = 0; c < cols && c < 8; ++c) {
                float x = b.x + c * tileSize;
                float y = b.y + r * tileSize;
                ctx.fill.SetColor(colors[idx % numColors]);
                ctx.fill.Rectangle(static_cast<int>(x + 1), static_cast<int>(y + 1), tileSize - 2, tileSize - 2, true);
                // Grid lines
                ctx.line.SetColor(30, 30, 35, 200);
                ctx.line.Line2D(x, b.y, x, b.y + rows * tileSize);
                ++idx;
            }
        }
        // Highlight selected tile (tile 0,0)
        ctx.line.SetColor(255, 255, 100, 200);
        ctx.line.Line2D(b.x, b.y, b.x + tileSize, b.y);
        ctx.line.Line2D(b.x + tileSize, b.y, b.x + tileSize, b.y + tileSize);
        ctx.line.Line2D(b.x + tileSize, b.y + tileSize, b.x, b.y + tileSize);
        ctx.line.Line2D(b.x, b.y + tileSize, b.x, b.y);
    });

    // ═══════ RIGHT: Properties ═══════════════════════════════════════════
    auto* rightPanel = outer->set<BoxLayout>(BorderRegion::Right, LayoutDir::Vertical);
    outer->setRegionSize(BorderRegion::Right, 220);
    rightPanel->setSpacing(0);

    auto* propCollapse = rightPanel->createChild<Collapsible>("Map Properties", true);
    auto* propGrid = propCollapse->createChild<PropertyGrid>();
    propGrid->addSection("Map");
    propGrid->addString("Name", "demo_map");
    propGrid->addCombo("Orientation", {"Orthogonal", "Isometric", "Hexagonal"}, 0);
    propGrid->addVec2("Size", 20, 15, 1, 100);
    propGrid->addVec2("Tile Size", 32, 32, 8, 128);

    propGrid->addSection("Layer: Ground");
    propGrid->addString("Name", "Ground");
    propGrid->addBool("Visible", true);
    propGrid->addBool("Locked", false);

    rightPanel->createChild<Line>();

    auto* objCollapse = rightPanel->createChild<Collapsible>("Objects", false);
    auto* objTree = objCollapse->createChild<TreeView>();
    objTree->setSize(0, 150);
    auto* objRoot = objTree->addRoot("Objects Layer");
    objRoot->setExpanded(true);
    objRoot->addChild("Spawn Point");
    objRoot->addChild("Chest_01");
    objRoot->addChild("NPC_Merchant");
    objRoot->addChild("Trigger_Zone_1");

    // ═══════ CENTER: Tile Map Canvas ═════════════════════════════════════
    auto* center = outer->set<Canvas>(BorderRegion::Center);
    center->setBgColor(Color(30, 30, 35, 255));
    center->setOnPaint([](PaintContext& ctx, const Rect& b){
        int tileSize = 32;
        int mapW = 20, mapH = 15;

        float mapPixW = mapW * tileSize;
        float mapPixH = mapH * tileSize;
        float ox = b.x + (b.w - mapPixW) * 0.5f;
        float oy = b.y + (b.h - mapPixH) * 0.5f;

        // Pseudo-random tile colors for the map
        Color grass(60, 120, 60, 255);
        Color dirt(100, 80, 50, 255);
        Color water(70, 70, 180, 255);
        Color stone(130, 130, 130, 255);
        Color sand(180, 160, 120, 255);

        for (int r = 0; r < mapH; ++r) {
            for (int c = 0; c < mapW; ++c) {
                float x = ox + c * tileSize;
                float y = oy + r * tileSize;
                // Simple deterministic pattern
                int hash = (c * 7 + r * 13) % 17;
                Color col = grass;
                if (hash < 3) col = dirt;
                else if (hash < 5) col = water;
                else if (hash < 7) col = stone;
                else if (hash < 8) col = sand;
                // Path through middle
                if (r == 7 || (c == 10 && r >= 3 && r <= 12)) col = dirt;
                // Water pond
                if (c >= 14 && c <= 17 && r >= 2 && r <= 5) col = water;

                ctx.fill.SetColor(col);
                ctx.fill.Rectangle(static_cast<int>(x + 0.5f), static_cast<int>(y + 0.5f), tileSize - 1, tileSize - 1, true);
            }
        }

        // Grid overlay
        ctx.line.SetColor(20, 20, 25, 120);
        for (int c = 0; c <= mapW; ++c) {
            float x = ox + c * tileSize;
            ctx.line.Line2D(x, oy, x, oy + mapPixH);
        }
        for (int r = 0; r <= mapH; ++r) {
            float y = oy + r * tileSize;
            ctx.line.Line2D(ox, y, ox + mapPixW, y);
        }

        // Cursor highlight on tile 5,3
        float hx = ox + 5 * tileSize, hy = oy + 3 * tileSize;
        ctx.line.SetColor(255, 255, 100, 200);
        ctx.line.Line2D(hx, hy, hx + tileSize, hy);
        ctx.line.Line2D(hx + tileSize, hy, hx + tileSize, hy + tileSize);
        ctx.line.Line2D(hx + tileSize, hy + tileSize, hx, hy + tileSize);
        ctx.line.Line2D(hx, hy + tileSize, hx, hy);

        // Object markers
        ctx.fill.SetColor(255, 200, 50, 200);
        ctx.fill.Circle(static_cast<int>(ox + 3 * tileSize + 16), static_cast<int>(oy + 10 * tileSize + 16), 6, true); // spawn
        ctx.fill.SetColor(200, 100, 255, 200);
        ctx.fill.Circle(static_cast<int>(ox + 12 * tileSize + 16), static_cast<int>(oy + 5 * tileSize + 16), 6, true); // chest

        // Coordinates display
        ctx.font.SetFontSize(11.0f);
        ctx.font.SetColor(Color(70, 70, 75, 255));
        ctx.font.SetBatch(&ctx.text);
        ctx.font.Print("20x15 Orthogonal | Tile 32x32 | Zoom 100%", b.x + 8, b.y + b.h - 16);
    });
}
