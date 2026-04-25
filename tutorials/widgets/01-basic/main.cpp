#include "WidgetApp.hpp"
#include "Widgets.hpp"

#include <string>

int main()
{
    auto &app = WidgetApp::instance();
    if (!app.init("Tutorial 01 - Basic Widgets", 1024, 680))
        return 1;

    Widget *root = app.addStage("main");
    auto *col = root->createChild<BoxLayout>(LayoutDir::Vertical);
    col->setPadding(16, 16, 16, 16);
    col->setSpacing(10);

    auto *title = col->createChild<Label>("Tutorial 01: Basic Widgets");
    title->setColor(Color(120, 190, 255, 255));

    auto *info = col->createChild<Label>("Button + CheckBox + ProgressBar");
    auto *progress = col->createChild<ProgressBar>(0.0f, 10.0f, 0.0f);
    progress->setSize(0, 24);
    progress->setText("0 / 10");

    auto *buttonRow = col->createChild<BoxLayout>(LayoutDir::Horizontal);
    buttonRow->setSpacing(8);
    auto *addBtn = buttonRow->createChild<Button>("Increment");
    addBtn->setSize(120, 28);
    auto *resetBtn = buttonRow->createChild<Button>("Reset");
    resetBtn->setSize(90, 28);

    auto *check = col->createChild<CheckBox>("Enable progress updates");
    check->setChecked(true);

    int counter = 0;
    addBtn->clicked.connect([&]
                            {
        if (!check->isChecked()) return;
        if (counter < 10) counter++;
        progress->setValue(static_cast<float>(counter));
        progress->setText(std::to_string(counter) + " / 10");
        info->setText("Counter updated"); });

    resetBtn->clicked.connect([&]
                              {
        counter = 0;
        progress->setValue(0.0f);
        progress->setText("0 / 10");
        info->setText("Counter reset"); });

    check->toggled.connect([info](bool enabled)
                           { info->setText(enabled ? "Updates enabled" : "Updates paused"); });

    app.setStage("main");
    return app.run();
}
