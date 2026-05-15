#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "InputWidgets.hpp"
#include "LayoutWidgets.hpp"
using namespace BuGUI;

void registerSlidePanelStage(WidgetApp& app)
{
    auto* root = app.addStage("slidepanel");

    // ── Full-size overlay container ──────────────────────────────────────
    auto* overlay = root->createChild<Overlay>();

    // ── Layer 0: Main content ────────────────────────────────────────────
    auto* main = overlay->createChild<BoxLayout>(LayoutDir::Vertical);
    main->setSpacing(0);
    main->setPadding(0);

    // Top bar with hamburger
    auto* topBar = main->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSize(0, 44);
    topBar->setPadding(6, 8, 6, 8);
    topBar->setSpacing(10);

    auto* burger = topBar->createChild<IconButton>(IconButton::Hamburger);
    burger->setAnimated(true);

    auto* title = topBar->createChild<Label>("SlidePanel Demo");
    title->setStretch(1);

    auto* backBtn = topBar->createChild<Button>("\u2190 Menu");
    backBtn->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });

    // Separator
    main->createChild<Line>();

    // Content area — switches when user picks from drawer
    auto* stack = main->createChild<StackLayout>();
    stack->setStretch(1);

    // Page 0: Welcome
    {
        auto* page = stack->createChild<BoxLayout>(LayoutDir::Vertical);
        page->setPadding(24);
        page->setSpacing(12);
        page->createChild<Label>("Welcome")->setColor(Color(120, 180, 255, 255));
        page->createChild<Label>("Open the hamburger menu to navigate between pages.");
        page->createChild<Label>("The drawer slides in with easing and has a scrim overlay.");
        page->createChild<Spacer>();
    }

    // Page 1: Settings-like
    {
        auto* page = stack->createChild<BoxLayout>(LayoutDir::Vertical);
        page->setPadding(24);
        page->setSpacing(8);
        page->createChild<Label>("Settings")->setColor(Color(120, 180, 255, 255));
        page->createChild<Line>();

        auto* row1 = page->createChild<BoxLayout>(LayoutDir::Horizontal);
        row1->setSpacing(8);
        row1->createChild<Label>("Dark Mode")->setStretch(1);
        row1->createChild<CheckBox>();

        auto* row2 = page->createChild<BoxLayout>(LayoutDir::Horizontal);
        row2->setSpacing(8);
        row2->createChild<Label>("Notifications")->setStretch(1);
        row2->createChild<CheckBox>();

        auto* row3 = page->createChild<BoxLayout>(LayoutDir::Horizontal);
        row3->setSpacing(8);
        row3->createChild<Label>("Auto-save")->setStretch(1);
        row3->createChild<CheckBox>();

        page->createChild<Spacer>();
    }

    // Page 2: About
    {
        auto* page = stack->createChild<BoxLayout>(LayoutDir::Vertical);
        page->setPadding(24);
        page->setSpacing(8);
        page->createChild<Label>("About")->setColor(Color(120, 180, 255, 255));
        page->createChild<Line>();
        page->createChild<Label>("BuGUI — Lightweight C++17 Widget Library");
        page->createChild<Label>("SDL2 + OpenGL 3.3 backend");
        page->createChild<Label>("SlidePanel widget with animated drawer.");
        page->createChild<Spacer>();
    }

    // Page 3: Progress
    {
        auto* page = stack->createChild<BoxLayout>(LayoutDir::Vertical);
        page->setPadding(24);
        page->setSpacing(12);
        page->createChild<Label>("Progress")->setColor(Color(120, 180, 255, 255));
        page->createChild<Line>();

        page->createChild<Label>("Build progress:");
        auto* pb1 = page->createChild<ProgressBar>();
        pb1->setValue(0.72f);
        pb1->setSize(0, 20);

        page->createChild<Label>("Test coverage:");
        auto* pb2 = page->createChild<ProgressBar>();
        pb2->setValue(0.45f);
        pb2->setSize(0, 20);

        page->createChild<Spacer>();
    }

    stack->setCurrentIndex(0);

    // ── Layer 1: SlidePanel (drawer) ─────────────────────────────────────
    auto* drawer = overlay->createChild<SlidePanel>(SlidePanel::Left, 240.0f);
    drawer->setAnimDuration(0.3f);
    drawer->setEaseType(EaseType::OutCubic);

    // Drawer content
    auto* drawerContent = drawer->createChild<BoxLayout>(LayoutDir::Vertical);
    drawerContent->setSpacing(2);
    drawerContent->setPadding(0, 12, 0, 8);

    // Drawer header — row with title + close button
    auto* drawerHeader = drawerContent->createChild<BoxLayout>(LayoutDir::Horizontal);
    drawerHeader->setPadding(12, 12, 8, 12);
    drawerHeader->setSpacing(8);

    auto* drawerTitle = drawerHeader->createChild<BoxLayout>(LayoutDir::Vertical);
    drawerTitle->setStretch(1);
    drawerTitle->setSpacing(2);
    drawerTitle->createChild<Label>("BuGUI")->setColor(Color(100, 180, 255, 255));
    drawerTitle->createChild<Label>("Navigation Drawer")->setColor(Color(140, 140, 150, 255));

    auto* closeBtn = drawerHeader->createChild<IconButton>(IconButton::Close);
    closeBtn->clicked.connect([drawer]() {
        drawer->close();
    });

    drawerContent->createChild<Line>();

    // Navigation items
    struct NavItem { const char* label; int pageIndex; };
    NavItem items[] = {
        {"Home",     0},
        {"Settings", 1},
        {"About",    2},
        {"Progress", 3},
    };

    for (auto& item : items)
    {
        auto* btn = drawerContent->createChild<Button>(item.label);
        int idx = item.pageIndex;
        btn->clicked.connect([stack, drawer, idx]() {
            stack->setCurrentIndex(idx);
            drawer->close();
        });
    }

    drawerContent->createChild<Spacer>();

    // Hamburger toggles drawer
    burger->clicked.connect([drawer, burger]() {
        drawer->toggle();
        // Animate hamburger ↔ close icon
        burger->setIcon(drawer->isOpen() ? IconButton::Close : IconButton::Hamburger);
    });

    // Sync icon when drawer closes (e.g. scrim click)
    drawer->openChanged.connect([burger](bool open) {
        burger->setIcon(open ? IconButton::Close : IconButton::Hamburger);
    });
}
