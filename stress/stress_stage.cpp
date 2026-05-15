#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "ScrollWidgets.hpp"
#include "InputWidgets.hpp"
#include "TextInputWidgets.hpp"
#include "ComboBox.hpp"
#include <cstdio>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  Stress stage
//
//  Tests:
//  - 500 rows in a ScrollView — layout + clip performance
//  - Mixed widget types per row (Label, Button, CheckBox, ProgressBar, Slider)
//  - Live count controls: add / remove rows dynamically
//  - Resize window at any time — layout must not break
//  - FPS counter visible in the Raylib overlay (from main.cpp)
// ─────────────────────────────────────────────────────────────────────────────

using namespace BuGUI;

// Global state so we can add/remove rows from buttons
static BoxLayout* g_col      = nullptr;
static Label*     g_countLbl = nullptr;
static int        g_rows     = 0;

static void addRows(int n)
{
    if (!g_col) return;
    for (int i = 0; i < n; ++i)
    {
        auto* row = g_col->createChild<BoxLayout>(LayoutDir::Horizontal);
        row->setSize(0, 26);
        row->setSpacing(6);
        row->setPadding(Edges(2, 4, 2, 4));

        char buf[32];
        std::snprintf(buf, sizeof(buf), "Row %d", g_rows);

        auto* lbl = row->createChild<Label>(buf);
        lbl->setSize(80, 0);

        auto* btn = row->createChild<Button>("Click");
        btn->setSize(52, 22);

        auto* chk = row->createChild<CheckBox>("");
        (void)chk;

        float ratio = std::fmod(static_cast<float>(g_rows) * 0.07f, 1.0f);
        auto* pb = row->createChild<ProgressBar>(0.f, 1.f, ratio);
        pb->setSize(120, 18);

        auto* sl = row->createChild<Slider>(0.f, 100.f, ratio * 100.f);
        sl->setSize(100, 22);

        ++g_rows;
    }
    if (g_countLbl)
    {
        char cbuf[32];
        std::snprintf(cbuf, sizeof(cbuf), "Rows: %d", g_rows);
        g_countLbl->setText(cbuf);
    }
    WidgetApp::instance().requestLayout();
}

static void removeRows(int n)
{
    if (!g_col) return;
    const auto& ch = g_col->children();
    int toRemove = std::min(n, static_cast<int>(ch.size()));
    for (int i = 0; i < toRemove; ++i)
    {
        g_col->removeChild(ch.back());
        --g_rows;
    }
    if (g_countLbl)
    {
        char cbuf[32];
        std::snprintf(cbuf, sizeof(cbuf), "Rows: %d", g_rows);
        g_countLbl->setText(cbuf);
    }
    WidgetApp::instance().requestLayout();
}

void registerStressStage(WidgetApp& app)
{
    g_col      = nullptr;
    g_countLbl = nullptr;
    g_rows     = 0;

    auto* root = app.addStage("stress");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setStretch(1);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Toolbar ──────────────────────────────────────────────────────────
    auto* bar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    bar->setSize(0, 34);
    bar->setPadding(Edges(4, 8, 4, 8));
    bar->setSpacing(8);

    bar->createChild<Label>("Stress Test —");

    auto* countLbl = bar->createChild<Label>("Rows: 0");
    countLbl->setSize(80, 0);
    g_countLbl = countLbl;

    bar->createChild<Spacer>(8.f);

    auto* btnAdd1    = bar->createChild<Button>("+1");       btnAdd1->setSize(40, 26);
    auto* btnAdd10   = bar->createChild<Button>("+10");      btnAdd10->setSize(46, 26);
    auto* btnAdd100  = bar->createChild<Button>("+100");     btnAdd100->setSize(54, 26);
    auto* btnAdd500  = bar->createChild<Button>("+500");     btnAdd500->setSize(54, 26);

    bar->createChild<Line>();

    auto* btnRem1    = bar->createChild<Button>("-1");       btnRem1->setSize(40, 26);
    auto* btnRem10   = bar->createChild<Button>("-10");      btnRem10->setSize(46, 26);
    auto* btnRem100  = bar->createChild<Button>("-100");     btnRem100->setSize(54, 26);
    auto* btnClear   = bar->createChild<Button>("Clear");    btnClear->setSize(54, 26);

    bar->createChild<Spacer>(0.f)->setStretch(1);

    // ── ScrollView ───────────────────────────────────────────────────────
    auto* sv  = outer->createChild<ScrollView>();
    sv->setStretch(1);
    auto* col = sv->setContent<BoxLayout>(LayoutDir::Vertical);
    col->setSpacing(1);
    col->setPadding(0);
    g_col = col;

    // Wire buttons
    btnAdd1->clicked.connect(  []() { addRows(1);    });
    btnAdd10->clicked.connect( []() { addRows(10);   });
    btnAdd100->clicked.connect([]() { addRows(100);  });
    btnAdd500->clicked.connect([]() { addRows(500);  });
    btnRem1->clicked.connect(  []() { removeRows(1);   });
    btnRem10->clicked.connect( []() { removeRows(10);  });
    btnRem100->clicked.connect([]() { removeRows(100); });
    btnClear->clicked.connect( []() { removeRows(g_rows); });

    // Start with 500 rows so it's immediately a stress test
    addRows(500);
}
