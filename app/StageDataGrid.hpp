#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  DataGrid demo stage
// ═════════════════════════════════════════════════════════════════════════════

inline void buildDataGridStage(WidgetApp& app)
{
    Widget* root = app.addStage("datagrid");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Navigation ───────────────────────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    nav->setPadding(16, 16, 0, 16);
    auto* back = nav->createChild<Button>("<  Demo");
    back->clicked.connect([]{ WidgetApp::instance().setStage("demo", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("DataGrid")->setColor(Clr::accent);
    auto* navTree = nav->createChild<Button>("TreeGrid >");
    navTree->clicked.connect([]{ WidgetApp::instance().setStage("treegrid", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── Main layout ──────────────────────────────────────────────────────
    auto* body = outer->createChild<BoxLayout>(LayoutDir::Vertical);
    body->setStretch(1);
    body->setSpacing(8);
    body->setPadding(8);

    // ── Info label ───────────────────────────────────────────────────────
    auto* info = body->createChild<Label>("Double-click to edit, click headers to sort, Ctrl+click multi-select, Shift+scroll horizontal");
    info->setColor(Clr::dim);

    // ── DataGrid ─────────────────────────────────────────────────────────
    auto* grid = body->createChild<DataGrid>();
    grid->setStretch(1);
    grid->setShowCheckboxes(true);
    grid->setMultiSelect(true);
    grid->setZebraStripes(true);

    // Columns
    grid->addColumn("ID", 50, true);
    grid->addColumn("Name", 160, true);
    grid->addColumn("Email", 220, true);
    grid->addColumn("Department", 130, true);
    grid->addColumn("Salary", 90, true);
    grid->addColumn("City", 120, true);
    grid->addColumn("Status", 90, true);

    // Sample data
    grid->addRow({"1",  "Alice Johnson",   "alice@example.com",   "Engineering", "95000", "Porto",     "Active"});
    grid->addRow({"2",  "Bob Smith",       "bob@example.com",     "Marketing",   "72000", "Lisboa",    "Active"});
    grid->addRow({"3",  "Carol Williams",  "carol@example.com",   "Engineering", "88000", "Braga",     "Active"});
    grid->addRow({"4",  "David Brown",     "david@example.com",   "HR",          "65000", "Coimbra",   "Inactive"});
    grid->addRow({"5",  "Eve Davis",       "eve@example.com",     "Engineering", "102000","Porto",     "Active"});
    grid->addRow({"6",  "Frank Miller",    "frank@example.com",   "Sales",       "78000", "Faro",      "Active"});
    grid->addRow({"7",  "Grace Wilson",    "grace@example.com",   "Engineering", "91000", "Lisboa",    "Active"});
    grid->addRow({"8",  "Henry Moore",     "henry@example.com",   "Marketing",   "68000", "Porto",     "Inactive"});
    grid->addRow({"9",  "Iris Taylor",     "iris@example.com",    "HR",          "71000", "Aveiro",    "Active"});
    grid->addRow({"10", "Jack Anderson",   "jack@example.com",    "Sales",       "82000", "Braga",     "Active"});
    grid->addRow({"11", "Karen Thomas",    "karen@example.com",   "Engineering", "97000", "Lisboa",    "Active"});
    grid->addRow({"12", "Leo Jackson",     "leo@example.com",     "Marketing",   "74000", "Porto",     "Active"});
    grid->addRow({"13", "Mia White",       "mia@example.com",     "Engineering", "105000","Coimbra",   "Active"});
    grid->addRow({"14", "Nick Harris",     "nick@example.com",    "HR",          "62000", "Faro",      "Inactive"});
    grid->addRow({"15", "Olivia Martin",   "olivia@example.com",  "Sales",       "85000", "Aveiro",    "Active"});
    grid->addRow({"16", "Paul Garcia",     "paul@example.com",    "Engineering", "99000", "Porto",     "Active"});
    grid->addRow({"17", "Quinn Robinson",  "quinn@example.com",   "Marketing",   "70000", "Lisboa",    "Active"});
    grid->addRow({"18", "Rachel Clark",    "rachel@example.com",  "Engineering", "93000", "Braga",     "Inactive"});
    grid->addRow({"19", "Sam Lewis",       "sam@example.com",     "Sales",       "80000", "Coimbra",   "Active"});
    grid->addRow({"20", "Tina Walker",     "tina@example.com",    "HR",          "67000", "Porto",     "Active"});

    // ── Status bar ───────────────────────────────────────────────────────
    auto* statusBar = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    statusBar->setSize(0, 24);
    statusBar->setSpacing(16);
    statusBar->setPadding(4, 4, 0, 4);

    auto* selLabel = statusBar->createChild<Label>("Selected: none");
    selLabel->setColor(Clr::dim);

    auto* checkLabel = statusBar->createChild<Label>("Checked: 0");
    checkLabel->setColor(Clr::dim);

    grid->selectionChanged.connect([selLabel, grid](int row) {
        char buf[64];
        auto& sel = grid->selectedRows();
        if (sel.size() <= 1)
            snprintf(buf, sizeof(buf), "Selected: row %d", row);
        else
            snprintf(buf, sizeof(buf), "Selected: %d rows", static_cast<int>(sel.size()));
        selLabel->setText(buf);
    });

    grid->rowCheckChanged.connect([checkLabel, grid](int, bool) {
        char buf[48];
        snprintf(buf, sizeof(buf), "Checked: %d", static_cast<int>(grid->checkedRows().size()));
        checkLabel->setText(buf);
    });

    grid->cellEdited.connect([](int row, int col) {
        (void)row; (void)col;
        // Could update a status label here
    });
}
