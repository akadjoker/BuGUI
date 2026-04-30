#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "InputWidgets.hpp"
#include "AutomotiveWidgets.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  BMW-style Instrument Cluster Demo
// ═════════════════════════════════════════════════════════════════════════════

void registerClusterStage(WidgetApp& app)
{
    auto* root = app.addStage("cluster");

    // Full-screen dark panel
    auto* bg = root->createChild<Panel>();
    bg->setBgColor(Color(10, 10, 12, 255));
    bg->setStretch(1.0f);

    auto* main = bg->createChild<BoxLayout>(LayoutDir::Vertical);
    main->setSpacing(0.0f);
    main->setPadding(0.0f);
    main->setStretch(1.0f);

    // ── Top Status Bar ───────────────────────────────────────────────────
    auto* topBar = main->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSpacing(0.0f);
    topBar->setPadding({6, 12, 6, 12});

    // Left: temperature + back button
    auto* topLeft = topBar->createChild<BoxLayout>(LayoutDir::Horizontal);
    topLeft->setSpacing(8.0f);
    topLeft->setStretch(1.0f);

    auto* back = topLeft->createChild<Button>("\u2190");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });

    auto* tempTile = topLeft->createChild<InfoTile>("\u2600", "24", "\u00b0");
    tempTile->setIconColor(Color(200, 200, 200, 200));
    tempTile->setSize(60, 24);

    // Center: status icons
    auto* topCenter = topBar->createChild<BoxLayout>(LayoutDir::Horizontal);
    topCenter->setSpacing(12.0f);

    auto* iconN  = topCenter->createChild<Label>("N");
    iconN->setColor(Color(220, 50, 50, 255));

    auto* iconBT = topCenter->createChild<Label>("\u00b7BT");
    iconBT->setColor(Color(100, 200, 100, 255));

    auto* icon5G = topCenter->createChild<Label>("5G");
    icon5G->setColor(Color(200, 200, 200, 255));

    auto* iconSig = topCenter->createChild<Label>("\u2581\u2582\u2583");
    iconSig->setColor(Color(200, 200, 200, 255));

    auto* iconABS = topCenter->createChild<Label>("ABS");
    iconABS->setColor(Color(220, 160, 40, 255));

    // Right: clock
    auto* topRight = topBar->createChild<BoxLayout>(LayoutDir::Horizontal);
    topRight->setSpacing(0.0f);
    topRight->setStretch(1.0f);
    topRight->setMainAlign(MainAlign::End);

    auto* clockLabel = topRight->createChild<Label>("06:56");
    clockLabel->setColor(Color(200, 200, 205, 255));

    // ── Separator ────────────────────────────────────────────────────────
    main->createChild<Line>();

    // ── Center area (gauge + speed + power bar) ──────────────────────────
    auto* centerArea = main->createChild<BoxLayout>(LayoutDir::Vertical);
    centerArea->setStretch(1.0f);
    centerArea->setSpacing(0.0f);
    centerArea->setPadding({8, 0, 0, 0});

    // RadialGauge (the arc behind the speed)
    auto* gauge = centerArea->createChild<RadialGauge>(0.0f, 260.0f, 56.0f);
    gauge->setSize(0, 0);
    gauge->setStretch(1.0f);
    gauge->setUnit("km/h");
    gauge->setArcColor(Color(200, 160, 40, 255));     // amber
    gauge->setGlowColor(Color(200, 160, 40, 60));
    gauge->setArcBgColor(Color(45, 45, 50, 255));
    gauge->setValueColor(Color(240, 240, 240, 255));
    gauge->setArcWidth(5.0f);
    gauge->setTickInterval(20.0f);
    gauge->setFilledArc(true);
    gauge->setShowNeedle(true);
    gauge->setNeedleColor(Color(220, 40, 40, 255));
    gauge->setNeedleWidth(3.0f);
    gauge->setNeedleLength(0.82f);

    // Power bar
    auto* powerBar = centerArea->createChild<PowerBar>(-100.0f, 200.0f, 0.0f);
    powerBar->setSize(0, 36);
    powerBar->setUnit("kW");
    powerBar->setPositiveColor(Color(200, 160, 40, 255));
    powerBar->setNegativeColor(Color(60, 180, 80, 255));

    // ── Bottom Status Bar ────────────────────────────────────────────────
    main->createChild<Line>();

    auto* bottomBar = main->createChild<BoxLayout>(LayoutDir::Horizontal);
    bottomBar->setSpacing(12.0f);
    bottomBar->setPadding({8, 16, 8, 16});

    // Range
    auto* rangeTile = bottomBar->createChild<InfoTile>("\u21bb", "2000", " km");
    rangeTile->setIconColor(Color(140, 140, 140, 255));
    rangeTile->setSize(90, 24);

    // Distance to destination
    auto* destTile = bottomBar->createChild<InfoTile>("\u25bc", "35", " km");
    destTile->setIconColor(Color(140, 140, 140, 255));
    destTile->setSize(70, 24);

    // Spacer to push center
    bottomBar->createChild<Spacer>()->setStretch(1.0f);

    // Drive mode
    auto* driveMode = bottomBar->createChild<DriveMode>();
    driveMode->addMode("NORMAL",  Color(200, 200, 205, 255));
    driveMode->addMode("SPORT",   Color(220, 60, 60, 255));
    driveMode->addMode("ECO PRO", Color(60, 180, 240, 255));
    driveMode->addMode("COMFORT", Color(180, 140, 220, 255));
    driveMode->setSize(130, 24);

    // Spacer
    bottomBar->createChild<Spacer>()->setStretch(1.0f);

    // Battery
    auto* battery = bottomBar->createChild<BatteryGauge>(100.0f);
    battery->setChargingState(true);
    battery->setSize(100, 24);

    // EV range
    auto* evRange = bottomBar->createChild<InfoTile>("\u26a1", "200", " km");
    evRange->setIconColor(Color(60, 180, 240, 255));
    evRange->setSize(80, 24);

    // Hamburger
    auto* menuBtn = bottomBar->createChild<IconButton>(IconButton::Hamburger);
    menuBtn->setSize(24, 24);

    // ══════════════════════════════════════════════════════════════════════
    //  Controls panel (hidden behind hamburger — shows sliders to adjust)
    // ══════════════════════════════════════════════════════════════════════
    // For demo, add sliders at the very bottom to drive the values
    main->createChild<Line>();

    auto* ctrlBar = main->createChild<BoxLayout>(LayoutDir::Horizontal);
    ctrlBar->setSpacing(12.0f);
    ctrlBar->setPadding({4, 12, 4, 12});

    ctrlBar->createChild<Label>("Speed:");
    auto* speedSlider = ctrlBar->createChild<Slider>(0.0f, 260.0f, 56.0f);
    speedSlider->setSize(200, 20);
    speedSlider->setStretch(1.0f);

    ctrlBar->createChild<Label>("Power:");
    auto* powerSlider = ctrlBar->createChild<Slider>(-100.0f, 200.0f, 0.0f);
    powerSlider->setSize(200, 20);
    powerSlider->setStretch(1.0f);

    // Wire sliders to gauges
    speedSlider->onValueChanged.connect([gauge](float v) {
        gauge->setValue(v);
    });

    powerSlider->onValueChanged.connect([powerBar](float v) {
        powerBar->setValue(v);
    });
}
