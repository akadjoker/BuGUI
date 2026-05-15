#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "ScrollWidgets.hpp"
#include "InputWidgets.hpp"
#include "TextInputWidgets.hpp"
#include "ComboBox.hpp"
#include "MenuWidgets.hpp"
#include "ConsoleWidget.hpp"
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  Stage — Focus & Keyboard Navigation
//
//  Demonstrates:
//  - Tab / Shift+Tab cycling through focusable widgets
//  - onFocusGained / onFocusLost callbacks (logged to console)
//  - Disabled widgets are skipped by Tab
//  - setTooltip() on any widget
//  - Context menu on right-click
// ─────────────────────────────────────────────────────────────────────────────

using namespace BuGUI;

// Widget subclass that logs focus events to a ConsoleWidget
template<typename Base>
class FocusLogged : public Base
{
public:
    template<typename... Args>
    FocusLogged(ConsoleWidget* console, const char* name, Args&&... args)
        : Base(std::forward<Args>(args)...), console_(console), name_(name) {}

    void onFocusGained() override
    {
        char buf[80];
        std::snprintf(buf, sizeof(buf), "  [onFocusGained] %s", name_);
        console_->log(LogLevel::Info, buf);
    }
    void onFocusLost() override
    {
        char buf[80];
        std::snprintf(buf, sizeof(buf), "  [onFocusLost]   %s", name_);
        console_->log(LogLevel::Trace, buf);
    }

private:
    ConsoleWidget* console_;
    const char*    name_;
};

void registerFocusStage(WidgetApp& app)
{
    auto* root = app.addStage("focus");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setStretch(1);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Header ───────────────────────────────────────────────────────────
    auto* header = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSize(0, 32);
    header->setPadding(Edges(4, 4, 4, 4));
    header->setSpacing(8);

    auto* back = header->createChild<Button>("\u2190 Menu");
    back->setSize(80, 24);
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
    header->createChild<Line>();
    header->createChild<Label>("Focus & Keyboard Navigation");
    header->createChild<Spacer>(0.f)->setStretch(1);

    // ── Hint bar ─────────────────────────────────────────────────────────
    auto* hint = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    hint->setSize(0, 24);
    hint->setPadding(Edges(2, 12, 2, 12));
    auto* hintLbl = hint->createChild<Label>(
        "Tab / Shift+Tab  \u2192  cycle focus.   Right-click  \u2192  context menu.");
    hintLbl->setColor(Color(140, 160, 200, 255));
    outer->createChild<Line>();

    // ── Two-column layout ────────────────────────────────────────────────
    auto* cols = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    cols->setStretch(1);
    cols->setSpacing(0);
    cols->setPadding(0);

    auto* left = cols->createChild<BoxLayout>(LayoutDir::Vertical);
    left->setSize(400, 0);
    left->setSpacing(10);
    left->setPadding(16);

    auto* console = cols->createChild<ConsoleWidget>();
    console->setStretch(1);
    console->log(LogLevel::Info, "Stage ready. Use Tab to navigate.");
    console->log(LogLevel::Info, "Focus events appear here.");

    // ── Focusable widgets ────────────────────────────────────────────────
    left->createChild<Label>("Buttons (A enabled, B enabled, C disabled):");

    auto* row1 = left->createChild<BoxLayout>(LayoutDir::Horizontal);
    row1->setSpacing(8);

    auto* fbA = row1->addChild(new FocusLogged<Button>(console, "Button A", "Button A"));
    auto* fbB = row1->addChild(new FocusLogged<Button>(console, "Button B", "Button B"));
    auto* fbC = row1->addChild(new FocusLogged<Button>(console, "Button C (disabled)", "Button C"));
    fbA->setSize(100, 28);  fbA->setTooltip("Tooltip on Button A");
    fbB->setSize(100, 28);  fbB->setTooltip("Tooltip on Button B");
    fbC->setSize(150, 28);  fbC->setEnabled(false);

    left->createChild<Label>("CheckBoxes:");

    auto* row2 = left->createChild<BoxLayout>(LayoutDir::Horizontal);
    row2->setSpacing(8);
    row2->addChild(new FocusLogged<CheckBox>(console, "CheckBox A", "CheckBox A"));
    auto* chkB = row2->addChild(new FocusLogged<CheckBox>(console, "CheckBox B (disabled)", "CheckBox B"));
    chkB->setEnabled(false);

    left->createChild<Label>("TextInput:");
    auto* ti = left->addChild(new FocusLogged<TextInput>(console, "TextInput"));
    ti->setSize(300, 28);
    ti->setPlaceholder("Click or Tab to focus, then type...");
    ti->setTooltip("TextInput — type anything");

    left->createChild<Label>("SpinBox:");
    auto* spin = left->addChild(new FocusLogged<SpinBox>(console, "SpinBox", 0.f, 100.f, 0.f, 1.f));
    spin->setSize(160, 28);
    spin->setTooltip("SpinBox — arrow keys or scroll");

    left->createChild<Label>("ComboBox:");
    auto* combo = left->addChild(new FocusLogged<ComboBox>(console, "ComboBox"));
    combo->setSize(200, 28);
    combo->addItem("Alpha");
    combo->addItem("Beta");
    combo->addItem("Gamma");

    left->createChild<Label>("Slider:");
    auto* slider = left->addChild(new FocusLogged<Slider>(console, "Slider", 0.f, 100.f, 0.f));
    slider->setSize(300, 28);

    left->createChild<Line>();

    // ── Context menu demo ─────────────────────────────────────────────────
    left->createChild<Label>("Right-click the button below:");

    auto* cmBtn = left->createChild<Button>("Right-click me \u25bc");
    cmBtn->setSize(170, 28);
    cmBtn->setTooltip("Right-click to open a context menu");

    auto* cm = new Menu();
    auto* actCopy  = cm->addAction("Copy");
    auto* actPaste = cm->addAction("Paste");
    cm->addSeparator();
    auto* actDel   = cm->addAction("Delete");
    cmBtn->setContextMenu(cm);

    actCopy->triggered.connect( [console]() { console->log(LogLevel::Info,  "Context: Copy");   });
    actPaste->triggered.connect([console]() { console->log(LogLevel::Info,  "Context: Paste");  });
    actDel->triggered.connect(  [console]() { console->log(LogLevel::Warn,  "Context: Delete"); });
}
