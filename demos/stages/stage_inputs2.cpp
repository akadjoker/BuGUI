#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "TextInputWidgets.hpp"
#include "ComboBox.hpp"

void registerInputs2Stage(WidgetApp& app)
{
    auto* root = app.addStage("inputs2");
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(8.0f);
    vbox->setPadding(16.0f);

    // Back button
    auto* back = vbox->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
    vbox->createChild<Line>();

    // TabLayout
    auto* tabs = vbox->createChild<TabLayout>();
    tabs->setStretch(1.0f);

    // ─────────────────────────────────────────────────────────────────────────
    //  Tab 1 — TextInput variants
    // ─────────────────────────────────────────────────────────────────────────
    auto* p1 = tabs->addTab<BoxLayout>("TextInput", LayoutDir::Vertical);
    p1->setSpacing(12.0f);
    p1->setPadding(12.0f);

    auto* form1 = p1->createChild<FormLayout>();
    form1->setLabelWidth(110.0f);

    // Normal
    auto* tiNormal = form1->addRow<TextInput>("Normal:", "", TextInput::Mode::Normal);
    tiNormal->setPlaceholder("Type here...");

    // Password
    auto* tiPwd = form1->addRow<TextInput>("Password:", "", TextInput::Mode::Password);
    tiPwd->setPlaceholder("Secret...");

    // Numbers only
    auto* tiNum = form1->addRow<TextInput>("Number:", "3.14", TextInput::Mode::NumberOnly);

    // Read-only
    form1->addRow<TextInput>("Read-only:", "Cannot edit me", TextInput::Mode::ReadOnly);

    p1->createChild<Line>();

    // Live feedback label
    auto* feedLabel = p1->createChild<Label>("Value: (empty)");
    tiNormal->textChanged.connect([feedLabel](const std::string& t) {
        feedLabel->setText("Value: " + (t.empty() ? "(empty)" : t));
    });
    tiNum->submitted.connect([feedLabel](const std::string& t) {
        feedLabel->setText("Submitted: " + t);
    });

    // ─────────────────────────────────────────────────────────────────────────
    //  Tab 2 — TextEdit (multi-line)
    // ─────────────────────────────────────────────────────────────────────────
    auto* p2 = tabs->addTab<BoxLayout>("TextEdit", LayoutDir::Vertical);
    p2->setSpacing(8.0f);
    p2->setPadding(12.0f);

    // Toolbar row
    auto* toolbar = p2->createChild<BoxLayout>(LayoutDir::Horizontal);
    toolbar->setSpacing(6.0f);

    auto* btnLineNums = toolbar->createChild<CheckBox>("Line numbers");
    btnLineNums->setChecked(true);
    auto* btnWrap = toolbar->createChild<CheckBox>("Word wrap");

    // Editor
    static const char* kInitialText =
        "// BuGUI TextEdit demo\n"
        "// Multi-line editor with line numbers,\n"
        "// word-wrap, selection and keyboard shortcuts.\n\n"
        "void hello() {\n"
        "    print(\"Hello, world!\");\n"
        "}\n";

    auto* editor = p2->createChild<TextEdit>(kInitialText);
    editor->setStretch(1.0f);
    editor->setShowLineNumbers(true);

    // Cursor position label
    auto* posLabel = p2->createChild<Label>("Ln 1, Col 1");
    editor->cursorMoved.connect([posLabel](TextEdit::TextPos pos) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Ln %d, Col %d", pos.line + 1, pos.col + 1);
        posLabel->setText(buf);
    });

    btnLineNums->toggled.connect([editor](bool v) { editor->setShowLineNumbers(v); });
    btnWrap->toggled.connect([editor](bool v)     { editor->setWordWrap(v); });

    // ─────────────────────────────────────────────────────────────────────────
    //  Tab 3 — ComboBox
    // ─────────────────────────────────────────────────────────────────────────
    auto* p3 = tabs->addTab<BoxLayout>("ComboBox", LayoutDir::Vertical);
    p3->setSpacing(12.0f);
    p3->setPadding(12.0f);

    auto* form3 = p3->createChild<FormLayout>();
    form3->setLabelWidth(110.0f);

    // Basic combo
    auto* cb1 = form3->addRow<ComboBox>("Fruit:");
    cb1->addItem("Apple");
    cb1->addItem("Banana");
    cb1->addItem("Cherry");
    cb1->addItem("Durian");
    cb1->addItem("Elderberry");
    cb1->addItem("Fig");
    cb1->setSelectedIndex(0);

    // Font size combo
    auto* cb2 = form3->addRow<ComboBox>("Font size:");
    for (const char* sz : {"10", "12", "14", "16", "18", "20", "24", "32", "48"})
        cb2->addItem(sz);
    cb2->setSelectedIndex(3);

    // Theme combo
    auto* cb3 = form3->addRow<ComboBox>("Theme:");
    cb3->addItem("Dark");
    cb3->addItem("Light");
    cb3->addItem("Solarized");
    cb3->addItem("Nord");
    cb3->setMaxVisible(4);

    p3->createChild<Line>();

    auto* selLabel = p3->createChild<Label>("Selected: Apple");
    cb1->selectionChanged.connect([cb1, selLabel](int /*idx*/) {
        selLabel->setText("Selected: " + cb1->currentText());
    });
}
