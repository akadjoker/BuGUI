#include "WidgetApp.hpp"
#include "Widgets.hpp"

#include <string>

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 07 - Tabs & Dock", 1280, 800))
        return 1;

    Widget* root = app.addStage("main");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(12, 12, 12, 12);
    outer->setSpacing(10);

    auto* title = outer->createChild<Label>("Tutorial 07: TabLayout + DockPanel");
    title->setColor(Color(120, 190, 255, 255));
    auto* status = outer->createChild<Label>("Drag dock tabs and switch top tabs.");

    auto* tabs = outer->createChild<TabLayout>();
    tabs->setStretch(1);
    tabs->setTabsClosable(false);

    auto* workspace = tabs->addTab<BoxLayout>("Workspace", LayoutDir::Vertical);
    workspace->setPadding(0);
    workspace->setSpacing(0);

    auto* dock = workspace->createChild<DockPanel>();
    dock->setStretch(1);

    auto* sceneTree = new TreeView();
    auto* r = sceneTree->addRoot("Scene");
    r->addChild("Camera");
    r->addChild("Light");
    r->addChild("Player");
    r->setExpanded(true);
    dock->addPanel("Scene", sceneTree);

    auto* inspector = new PropertyGrid();
    inspector->addSection("Transform");
    inspector->addVec3("Position", 0, 0, 0, -1000, 1000);
    inspector->addVec3("Rotation", 0, 0, 0, -360, 360);
    inspector->addVec3("Scale", 1, 1, 1, 0, 100);
    dock->addPanel("Inspector", inspector);

    auto* console = new ListWidget();
    console->addRow<Label>("[Info] Editor started");
    console->addRow<Label>("[Info] Scene loaded");
    console->addRow<Label>("[Warn] Missing metadata");
    dock->addPanel("Console", console);

    auto* text = new TextEdit();
    text->setText("// Script\nprint('hello world')\n");
    dock->addPanel("Script", text);

    dock->splitOff("Inspector", DockSide::Right, 0.28f);
    dock->splitOff("Console", DockSide::Bottom, 0.30f);

    auto* settings = tabs->addTab<BoxLayout>("Settings", LayoutDir::Vertical);
    settings->setPadding(12, 12, 12, 12);
    settings->setSpacing(8);
    settings->createChild<Label>("Editor Settings");
    settings->createChild<CheckBox>("Enable snapping");
    settings->createChild<CheckBox>("Auto-save");
    settings->createChild<Slider>(0.0f, 1.0f, 0.5f);

    auto* about = tabs->addTab<BoxLayout>("About", LayoutDir::Vertical);
    about->setPadding(12, 12, 12, 12);
    about->setSpacing(8);
    about->createChild<Label>("BuGUI tutorial workspace");
    about->createChild<Label>("Use this tab to test page switching.");

    tabs->currentChanged.connect([status](int idx) {
        status->setText("Active tab index: " + std::to_string(idx));
    });

    dock->panelActivated.connect([status](const std::string& name) {
        status->setText("Dock panel activated: " + name);
    });

    app.setStage("main");
    return app.run();
}
