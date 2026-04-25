#include "WidgetApp.hpp"
#include "Widgets.hpp"

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 08 - Node/Curve/Timeline", 1360, 860))
        return 1;

    Widget* root = app.addStage("main");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(12, 12, 12, 12);
    outer->setSpacing(10);

    auto* title = outer->createChild<Label>("Tutorial 08: NodeEditor + CurveEditor + Timeline");
    title->setColor(Color(120, 190, 255, 255));
    auto* status = outer->createChild<Label>("Edit graph/curves and scrub timeline.");

    auto* top = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    top->setSpacing(10);
    top->setStretch(1);

    auto* nodePanel = top->createChild<Panel>();
    nodePanel->setStretch(1);
    auto* nodeCol = nodePanel->createChild<BoxLayout>(LayoutDir::Vertical);
    nodeCol->setPadding(8, 8, 8, 8);
    nodeCol->createChild<Label>("NodeEditor");
    auto* nodeEd = nodeCol->createChild<NodeEditor>();
    nodeEd->setStretch(1);

    int nA = nodeEd->addNode("Texture", 80, 100);
    nodeEd->addPin(nA, "UV", PinDir::Input, PinType::Vec2);
    nodeEd->addPin(nA, "Color", PinDir::Output, PinType::Color);

    int nB = nodeEd->addNode("Multiply", 380, 140);
    nodeEd->addPin(nB, "A", PinDir::Input, PinType::Color);
    nodeEd->addPin(nB, "B", PinDir::Input, PinType::Color);
    nodeEd->addPin(nB, "Out", PinDir::Output, PinType::Color);
    nodeEd->addLink(nA, 1, nB, 0);

    auto* curvePanel = top->createChild<Panel>();
    curvePanel->setStretch(1);
    auto* curveCol = curvePanel->createChild<BoxLayout>(LayoutDir::Vertical);
    curveCol->setPadding(8, 8, 8, 8);
    curveCol->createChild<Label>("CurveEditor");
    auto* curve = curveCol->createChild<CurveEditor>();
    curve->setStretch(1);
    int curveId = curve->addCurve("Opacity", Color(100, 210, 255, 255));
    curve->addKey(curveId, 0.0f, 0.0f);
    curve->addKey(curveId, 1.0f, 1.0f);
    curve->addKey(curveId, 2.0f, 0.5f);
    curve->fitView();

    auto* timelinePanel = outer->createChild<Panel>();
    timelinePanel->setStretch(1);
    auto* tlCol = timelinePanel->createChild<BoxLayout>(LayoutDir::Vertical);
    tlCol->setPadding(8, 8, 8, 8);
    tlCol->createChild<Label>("Timeline");
    auto* timeline = tlCol->createChild<Timeline>();
    timeline->setStretch(1);

    int tTransform = timeline->addTrack("Transform");
    timeline->addKeyframe(tTransform, 0.0f);
    timeline->addKeyframe(tTransform, 0.8f);
    timeline->addKeyframe(tTransform, 1.6f);
    timeline->addClip(tTransform, 0.2f, 1.4f, "Move");

    int tColor = timeline->addTrack("Color");
    timeline->addKeyframe(tColor, 0.0f);
    timeline->addKeyframe(tColor, 1.0f);
    timeline->addKeyframe(tColor, 2.0f);
    timeline->addClip(tColor, 0.5f, 1.8f, "Fade");

    timeline->onPlayheadChanged.connect([status, curve](float t) {
        curve->setPlayhead(t);
        status->setText("Playhead: " + std::to_string(t));
    });

    nodeEd->onNodeSelected.connect([status](int id) {
        status->setText("Node selected: " + std::to_string(id));
    });

    app.setStage("main");
    return app.run();
}
