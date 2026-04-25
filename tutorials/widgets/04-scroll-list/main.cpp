#include "WidgetApp.hpp"
#include "Widgets.hpp"

#include <string>

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 04 - Scroll & Lists", 1180, 760))
        return 1;

    Widget* root = app.addStage("main");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(14, 14, 14, 14);
    outer->setSpacing(10);

    auto* title = outer->createChild<Label>("Tutorial 04: ScrollView + ListBox + ListWidget");
    title->setColor(Color(120, 190, 255, 255));

    auto* status = outer->createChild<Label>("Select items and scroll panels.");

    auto* row = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    row->setSpacing(10);
    row->setStretch(1);

    auto* scrollPanel = row->createChild<Panel>();
    scrollPanel->setStretch(1);
    auto* scrollCol = scrollPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    scrollCol->setPadding(8, 8, 8, 8);
    scrollCol->setSpacing(6);
    scrollCol->createChild<Label>("ScrollView");

    auto* sv = scrollCol->createChild<ScrollView>();
    sv->setStretch(1);
    auto* longCol = sv->setContent<BoxLayout>(LayoutDir::Vertical);
    longCol->setPadding(8, 8, 8, 8);
    longCol->setSpacing(4);
    for (int i = 0; i < 80; ++i)
        longCol->createChild<Label>("Log line " + std::to_string(i + 1));

    auto* listPanel = row->createChild<Panel>();
    listPanel->setStretch(1);
    auto* listCol = listPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    listCol->setPadding(8, 8, 8, 8);
    listCol->setSpacing(6);
    listCol->createChild<Label>("ListBox");

    auto* lb = listCol->createChild<ListBox>();
    lb->setStretch(1);
    for (int i = 0; i < 40; ++i)
        lb->addItem("Asset_" + std::to_string(i + 1));

    auto* customPanel = row->createChild<Panel>();
    customPanel->setStretch(1);
    auto* customCol = customPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    customCol->setPadding(8, 8, 8, 8);
    customCol->setSpacing(6);
    customCol->createChild<Label>("ListWidget");

    auto* lw = customCol->createChild<ListWidget>();
    lw->setStretch(1);
    for (int i = 0; i < 24; ++i)
    {
        auto* itemRow = lw->addRow<BoxLayout>(LayoutDir::Horizontal);
        itemRow->setSpacing(8);
        itemRow->createChild<Label>("Object " + std::to_string(i + 1));
        itemRow->createChild<Button>("Edit");
    }

    lb->selectionChanged.connect([status, lb](int idx) {
        if (idx >= 0)
            status->setText("ListBox selected: " + lb->itemText(idx));
    });

    lw->selectionChanged.connect([status](int idx) {
        status->setText("ListWidget selected row: " + std::to_string(idx));
    });

    app.setStage("main");
    return app.run();
}
