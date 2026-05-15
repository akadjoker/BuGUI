#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "ScrollWidgets.hpp"

#include <string>
using namespace BuGUI;

void registerScrollStage(WidgetApp& app)
{
    auto* root = app.addStage("scroll");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(14.0f);
    outer->setSpacing(10.0f);
    outer->setStretch(1);

    // Title + status
    auto* title = outer->createChild<Label>("04 - ScrollView / ListBox / ListWidget");
    title->setColor(Color(120, 190, 255, 255));
    auto* status = outer->createChild<Label>("Select items or scroll the panels.");

    // Three-column row
    auto* row = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    row->setSpacing(10.0f);
    row->setStretch(1);

    // ── Column 1: ScrollView ─────────────────────────────────────────────
    auto* col1 = row->createChild<Panel>();
    col1->setStretch(1);
    auto* c1inner = col1->createChild<BoxLayout>(LayoutDir::Vertical);
    c1inner->setPadding(8.0f);
    c1inner->setSpacing(4.0f);
    c1inner->setStretch(1);
    c1inner->createChild<Label>("ScrollView");

    auto* sv = c1inner->createChild<ScrollView>();
    sv->setStretch(1);
    auto* longCol = sv->setContent<BoxLayout>(LayoutDir::Vertical);
    longCol->setPadding(8.0f);
    longCol->setSpacing(4.0f);
    for (int i = 0; i < 80; ++i)
        longCol->createChild<Label>("Log line " + std::to_string(i + 1));

    // ── Column 2: ListBox ────────────────────────────────────────────────
    auto* col2 = row->createChild<Panel>();
    col2->setStretch(1);
    auto* c2inner = col2->createChild<BoxLayout>(LayoutDir::Vertical);
    c2inner->setPadding(8.0f);
    c2inner->setSpacing(4.0f);
    c2inner->setStretch(1);
    c2inner->createChild<Label>("ListBox");

    auto* lb = c2inner->createChild<ListBox>();
    lb->setStretch(1);
    for (int i = 0; i < 40; ++i)
        lb->addItem("Asset_" + std::to_string(i + 1));

    lb->selectionChanged.connect([status, lb](int idx) {
        if (idx >= 0)
            status->setText("ListBox selected: " + lb->itemText(idx));
    });

    // ── Column 3: ListWidget ─────────────────────────────────────────────
    auto* col3 = row->createChild<Panel>();
    col3->setStretch(1);
    auto* c3inner = col3->createChild<BoxLayout>(LayoutDir::Vertical);
    c3inner->setPadding(8.0f);
    c3inner->setSpacing(4.0f);
    c3inner->setStretch(1);
    c3inner->createChild<Label>("ListWidget");

    auto* lw = c3inner->createChild<ListWidget>();
    lw->setStretch(1);
    for (int i = 0; i < 24; ++i)
    {
        auto* itemRow = lw->addRow<BoxLayout>(LayoutDir::Horizontal);
        itemRow->setSpacing(8.0f);
        itemRow->createChild<Label>("Object " + std::to_string(i + 1));
        itemRow->createChild<Button>("Edit");
    }

    lw->selectionChanged.connect([status](int idx) {
        if (idx >= 0)
            status->setText("ListWidget selected row: " + std::to_string(idx));
    });

    // ── Back button ──────────────────────────────────────────────────────
    auto* back = outer->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
}
