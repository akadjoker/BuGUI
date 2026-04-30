#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"

void registerLayoutsStage(WidgetApp& app)
{
    auto* root = app.addStage("layouts");
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(8.0f);
    vbox->setPadding(16.0f);

    auto* back = vbox->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
    vbox->createChild<Line>();

    vbox->createChild<Label>("GridLayout (3 cols)");
    auto* grid = vbox->createChild<GridLayout>(3);
    grid->setSpacing(6.0f);
    for (int i = 1; i <= 9; ++i)
    {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "Cell %d", i);
        grid->createChild<Button>(buf);
    }

    vbox->createChild<Spacer>(8.0f);
    vbox->createChild<Label>("Collapsible section");
    auto* col = vbox->createChild<Collapsible>("Details");
    col->createChild<Label>("Content inside collapsible");
    col->createChild<CheckBox>("Sub-option");
}
