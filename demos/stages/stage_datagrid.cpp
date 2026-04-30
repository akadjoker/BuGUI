#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "DataWidgets.hpp"
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  Stage 10 — DataGrid + TreeGrid
// ─────────────────────────────────────────────────────────────────────────────

static Label* s_dgStatus = nullptr;

void registerDataGridStage(WidgetApp& app)
{
    s_dgStatus = nullptr;

    auto* root = app.addStage("datagrid");
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(0.0f);
    vbox->setPadding(0.0f);

    // ── Top bar ────────────────────────────────────────────────────────────
    auto* topBar = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSpacing(8.0f);
    topBar->setPadding(Edges(8.0f, 12.0f));

    auto* back = topBar->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
    topBar->createChild<Spacer>(0.0f)->setStretch(1.0f);
    auto* titleLbl = topBar->createChild<Label>("10 \u2014 DataGrid / TreeGrid");
    titleLbl->setAlign(TextAlign::RIGHT);

    vbox->createChild<Line>();

    // ── Main content: 2 columns ────────────────────────────────────────────
    auto* cols = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    cols->setSpacing(10.0f);
    cols->setPadding(12.0f);
    cols->setStretch(1.0f);

    // ── Left: DataGrid ─────────────────────────────────────────────────────
    auto* leftCol = cols->createChild<BoxLayout>(LayoutDir::Vertical);
    leftCol->setSpacing(6.0f);
    leftCol->setPadding(0.0f);
    leftCol->setStretch(1.0f);

    leftCol->createChild<Label>("DataGrid  (double-click to edit, Ctrl+C copy, Shift+scroll H)");
    leftCol->createChild<Line>();

    auto* grid = leftCol->createChild<DataGrid>();
    grid->setStretch(1.0f);
    grid->setShowCheckboxes(true);
    grid->setZebraStripes(true);
    grid->setMultiSelect(true);

    grid->addColumn("Name",       180.f, true);
    grid->addColumn("Type",        90.f, true);
    grid->addColumn("Size (KB)",  100.f, true);
    grid->addColumn("Modified",   130.f, true);
    grid->addColumn("Status",      90.f, false);

    // Sample data — file browser style
    struct Row { const char* name; const char* type; const char* size; const char* date; const char* status; };
    static const Row kRows[] = {
        {"main.cpp",           "C++ Source",  "12",   "2026-04-27", "OK"},
        {"Widget.hpp",         "C++ Header",   "8",   "2026-04-26", "OK"},
        {"DataWidgets.cpp",    "C++ Source",  "24",   "2026-04-27", "OK"},
        {"DataWidgets.hpp",    "C++ Header",   "5",   "2026-04-27", "OK"},
        {"CMakeLists.txt",     "CMake",         "2",   "2026-04-25", "OK"},
        {"layout.json",        "JSON",          "1",   "2026-04-20", "OK"},
        {"pch.hpp",            "C++ Header",   "1",   "2026-04-10", "OK"},
        {"Font.cpp",           "C++ Source",  "18",   "2026-04-22", "OK"},
        {"Texture.cpp",        "C++ Source",  "11",   "2026-04-21", "Modified"},
        {"Batch.cpp",          "C++ Source",  "30",   "2026-04-23", "OK"},
        {"RenderState.cpp",    "C++ Source",   "9",   "2026-04-18", "OK"},
        {"Input.cpp",          "C++ Source",   "7",   "2026-04-17", "Error"},
        {"FileSystem.cpp",     "C++ Source",  "15",   "2026-04-15", "OK"},
        {"Device.cpp",         "C++ Source",  "22",   "2026-04-14", "OK"},
        {"glad.cpp",           "C Source",    "40",   "2026-03-01", "OK"},
        {"stb_image.h",        "C Header",    "85",   "2025-12-01", "OK"},
        {"nlohmann/json.hpp",  "C++ Header", "120",   "2025-11-10", "OK"},
    };
    for (auto& r : kRows)
        grid->addRow({r.name, r.type, r.size, r.date, r.status});

    // Status bar under DataGrid
    auto* dgStatus = leftCol->createChild<Label>("Select a row to see details");
    grid->selectionChanged.connect([dgStatus, grid](int row) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Selected row %d: %s (%s KB)",
            row, grid->cell(row, 0).c_str(), grid->cell(row, 2).c_str());
        dgStatus->setText(buf);
    });
    grid->cellEdited.connect([dgStatus, grid](int row, int col) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Edited [%d,%d] = \"%s\"",
            row, col, grid->cell(row, col).c_str());
        dgStatus->setText(buf);
    });
    grid->rowCheckChanged.connect([dgStatus, grid](int row, bool checked) {
        char buf[128];
        auto checked_rows = grid->checkedRows();
        std::snprintf(buf, sizeof(buf), "Row %d %s  (%d checked total)",
            row, checked ? "checked" : "unchecked", (int)checked_rows.size());
        dgStatus->setText(buf);
    });

    // ── Right: TreeGrid ────────────────────────────────────────────────────
    auto* rightCol = cols->createChild<BoxLayout>(LayoutDir::Vertical);
    rightCol->setSpacing(6.0f);
    rightCol->setPadding(0.0f);
    rightCol->setStretch(1.0f);

    rightCol->createChild<Label>("TreeGrid  (click arrows to expand, double-click to edit)");
    rightCol->createChild<Line>();

    auto* tg = rightCol->createChild<TreeGrid>();
    tg->setStretch(1.0f);
    tg->setZebraStripes(true);

    tg->addColumn("Name",     200.f, true);
    tg->addColumn("Kind",      90.f, true);
    tg->addColumn("Size",      80.f, true);
    tg->addColumn("Read-only", 80.f, false);

    // Build scene hierarchy
    auto* sceneRoot = tg->addRoot({"Scene", "Group", "", "No"});
    {
        auto* env = sceneRoot->addChild({"Environment", "Group", "", "No"});
        {
            env->addChild({"Sun",         "Light",  "—", "No"});
            env->addChild({"Sky",         "Sky",    "—", "No"});
            env->addChild({"Fog",         "Volume", "—", "Yes"});
        }
        auto* cam = sceneRoot->addChild({"Camera", "Camera", "—", "No"});
        {
            cam->addChild({"Lens", "Component", "—", "No"});
            cam->addChild({"DOF",  "Component", "—", "No"});
        }
        auto* geom = sceneRoot->addChild({"Geometry", "Group", "", "No"});
        {
            auto* car = geom->addChild({"Car", "Mesh", "1.2 MB", "No"});
            {
                car->addChild({"Body",    "SubMesh", "600 KB", "No"});
                car->addChild({"Glass",   "SubMesh", "200 KB", "No"});
                car->addChild({"Wheels",  "SubMesh", "400 KB", "No"});
            }
            geom->addChild({"Ground",   "Mesh", "300 KB", "Yes"});
            geom->addChild({"Trees",    "Mesh", "800 KB", "No"});
            geom->addChild({"Buildings","Mesh", "2.1 MB", "No"});
        }
        auto* lights = sceneRoot->addChild({"Lights", "Group", "", "No"});
        {
            lights->addChild({"Point_0", "PointLight",  "—", "No"});
            lights->addChild({"Point_1", "PointLight",  "—", "No"});
            lights->addChild({"Spot_0",  "SpotLight",   "—", "No"});
        }
        auto* ui = sceneRoot->addChild({"UI", "Canvas", "", "No"});
        {
            ui->addChild({"HUD",     "Panel", "—", "No"});
            ui->addChild({"Minimap", "Panel", "—", "No"});
        }
    }

    // Expand top-level by default (rebuild happens automatically)
    sceneRoot->setExpanded(true);

    auto* tgStatus = rightCol->createChild<Label>("Click a node to select");
    tg->selectionChanged.connect([tgStatus](TreeGrid::Node* n) {
        if (!n) return;
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Selected: %s  [%s]",
            n->cells.empty() ? "?" : n->cells[0].c_str(),
            n->cells.size() > 1 ? n->cells[1].c_str() : "");
        tgStatus->setText(buf);
    });
    tg->cellEdited.connect([tgStatus](TreeGrid::Node* n, int col) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Edited col %d of \"%s\"",
            col, n->cells.empty() ? "?" : n->cells[0].c_str());
        tgStatus->setText(buf);
    });

    // ── Status bar ─────────────────────────────────────────────────────────
    vbox->createChild<Line>();
    auto* statusBar = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    statusBar->setSpacing(8.0f);
    statusBar->setPadding(Edges(6.0f, 8.0f));
    s_dgStatus = statusBar->createChild<Label>("DataGrid: 17 rows | TreeGrid: scene hierarchy");
    s_dgStatus->setStretch(1.0f);
}
