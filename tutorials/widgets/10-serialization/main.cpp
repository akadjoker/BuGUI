#include "WidgetApp.hpp"
#include "Widgets.hpp"
#include "WidgetSerializer.hpp"

#include <string>

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 10 - Serialization", 1180, 760))
        return 1;

    Widget* root = app.addStage("main");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(12, 12, 12, 12);
    outer->setSpacing(10);

    auto* title = outer->createChild<Label>("Tutorial 10: WidgetSerializer Save/Load");
    title->setColor(Color(120, 190, 255, 255));
    auto* status = outer->createChild<Label>("Press Save then Load.");

    auto* actions = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    actions->setSpacing(8);
    auto* saveBtn = actions->createChild<Button>("Save JSON");
    saveBtn->setSize(120, 28);
    auto* loadBtn = actions->createChild<Button>("Load JSON");
    loadBtn->setSize(120, 28);
    auto* clearBtn = actions->createChild<Button>("Clear Loaded");
    clearBtn->setSize(120, 28);

    auto* row = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    row->setSpacing(10);
    row->setStretch(1);

    auto* left = row->createChild<Panel>();
    left->setStretch(1);
    auto* leftCol = left->createChild<BoxLayout>(LayoutDir::Vertical);
    leftCol->setPadding(8, 8, 8, 8);
    leftCol->createChild<Label>("Source Tree");
    auto* source = leftCol->createChild<Panel>();
    source->setStretch(1);
    source->setId("source-panel");
    auto* srcLayout = source->createChild<BoxLayout>(LayoutDir::Vertical);
    srcLayout->setPadding(10, 10, 10, 10);
    srcLayout->setSpacing(8);
    srcLayout->createChild<Label>("Serialized content");
    srcLayout->createChild<TextInput>("Player");
    srcLayout->createChild<CheckBox>("Visible");
    srcLayout->createChild<Slider>(0.0f, 1.0f, 0.75f);

    auto* right = row->createChild<Panel>();
    right->setStretch(1);
    auto* rightCol = right->createChild<BoxLayout>(LayoutDir::Vertical);
    rightCol->setPadding(8, 8, 8, 8);
    rightCol->createChild<Label>("Loaded Tree");
    auto* host = rightCol->createChild<Panel>();
    host->setStretch(1);
    host->setId("loaded-host");

    const std::string path = "tutorial_widgets_10_layout.json";
    Widget* loaded = nullptr;

    saveBtn->clicked.connect([status, source, path] {
        if (WidgetSerializer::saveToFile(source, path))
            status->setText("Saved: " + path);
        else
            status->setText("Save failed: " + path);
    });

    loadBtn->clicked.connect([status, host, path, &loaded] {
        if (loaded && loaded->parent() == host)
        {
            host->removeChild(loaded);
            loaded = nullptr;
        }
        loaded = WidgetSerializer::loadFromFile(path, host);
        if (loaded)
        {
            loaded->setPosition(10, 10);
            loaded->setSize(host->rect().w - 20, host->rect().h - 20);
            status->setText("Loaded: " + path);
        }
        else
        {
            status->setText("Load failed: " + path);
        }
    });

    clearBtn->clicked.connect([status, host, &loaded] {
        if (loaded && loaded->parent() == host)
        {
            host->removeChild(loaded);
            loaded = nullptr;
            status->setText("Loaded tree cleared");
        }
    });

    app.setStage("main");
    return app.run();
}
