#pragma once

#include "StageCommon.hpp"
#include "DockPanel.hpp"
#include "TreeWidgets.hpp"
#include "BasicWidgets.hpp"
#include "TextWidgets.hpp"
#include "DataWidgets.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  DockPanel demo stage
//  Shows a 4-panel Blender-style layout:
//    Scene (left) | Viewport (center) | Properties (right)
//                    Console (bottom)
// ═════════════════════════════════════════════════════════════════════════════

inline void buildDockStage(WidgetApp& app)
{
    Widget* root = app.addStage("dock");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Header bar ────────────────────────────────────────────────────────
    {
        auto* hdr = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
        hdr->setSize(0, 36);
        hdr->setPadding(8, 8, 4, 4);
        hdr->setSpacing(8);

        auto* back = hdr->createChild<Button>("<- Home");
        back->setSize(80, 26);
        back->clicked.connect([] {
            WidgetApp::instance().setStage("home",
                TransitionType::SlideRight, EaseType::OutCubic);
        });
        auto* lbl = hdr->createChild<Label>("Dock Panel  -  drag tabs to rearrange");
        lbl->setColor(Clr::accent);
    }

    // ── DockPanel ─────────────────────────────────────────────────────────
    auto* dock = outer->createChild<DockPanel>();
    dock->setStretch(1);

    // ── Panel contents ────────────────────────────────────────────────────

    // Scene Outliner
    {
        auto* tree = new TreeView();
        auto* root_node = tree->addRoot("Scene");
        auto* ch1 = root_node->addChild("Camera");
        auto* ch2 = root_node->addChild("Lighting");
        ch2->addChild("Sun");
        ch2->addChild("Ambient");
        auto* ch3 = root_node->addChild("Objects");
        ch3->addChild("Player");
        ch3->addChild("Ground");
        ch3->addChild("Trees");
        root_node->setExpanded(true);
        ch2->setExpanded(true);
        ch3->setExpanded(true);
        dock->addPanel("Scene", tree);
    }

    // Viewport (fake canvas)
    {
        auto* cv = new Canvas();
        cv->setOnPaint([](PaintContext& ctx, const Rect& b) {
            // Dark viewport background
            ctx.fill.SetColor(28, 28, 32, 255);
            ctx.fillRect(b.x, b.y, b.w, b.h);

            // Grid lines
            ctx.line.SetColor(45, 45, 50, 255);
            float step = 40.f;
            for (float x = b.x; x < b.x + b.w; x += step)
                ctx.drawLine(x, b.y, x, b.y + b.h);
            for (float y = b.y; y < b.y + b.h; y += step)
                ctx.drawLine(b.x, y, b.x + b.w, y);

            // Fake 3-D cube wireframe (centred)
            float cx = b.x + b.w * 0.5f, cy = b.y + b.h * 0.5f;
            float s  = std::min(b.w, b.h) * 0.18f;
            float ox = s * 0.5f, oy = s * 0.35f;

            // Front face
            ctx.line.SetColor(100, 180, 255, 220);
            ctx.drawLine(cx-s, cy-s, cx+s, cy-s);
            ctx.drawLine(cx+s, cy-s, cx+s, cy+s);
            ctx.drawLine(cx+s, cy+s, cx-s, cy+s);
            ctx.drawLine(cx-s, cy+s, cx-s, cy-s);
            // Back face
            ctx.line.SetColor(60, 120, 200, 140);
            ctx.drawLine(cx-s+ox, cy-s-oy, cx+s+ox, cy-s-oy);
            ctx.drawLine(cx+s+ox, cy-s-oy, cx+s+ox, cy+s-oy);
            ctx.drawLine(cx+s+ox, cy+s-oy, cx-s+ox, cy+s-oy);
            ctx.drawLine(cx-s+ox, cy+s-oy, cx-s+ox, cy-s-oy);
            // Connecting edges
            ctx.line.SetColor(80, 150, 220, 160);
            ctx.drawLine(cx-s, cy-s, cx-s+ox, cy-s-oy);
            ctx.drawLine(cx+s, cy-s, cx+s+ox, cy-s-oy);
            ctx.drawLine(cx+s, cy+s, cx+s+ox, cy+s-oy);
            ctx.drawLine(cx-s, cy+s, cx-s+ox, cy+s-oy);

            Font::ClipRect fc = {b.x, b.y, b.w, b.h};
            ctx.font.SetColor(100, 100, 110, 200);
            ctx.font.Print("Viewport  (drag tabs to rearrange)", b.x + 8, b.y + 8, &fc);
        });
        dock->addPanel("Viewport", cv);
    }

    // Shader editor (second tab in viewport leaf for demo)
    {
        auto* ed = new TextEdit();
        ed->setText(
            "// Simple fragment shader\n"
            "uniform sampler2D uTexture;\n"
            "varying vec2 vUV;\n\n"
            "void main() {\n"
            "    vec4 col = texture2D(uTexture, vUV);\n"
            "    gl_FragColor = col;\n"
            "}\n"
        );
        dock->addPanel("Shader", ed);
    }

    // Properties panel
    {
        auto* pg = new PropertyGrid();
        pg->addSection("Transform");
        pg->addVec3("Position",  0.f, 0.f, 0.f, -1000.f, 1000.f);
        pg->addVec3("Rotation",  0.f, 0.f, 0.f, -360.f, 360.f);
        pg->addVec3("Scale",     1.f, 1.f, 1.f, 0.f, 100.f);
        pg->addSection("Material");
        pg->addColor("Albedo",   Color(200, 200, 200, 255));
        pg->addFloat("Roughness", 0.5f, 0.f, 1.f);
        pg->addFloat("Metallic",  0.0f, 0.f, 1.f);
        pg->addBool("Cast Shadow", true);
        pg->addSection("Physics");
        pg->addFloat("Mass",     1.f, 0.f, 1000.f);
        pg->addBool("Static",    false);
        dock->addPanel("Properties", pg);
    }

    // Console
    {
        auto* box = new BoxLayout(LayoutDir::Vertical);
        box->setSpacing(4);
        box->setPadding(6);

        auto* list = box->createChild<ListWidget>();
        list->setStretch(1);
        list->addRow<Label>("[Info]  Scene loaded in 0.03s");
        list->addRow<Label>("[Info]  Shader compiled OK");
        list->addRow<Label>("[Warn]  Mesh 'tree' has no UV map");
        list->addRow<Label>("[Info]  Physics world initialised");
        list->addRow<Label>("[Error] Script 'Player.lua' line 42: nil value");
        list->addRow<Label>("[Info]  Ready.");

        dock->addPanel("Console", box);
    }

    // ── Initial layout: Scene left 20%, Console bottom 25%,
    //    Viewport + Shader center, Properties right 22%
    dock->splitOff("Scene",      DockSide::Left,   0.20f);
    dock->splitOff("Console",    DockSide::Bottom,  0.25f);
    dock->splitOff("Properties", DockSide::Right,   0.22f);
}
