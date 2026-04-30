#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "InputWidgets.hpp"
#include "ViewWidgets.hpp"
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  Stage 11 — FloatWindow (draggable / resizable / closable floating panels)
// ─────────────────────────────────────────────────────────────────────────────

static int s_fwCounter = 0;

static void spawnFloatWindow(WidgetApp& app, float x, float y)
{
    ++s_fwCounter;
    char title[32];
    std::snprintf(title, sizeof(title), "Panel #%d", s_fwCounter);

    auto* fw = app.addFloat<FloatWindow>(title);
    fw->setFloatPos(x, y);
    fw->setFloatSize(260.0f, 180.0f);
    fw->setClosable(true);
    fw->setMinimizable(true);
    fw->setResizable(true);
    fw->closed.connect([&app, fw]()
    {
        app.removeFloat(fw);
    });

    auto* vbox = fw->setContent<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(8.0f);
    vbox->setPadding(10.0f);

    char lbl[48];
    std::snprintf(lbl, sizeof(lbl), "Window %d", s_fwCounter);
    vbox->createChild<Label>(lbl);

    auto* sl = vbox->createChild<Slider>(0.0f, 100.0f);
    sl->setValue(40.0f);

    auto* chk = vbox->createChild<CheckBox>("Option A");
    chk->setChecked(true);
    vbox->createChild<CheckBox>("Option B");

    auto* closeBtn = vbox->createChild<Button>("Close");
    closeBtn->clicked.connect([&app, fw]()
    {
        app.removeFloat(fw);
    });
}

void registerFloatWindowStage(WidgetApp& app)
{
    s_fwCounter = 0;

    auto* root = app.addStage("floatwindow");
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(0.0f);
    vbox->setPadding(0.0f);

    // ── Top bar ────────────────────────────────────────────────────────────
    auto* topBar = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSpacing(8.0f);
    topBar->setPadding(Edges(8.0f, 10.0f));

    auto* back = topBar->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]()
    {
        while (!app.floats().empty())
            app.removeFloat(app.floats().front());
        app.setStage("menu", TransitionType::CoverRight);
    });

    topBar->createChild<Spacer>(12.0f);
    topBar->createChild<Label>("FloatWindow — drag title bar, resize corner, close \u00d7, minimize \u2013");
    topBar->createChild<Spacer>(0.0f)->setStretch(1.0f);

    auto* spawnBtn = topBar->createChild<Button>("+ New Window");
    spawnBtn->clicked.connect([&app]()
    {
        float ox = 80.0f + (s_fwCounter % 5) * 30.0f;
        float oy = 120.0f + (s_fwCounter % 4) * 30.0f;
        spawnFloatWindow(app, ox, oy);
    });

    // ── Body: hint text ────────────────────────────────────────────────────
    auto* body = vbox->createChild<BoxLayout>(LayoutDir::Vertical);
    body->setStretch(1.0f);
    body->setSpacing(10.0f);
    body->setPadding(20.0f);

    body->createChild<Label>("Click '+ New Window' to spawn floating panels.");
    body->createChild<Label>("Drag a title bar to move.  Drag the bottom-right grip to resize.");
    body->createChild<Label>("Click \u00d7 to close,  \u2013 to minimize.");

    // spawn 2 windows immediately when stage is entered
    // We defer via a signal trick: connect a one-shot to spawnBtn, or just spawn directly.
    // Direct spawn is fine — addFloat works any time.
    // spawnFloatWindow(app, 80.0f,  130.0f);
    // spawnFloatWindow(app, 360.0f, 160.0f);
}
