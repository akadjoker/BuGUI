#pragma once

#include "StageCommon.hpp"
#include "CurveEditor.hpp"
#include "NodeEditor.hpp"
#include "Timeline.hpp"
#include "ConsoleWidget.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Editor Widgets stage — CurveEditor, NodeEditor, Timeline, Console
//  Layout: 2×2 grid using BorderLayout and BoxLayout
//
//  ┌─────────────┬───────────────┐
//  │ CurveEditor  │  NodeEditor   │
//  ├─────────────┴───────────────┤
//  │   Timeline                  │
//  ├─────────────────────────────┤
//  │   ConsoleWidget             │
//  └─────────────────────────────┘
// ═════════════════════════════════════════════════════════════════════════════

inline void buildEditorWidgetsStage(WidgetApp& app)
{
    Widget* root = app.addStage("editorwidgets");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Header bar ───────────────────────────────────────────────────────
    auto* header = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSize(0, 36);
    header->setPadding(8, 8, 4, 4);
    header->setSpacing(8);

    auto* back = header->createChild<Button>("<- Home");
    back->setSize(80, 26);
    back->clicked.connect([] {
        WidgetApp::instance().setStage("home", TransitionType::SlideRight, EaseType::OutCubic);
    });

    header->createChild<Label>("Editor Widgets")->setColor(Clr::accent);

    // ── Top row: CurveEditor + NodeEditor side by side ───────────────────
    auto* topRow = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    topRow->setStretch(3);
    topRow->setSpacing(1);
    topRow->setPadding(0);

    auto* curveEd = topRow->createChild<CurveEditor>();
    curveEd->setStretch(1);

    // Populate some demo curves
    {
        int c0 = curveEd->addCurve("Position X", Color(220, 80, 80, 255));
        curveEd->addKey(c0, 0.0f, 0.0f);
        curveEd->addKey(c0, 1.0f, 1.0f);
        curveEd->addKey(c0, 2.0f, 0.5f);
        curveEd->addKey(c0, 3.5f, 1.5f);

        int c1 = curveEd->addCurve("Position Y", Color(80, 200, 80, 255));
        curveEd->addKey(c1, 0.0f, 0.5f);
        curveEd->addKey(c1, 1.5f, 2.0f);
        curveEd->addKey(c1, 3.0f, -0.5f);

        int c2 = curveEd->addCurve("Rotation Z", Color(80, 120, 220, 255));
        curveEd->addKey(c2, 0.0f, 0.0f);
        curveEd->addKey(c2, 2.0f, 180.0f);
        curveEd->addKey(c2, 4.0f, 360.0f);
    }

    auto* nodeEd = topRow->createChild<NodeEditor>();
    nodeEd->setStretch(1);

    // Populate demo node graph
    {
        int nTex = nodeEd->addNode("Texture", 50, 50);
        nodeEd->addPin(nTex, "UV",    PinDir::Input,  PinType::Vec2);
        nodeEd->addPin(nTex, "Color", PinDir::Output, PinType::Color);
        nodeEd->addPin(nTex, "Alpha", PinDir::Output, PinType::Float);

        int nMix = nodeEd->addNode("Mix", 280, 80);
        nodeEd->addPin(nMix, "A",      PinDir::Input,  PinType::Color);
        nodeEd->addPin(nMix, "B",      PinDir::Input,  PinType::Color);
        nodeEd->addPin(nMix, "Factor", PinDir::Input,  PinType::Float);
        nodeEd->addPin(nMix, "Result", PinDir::Output, PinType::Color);

        int nOut = nodeEd->addNode("Output", 500, 100);
        nodeEd->addPin(nOut, "Surface", PinDir::Input,  PinType::Color);

        int nVal = nodeEd->addNode("Value", 50, 200);
        nodeEd->addPin(nVal, "Value",  PinDir::Output, PinType::Float);

        nodeEd->addLink(nTex, 1, nMix, 0);  // Texture.Color -> Mix.A
        nodeEd->addLink(nVal, 0, nMix, 2);  // Value.Value   -> Mix.Factor
        nodeEd->addLink(nMix, 3, nOut, 0);  // Mix.Result    -> Output.Surface
    }

    // ── Timeline ─────────────────────────────────────────────────────────
    auto* timeline = outer->createChild<Timeline>();
    timeline->setStretch(1);

    {
        int t0 = timeline->addTrack("Position", Color(220, 100, 100, 255));
        timeline->addKeyframe(t0, 0.0f);
        timeline->addKeyframe(t0, 1.0f);
        timeline->addKeyframe(t0, 2.5f);
        timeline->addKeyframe(t0, 4.0f);
        timeline->addClip(t0, 0.0f, 2.0f, "Move", Color(100, 60, 60, 180));

        int t1 = timeline->addTrack("Rotation", Color(100, 200, 100, 255));
        timeline->addKeyframe(t1, 0.5f);
        timeline->addKeyframe(t1, 2.0f);
        timeline->addKeyframe(t1, 3.5f);

        int t2 = timeline->addTrack("Scale", Color(100, 140, 220, 255));
        timeline->addClip(t2, 1.0f, 3.0f, "Grow", Color(60, 80, 120, 180));
        timeline->addKeyframe(t2, 1.0f);
        timeline->addKeyframe(t2, 3.0f);

        int t3 = timeline->addTrack("Visibility", Color(200, 180, 80, 255));
        timeline->addClip(t3, 0.0f, 4.5f, "Visible", Color(80, 70, 40, 160));

        timeline->setPlayhead(0.0f);
        timeline->setTimeRange(0, 5);
    }

    // ── Console ──────────────────────────────────────────────────────────
    auto* console = outer->createChild<ConsoleWidget>();
    console->setStretch(1);

    console->log(LogLevel::Info,  "Editor Widgets stage loaded.");
    console->log(LogLevel::Info,  "CurveEditor: 3 curves with demo keyframes.");
    console->log(LogLevel::Info,  "NodeEditor: 4 nodes (Texture, Mix, Value, Output).");
    console->log(LogLevel::Warn,  "Timeline: 4 tracks — try scrubbing the ruler.");
    console->log(LogLevel::Error, "This is a sample error message.");
    console->log(LogLevel::Info,  "Scroll with mouse-wheel. Click the filter buttons (INF/WRN/ERR).");
    console->log(LogLevel::Info,  "Type in the input bar below and press Enter.");
}
