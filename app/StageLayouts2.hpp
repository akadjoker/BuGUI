#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Layouts-2 demo: StackLayout, FormLayout, FlowLayout, Overlay, Splitter
// ═════════════════════════════════════════════════════════════════════════════

inline void buildLayouts2Stage(WidgetApp& app)
{
    Widget* root = app.addStage("layouts2");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Navigation ───────────────────────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    nav->setPadding(16, 16, 0, 16);
    auto* back = nav->createChild<Button>("<  Border");
    back->clicked.connect([]{ WidgetApp::instance().setStage("border", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("New Layouts")->setColor(Clr::accent);
    auto* fwd = nav->createChild<Button>("Tabs  >");
    fwd->clicked.connect([]{ WidgetApp::instance().setStage("tabs", TransitionType::SlideLeft, EaseType::OutBack); });

    auto* body = outer->createChild<BoxLayout>(LayoutDir::Vertical);
    body->setStretch(1.0f);
    body->setSpacing(6);
    body->setPadding(8, 4, 8, 4);

    // ═════════════════════════════════════════════════════════════════════
    //  1) StackLayout — page switcher
    // ═════════════════════════════════════════════════════════════════════
    sectionLabel(body, "StackLayout");

    auto* stackRow = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    stackRow->setSpacing(6);
    stackRow->setSize(0, 90);

    // Buttons column
    auto* stackBtns = stackRow->createChild<BoxLayout>(LayoutDir::Vertical);
    stackBtns->setSpacing(4);
    stackBtns->setSize(80, 0);

    auto* stack = stackRow->createChild<StackLayout>();
    stack->setStretch(1.0f);

    // Page 0 - red
    auto* page0 = stack->createChild<Panel>();
    page0->setBgColor(Color(180, 60, 60, 255));
    page0->createChild<Label>("Page 0 (Red)")->setColor(Color(255, 255, 255, 255));

    // Page 1 - green
    auto* page1 = stack->createChild<Panel>();
    page1->setBgColor(Color(60, 160, 60, 255));
    page1->createChild<Label>("Page 1 (Green)")->setColor(Color(255, 255, 255, 255));

    // Page 2 - blue
    auto* page2 = stack->createChild<Panel>();
    page2->setBgColor(Color(60, 80, 180, 255));
    page2->createChild<Label>("Page 2 (Blue)")->setColor(Color(255, 255, 255, 255));

    auto* btn0 = stackBtns->createChild<Button>("Page 0");
    auto* btn1 = stackBtns->createChild<Button>("Page 1");
    auto* btn2 = stackBtns->createChild<Button>("Page 2");
    btn0->clicked.connect([stack]{ stack->setCurrentIndex(0); });
    btn1->clicked.connect([stack]{ stack->setCurrentIndex(1); });
    btn2->clicked.connect([stack]{ stack->setCurrentIndex(2); });

    // ═════════════════════════════════════════════════════════════════════
    //  2) FormLayout — label:widget pairs
    // ═════════════════════════════════════════════════════════════════════
    sectionLabel(body, "FormLayout");

    auto* formBox = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    formBox->setSpacing(12);

    auto* form = formBox->createChild<FormLayout>();
    form->setSize(320, 0);
    form->setLabelWidth(90);
    form->setSpacing(8, 6);
    form->setPadding(6);

    form->addRow<Slider>("Position X");
    form->addRow<Slider>("Position Y");
    form->addRow<Slider>("Rotation");
    form->addRow<Slider>("Scale");
    form->addRow<CheckBox>("Visible");
    form->addRow<CheckBox>("Lock");

    dimLabel(formBox, "90px label column\n8x6 spacing\n6px padding");

    // ═════════════════════════════════════════════════════════════════════
    //  3) FlowLayout — wrapped tags
    // ═════════════════════════════════════════════════════════════════════
    sectionLabel(body, "FlowLayout");

    auto* flow = body->createChild<FlowLayout>();
    flow->setSpacing(4, 4);
    flow->setPadding(6);

    const char* tags[] = {
        "C++17", "SDL2", "OpenGL", "BuGUI", "Widgets",
        "Layouts", "Signals", "Tooltips", "Clipping",
        "Scrolling", "Themes", "Serialization", "Cross-Platform",
        "FlowLayout", "StackLayout", "FormLayout"
    };
    const Color tagColors[] = {
        {100, 160, 220, 255}, {200, 100, 60, 255}, {80, 180, 80, 255},
        {180, 120, 200, 255}, {200, 180, 60, 255}, {120, 200, 180, 255},
        {100, 160, 220, 255}, {200, 100, 60, 255}, {80, 180, 80, 255},
        {180, 120, 200, 255}, {200, 180, 60, 255}, {120, 200, 180, 255},
        {100, 160, 220, 255}, {200, 100, 60, 255}, {80, 180, 80, 255},
        {180, 120, 200, 255}
    };
    for (int i = 0; i < 16; ++i) {
        auto* b = flow->createChild<Button>(tags[i]);
        b->setBgColor(tagColors[i]);
    }

    // ═════════════════════════════════════════════════════════════════════
    //  4) Splitter — draggable divider
    // ═════════════════════════════════════════════════════════════════════
    sectionLabel(body, "Splitter (drag handle, ratio clamped 0.2–0.8)");

    auto* split = body->createChild<Splitter>(LayoutDir::Horizontal);
    split->setStretch(1.0f);
    split->setRatio(0.4f);
    split->setMinRatio(0.2f);
    split->setMaxRatio(0.8f);
    split->setHandleSize(6);
    split->setMinSize(60);

    // Left panel
    auto* leftPanel = split->createChild<Panel>();
    leftPanel->setBgColor(Color(45, 48, 55, 255));
    auto* leftVBox = leftPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    leftVBox->setPadding(8);
    leftVBox->setSpacing(4);
    leftVBox->createChild<Label>("Left Panel")->setColor(Clr::accent);
    leftVBox->createChild<Label>("40% initial ratio")->setColor(Clr::dim);
    leftVBox->createChild<Slider>();
    leftVBox->createChild<CheckBox>("Option A");
    leftVBox->createChild<CheckBox>("Option B");

    // Right panel — nested vertical splitter
    auto* rightSplit = split->createChild<Splitter>(LayoutDir::Vertical);
    rightSplit->setRatio(0.6f);
    rightSplit->setMinRatio(0.25f);
    rightSplit->setMaxRatio(0.75f);
    rightSplit->setHandleSize(5);
    rightSplit->setMinSize(40);

    auto* topPanel = rightSplit->createChild<Panel>();
    topPanel->setBgColor(Color(50, 55, 60, 255));
    auto* topVBox = topPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    topVBox->setPadding(8);
    topVBox->setSpacing(4);
    topVBox->createChild<Label>("Top (60%)")->setColor(Clr::accent);
    topVBox->createChild<ProgressBar>()->setValue(0.75f);

    auto* botPanel = rightSplit->createChild<Panel>();
    botPanel->setBgColor(Color(40, 42, 48, 255));
    auto* botVBox = botPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    botVBox->setPadding(8);
    botVBox->createChild<Label>("Bottom (40%)")->setColor(Clr::accent);
    auto* vRatioLbl = botVBox->createChild<Label>("V-ratio: 0.60  [0.25–0.75]");
    vRatioLbl->setColor(Clr::dim);
    rightSplit->ratioChanged.connect([vRatioLbl](float r) {
        char buf[48];
        snprintf(buf, sizeof(buf), "V-ratio: %.2f  [0.25–0.75]", r);
        vRatioLbl->setText(buf);
    });

    // Ratio display label
    auto* ratioLbl = botVBox->createChild<Label>("H-ratio: 0.40  [0.20–0.80]");
    ratioLbl->setColor(Clr::dim);
    split->ratioChanged.connect([ratioLbl](float r) {
        char buf[48];
        snprintf(buf, sizeof(buf), "H-ratio: %.2f  [0.20–0.80]", r);
        ratioLbl->setText(buf);
    });

    // ═════════════════════════════════════════════════════════════════════
    //  5) TabLayout — tabbed pages with closable tabs
    // ═════════════════════════════════════════════════════════════════════
    sectionLabel(body, "TabLayout (closable tabs, position: Top)");

    auto* tabRow = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    tabRow->setSpacing(8);
    tabRow->setSize(0, 140);

    auto* tabs = tabRow->createChild<TabLayout>();
    tabs->setStretch(1.0f);
    tabs->setTabsClosable(true);

    // Tab 0: Scene
    auto* sceneTab = tabs->addTab<Panel>("Scene");
    sceneTab->setBgColor(Color(40, 42, 50, 255));
    auto* sceneBox = sceneTab->createChild<BoxLayout>(LayoutDir::Vertical);
    sceneBox->setPadding(8);
    sceneBox->setSpacing(4);
    sceneBox->createChild<Label>("Scene View")->setColor(Clr::accent);
    sceneBox->createChild<ProgressBar>()->setValue(0.6f);
    sceneBox->createChild<CheckBox>("Wireframe");

    // Tab 1: Assets
    auto* assetsTab = tabs->addTab<Panel>("Assets");
    assetsTab->setBgColor(Color(42, 45, 52, 255));
    auto* assetsBox = assetsTab->createChild<BoxLayout>(LayoutDir::Vertical);
    assetsBox->setPadding(8);
    assetsBox->createChild<Label>("Asset Browser")->setColor(Clr::accent);
    assetsBox->createChild<Label>("12 textures, 3 models")->setColor(Clr::dim);

    // Tab 2: Console
    auto* consoleTab = tabs->addTab<Panel>("Console");
    consoleTab->setBgColor(Color(35, 38, 44, 255));
    auto* consoleBox = consoleTab->createChild<BoxLayout>(LayoutDir::Vertical);
    consoleBox->setPadding(8);
    consoleBox->createChild<Label>("Console Output")->setColor(Clr::accent);
    consoleBox->createChild<Label>("> Build succeeded (0.42s)")->setColor(Color(80, 200, 80, 255));

    // Tab 3: Properties
    tabs->addTab<Panel>("Properties")->setBgColor(Color(38, 40, 48, 255));

    // Position selector
    auto* posCol = tabRow->createChild<BoxLayout>(LayoutDir::Vertical);
    posCol->setSpacing(2);
    posCol->setSize(80, 0);
    posCol->createChild<Label>("Position:")->setColor(Clr::dim);
    auto* bTop = posCol->createChild<Button>("Top");
    auto* bBot = posCol->createChild<Button>("Bottom");
    auto* bLft = posCol->createChild<Button>("Left");
    auto* bRgt = posCol->createChild<Button>("Right");
    bTop->clicked.connect([tabs]{ tabs->setTabPosition(TabPosition::Top); });
    bBot->clicked.connect([tabs]{ tabs->setTabPosition(TabPosition::Bottom); });
    bLft->clicked.connect([tabs]{ tabs->setTabPosition(TabPosition::Left); });
    bRgt->clicked.connect([tabs]{ tabs->setTabPosition(TabPosition::Right); });

    // ═════════════════════════════════════════════════════════════════════
    //  6) AnchorLayout — positioned by anchors
    // ═════════════════════════════════════════════════════════════════════
    sectionLabel(body, "AnchorLayout (anchors: center, corners, stretch)");

    auto* anchorBox = body->createChild<AnchorLayout>();
    anchorBox->setSize(0, 120);
    anchorBox->setStretch(0);

    // Background panel — fills entire area
    auto* ancBg = anchorBox->createChild<Panel>();
    ancBg->setBgColor(Color(35, 38, 44, 255));

    // Top-left label
    auto* tlLbl = anchorBox->createChild<Label>("Top-Left");
    tlLbl->setColor(Color(255, 180, 80, 255));
    AnchorLayout::setAnchors(tlLbl, {0, 0, 0, 0, 8, 80, 4, 22});

    // Top-right label
    auto* trLbl = anchorBox->createChild<Label>("Top-Right");
    trLbl->setColor(Color(80, 200, 255, 255));
    AnchorLayout::setAnchors(trLbl, {1, 1, 0, 0, -80, -8, 4, 22});

    // Bottom-left button
    auto* blBtn = anchorBox->createChild<Button>("BL");
    AnchorLayout::setAnchors(blBtn, {0, 0, 1, 1, 8, 60, -32, -4});

    // Bottom-right button
    auto* brBtn = anchorBox->createChild<Button>("BR");
    AnchorLayout::setAnchors(brBtn, {1, 1, 1, 1, -60, -8, -32, -4});

    // Center button — centered horizontally and vertically
    auto* centerBtn = anchorBox->createChild<Button>("Centered");
    centerBtn->setBgColor(Color(80, 120, 180, 255));
    AnchorLayout::setAnchors(centerBtn, {0.5f, 0.5f, 0.5f, 0.5f, -50, 50, -14, 14});

    // Stretch bar — full width near bottom
    auto* stretchBar = anchorBox->createChild<ProgressBar>();
    stretchBar->setValue(0.7f);
    AnchorLayout::setAnchors(stretchBar, {0, 1, 1, 1, 70, -70, -55, -40});
}
