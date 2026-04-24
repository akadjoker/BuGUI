#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Demo stage — SDL_GUI 0100_GUI_App recreation
// ═════════════════════════════════════════════════════════════════════════════

inline void buildDemoStage(WidgetApp& app)
{
    Widget* root = app.addStage("demo");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Menu bar ─────────────────────────────────────────────────────────
    auto* menuBar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    menuBar->setSize(0, 30);
    menuBar->setSpacing(0);
    menuBar->setPadding(4, 4, 0, 0);

    auto* burger = menuBar->createChild<IconButton>(IconButton::Hamburger);
    burger->setSize(36, 26);
    burger->setAnimated(true);

    menuBar->createChild<Button>("File")->setSize(50, 26);
    menuBar->createChild<Button>("Edit")->setSize(50, 26);
    menuBar->createChild<Button>("View")->setSize(50, 26);
    menuBar->createChild<Button>("Format")->setSize(60, 26);
    menuBar->createChild<Button>("Help")->setSize(50, 26);

    menuBar->createChild<Spacer>();

    auto* navPrev = menuBar->createChild<Button>("< Inputs");
    navPrev->setSize(80, 26);
    navPrev->clicked.connect([]{ WidgetApp::instance().setStage("inputs", TransitionType::SlideRight, EaseType::OutBack); });

    // ── Input bar ────────────────────────────────────────────────────────
    auto* inputBar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    inputBar->setSize(0, 38);
    inputBar->setSpacing(0);
    inputBar->setPadding(5, 10, 5, 10);

    // Placeholder for text input (Panel as white input box)
    auto* inputPanel = inputBar->createChild<Panel>();
    inputPanel->setStretch(1);
    inputPanel->setBgColor(Color(255, 255, 255, 255));

    // ── Content area (4 columns) ─────────────────────────────────────────
    auto* content = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    content->setStretch(1);
    content->setSpacing(0);
    content->setPadding(5, 5, 5, 5);

    // ══════════════════════════════════════════════════════════════════════
    //  COLUMN 1 — 240px: Label, List, Button, Label2, RadioGroup
    // ══════════════════════════════════════════════════════════════════════
    auto* col1 = content->createChild<BoxLayout>(LayoutDir::Vertical);
    col1->setSize(240, 0);
    col1->setStretch(0);
    col1->setSpacing(0);
    col1->setPadding(0, 5, 5, 5);

    auto* label1 = col1->createChild<Label>("Label 1");
    label1->setMargins(10, 0, 0, 0);

    auto* list = col1->createChild<ListBox>();
    list->setStretch(1);
    list->setMargins(10, 0, 0, 0);
    for (int i = 1; i <= 20; i++)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "Simple List Item %d", i);
        list->addItem(buf);
    }

    auto* btn1 = col1->createChild<Button>("Button 1");
    btn1->setMargins(10, 0, 0, 0);

    auto* label2 = col1->createChild<Label>("Label 2");
    label2->setMargins(10, 0, 0, 0);
    label2->setColor(Color(180, 180, 180, 255));

    // RadioGroup in bordered container
    static RadioGroup demoRadioGroup;
    auto* radioPanel = col1->createChild<Panel>();
    radioPanel->setMargins(10, 0, 0, 0);
    auto* radioBox = radioPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    radioBox->setSpacing(2);
    radioBox->setPadding(5, 10, 5, 10);
    auto* rb1 = radioBox->createChild<RadioButton>("Radio 1", &demoRadioGroup);
    rb1->setSelected(true);
    radioBox->createChild<RadioButton>("Radio 2", &demoRadioGroup);
    radioBox->createChild<RadioButton>("Radio 3", &demoRadioGroup);

    // Wire list selection → label1
    list->selectionChanged.connect([label1, list](int idx){
        if (idx >= 0 && idx < list->itemCount())
            label1->setText(list->itemText(idx));
    });

    // Wire radio → label2
    demoRadioGroup.selectionChanged.connect([label2](int idx){
        char buf[16];
        snprintf(buf, sizeof(buf), "Radio %d", idx + 1);
        label2->setText(buf);
    });

    // ══════════════════════════════════════════════════════════════════════
    //  COLUMN 2 — 200px: CheckBoxes, icon buttons, image area, buttons, Test Popup
    // ══════════════════════════════════════════════════════════════════════
    auto* col2 = content->createChild<BoxLayout>(LayoutDir::Vertical);
    col2->setSize(200, 0);
    col2->setStretch(0);
    col2->setSpacing(0);
    col2->setPadding(0, 5, 5, 5);

    // CheckBoxes in bordered panel
    auto* cbPanel = col2->createChild<Panel>();
    auto* cbBox = cbPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    cbBox->setSpacing(2);
    cbBox->setPadding(5, 10, 5, 10);
    cbBox->createChild<CheckBox>("Check Box 1");
    auto* cb2 = cbBox->createChild<CheckBox>("Check Box 1");
    cb2->setChecked(true);
    cbBox->createChild<CheckBox>("Check Mix 1");

    // Currency icon row (centered)
    auto* iconRow1 = col2->createChild<BoxLayout>(LayoutDir::Horizontal);
    iconRow1->setMargins(10, 0, 0, 0);
    iconRow1->setSpacing(5);
    iconRow1->setMainAlign(MainAlign::Center);
    iconRow1->createChild<Button>("$")->setSize(36, 30);
    iconRow1->createChild<Button>("\xc2\xa5")->setSize(36, 30);    // ¥
    iconRow1->createChild<Button>("\xc2\xa3")->setSize(36, 30);    // £

    // Image placeholder (grey box)
    auto* imgPlaceholder = col2->createChild<Canvas>();
    imgPlaceholder->setStretch(1);
    imgPlaceholder->setMargins(10, 0, 0, 0);
    imgPlaceholder->setBgColor(Color(60, 60, 65, 255));
    imgPlaceholder->setOnPaint([](PaintContext& ctx, const Rect& b) {
        ctx.font.SetFontSize(13.0f);
        ctx.font.SetColor(Color(120, 120, 130, 255));
        ctx.font.SetBatch(&ctx.text);
        ctx.font.Print("(image placeholder)", b.x + 10, b.y + b.h * 0.5f - 7);
    });

    // Currency row on grey bg
    auto* iconRow2 = col2->createChild<BoxLayout>(LayoutDir::Horizontal);
    iconRow2->setMargins(10, 0, 0, 0);
    iconRow2->setSpacing(5);
    iconRow2->createChild<Button>("$")->setSize(36, 30);
    iconRow2->createChild<Button>("\xc2\xa5")->setSize(36, 30);    // ¥
    iconRow2->createChild<Button>("\xc2\xa3")->setSize(36, 30);    // £

    // Test Popup button
    auto* testPopupBtn = col2->createChild<Button>("Test Popup");
    testPopupBtn->setMargins(10, 0, 0, 0);

    // ══════════════════════════════════════════════════════════════════════
    //  COLUMN 3 — fill: Panel with Progress/Slider left, Switch/Combo right
    // ══════════════════════════════════════════════════════════════════════
    auto* col3 = content->createChild<Panel>();
    col3->setStretch(1);
    col3->setMargins(5, 5, 5, 5);

    auto* col3Inner = col3->createChild<BoxLayout>(LayoutDir::Vertical);
    col3Inner->setSpacing(0);
    col3Inner->setPadding(5);

    // Top row: left panel (Progress+Slider) | right panel (Switch+Combo)
    auto* topRow = col3Inner->createChild<BoxLayout>(LayoutDir::Horizontal);
    topRow->setSpacing(10);
    topRow->setPadding(5);

    // ── Left sub-panel ───────────────────────────────────────────────────
    auto* leftSub = topRow->createChild<BoxLayout>(LayoutDir::Vertical);
    leftSub->setSize(200, 0);
    leftSub->setStretch(0);
    leftSub->setSpacing(6);

    auto* progress = leftSub->createChild<ProgressBar>(0.0f, 100.0f, 25.0f);
    progress->setMargins(0, 10, 0, 10);

    auto* sliderLabel = leftSub->createChild<Label>("25 %");
    sliderLabel->setMargins(0, 10, 0, 10);
    sliderLabel->setColor(Color(180, 180, 180, 255));

    auto* slider = leftSub->createChild<Slider>();
    slider->setRange(0, 100);
    slider->setValue(25);
    slider->setMargins(0, 2, 0, 2);

    slider->onValueChanged.connect([sliderLabel, progress](float val){
        char buf[32];
        snprintf(buf, sizeof(buf), "%d %%", (int)val);
        sliderLabel->setText(buf);
        progress->setValue(val);
    });

    // ── Right sub-panel ──────────────────────────────────────────────────
    auto* rightSub = topRow->createChild<BoxLayout>(LayoutDir::Vertical);
    rightSub->setStretch(1);
    rightSub->setSpacing(6);
    rightSub->setMargins(0, 10, 0, 20);

    // On/Off Switch row
    auto* switchRow = rightSub->createChild<BoxLayout>(LayoutDir::Horizontal);
    switchRow->setSpacing(4);
    switchRow->createChild<Label>("On/Off Switch:");
    switchRow->createChild<Spacer>();
    switchRow->createChild<Switch>("");

    // ComboBox
    auto* combo = rightSub->createChild<ComboBox>();
    combo->setMargins(10, 0, 0, 0);
    combo->addItem("Combo Box Item #1");
    combo->addItem("Combo Box Item #2");
    combo->addItem("Combo Box Item #3");
    combo->addItem("Combo Box Item #4");
    combo->addItem("Combo Box Item #5");
    combo->addItem("Combo Box Item #6");
    combo->addItem("Combo Box Item #7");

    // ══════════════════════════════════════════════════════════════════════
    //  COLUMN 4 — small: 3 currency icon buttons stacked
    // ══════════════════════════════════════════════════════════════════════
    auto* col4 = content->createChild<Panel>();
    col4->setSize(60, 0);
    col4->setStretch(0);
    col4->setMargins(5, 5, 5, 5);

    auto* col4Inner = col4->createChild<BoxLayout>(LayoutDir::Vertical);
    col4Inner->setSpacing(5);
    col4Inner->setPadding(5);
    col4Inner->setMainAlign(MainAlign::Start);

    col4Inner->createChild<Button>("$")->setSize(40, 32);
    col4Inner->createChild<Button>("\xc2\xa5")->setSize(40, 32);   // ¥
    col4Inner->createChild<Button>("\xc2\xa3")->setSize(40, 32);   // £

    // ── Sidebar drawer (hamburger) ───────────────────────────────────────
    auto* drawer = root->createChild<SidePanel>(SidePanel::Left, 260.0f);
    auto* drawerContent = drawer->setContent<BoxLayout>(LayoutDir::Vertical);
    drawerContent->setSpacing(4);
    drawerContent->setPadding(12);

    // Drawer header row with title + close button
    auto* drawerHeader = drawerContent->createChild<BoxLayout>(LayoutDir::Horizontal);
    drawerHeader->setSize(0, 28);
    drawerHeader->setSpacing(0);
    auto* drawerTitle = drawerHeader->createChild<Label>("Navigation");
    drawerTitle->setColor(Clr::accent);
    drawerTitle->setStretch(1);
    auto* drawerClose = drawerHeader->createChild<IconButton>(IconButton::Close);
    drawerClose->setSize(28, 28);
    drawerClose->clicked.connect([drawer]{ drawer->close(); });

    drawerContent->createChild<Line>();

    // Stage links in drawer
    const char* stageNames[] = {
        "layout", "canvas", "widgets", "scroll", "calc",
        "grid", "border", "layouts2", "tabs", "anchor",
        "inputs", "demo", "atlas"
    };
    for (auto* name : stageNames)
    {
        auto* btn = drawerContent->createChild<Button>(name);
        std::string sn = name;
        btn->clicked.connect([sn, drawer]{
            drawer->close();
            WidgetApp::instance().setStage(sn);
        });
    }

    // Wire hamburger button to drawer (animate burger ↔ X)
    burger->clicked.connect([drawer, burger]{ drawer->toggle(); });
    drawer->openChanged.connect([burger](bool open){
        burger->setIcon(open ? IconButton::Close : IconButton::Hamburger);
    });

    // ── Status bar ───────────────────────────────────────────────────────
    auto* status = outer->createChild<StatusBar>();
    status->setText("Demo Stage \xe2\x80\x94 SDL_GUI Style"); // —
}
