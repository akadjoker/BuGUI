#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "ToolBar.hpp"
#include "ConsoleWidget.hpp"
#include "IconAtlas.hpp"
#include "BuImage.hpp"
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  Stage — ToolBar demo
//
//  Tests the ToolBar widget using IconAtlas icons:
//  - Push buttons with atlas icons
//  - Toggle buttons
//  - Separators (draggable)
//  - Spacers
//  - Drag reorder
//  - Enable/disable items
//  - Console log of all events
// ─────────────────────────────────────────────────────────────────────────────

void registerToolBarStage(WidgetApp& app)
{
    auto* root = app.addStage("toolbar");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setStretch(1);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Header ───────────────────────────────────────────────────────────
    auto* header = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSize(0, 32);
    header->setPadding(Edges(4, 4, 4, 4));
    header->setSpacing(8);
    {
        auto* back = header->createChild<Button>("\u2190 Menu");
        back->setSize(80, 24);
        back->clicked.connect([&app]() {
            app.setStage("menu", TransitionType::CoverRight);
        });
        header->createChild<Line>();
        header->createChild<Label>("ToolBar Widget Demo");
        header->createChild<Spacer>(0.f)->setStretch(1);
    }

    // ── ToolBar 1: File operations (atlas icons) ─────────────────────────
    auto* tb1 = outer->createChild<ToolBar>();
    tb1->setSize(0, 34);
    tb1->setButtonSize(28);

    int bNew    = tb1->addButton(IconId::File,       "New File");
    int bOpen   = tb1->addButton(IconId::FolderOpen, "Open");
    int bSave   = tb1->addButton(IconId::Edit,       "Save");
    tb1->addSeparator();
    int bCut    = tb1->addButton(IconId::Cross,      "Cut");
    int bCopy   = tb1->addButton(IconId::Check,      "Copy");
    int bPaste  = tb1->addButton(IconId::Plus,       "Paste");
    tb1->addSeparator();
    int bUndo   = tb1->addButton(IconId::ArrowLeft,  "Undo");
    int bRedo   = tb1->addButton(IconId::ArrowRight, "Redo");
    tb1->addSpacer();
    int bSearch = tb1->addButton(IconId::Search,     "Search");
    int bHome   = tb1->addButton(IconId::Home,       "Home");

    // ── ToolBar 2: Toggle tools (atlas icons) ────────────────────────────
    auto* tb2 = outer->createChild<ToolBar>();
    tb2->setSize(0, 34);
    tb2->setButtonSize(28);

    int tBold   = tb2->addToggle(IconId::Edit,    "Bold",      false);
    int tItal   = tb2->addToggle(IconId::Book,    "Italic",    false);
    int tUnder  = tb2->addToggle(IconId::Minus,   "Underline", false);
    tb2->addSeparator();
    int tGrid   = tb2->addToggle(IconId::ViewGrid,   "Grid View",   true);
    int tList   = tb2->addToggle(IconId::ViewList,   "List View",   false);
    int tDetail = tb2->addToggle(IconId::ViewDetail, "Detail View", false);
    tb2->addSeparator();
    int tLock   = tb2->addToggle(IconId::Lock,    "Lock Layer",  false);
    int tEye    = tb2->addToggle(IconId::Eye,     "Visible",     true);

    // ── ToolBar 3: Transport controls ────────────────────────────────────
    auto* tb3 = outer->createChild<ToolBar>();
    tb3->setSize(0, 34);
    tb3->setButtonSize(28);

    int bStepBack = tb3->addButton(IconId::StepBack,    "Step Back");
    int bStop     = tb3->addButton(IconId::Stop,        "Stop");
    int bPlay     = tb3->addButton(IconId::Play,        "Play");
    int bPause    = tb3->addButton(IconId::Pause,       "Pause");
    int bStepFwd  = tb3->addButton(IconId::StepForward, "Step Forward");
    tb3->addSeparator();
    int bRecord   = tb3->addButton(IconId::Record,      "Record");
    tb3->addSpacer();
    int bRefresh  = tb3->addButton(IconId::Refresh,     "Refresh");
    int bTrash    = tb3->addButton(IconId::Trash,       "Delete");

    // Start with Pause disabled
    tb3->setItemEnabled(bPause, false);

    // ── ToolBar 4: Sprite-sheet grid (programmatic test) ─────────────────
    // Build an 8×2 grid of 24×24 colored cells as a BuImage sprite sheet
    constexpr int cellSz = 24;
    constexpr int cols   = 8;
    constexpr int rows   = 2;
    auto* sheet = new BuGUI::BuImage(cols * cellSz, rows * cellSz, 4);
    sheet->Fill(40, 40, 50, 255);   // dark bg
    {
        // 16 unique colored cells with simple shapes
        BuGUI::Color palette[16] = {
            {220, 60, 60,255},  {60,180, 60,255},  {60, 80,220,255},  {220,180, 40,255},
            {180, 60,200,255},  {60,200,200,255},  {220,140, 40,255},  {160,160,170,255},
            {255,100,100,255},  {100,220,100,255},  {100,140,255,255},  {255,220, 80,255},
            {220,100,255,255},  {100,240,240,255},  {255,180, 80,255},  {200,200,210,255},
        };
        for (int i = 0; i < cols * rows; ++i) {
            int cx = (i % cols) * cellSz;
            int cy = (i / cols) * cellSz;
            auto& c = palette[i];
            // Filled square with 2px margin
            sheet->DrawRect(cx + 3, cy + 3, cellSz - 6, cellSz - 6, c, true);
            // Small inner circle for visual distinction
            sheet->DrawCircle(cx + cellSz/2, cy + cellSz/2, 4,
                              BuGUI::Color{255,255,255,180}, true);
        }
    }
    auto* tb4 = outer->createChild<ToolBar>();
    tb4->setSize(0, 34);
    tb4->setButtonSize(28);
    tb4->setImageGrid(sheet, cellSz, cellSz);  // ToolBar takes ownership

    // Add buttons referencing grid cell indices
    int g0 = tb4->addButton(0,  "Red");
    int g1 = tb4->addButton(1,  "Green");
    int g2 = tb4->addButton(2,  "Blue");
    int g3 = tb4->addButton(3,  "Yellow");
    tb4->addSeparator();
    int g4 = tb4->addButton(4,  "Purple");
    int g5 = tb4->addButton(5,  "Cyan");
    int g6 = tb4->addButton(6,  "Orange");
    int g7 = tb4->addButton(7,  "Gray");
    tb4->addSpacer();
    int g8 = tb4->addButton(8,  "Light Red");
    int g9 = tb4->addButton(9,  "Light Green");
    int g10= tb4->addButton(10, "Light Blue");
    int g11= tb4->addButton(11, "Light Yellow");

    // ── Console (event log) ──────────────────────────────────────────────
    auto* con = outer->createChild<ConsoleWidget>();
    con->setStretch(1);
    con->log(LogLevel::Info, "ToolBar demo loaded. Click buttons, toggle items, drag to reorder.");
    con->log(LogLevel::Info, "Toolbar 1: File ops | Toolbar 2: Toggles | Toolbar 3: Transport");
    con->log(LogLevel::Info, "Toolbar 4: Sprite-sheet grid (colored cells from BuImage)");

    // ── Wire up signals ──────────────────────────────────────────────────

    // Toolbar 1 — button clicks
    tb1->buttonClicked.connect([con, bNew, bOpen, bSave, bCut, bCopy, bPaste,
                                bUndo, bRedo, bSearch, bHome](int id) {
        const char* name = "?";
        if      (id == bNew)    name = "New";
        else if (id == bOpen)   name = "Open";
        else if (id == bSave)   name = "Save";
        else if (id == bCut)    name = "Cut";
        else if (id == bCopy)   name = "Copy";
        else if (id == bPaste)  name = "Paste";
        else if (id == bUndo)   name = "Undo";
        else if (id == bRedo)   name = "Redo";
        else if (id == bSearch) name = "Search";
        else if (id == bHome)   name = "Home";
        char buf[128];
        snprintf(buf, sizeof(buf), "[TB1] Button clicked: %s (id=%d)", name, id);
        con->log(LogLevel::Info, buf);
    });

    // Toolbar 2 — toggle changes
    tb2->toggleChanged.connect([con, tBold, tItal, tUnder, tGrid, tList,
                                tDetail, tLock, tEye](int id, bool checked) {
        const char* name = "?";
        if      (id == tBold)   name = "Bold";
        else if (id == tItal)   name = "Italic";
        else if (id == tUnder)  name = "Underline";
        else if (id == tGrid)   name = "Grid View";
        else if (id == tList)   name = "List View";
        else if (id == tDetail) name = "Detail View";
        else if (id == tLock)   name = "Lock";
        else if (id == tEye)    name = "Visible";
        char buf[128];
        snprintf(buf, sizeof(buf), "[TB2] Toggle %s: %s (id=%d)",
                 name, checked ? "ON" : "OFF", id);
        con->log(checked ? LogLevel::Info : LogLevel::Warn, buf);
    });

    // Toolbar 3 — transport
    tb3->buttonClicked.connect([con, tb3, bPlay, bPause, bStop, bStepBack,
                                bStepFwd, bRecord, bRefresh, bTrash](int id) {
        const char* name = "?";
        if      (id == bPlay)     { name = "Play";     tb3->setItemEnabled(bPause, true);  }
        else if (id == bPause)    { name = "Pause";    tb3->setItemEnabled(bPause, false); }
        else if (id == bStop)     { name = "Stop";     tb3->setItemEnabled(bPause, false); }
        else if (id == bStepBack) name = "Step Back";
        else if (id == bStepFwd)  name = "Step Forward";
        else if (id == bRecord)   name = "Record";
        else if (id == bRefresh)  name = "Refresh";
        else if (id == bTrash)    name = "Trash";
        char buf[128];
        snprintf(buf, sizeof(buf), "[TB3] Transport: %s (id=%d)", name, id);
        con->log(LogLevel::Info, buf);
    });

    // Toolbar 4 — grid buttons
    tb4->buttonClicked.connect([con](int id) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[TB4] Grid button clicked: cell=%d", id);
        con->log(LogLevel::Info, buf);
    });

    // ── Control panel (bottom) ───────────────────────────────────────────
    auto* controls = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    controls->setSize(0, 36);
    controls->setPadding(Edges(4, 8, 4, 8));
    controls->setSpacing(8);

    controls->createChild<Label>("Controls:");
    controls->createChild<Line>();

    auto* btnDisable = controls->createChild<Button>("Disable Save");
    btnDisable->setSize(120, 24);
    btnDisable->clicked.connect([tb1, bSave, btnDisable, con]() {
        bool en = tb1->isItemEnabled(bSave);
        tb1->setItemEnabled(bSave, !en);
        btnDisable->setText(en ? "Enable Save" : "Disable Save");
        char buf[64];
        snprintf(buf, sizeof(buf), "Save button %s", en ? "disabled" : "enabled");
        con->log(LogLevel::Warn, buf);
    });

    auto* btnHide = controls->createChild<Button>("Hide Undo/Redo");
    btnHide->setSize(130, 24);
    btnHide->clicked.connect([tb1, bUndo, bRedo, btnHide, con]() {
        bool vis = tb1->isItemVisible(bUndo);
        tb1->setItemVisible(bUndo, !vis);
        tb1->setItemVisible(bRedo, !vis);
        btnHide->setText(vis ? "Show Undo/Redo" : "Hide Undo/Redo");
        con->log(LogLevel::Warn, vis ? "Undo/Redo hidden" : "Undo/Redo shown");
    });

    controls->createChild<Spacer>(0.f)->setStretch(1);
    auto* dragLabel = controls->createChild<Label>("Drag items to reorder!");
    dragLabel->setColor(Color(120, 180, 100, 255));
}
