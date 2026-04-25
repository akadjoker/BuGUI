#include "WidgetApp.hpp"
#include "Widgets.hpp"

#include <string>

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 03 - Layouts", 1180, 760))
        return 1;

    Widget* root = app.addStage("main");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(14, 14, 14, 14);
    outer->setSpacing(10);

    auto* title = outer->createChild<Label>("Tutorial 03: BoxLayout + GridLayout + FormLayout");
    title->setColor(Color(120, 190, 255, 255));

    auto* row = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    row->setSpacing(10);
    row->setStretch(1);

    auto* left = row->createChild<Panel>();
    left->setStretch(1);
    auto* leftCol = left->createChild<BoxLayout>(LayoutDir::Vertical);
    leftCol->setPadding(10, 10, 10, 10);
    leftCol->setSpacing(8);
    leftCol->createChild<Label>("Grid Layout");

    auto* grid = leftCol->createChild<GridLayout>(3);
    grid->setSpacing(6);
    grid->setStretch(1);
    for (int i = 0; i < 12; ++i)
    {
        auto* b = grid->createChild<Button>("Cell " + std::to_string(i + 1));
        b->setSize(0, 34);
    }

    auto* right = row->createChild<Panel>();
    right->setStretch(1);
    auto* rightCol = right->createChild<BoxLayout>(LayoutDir::Vertical);
    rightCol->setPadding(10, 10, 10, 10);
    rightCol->setSpacing(8);
    rightCol->createChild<Label>("Form Layout");

    auto* form = rightCol->createChild<FormLayout>();
    form->setSpacing(8, 8);
    form->setLabelWidth(110);
    form->setStretch(1);
    form->addRow<TextInput>("Name", "Player");
    form->addRow<TextInput>("Email", "mail@domain.com");
    form->addRow<Slider>("Volume");
    form->addRow<CheckBox>("Enable FX");

    app.setStage("main");
    return app.run();
}
