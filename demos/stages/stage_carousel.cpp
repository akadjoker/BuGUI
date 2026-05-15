#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "InputWidgets.hpp"
#include <cstdio>
using namespace BuGUI;

// Stage 14 - Carousel (matches tmp/app/StageAnchor.hpp carousel)

static StatusBar* s_carStatus = nullptr;
static Carousel*  s_carousel  = nullptr;
static Label*     s_pageLabel = nullptr;

static void updatePageLabel(int idx)
{
    if (!s_pageLabel || !s_carousel) return;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Page %d / %d", idx + 1, s_carousel->pageCount());
    s_pageLabel->setText(buf);
}

void registerCarouselStage(WidgetApp& app)
{
    s_carStatus = nullptr;
    s_carousel  = nullptr;
    s_pageLabel = nullptr;

    auto* root = app.addStage("carousel");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0.0f);
    outer->setPadding(0.0f);

    // ── Navigation bar ───────────────────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8.0f);
    nav->setPadding(12.0f, 12.0f, 0.0f, 12.0f);

    auto* back = nav->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]()
    {
        app.setStage("menu", TransitionType::CoverRight);
    });

    nav->createChild<Spacer>(0.0f)->setStretch(1.0f);
    nav->createChild<Label>("14 \u2014 Carousel + AnchorLayout");
    nav->createChild<Spacer>(0.0f)->setStretch(1.0f);
    s_pageLabel = nav->createChild<Label>("Page 1 / 4");

    // ── Body: vertical Splitter ──────────────────────────────────────────
    auto* split = outer->createChild<Splitter>(LayoutDir::Vertical);
    split->setStretch(1.0f);
    split->setRatio(0.55f);
    split->setMinRatio(0.3f);
    split->setMaxRatio(0.75f);
    split->setHandleSize(5.0f);

    // ══════════════════════════════════════════════════════════════════════
    //  TOP: Carousel
    // ══════════════════════════════════════════════════════════════════════
    auto* topPanel = split->createChild<Panel>();
    topPanel->setBgColor(Color(30, 32, 38, 255));
    auto* topBox = topPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    topBox->setPadding(8.0f);
    topBox->setSpacing(4.0f);

    auto* secLbl = topBox->createChild<Label>("Carousel (auto-play 3s, loop, arrows + dots)");
    secLbl->setColor(Color(170, 170, 175, 255));

    auto* carousel = topBox->createChild<Carousel>();
    carousel->setStretch(1.0f);
    carousel->setAutoPlay(true);
    carousel->setAutoPlayInterval(3.0f);
    carousel->setLoop(true);
    carousel->setAnimDuration(0.4f);
    s_carousel = carousel;

    // Page 0: Welcome
    {
        auto* p = carousel->addPage<Panel>();
        p->setBgColor(Color(45, 50, 70, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(20.0f);
        box->setSpacing(8.0f);
        box->createChild<Label>("Welcome to BuGUI")->setColor(Color(100, 180, 255, 255));
        box->createChild<Label>("A Qt-like widget toolkit built with SDL2 + OpenGL")->setColor(Color(200, 200, 205, 255));
        box->createChild<Label>("11 layouts \u2022 signals \u2022 transitions \u2022 themes")->setColor(Color(120, 120, 125, 255));
        auto* bar = box->createChild<ProgressBar>();
        bar->setValue(0.85f);
        bar->setText("85% complete");
    }

    // Page 1: Features (FlowLayout)
    {
        auto* p = carousel->addPage<Panel>();
        p->setBgColor(Color(50, 40, 55, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(20.0f);
        box->setSpacing(4.0f);
        box->createChild<Label>("Features")->setColor(Color(200, 140, 255, 255));

        auto* flow = box->createChild<FlowLayout>();
        flow->setSpacing(4.0f, 4.0f);
        flow->setStretch(1.0f);
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

    // Page 2: Performance stats
    {
        auto* p = carousel->addPage<Panel>();
        p->setBgColor(Color(40, 55, 45, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(20.0f);
        box->setSpacing(6.0f);
        box->createChild<Label>("Performance")->setColor(Color(100, 220, 140, 255));
        box->createChild<Label>("3-batch rendering: Fill + Line + Text")->setColor(Color(200, 200, 205, 255));
        box->createChild<Label>("CPU clip stack (no GL scissor)")->setColor(Color(120, 120, 125, 255));
        box->createChild<Label>("Sutherland-Hodgman polygon clipping")->setColor(Color(120, 120, 125, 255));

        auto* row = box->createChild<BoxLayout>(LayoutDir::Horizontal);
        row->setSpacing(8.0f);
        auto* p1 = row->createChild<ProgressBar>();
        p1->setStretch(1.0f); p1->setValue(0.6f); p1->setText("Fill 60%");
        auto* p2 = row->createChild<ProgressBar>();
        p2->setStretch(1.0f); p2->setValue(0.3f); p2->setText("Line 30%");
        auto* p3 = row->createChild<ProgressBar>();
        p3->setStretch(1.0f); p3->setValue(0.1f); p3->setText("Text 10%");
    }

    // Page 3: Credits
    {
        auto* p = carousel->addPage<Panel>();
        p->setBgColor(Color(55, 45, 40, 255));
        auto* box = p->createChild<BoxLayout>(LayoutDir::Vertical);
        box->setPadding(20.0f);
        box->setSpacing(6.0f);
        box->createChild<Label>("Credits")->setColor(Color(255, 180, 100, 255));
        box->createChild<Label>("C++17 \u2022 SDL2 \u2022 OpenGL 3.3")->setColor(Color(200, 200, 205, 255));
        box->createChild<Label>("stb_truetype \u2022 stb_image \u2022 poly2tri")->setColor(Color(120, 120, 125, 255));
        box->createChild<Label>("nlohmann/json (serialization)")->setColor(Color(120, 120, 125, 255));
    }

    // Page label below carousel
    auto* pageLbl = topBox->createChild<Label>("Page 1 / 4");
    pageLbl->setColor(Color(120, 120, 125, 255));

    // ══════════════════════════════════════════════════════════════════════
    //  BOTTOM: AnchorLayout
    // ══════════════════════════════════════════════════════════════════════
    auto* botPanel = split->createChild<Panel>();
    botPanel->setBgColor(Color(28, 30, 36, 255));
    auto* botBox = botPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    botBox->setPadding(8.0f);
    botBox->setSpacing(4.0f);

    auto* ancLbl = botBox->createChild<Label>("AnchorLayout (relative positioning)");
    ancLbl->setColor(Color(170, 170, 175, 255));

    auto* anchor = botBox->createChild<AnchorLayout>();
    anchor->setStretch(1.0f);

    // Background fill
    auto* bg = anchor->createChild<Panel>();
    bg->setBgColor(Color(35, 38, 44, 255));

    // Top-left label
    auto* tl = anchor->createChild<Label>("Top-Left (0,0)");
    tl->setColor(Color(255, 180, 80, 255));
    AnchorLayout::setAnchors(tl, {0, 0, 0, 0, 10, 90, 6, 24});

    // Top-right label
    auto* tr2 = anchor->createChild<Label>("Top-Right (1,0)");
    tr2->setColor(Color(80, 200, 255, 255));
    AnchorLayout::setAnchors(tr2, {1, 1, 0, 0, -100, -10, 6, 24});

    // Bottom-left button
    auto* bl = anchor->createChild<Button>("BL");
    bl->setBgColor(Color(180, 80, 80, 255));
    AnchorLayout::setAnchors(bl, {0, 0, 1, 1, 10, 60, -34, -6});

    // Bottom-right button
    auto* br = anchor->createChild<Button>("BR");
    br->setBgColor(Color(80, 180, 80, 255));
    AnchorLayout::setAnchors(br, {1, 1, 1, 1, -60, -10, -34, -6});

    // Center button
    auto* center = anchor->createChild<Button>("Centered (50%, 50%)");
    center->setBgColor(Color(80, 120, 200, 255));
    AnchorLayout::setAnchors(center, {0.5f, 0.5f, 0.5f, 0.5f, -70, 70, -14, 14});

    // Horizontal stretch bar at 35% height
    auto* hbar = anchor->createChild<ProgressBar>();
    hbar->setValue(0.65f);
    AnchorLayout::setAnchors(hbar, {0.1f, 0.9f, 0.35f, 0.35f, 0, 0, 0, 20});

    // Vertical progress on the right
    auto* vbar = anchor->createChild<ProgressBar>();
    vbar->setOrientation(LayoutDir::Vertical);
    vbar->setValue(0.4f);
    AnchorLayout::setAnchors(vbar, {0.92f, 0.92f, 0.1f, 0.85f, 0, 16, 0, 0});

    // ── Status bar ───────────────────────────────────────────────────────
    s_carStatus = outer->createChild<StatusBar>();
    s_carStatus->setText("AnchorLayout + Carousel | auto-play 3s | loop | dots + arrows");

    // ── Signals ──────────────────────────────────────────────────────────
    carousel->pageChanged.connect([pageLbl, carousel](int idx) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Page %d / %d", idx + 1, carousel->pageCount());
        pageLbl->setText(buf);
        updatePageLabel(idx);
        if (s_carStatus)
        {
            const char* names[] = {"Welcome", "Features", "Performance", "Credits"};
            char buf2[80];
            std::snprintf(buf2, sizeof(buf2), "Navigated to %s", names[idx]);
            s_carStatus->setText(buf2);
        }
    });

    updatePageLabel(0);
}
