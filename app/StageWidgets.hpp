#pragma once

#include "StageCommon.hpp"

inline void buildWidgetsStage(WidgetApp& app)
{
    Widget* root = app.addStage("widgets");
    auto* page = root->createChild<BoxLayout>(LayoutDir::Vertical);
    page->setSpacing(6);
    page->setPadding(16, 16, 16, 16);

    // ── Header ───────────────────────────────────────────────────────────
    auto* header = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSpacing(8);
    header->setSize(0, 32);
    auto* back = header->createChild<Button>("<  Canvas");
    back->clicked.connect([]{ WidgetApp::instance().setStage("canvas", TransitionType::SlideRight, EaseType::OutBack); });
    header->createChild<Label>("Widgets")->setColor(Clr::accent);
    auto* toScroll = header->createChild<Button>("Scroll  >");
    toScroll->clicked.connect([]{ WidgetApp::instance().setStage("scroll", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── ProgressBar (horizontal) ─────────────────────────────────────────
    sectionLabel(page, "ProgressBar  (horizontal)");

    auto* pbRow = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    pbRow->setSpacing(12);
    pbRow->setPadding(6);
    pbRow->setSize(0, 28);

    auto* pb1 = pbRow->createChild<ProgressBar>(0.0f, 100.0f, 35.0f);
    pb1->setStretch(1.0f);
    pb1->setText("35%");
    pb1->setTextAlign(TextAlign::CENTER);
    pb1->setId("pb1");

    auto* pb2 = pbRow->createChild<ProgressBar>(0.0f, 100.0f, 72.0f);
    pb2->setStretch(1.0f);
    pb2->setText("Loading...");
    pb2->setTextAlign(TextAlign::LEFT);
    pb2->setBarColor(Color(80, 170, 90, 255));

    // Slider to control pb1
    auto* pbCtrl = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    pbCtrl->setSpacing(8);
    pbCtrl->setPadding(6);
    pbCtrl->setSize(0, 24);
    pbCtrl->createChild<Label>("Value:");
    auto* pbSlider = pbCtrl->createChild<Slider>(0.0f, 100.0f, 35.0f);
    pbSlider->setStretch(1.0f);
    auto* pbVal = pbCtrl->createChild<Label>("35");
    pbVal->setSize(32, 0);
    pbSlider->onValueChanged.connect([pb1, pbVal](float v) {
        pb1->setValue(v);
        char b[8]; snprintf(b, sizeof(b), "%.0f%%", v);
        pb1->setText(b);
        char b2[8]; snprintf(b2, sizeof(b2), "%.0f", v);
        pbVal->setText(b2);
    });

    // ── ProgressBar (vertical) + Slider (vertical) ───────────────────────
    sectionLabel(page, "Vertical  (ProgressBar + Slider)");

    auto* vertRow = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    vertRow->setSpacing(16);
    vertRow->setPadding(6);
    vertRow->setSize(0, 160);

    auto* vpb = vertRow->createChild<ProgressBar>(0.0f, 100.0f, 60.0f);
    vpb->setOrientation(LayoutDir::Vertical);
    vpb->setSize(24, 150);
    vpb->setBarColor(Color(180, 100, 60, 255));

    auto* vslider = vertRow->createChild<Slider>(0.0f, 100.0f, 60.0f);
    vslider->setOrientation(LayoutDir::Vertical);
    vslider->setSize(20, 150);
    vslider->onValueChanged.connect([vpb](float v) { vpb->setValue(v); });

    dimLabel(vertRow, "Drag the vertical slider\nto control the bar");

    // ── Spacer demo ──────────────────────────────────────────────────────
    sectionLabel(page, "Spacer  (push buttons apart)");

    auto* spRow = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    spRow->setSpacing(4);
    spRow->setPadding(6);
    spRow->setSize(0, 34);

    spRow->createChild<Button>("Left");
    auto* sp = spRow->createChild<Spacer>();
    sp->setStretch(1.0f);
    spRow->createChild<Button>("Right");

    // ── Line demo ────────────────────────────────────────────────────────
    sectionLabel(page, "Line  (horizontal separator)");

    page->createChild<Line>();

    auto* lineRow = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    lineRow->setSpacing(8);
    lineRow->setPadding(6);
    lineRow->setSize(0, 34);
    lineRow->createChild<Button>("A");
    lineRow->createChild<Line>();    // vertical (inside HBox)
    lineRow->createChild<Button>("B");
    lineRow->createChild<Line>(2.0f);
    lineRow->createChild<Button>("C");

    page->createChild<Line>();

    // ── Footer ───────────────────────────────────────────────────────────
    dimLabel(page, "New widgets: ProgressBar, Spacer, Line, vertical Slider");
}
