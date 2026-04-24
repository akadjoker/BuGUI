#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Input widgets demo — RadioButton, Switch, ListBox
// ═════════════════════════════════════════════════════════════════════════════

inline void buildInputsStage(WidgetApp& app)
{
    Widget* root = app.addStage("inputs");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Navigation ───────────────────────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    nav->setPadding(16, 16, 0, 16);
    auto* back = nav->createChild<Button>("<  Anchor");
    back->clicked.connect([]{ WidgetApp::instance().setStage("anchor", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("Input Widgets")->setColor(Clr::accent);
    nav->createChild<Spacer>();
    auto* next = nav->createChild<Button>("Demo  >");
    next->clicked.connect([]{ WidgetApp::instance().setStage("demo"); });

    // ── Scrollable body ──────────────────────────────────────────────────
    auto* scroll = outer->createChild<ScrollView>();
    scroll->setStretch(1);
    auto* body = scroll->setContent<BoxLayout>(LayoutDir::Vertical);
    body->setSpacing(6);
    body->setPadding(16, 16, 8, 16);

    // ══════════════════════════════════════════════════════════════════════
    //  1) RadioButton + RadioGroup
    // ══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "RadioButton + RadioGroup (exclusive selection)");

    auto* radioRow = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    radioRow->setSpacing(16);

    // Group A: Theme selector
    auto* colA = radioRow->createChild<BoxLayout>(LayoutDir::Vertical);
    colA->setSpacing(4);
    colA->createChild<Label>("Theme")->setColor(Clr::dim);

    static RadioGroup themeGroup;
    auto* rbDark  = colA->createChild<RadioButton>("Dark",  &themeGroup);
    auto* rbLight = colA->createChild<RadioButton>("Light", &themeGroup);
                    colA->createChild<RadioButton>("System", &themeGroup);
    rbDark->setSelected(true); // default

    auto* themeLabel = colA->createChild<Label>("Selected: Dark");
    themeLabel->setColor(Color(100, 200, 100, 255));
    themeGroup.selectionChanged.connect([themeLabel](int idx) {
        const char* names[] = {"Dark", "Light", "System"};
        char buf[48];
        snprintf(buf, sizeof(buf), "Selected: %s", (idx >= 0 && idx < 3) ? names[idx] : "?");
        themeLabel->setText(buf);
    });

    // Group B: Size selector
    auto* colB = radioRow->createChild<BoxLayout>(LayoutDir::Vertical);
    colB->setSpacing(4);
    colB->createChild<Label>("Font Size")->setColor(Clr::dim);

    static RadioGroup sizeGroup;
    colB->createChild<RadioButton>("Small",  &sizeGroup);
    auto* rbMed = colB->createChild<RadioButton>("Medium", &sizeGroup);
    colB->createChild<RadioButton>("Large",  &sizeGroup);
    rbMed->setSelected(true);

    auto* sizeLabel = colB->createChild<Label>("Selected: Medium");
    sizeLabel->setColor(Color(100, 200, 100, 255));
    sizeGroup.selectionChanged.connect([sizeLabel](int idx) {
        const char* names[] = {"Small", "Medium", "Large"};
        char buf[48];
        snprintf(buf, sizeof(buf), "Selected: %s", (idx >= 0 && idx < 3) ? names[idx] : "?");
        sizeLabel->setText(buf);
    });

    // ══════════════════════════════════════════════════════════════════════
    //  2) Switch — on/off toggles
    // ══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "Switch (on/off toggle)");

    auto* switchCol = body->createChild<BoxLayout>(LayoutDir::Vertical);
    switchCol->setSpacing(4);

    auto* sw1 = switchCol->createChild<Switch>("Dark Mode");
    sw1->setOn(true);
    auto* sw2 = switchCol->createChild<Switch>("Notifications");
    auto* sw3 = switchCol->createChild<Switch>("Auto-save");
    sw3->setOn(true);
    auto* sw4 = switchCol->createChild<Switch>("GPU Acceleration");
    sw4->setOnColor(Color(60, 120, 200, 255));

    auto* switchStatus = switchCol->createChild<Label>("");
    switchStatus->setColor(Clr::dim);

    auto updateSwitchStatus = [=]() {
        char buf[128];
        snprintf(buf, sizeof(buf), "Dark:%s  Notif:%s  Save:%s  GPU:%s",
            sw1->isOn() ? "ON" : "off",
            sw2->isOn() ? "ON" : "off",
            sw3->isOn() ? "ON" : "off",
            sw4->isOn() ? "ON" : "off");
        switchStatus->setText(buf);
    };
    updateSwitchStatus();
    sw1->toggled.connect([=](bool) { updateSwitchStatus(); });
    sw2->toggled.connect([=](bool) { updateSwitchStatus(); });
    sw3->toggled.connect([=](bool) { updateSwitchStatus(); });
    sw4->toggled.connect([=](bool) { updateSwitchStatus(); });

    // ══════════════════════════════════════════════════════════════════════
    //  3) ListBox — selectable list
    // ══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "ListBox (scrollable, selectable items)");

    auto* listRow = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    listRow->setSpacing(12);
    listRow->setSize(0, 180);

    // List A: Fonts
    auto* listColA = listRow->createChild<BoxLayout>(LayoutDir::Vertical);
    listColA->setSpacing(4);
    listColA->setStretch(1);
    listColA->createChild<Label>("Fonts")->setColor(Clr::dim);

    auto* listA = listColA->createChild<ListBox>();
    listA->setStretch(1);
    listA->addItem("Noto Sans");
    listA->addItem("Roboto");
    listA->addItem("Inter");
    listA->addItem("JetBrains Mono");
    listA->addItem("Fira Code");
    listA->addItem("Source Code Pro");
    listA->addItem("Cascadia Code");
    listA->addItem("IBM Plex Mono");
    listA->addItem("Inconsolata");
    listA->addItem("Hack");
    listA->addItem("Ubuntu Mono");
    listA->addItem("Droid Sans Mono");
    listA->setSelectedIndex(0);

    // List B: Colors
    auto* listColB = listRow->createChild<BoxLayout>(LayoutDir::Vertical);
    listColB->setSpacing(4);
    listColB->setStretch(1);
    listColB->createChild<Label>("Colors")->setColor(Clr::dim);

    auto* listB = listColB->createChild<ListBox>();
    listB->setStretch(1);
    listB->addItem("Red");
    listB->addItem("Green");
    listB->addItem("Blue");
    listB->addItem("Yellow");
    listB->addItem("Cyan");
    listB->addItem("Magenta");
    listB->addItem("Orange");
    listB->addItem("Purple");
    listB->setSelectedIndex(2);

    // Selection feedback
    auto* selLabel = body->createChild<Label>("Font: Noto Sans | Color: Blue");
    selLabel->setColor(Color(100, 200, 100, 255));

    auto updateSelLabel = [=]() {
        int fi = listA->selectedIndex();
        int ci = listB->selectedIndex();
        char buf[128];
        snprintf(buf, sizeof(buf), "Font: %s | Color: %s",
            fi >= 0 ? listA->itemText(fi).c_str() : "none",
            ci >= 0 ? listB->itemText(ci).c_str() : "none");
        selLabel->setText(buf);
    };
    listA->selectionChanged.connect([=](int) { updateSelLabel(); });
    listB->selectionChanged.connect([=](int) { updateSelLabel(); });

    // ══════════════════════════════════════════════════════════════════════
    //  4) CheckBox — vertical + horizontal groups
    // ══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "CheckBox (vertical group)");

    auto* cbCol = body->createChild<BoxLayout>(LayoutDir::Vertical);
    cbCol->setSpacing(4);
    cbCol->createChild<CheckBox>("Wireframe");
    cbCol->createChild<CheckBox>("Shadows")->setChecked(true);
    cbCol->createChild<CheckBox>("Anti-aliasing")->setChecked(true);
    cbCol->createChild<CheckBox>("VSync");

    sectionLabel(body, "CheckBox (horizontal group)");

    auto* cbRow = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    cbRow->setSpacing(12);
    cbRow->createChild<CheckBox>("Bold");
    cbRow->createChild<CheckBox>("Italic");
    cbRow->createChild<CheckBox>("Underline")->setChecked(true);
    cbRow->createChild<CheckBox>("Strikethrough");

    // ══════════════════════════════════════════════════════════════════════
    //  5) ComboBox — dropdown selectors
    // ══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "ComboBox (dropdown selector)");

    auto* comboRow = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    comboRow->setSpacing(12);

    auto* comboColA = comboRow->createChild<BoxLayout>(LayoutDir::Vertical);
    comboColA->setSpacing(4);
    comboColA->setStretch(1);
    comboColA->createChild<Label>("Font")->setColor(Clr::dim);
    auto* comboFont = comboColA->createChild<ComboBox>();
    comboFont->addItem("Noto Sans");
    comboFont->addItem("Roboto");
    comboFont->addItem("Inter");
    comboFont->addItem("JetBrains Mono");
    comboFont->addItem("Fira Code");
    comboFont->addItem("Source Code Pro");
    comboFont->setSelectedIndex(0);

    auto* comboColB = comboRow->createChild<BoxLayout>(LayoutDir::Vertical);
    comboColB->setSpacing(4);
    comboColB->setStretch(1);
    comboColB->createChild<Label>("Color")->setColor(Clr::dim);
    auto* comboColor = comboColB->createChild<ComboBox>();
    comboColor->addItem("Red");
    comboColor->addItem("Green");
    comboColor->addItem("Blue");
    comboColor->addItem("Yellow");
    comboColor->addItem("Cyan");
    comboColor->setSelectedIndex(2);

    auto* comboStatus = body->createChild<Label>("");
    comboStatus->setColor(Color(100, 200, 100, 255));
    auto updateComboStatus = [=]() {
        char buf[128];
        snprintf(buf, sizeof(buf), "Font: %s | Color: %s",
            comboFont->currentText().c_str(),
            comboColor->currentText().c_str());
        comboStatus->setText(buf);
    };
    updateComboStatus();
    comboFont->selectionChanged.connect([=](int) { updateComboStatus(); });
    comboColor->selectionChanged.connect([=](int) { updateComboStatus(); });

    // ══════════════════════════════════════════════════════════════════════
    //  6) ListWidget — widget rows
    // ══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "ListWidget (custom widget rows)");

    auto* lw = body->createChild<ListWidget>();
    lw->setSize(0, 180);

    for (int i = 0; i < 10; ++i)
    {
        auto* row = lw->addRow<BoxLayout>(LayoutDir::Horizontal);
        row->setSpacing(8);
        row->setPadding(4, 4, 4, 4);

        char buf[64];
        snprintf(buf, sizeof(buf), "Item %d", i + 1);
        auto* lbl = row->createChild<Label>(buf);
        lbl->setStretch(1);

        auto* btn = row->createChild<Button>("Edit");
        btn->setSize(50, 22);
    }

    auto* lwStatus = body->createChild<Label>("Selected: none");
    lwStatus->setColor(Color(100, 200, 100, 255));
    lw->selectionChanged.connect([lwStatus](int idx) {
        char buf[64];
        if (idx >= 0)
            snprintf(buf, sizeof(buf), "Selected: Item %d", idx + 1);
        else
            snprintf(buf, sizeof(buf), "Selected: none");
        lwStatus->setText(buf);
    });

    // ══════════════════════════════════════════════════════════════════════
    //  7) Slider + ProgressBar (recap)
    // ══════════════════════════════════════════════════════════════════════
    sectionLabel(body, "Slider + ProgressBar (recap)");

    auto* sliderCol = body->createChild<BoxLayout>(LayoutDir::Vertical);
    sliderCol->setSpacing(6);

    auto* sliderLbl = sliderCol->createChild<Label>("Brightness: 50%");
    sliderLbl->setColor(Clr::dim);
    auto* slider = sliderCol->createChild<Slider>(0.0f, 1.0f, 0.5f);
    slider->onValueChanged.connect([sliderLbl](float v) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Brightness: %d%%", static_cast<int>(v * 100));
        sliderLbl->setText(buf);
    });

    auto* pb = sliderCol->createChild<ProgressBar>();
    pb->setValue(0.72f);
    pb->setText("72% loaded");

    auto* vSlider = sliderCol->createChild<Slider>(0.0f, 100.0f, 65.0f);
    vSlider->setOrientation(LayoutDir::Vertical);
    vSlider->setSize(20, 80);

    // ── StatusBar ────────────────────────────────────────────────────────
    auto* status = outer->createChild<StatusBar>();
    status->setText("RadioButton + Switch + ListBox + ComboBox + ListWidget");
}
