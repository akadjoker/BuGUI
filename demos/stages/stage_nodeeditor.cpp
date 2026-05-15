#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "NodeEditor.hpp"
using namespace BuGUI;

void registerNodeEditorStage(WidgetApp& app)
{
    auto* root = app.addStage("nodeeditor");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(4);
    outer->setPadding(8);

    // Header
    auto* header = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSize(0, 32);
    auto* back = header->createChild<Button>("< Back");
    back->setSize(70, 26);
    back->clicked.connect([&app]() { app.setStage("menu", TransitionType::CoverRight); });
    header->createChild<Label>("Node Editor — Drag pins to connect · Middle-drag / left-drag bg to pan · Scroll to zoom · Right-click link to delete");

    // ── Node Editor ──────────────────────────────────────────────────────
    auto* ed = outer->createChild<NodeEditor>();
    ed->setStretch(1);

    // ── Build a sample shader graph ──────────────────────────────────────
    //
    //  Texture  →  Mix  →  Output
    //  Color    ↗
    //  Noise    →  Math →  ↗
    //

    int nTex = ed->addNode("Texture Sample", 40, 60);
    ed->setNodeHeader(nTex, Color(120, 70,  40, 255));
    ed->addPin(nTex, "UV",      PinDir::Input,  PinType::Vec2);
    ed->addPin(nTex, "Color",   PinDir::Output, PinType::Vec4);
    ed->addPin(nTex, "Alpha",   PinDir::Output, PinType::Float);

    int nColor = ed->addNode("Constant Color", 40, 200);
    ed->setNodeHeader(nColor, Color(160, 100, 40, 255));
    ed->addPin(nColor, "R",     PinDir::Input,  PinType::Float);
    ed->addPin(nColor, "G",     PinDir::Input,  PinType::Float);
    ed->addPin(nColor, "B",     PinDir::Input,  PinType::Float);
    ed->addPin(nColor, "Color", PinDir::Output, PinType::Color);

    int nNoise = ed->addNode("Perlin Noise", 40, 380);
    ed->setNodeHeader(nNoise, Color(60, 120, 90, 255));
    ed->addPin(nNoise, "Scale",  PinDir::Input,  PinType::Float);
    ed->addPin(nNoise, "Octaves",PinDir::Input,  PinType::Int);
    ed->addPin(nNoise, "Value",  PinDir::Output, PinType::Float);

    int nMath = ed->addNode("Math (Multiply)", 280, 360);
    ed->setNodeHeader(nMath, Color(80, 80, 140, 255));
    ed->addPin(nMath, "A",      PinDir::Input,  PinType::Float);
    ed->addPin(nMath, "B",      PinDir::Input,  PinType::Float);
    ed->addPin(nMath, "Result", PinDir::Output, PinType::Float);

    int nMix = ed->addNode("Mix / Lerp", 280, 100);
    ed->setNodeHeader(nMix, Color(100, 60, 140, 255));
    ed->addPin(nMix, "A",     PinDir::Input,  PinType::Vec4);
    ed->addPin(nMix, "B",     PinDir::Input,  PinType::Color);
    ed->addPin(nMix, "Factor",PinDir::Input,  PinType::Float);
    ed->addPin(nMix, "Result",PinDir::Output, PinType::Vec4);

    int nOut = ed->addNode("Material Output", 520, 160);
    ed->setNodeHeader(nOut, Color(140, 50, 50, 255));
    ed->addPin(nOut, "Surface",  PinDir::Input, PinType::Vec4);
    ed->addPin(nOut, "Emission", PinDir::Input, PinType::Vec4);
    ed->addPin(nOut, "Alpha",    PinDir::Input, PinType::Float);

    // Links
    ed->addLink(nTex,   1, nMix,  0);   // Texture.Color  → Mix.A
    ed->addLink(nColor, 3, nMix,  1);   // Color.Color    → Mix.B
    ed->addLink(nNoise, 2, nMath, 0);   // Noise.Value    → Math.A
    ed->addLink(nMath,  2, nMix,  2);   // Math.Result    → Mix.Factor
    ed->addLink(nMix,   3, nOut,  0);   // Mix.Result     → Output.Surface
    ed->addLink(nTex,   2, nOut,  2);   // Texture.Alpha  → Output.Alpha
}
