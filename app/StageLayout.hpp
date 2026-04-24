#pragma once

#include "StageCommon.hpp"

inline void buildLayoutStage(WidgetApp& app)
{
    Widget* root = app.addStage("layout");
    auto* page = root->createChild<BoxLayout>(LayoutDir::Vertical);
    page->setSpacing(6);
    page->setPadding(16, 16, 16, 16);

    // ── Header ───────────────────────────────────────────────────────────
    auto* header = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSpacing(8);
    header->setSize(0, 32);
    header->createChild<Label>("Layout")->setColor(Clr::accent);
    auto* toCanvas = header->createChild<Button>("Canvas  >");
    toCanvas->clicked.connect([]{ WidgetApp::instance().setStage("canvas", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── SpaceBetween ─────────────────────────────────────────────────────
    sectionLabel(page, "MainAlign::SpaceBetween");

    auto* r1 = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    r1->setPadding(6);
    r1->setMainAlign(MainAlign::SpaceBetween);
    r1->setSize(0, 34);
    r1->createChild<Button>("First");
    r1->createChild<Button>("Second");
    r1->createChild<Button>("Third");
    r1->createChild<Button>("Last");

    // ── SpaceEvenly ──────────────────────────────────────────────────────
    sectionLabel(page, "MainAlign::SpaceEvenly");

    auto* r2 = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    r2->setPadding(6);
    r2->setMainAlign(MainAlign::SpaceEvenly);
    r2->setSize(0, 34);
    r2->createChild<Button>("A");
    r2->createChild<Button>("B");
    r2->createChild<Button>("C");

    // ── Stretch ──────────────────────────────────────────────────────────
    sectionLabel(page, "Stretch  (slider fills remaining space)");

    auto* r3 = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    r3->setSpacing(8);
    r3->setPadding(6);
    r3->setSize(0, 34);
    r3->createChild<Label>("Volume:");
    auto* slider = r3->createChild<Slider>(0.0f, 100.0f, 65.0f);
    slider->setStretch(1.0f);
    auto* valLabel = r3->createChild<Label>("65");
    valLabel->setSize(36, 0);
    slider->onValueChanged.connect([valLabel](float v) {
        char b[8]; snprintf(b, sizeof(b), "%.0f", v);
        valLabel->setText(b);
    });

    // ── Cross-axis alignment ─────────────────────────────────────────────
    sectionLabel(page, "Cross-axis Align  (container h=72)");

    auto* r4 = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    r4->setSpacing(6);
    r4->setPadding(6);
    r4->setSize(0, 72);
    r4->createChild<Button>("Start") ->setLayoutAlign(Align::Start);
    r4->createChild<Button>("Center")->setLayoutAlign(Align::Center);
    r4->createChild<Button>("End")   ->setLayoutAlign(Align::End);
    r4->createChild<Button>("Fill")  ->setLayoutAlign(Align::Fill);

    // ── Margins ──────────────────────────────────────────────────────────
    sectionLabel(page, "Margins  (middle button: left 24 px)");

    auto* r5 = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    r5->setSpacing(4);
    r5->setPadding(6);
    r5->setSize(0, 34);
    r5->createChild<Button>("Normal");
    r5->createChild<Button>("Indented")->setMargins(0, 0, 0, 24);
    r5->createChild<Button>("Normal");

    // ── Serialize ────────────────────────────────────────────────────────
    sectionLabel(page, "Serialization");

    auto* ioRow = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    ioRow->setSpacing(8);
    ioRow->setPadding(6);
    ioRow->setSize(0, 34);
    auto* saveBtn = ioRow->createChild<Button>("Save JSON");
    auto* ioMsg   = ioRow->createChild<Label>("");
    ioMsg->setColor(Clr::dim);
    ioMsg->setId("ioMsg");

    saveBtn->clicked.connect([page, ioMsg]() {
        bool ok = WidgetSerializer::saveToFile(page, "layout.json");
        ioMsg->setText(ok ? "layout.json saved" : "save failed!");
    });

    // ── Footer ───────────────────────────────────────────────────────────
    dimLabel(page, "F1 = debug overlay  |  Toggle stages with the header buttons");
}
