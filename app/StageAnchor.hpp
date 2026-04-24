#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  AnchorLayout + Carousel demo — full stage
// ═════════════════════════════════════════════════════════════════════════════

inline void buildAnchorStage(WidgetApp& app)
{
    Widget* root = app.addStage("anchor");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Navigation ───────────────────────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    nav->setPadding(16, 16, 0, 16);
    auto* back = nav->createChild<Button>("<  Tabs");
    back->clicked.connect([]{ WidgetApp::instance().setStage("tabs", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("Anchor + Carousel")->setColor(Clr::accent);
    auto* fwd = nav->createChild<Button>("Inputs  >");
    fwd->clicked.connect([]{ WidgetApp::instance().setStage("inputs", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── Body: vertical split with Splitter ───────────────────────────────
    auto* split = outer->createChild<Splitter>(LayoutDir::Vertical);
    split->setStretch(1);
    split->setRatio(0.55f);
    split->setMinRatio(0.3f);
    split->setMaxRatio(0.75f);
    split->setHandleSize(5);

    // ══════════════════════════════════════════════════════════════════════
    //  TOP: Carousel
    // ══════════════════════════════════════════════════════════════════════
    auto* topPanel = split->createChild<Panel>();
    topPanel->setBgColor(Color(30, 32, 38, 255));
    auto* topBox = topPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    topBox->setPadding(8);
    topBox->setSpacing(4);

    sectionLabel(topBox, "Carousel (auto-play 3s, loop, arrows + dots)");

    auto* carousel = topBox->createChild<Carousel>();
    carousel->setStretch(1);
    carousel->setAutoPlay(true);
    carousel->setAutoPlayInterval(3.0f);
    carousel->setLoop(true);
    carousel->setAnimDuration(0.4f);

    // Page 0: Welcome
    {
        auto* p = carousel->addPage<Panel>();
        p->setBgColor(Color(45, 50, 70, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(20);
        box->setSpacing(8);
        box->createChild<Label>("Welcome to BuGUI")->setColor(Color(100, 180, 255, 255));
        box->createChild<Label>("A Qt-like widget toolkit built with SDL2 + OpenGL")->setColor(Color(200, 200, 205, 255));
        box->createChild<Label>("11 layouts • signals • transitions • themes")->setColor(Clr::dim);
        auto* bar = box->createChild<ProgressBar>();
        bar->setValue(0.85f);
        bar->setText("85% complete");
    }

    // Page 1: Features
    {
        auto* p = carousel->addPage<Panel>();
        p->setBgColor(Color(50, 40, 55, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(20);
        box->setSpacing(4);
        box->createChild<Label>("Features")->setColor(Color(200, 140, 255, 255));

        auto* flow = box->createChild<FlowLayout>();
        flow->setSpacing(4, 4);
        flow->setStretch(1);
        const char* features[] = {
            "BoxLayout", "GridLayout", "BorderLayout", "StackLayout",
            "FormLayout", "FlowLayout", "Splitter", "TabLayout",
            "AnchorLayout", "Carousel", "Overlay"
        };
        const Color fColors[] = {
            {100, 160, 220, 255}, {200, 100, 60, 255}, {80, 180, 80, 255},
            {180, 120, 200, 255}, {200, 180, 60, 255}, {120, 200, 180, 255},
            {100, 160, 220, 255}, {200, 100, 60, 255}, {80, 180, 80, 255},
            {180, 120, 200, 255}, {200, 180, 60, 255}
        };
        for (int i = 0; i < 11; ++i)
        {
            auto* b = flow->createChild<Button>(features[i]);
            b->setBgColor(fColors[i]);
        }
    }

    // Page 2: Stats
    {
        auto* p = carousel->addPage<Panel>();
        p->setBgColor(Color(40, 55, 45, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(20);
        box->setSpacing(6);
        box->createChild<Label>("Performance")->setColor(Color(100, 220, 140, 255));
        box->createChild<Label>("3-batch rendering: Fill + Line + Text")->setColor(Color(200, 200, 205, 255));
        box->createChild<Label>("CPU clip stack (no GL scissor)")->setColor(Clr::dim);
        box->createChild<Label>("Sutherland-Hodgman polygon clipping")->setColor(Clr::dim);

        auto* row = box->createChild<BoxLayout>(LayoutDir::Horizontal);
        row->setSpacing(8);
        auto* p1 = row->createChild<ProgressBar>();
        p1->setStretch(1); p1->setValue(0.6f); p1->setText("Fill 60%");
        auto* p2 = row->createChild<ProgressBar>();
        p2->setStretch(1); p2->setValue(0.3f); p2->setText("Line 30%");
        auto* p3 = row->createChild<ProgressBar>();
        p3->setStretch(1); p3->setValue(0.1f); p3->setText("Text 10%");
    }

    // Page 3: Credits
    {
        auto* p = carousel->addPage<Panel>();
        p->setBgColor(Color(55, 45, 40, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(20);
        box->setSpacing(6);
        box->createChild<Label>("Credits")->setColor(Color(255, 180, 100, 255));
        box->createChild<Label>("C++17 • SDL2 • OpenGL 3.3")->setColor(Color(200, 200, 205, 255));
        box->createChild<Label>("stb_truetype • stb_image • poly2tri")->setColor(Clr::dim);
        box->createChild<Label>("nlohmann/json (serialization)")->setColor(Clr::dim);
    }

    // Page label
    auto* pageLbl = topBox->createChild<Label>("Page 1 / 4");
    pageLbl->setColor(Clr::dim);
    carousel->pageChanged.connect([pageLbl, carousel](int idx) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Page %d / %d", idx + 1, carousel->pageCount());
        pageLbl->setText(buf);
    });

    // ══════════════════════════════════════════════════════════════════════
    //  BOTTOM: AnchorLayout
    // ══════════════════════════════════════════════════════════════════════
    auto* botPanel = split->createChild<Panel>();
    botPanel->setBgColor(Color(28, 30, 36, 255));
    auto* botBox = botPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    botBox->setPadding(8);
    botBox->setSpacing(4);

    sectionLabel(botBox, "AnchorLayout (relative positioning)");

    auto* anchor = botBox->createChild<AnchorLayout>();
    anchor->setStretch(1);

    // Background
    auto* bg = anchor->createChild<Panel>();
    bg->setBgColor(Color(35, 38, 44, 255));

    // Top-left
    auto* tl = anchor->createChild<Label>("Top-Left (0,0)");
    tl->setColor(Color(255, 180, 80, 255));
    AnchorLayout::setAnchors(tl, {0, 0, 0, 0, 10, 90, 6, 24});

    // Top-right
    auto* tr = anchor->createChild<Label>("Top-Right (1,0)");
    tr->setColor(Color(80, 200, 255, 255));
    AnchorLayout::setAnchors(tr, {1, 1, 0, 0, -100, -10, 6, 24});

    // Bottom-left
    auto* bl = anchor->createChild<Button>("BL");
    bl->setBgColor(Color(180, 80, 80, 255));
    AnchorLayout::setAnchors(bl, {0, 0, 1, 1, 10, 60, -34, -6});

    // Bottom-right
    auto* br = anchor->createChild<Button>("BR");
    br->setBgColor(Color(80, 180, 80, 255));
    AnchorLayout::setAnchors(br, {1, 1, 1, 1, -60, -10, -34, -6});

    // Center button
    auto* center = anchor->createChild<Button>("Centered (50%, 50%)");
    center->setBgColor(Color(80, 120, 200, 255));
    AnchorLayout::setAnchors(center, {0.5f, 0.5f, 0.5f, 0.5f, -70, 70, -14, 14});

    // Horizontal stretch bar at 30% height
    auto* hbar = anchor->createChild<ProgressBar>();
    hbar->setValue(0.65f);
    AnchorLayout::setAnchors(hbar, {0.1f, 0.9f, 0.35f, 0.35f, 0, 0, 0, 20});

    // Vertical progress on the right
    auto* vbar = anchor->createChild<ProgressBar>();
    vbar->setOrientation(LayoutDir::Vertical);
    vbar->setValue(0.4f);
    AnchorLayout::setAnchors(vbar, {0.92f, 0.92f, 0.1f, 0.85f, 0, 16, 0, 0});

    // ── StatusBar ────────────────────────────────────────────────────────
    auto* status = outer->createChild<StatusBar>();
    status->setText("AnchorLayout + Carousel | auto-play 3s | loop | dots + arrows");
}
