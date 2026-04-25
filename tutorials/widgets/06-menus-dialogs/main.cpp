#include "WidgetApp.hpp"
#include "Widgets.hpp"

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 06 - Menus & Dialogs", 1140, 740))
        return 1;

    Widget* root = app.addStage("main");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(0);
    outer->setSpacing(0);

    auto* menuBar = outer->createChild<MenuBar>();
    menuBar->setSize(0, 30);

    auto* body = outer->createChild<BoxLayout>(LayoutDir::Vertical);
    body->setStretch(1);
    body->setPadding(14, 14, 14, 14);
    body->setSpacing(10);

    auto* title = body->createChild<Label>("Tutorial 06: MenuBar + ContextMenu + Dialogs");
    title->setColor(Color(120, 190, 255, 255));
    auto* status = body->createChild<Label>("Use menu actions or right-click panel.");

    auto* actions = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    actions->setSpacing(8);
    auto* alertBtn = actions->createChild<Button>("Open Alert");
    alertBtn->setSize(120, 28);
    auto* confirmBtn = actions->createChild<Button>("Open Confirm");
    confirmBtn->setSize(130, 28);
    auto* inputBtn = actions->createChild<Button>("Open InputBox");
    inputBtn->setSize(130, 28);

    auto* panel = body->createChild<Panel>();
    panel->setStretch(1);
    auto* panelCol = panel->createChild<BoxLayout>(LayoutDir::Vertical);
    panelCol->setPadding(10, 10, 10, 10);
    panelCol->createChild<Label>("Right-click here for context menu");

    auto* fileMenu = menuBar->addMenu("File");
    fileMenu->addAction("New", [status] { status->setText("File > New"); }, "Ctrl+N");
    fileMenu->addAction("Save", [status] { status->setText("File > Save"); }, "Ctrl+S");
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", [&app] { app.quit(); }, "Esc");

    auto* viewMenu = menuBar->addMenu("View");
    viewMenu->addCheckable("Debug Overlay", false, [status](bool on) {
        status->setText(std::string("View > Debug Overlay: ") + (on ? "ON" : "OFF"));
    });

    auto* ctx = new Menu();
    ctx->addAction("Create Object", [status] { status->setText("Context: Create Object"); });
    ctx->addAction("Duplicate", [status] { status->setText("Context: Duplicate"); });
    ctx->addSeparator();
    ctx->addAction("Delete", [status] { status->setText("Context: Delete"); });
    panel->setContextMenu(ctx);
    app.setOnDestroy([ctx] { delete ctx; });

    alertBtn->clicked.connect([status] {
        auto* dlg = new AlertDialog("Alert", "This is an alert dialog.");
        dlg->buttonClicked.connect([status](int) { status->setText("Alert closed"); });
        WidgetApp::instance().showPopup(dlg, nullptr, true);
    });

    confirmBtn->clicked.connect([status] {
        auto* dlg = new ConfirmDialog("Confirm", "Apply these changes?");
        dlg->confirmed.connect([status] { status->setText("Confirm: accepted"); });
        dlg->cancelled.connect([status] { status->setText("Confirm: cancelled"); });
        WidgetApp::instance().showPopup(dlg, nullptr, true);
    });

    inputBtn->clicked.connect([status, &app] {
        auto* ib = app.addFloat<InputBox>("Rename", "Enter object name:");
        ib->setFloatPos(360, 180);
        ib->setFloatSize(420, 170);
        ib->setText("Player");
        ib->accepted.connect([status](const std::string& txt) {
            status->setText("InputBox accepted: " + txt);
        });
        ib->cancelled.connect([status] { status->setText("InputBox cancelled"); });
    });

    app.setStage("main");
    return app.run();
}
