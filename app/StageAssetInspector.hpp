#pragma once

#include "StageCommon.hpp"
#include "AssetBrowser.hpp"
#include "TreeWidgets.hpp"
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
//  Asset Browser + PropertyGrid inspector demo stage
// ═════════════════════════════════════════════════════════════════════════════

inline void buildAssetInspectorStage(WidgetApp& app)
{
    Widget* root = app.addStage("assetinspector");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Header ───────────────────────────────────────────────────────────
    {
        auto* hdr = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
        hdr->setSize(0, 36);
        hdr->setPadding(8, 8, 4, 4);
        hdr->setSpacing(8);

        auto* back = hdr->createChild<Button>("<- Home");
        back->setSize(80, 26);
        back->clicked.connect([] {
            WidgetApp::instance().setStage("home", TransitionType::SlideRight, EaseType::OutCubic);
        });
        hdr->createChild<Label>("Asset Browser  ·  Inspector Panel")->setColor(Clr::accent);
    }

    // ── Main row ─────────────────────────────────────────────────────────
    auto* row = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    row->setStretch(1);
    row->setSpacing(2);
    row->setPadding(4);

    // ── Asset Browser ─────────────────────────────────────────────────────
    auto* ab = row->createChild<AssetBrowser>();
    ab->setStretch(3);
    ab->setPath("Assets/Textures");
    ab->setThumbSize(64.f);

    struct FakeAsset { const char* name; AssetType type; Color col; };
    static const FakeAsset assets[] = {
        {"Models",        AssetType::Folder,   Color(200, 160,  60, 255)},
        {"Textures",      AssetType::Folder,   Color(200, 160,  60, 255)},
        {"Scripts",       AssetType::Folder,   Color(200, 160,  60, 255)},
        {"sky.png",       AssetType::Image,    Color( 60, 140, 210, 255)},
        {"grass.png",     AssetType::Image,    Color( 80, 180,  80, 255)},
        {"rock.png",      AssetType::Image,    Color(140, 120, 100, 255)},
        {"normal_map.png",AssetType::Image,    Color(120, 120, 200, 255)},
        {"player.mesh",   AssetType::Mesh,     Color(180, 120, 220, 255)},
        {"tree.mesh",     AssetType::Mesh,     Color(140, 180, 100, 255)},
        {"ambient.ogg",   AssetType::Audio,    Color(100, 200, 140, 255)},
        {"hit.wav",       AssetType::Audio,    Color( 80, 200, 180, 255)},
        {"Player.lua",    AssetType::Script,   Color(220, 180,  60, 255)},
        {"Enemy.lua",     AssetType::Script,   Color(220, 140,  60, 255)},
        {"pbr.mat",       AssetType::Material, Color( 60, 200, 200, 255)},
        {"glass.mat",     AssetType::Material, Color(180, 220, 240, 255)},
        {"main.scene",    AssetType::Scene,    Color(240, 100,  80, 255)},
        {"README.txt",    AssetType::Generic,  Color( 90,  95, 105, 255)},
    };
    for (auto& a : assets)
        ab->addItem({a.name, a.type, a.col});

    // ── PropertyGrid inspector ────────────────────────────────────────────
    auto* ins = row->createChild<PropertyGrid>();
    ins->setStretch(1);
    ins->setDescHeight(0.f);

    ins->addSection("Transform");
    ins->addVec3("Position",  1.5f,  0.0f, -3.2f, -999.f, 999.f);
    ins->addVec3("Rotation",  0.0f, 45.0f,  0.0f, -360.f, 360.f);
    ins->addVec3("Scale",     1.0f,  1.0f,  1.0f,   0.001f, 100.f);

    ins->addSection("Physics");
    ins->addFloat("Mass",      1.0f,  0.f, 100.f);
    ins->addFloat("Friction",  0.4f,  0.f,   1.f);
    ins->addBool ("Is Static", false);
    ins->addCombo("Layer", {"Default","UI","Ignore","Water","PostFX"}, 0);

    ins->addSection("Rendering");
    ins->addBool ("Cast Shadow", true);
    ins->addColor("Albedo",    Color(200, 120, 60, 255));
    ins->addColor("Emission",  Color(0,   0,   0, 255));
    ins->addFloat("Roughness", 0.6f, 0.f, 1.f);
    ins->addFloat("Metallic",  0.1f, 0.f, 1.f);
    ins->addInt  ("Render Order", 0, -100, 100);

    ins->addSection("Meta");
    ins->addString("Tag", "Player");
    ins->addInt   ("Instance ID", 42, 0, 9999);
}
