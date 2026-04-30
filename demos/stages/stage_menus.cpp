#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "MenuWidgets.hpp"
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  Stage 07 — Menus & Context
// ─────────────────────────────────────────────────────────────────────────────

// Static pointers — lambdas capture these safely across frames
static Label* s_status  = nullptr;
static Menu*  s_ctxMenu = nullptr;

void registerMenusStage(WidgetApp& app)
{
    s_status = nullptr;  // reset on re-register (if called again)

    auto* root = app.addStage("menus");
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(0.0f);
    vbox->setPadding(0.0f);

    // ── Top bar ────────────────────────────────────────────────────────────
    auto* topBar = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSpacing(8.0f);
    topBar->setPadding(Edges(8.0f, 12.0f));

    auto* back = topBar->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
    topBar->createChild<Spacer>(0.0f)->setStretch(1.0f);
    auto* titleLbl = topBar->createChild<Label>("07 \u2014 Menus & Context");
    titleLbl->setAlign(TextAlign::RIGHT);

    vbox->createChild<Line>();

    // ── MenuBar ────────────────────────────────────────────────────────────
    auto* bar = vbox->createChild<MenuBar>();

    // File
    {
        Menu* file = bar->addMenu("File");
        file->addAction("New",        []() { if (s_status) s_status->setText("File > New");        })->setShortcut("Ctrl+N");
        file->addAction("Open...",    []() { if (s_status) s_status->setText("File > Open...");    })->setShortcut("Ctrl+O");

        // Open Recent submenu
        static Menu* recentMenu = nullptr;
        if (!recentMenu)
        {
            recentMenu = new Menu();
            recentMenu->addAction("project_alpha.bugui", []() { if (s_status) s_status->setText("Recent > project_alpha.bugui"); });
            recentMenu->addAction("ui_mockup_v2.bugui",  []() { if (s_status) s_status->setText("Recent > ui_mockup_v2.bugui");  });
            recentMenu->addAction("demo_layout.bugui",   []() { if (s_status) s_status->setText("Recent > demo_layout.bugui");   });
            recentMenu->addSeparator();
            recentMenu->addAction("Clear Recent", []() { if (s_status) s_status->setText("Recent > Clear Recent"); });
        }
        auto* recent = file->addAction("Open Recent");
        recent->setSubmenu(recentMenu);

        file->addAction("Save",       []() { if (s_status) s_status->setText("File > Save");       })->setShortcut("Ctrl+S");
        file->addAction("Save As...", []() { if (s_status) s_status->setText("File > Save As..."); })->setShortcut("Ctrl+Shift+S");
        file->addSeparator();
        file->addAction("Exit",       []() { if (s_status) s_status->setText("File > Exit");       });
    }

    // Edit
    {
        Menu* edit = bar->addMenu("Edit");
        edit->addAction("Undo",       []() { if (s_status) s_status->setText("Edit > Undo");       })->setShortcut("Ctrl+Z");
        edit->addAction("Redo",       []() { if (s_status) s_status->setText("Edit > Redo");       })->setShortcut("Ctrl+Y");
        edit->addSeparator();
        edit->addAction("Cut",        []() { if (s_status) s_status->setText("Edit > Cut");        })->setShortcut("Ctrl+X");
        edit->addAction("Copy",       []() { if (s_status) s_status->setText("Edit > Copy");       })->setShortcut("Ctrl+C");
        edit->addAction("Paste",      []() { if (s_status) s_status->setText("Edit > Paste");      })->setShortcut("Ctrl+V");
        edit->addSeparator();
        edit->addAction("Select All", []() { if (s_status) s_status->setText("Edit > Select All"); })->setShortcut("Ctrl+A");
        // Disabled example
        auto* find = edit->addAction("Find...");
        find->setShortcut("Ctrl+F");
        find->setEnabled(false);
    }

    // View
    {
        Menu* view = bar->addMenu("View");
        auto* grid   = view->addCheckable("Show Grid",       true);
        auto* rulers = view->addCheckable("Show Rulers",     false);
        auto* darkBg = view->addCheckable("Dark Background", true);

        grid->triggered.connect([grid]() {
            if (!s_status) return;
            char buf[64]; std::snprintf(buf, sizeof(buf), "Show Grid: %s", grid->isChecked() ? "on" : "off");
            s_status->setText(buf);
        });
        rulers->triggered.connect([rulers]() {
            if (!s_status) return;
            char buf[64]; std::snprintf(buf, sizeof(buf), "Show Rulers: %s", rulers->isChecked() ? "on" : "off");
            s_status->setText(buf);
        });
        darkBg->triggered.connect([darkBg]() {
            if (!s_status) return;
            char buf[64]; std::snprintf(buf, sizeof(buf), "Dark Background: %s", darkBg->isChecked() ? "on" : "off");
            s_status->setText(buf);
        });

        view->addSeparator();
        view->addAction("Zoom In",       []() { if (s_status) s_status->setText("View > Zoom In");       })->setShortcut("Ctrl++");
        view->addAction("Zoom Out",      []() { if (s_status) s_status->setText("View > Zoom Out");      })->setShortcut("Ctrl+-");
        view->addAction("Fit to Window", []() { if (s_status) s_status->setText("View > Fit to Window"); })->setShortcut("Ctrl+0");
    }

    // ── Canvas (right-click area) ─────────────────────────────────────────
    auto* canvas = vbox->createChild<Panel>();
    canvas->setStretch(1.0f);
    canvas->setBgColor({28, 28, 32, 255});

    // Centred hint label inside the canvas
    auto* inner = canvas->createChild<BoxLayout>(LayoutDir::Vertical);
    inner->setMainAlign(MainAlign::Center);
    inner->setPadding(8.0f);
    auto* hint = inner->createChild<Label>("Right-click anywhere for context menu");
    hint->setColor({90, 90, 98, 255});
    hint->setAlign(TextAlign::CENTER);

    // Context menu — static so it persists for the app lifetime
    if (!s_ctxMenu)
    {
        s_ctxMenu = new Menu();
        s_ctxMenu->addAction("Cut",        []() { if (s_status) s_status->setText("Context > Cut");        })->setShortcut("Ctrl+X");
        s_ctxMenu->addAction("Copy",       []() { if (s_status) s_status->setText("Context > Copy");       })->setShortcut("Ctrl+C");
        s_ctxMenu->addAction("Paste",      []() { if (s_status) s_status->setText("Context > Paste");      })->setShortcut("Ctrl+V");
        s_ctxMenu->addSeparator();
        s_ctxMenu->addAction("Select All", []() { if (s_status) s_status->setText("Context > Select All"); })->setShortcut("Ctrl+A");
        s_ctxMenu->addSeparator();

        // Insert submenu
        static Menu* insertMenu = nullptr;
        if (!insertMenu)
        {
            insertMenu = new Menu();
            insertMenu->addAction("Label",   []() { if (s_status) s_status->setText("Insert > Label");   });
            insertMenu->addAction("Button",  []() { if (s_status) s_status->setText("Insert > Button");  });
            insertMenu->addAction("TextBox", []() { if (s_status) s_status->setText("Insert > TextBox"); });
            insertMenu->addAction("Image",   []() { if (s_status) s_status->setText("Insert > Image");   });
            insertMenu->addSeparator();
            insertMenu->addAction("Group",   []() { if (s_status) s_status->setText("Insert > Group");   });
        }
        auto* insertAct = s_ctxMenu->addAction("Insert");
        insertAct->setSubmenu(insertMenu);

        s_ctxMenu->addSeparator();
        s_ctxMenu->addAction("Properties...", []() { if (s_status) s_status->setText("Context > Properties..."); });
    }

    canvas->pressed.connect([](int btn) {
        if (btn == 1 && s_ctxMenu)
            s_ctxMenu->exec(WidgetApp::instance().mouseX(),
                            WidgetApp::instance().mouseY());
    });

    vbox->createChild<Line>();

    // ── Status bar ────────────────────────────────────────────────────────
    auto* statusRow = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    statusRow->setSpacing(0.0f);
    statusRow->setPadding(Edges(5.0f, 12.0f));
    s_status = statusRow->createChild<Label>("Last action: \u2014");
}
