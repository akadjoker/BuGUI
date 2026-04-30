#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "DockPanel.hpp"
#include "TreePropertyColorWidgets.hpp"
#include "TextInputWidgets.hpp"
#include "ConsoleWidget.hpp"
#include "NodeEditor.hpp"
#include "Timeline.hpp"
#include "MenuWidgets.hpp"
#include "DataWidgets.hpp"
#include "ChartWidgets.hpp"
#include "InputWidgets.hpp"
#include "ScrollWidgets.hpp"
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  Stage — Fake Blender
//
//  A realistic 3D-editor-style layout with:
//  - MenuBar with deep submenus
//  - DockPanel with Scene Outliner, Properties, Shader Editor,
//    Node Graph, Timeline, Console, Viewport placeholder
//  - Heavy PropertyGrid (many sections, all property types)
//  - Deep TreeView (~100+ nodes)
//  - NodeEditor with interconnected shader nodes
//  - Timeline with multiple tracks, keyframes, clips
// ─────────────────────────────────────────────────────────────────────────────

// Generate a deep scene tree (~120 nodes)
static void buildSceneTree(TreeView* tree)
{
    auto* root = tree->addRoot("Scene Collection");
    root->setExpanded(true);

    // Camera rig
    auto* cameras = root->addChild("Cameras");
    cameras->addChild("Camera.Main");
    cameras->addChild("Camera.Cinematic");
    cameras->addChild("Camera.Debug");
    cameras->setExpanded(true);

    // Lights
    auto* lights = root->addChild("Lighting");
    lights->addChild("Sun");
    lights->addChild("HDRI Environment");
    auto* area = lights->addChild("Area Lights");
    for (int i = 0; i < 6; ++i) {
        char buf[32]; snprintf(buf, sizeof(buf), "AreaLight.%03d", i);
        area->addChild(buf);
    }
    auto* point = lights->addChild("Point Lights");
    for (int i = 0; i < 8; ++i) {
        char buf[32]; snprintf(buf, sizeof(buf), "PointLight.%03d", i);
        point->addChild(buf);
    }
    lights->setExpanded(true);
    area->setExpanded(true);

    // Characters
    auto* chars = root->addChild("Characters");
    const char* charNames[] = {"Player", "NPC_Guard", "NPC_Merchant", "NPC_Villager", "Boss"};
    for (auto& name : charNames) {
        auto* ch = chars->addChild(name);
        ch->addChild("Armature");
        ch->addChild("Mesh");
        ch->addChild("Hair");
        auto* mats = ch->addChild("Materials");
        mats->addChild("Skin");
        mats->addChild("Cloth");
        mats->addChild("Eyes");
        mats->addChild("Metal");
    }
    chars->setExpanded(true);

    // Environment
    auto* env = root->addChild("Environment");
    auto* terrain = env->addChild("Terrain");
    terrain->addChild("HeightMap");
    terrain->addChild("SplatMap");
    terrain->addChild("GrassSystem");
    terrain->addChild("RockScatter");

    auto* buildings = env->addChild("Buildings");
    for (int i = 0; i < 12; ++i) {
        char buf[32]; snprintf(buf, sizeof(buf), "Building.%03d", i);
        auto* b = buildings->addChild(buf);
        b->addChild("LOD0");
        b->addChild("LOD1");
        b->addChild("Collider");
    }

    auto* vegetation = env->addChild("Vegetation");
    const char* plants[] = {"Oak", "Pine", "Birch", "Bush_A", "Bush_B", "Fern", "Grass_Clump"};
    for (auto& p : plants) vegetation->addChild(p);

    auto* props = env->addChild("Props");
    const char* propNames[] = {"Barrel", "Crate", "Lantern", "Bench", "Fountain",
                               "Cart", "Fence_Segment", "Sign_Post", "Well"};
    for (auto& p : propNames) props->addChild(p);
    env->setExpanded(true);
    buildings->setExpanded(true);

    // VFX
    auto* vfx = root->addChild("VFX");
    vfx->addChild("ParticleSystem_Fire");
    vfx->addChild("ParticleSystem_Smoke");
    vfx->addChild("ParticleSystem_Rain");
    vfx->addChild("ParticleSystem_Dust");
    vfx->addChild("VolumetricFog");
    vfx->addChild("GodRays");
}

// Build a heavy property grid (~60 properties)
static void buildProperties(PropertyGrid* pg)
{

    pg->addSection("Object");
    pg->addString("Name", "Player");
    pg->addBool("Visible", true);
    pg->addBool("Selectable", true);
    pg->addCombo("Display As", {"Solid", "Wire", "Bounds", "Textured"}, 0);

    pg->addSection("Transform");
    pg->addVec3("Location", 3.2f, 0.0f, -1.5f, -10000.f, 10000.f);
    pg->addVec3("Rotation", 0.f, 45.f, 0.f, -360.f, 360.f);
    pg->addVec3("Scale", 1.f, 1.f, 1.f, 0.001f, 1000.f);

    pg->addSection("Mesh");
    pg->addString("Vertices", "124,563");
    pg->addString("Faces", "62,281");
    pg->addString("Triangles", "124,562");
    pg->addBool("Smooth Shading", true);
    pg->addBool("Auto Smooth", true);
    pg->addFloat("Auto Smooth Angle", 30.f, 0.f, 180.f);

    pg->addSection("Physics");
    pg->addCombo("Type", {"None", "Rigid Body", "Soft Body", "Cloth", "Fluid"}, 1);
    pg->addFloat("Mass", 80.f, 0.01f, 10000.f);
    pg->addFloat("Friction", 0.5f, 0.f, 1.f);
    pg->addFloat("Bounciness", 0.3f, 0.f, 1.f);
    pg->addBool("Use Margin", true);
    pg->addFloat("Collision Margin", 0.04f, 0.f, 1.f);

    pg->addSection("Material — Skin");
    pg->addColor("Base Color", Color(220, 180, 160, 255));
    pg->addFloat("Metallic", 0.0f, 0.f, 1.f);
    pg->addFloat("Roughness", 0.65f, 0.f, 1.f);
    pg->addFloat("Specular", 0.5f, 0.f, 1.f);
    pg->addFloat("IOR", 1.45f, 1.f, 3.f);
    pg->addColor("Emission", Color(0, 0, 0, 255));
    pg->addFloat("Emission Strength", 0.f, 0.f, 100.f);
    pg->addFloat("Alpha", 1.f, 0.f, 1.f);
    pg->addBool("Backface Culling", false);
    pg->addCombo("Blend Mode", {"Opaque", "Alpha Clip", "Alpha Hashed", "Alpha Blend"}, 0);

    pg->addSection("Modifiers");
    pg->addString("1: Subdivision Surface", "Level 2");
    pg->addInt("Render Level", 3, 0, 6);
    pg->addInt("Viewport Level", 2, 0, 6);
    pg->addBool("Optimal Display", true);
    pg->addSeparator();
    pg->addString("2: Armature", "Armature");
    pg->addBool("Preserve Volume", true);
    pg->addSeparator();
    pg->addString("3: Mirror", "X Axis");
    pg->addBool("Clipping", true);
    pg->addFloat("Merge Distance", 0.001f, 0.f, 1.f);

    pg->addSection("Constraints");
    pg->addCombo("1: Copy Location", {"World", "Local", "Pose"}, 0);
    pg->addString("Target", "Armature.Root");
    pg->addFloat("Influence", 1.f, 0.f, 1.f);

    pg->addSection("Render");
    pg->addBool("Show in Renders", true);
    pg->addBool("Shadow Caster", true);
    pg->addBool("Shadow Receiver", true);
    pg->addFloat("Light Probe Volume", 1.f, 0.f, 10.f);

    pg->addSection("Custom Properties");
    pg->addString("team", "ally");
    pg->addInt("health", 100, 0, 999);
    pg->addFloat("speed", 5.5f, 0.f, 50.f);
    pg->addBool("is_player", true);
    pg->addColor("debug_color", Color(0, 255, 100, 255));
}

// Build a shader node graph (~15 nodes, connected)
static void buildNodeGraph(NodeEditor* ed)
{
    int texCoord  = ed->addNode("Texture Coord",   -500, -80);
    ed->addPin(texCoord, "Generated",  PinDir::Output, PinType::Vec3);
    ed->addPin(texCoord, "UV",         PinDir::Output, PinType::Vec2);
    ed->addPin(texCoord, "Object",     PinDir::Output, PinType::Vec3);
    ed->setNodeHeader(texCoord, Color(90, 60, 120, 255));

    int mapping   = ed->addNode("Mapping",          -300, -80);
    ed->addPin(mapping, "Vector",   PinDir::Input,  PinType::Vec3);
    ed->addPin(mapping, "Location", PinDir::Input,  PinType::Vec3);
    ed->addPin(mapping, "Rotation", PinDir::Input,  PinType::Vec3);
    ed->addPin(mapping, "Scale",    PinDir::Input,  PinType::Vec3);
    ed->addPin(mapping, "Vector",   PinDir::Output, PinType::Vec3);
    ed->setNodeHeader(mapping, Color(90, 60, 120, 255));

    int noiseTex  = ed->addNode("Noise Texture",    -80, -180);
    ed->addPin(noiseTex, "Vector",  PinDir::Input,  PinType::Vec3);
    ed->addPin(noiseTex, "Scale",   PinDir::Input,  PinType::Float);
    ed->addPin(noiseTex, "Detail",  PinDir::Input,  PinType::Float);
    ed->addPin(noiseTex, "Fac",     PinDir::Output, PinType::Float);
    ed->addPin(noiseTex, "Color",   PinDir::Output, PinType::Color);
    ed->setNodeHeader(noiseTex, Color(120, 80, 50, 255));

    int imgTex    = ed->addNode("Image Texture",    -80, 60);
    ed->addPin(imgTex, "Vector",    PinDir::Input,  PinType::Vec3);
    ed->addPin(imgTex, "Color",     PinDir::Output, PinType::Color);
    ed->addPin(imgTex, "Alpha",     PinDir::Output, PinType::Float);
    ed->setNodeHeader(imgTex, Color(120, 80, 50, 255));

    int normalMap = ed->addNode("Normal Map",        -80, 240);
    ed->addPin(normalMap, "Strength", PinDir::Input,  PinType::Float);
    ed->addPin(normalMap, "Color",    PinDir::Input,  PinType::Color);
    ed->addPin(normalMap, "Normal",   PinDir::Output, PinType::Vec3);
    ed->setNodeHeader(normalMap, Color(90, 60, 120, 255));

    int colorRamp = ed->addNode("ColorRamp",         150, -180);
    ed->addPin(colorRamp, "Fac",     PinDir::Input,  PinType::Float);
    ed->addPin(colorRamp, "Color",   PinDir::Output, PinType::Color);
    ed->addPin(colorRamp, "Alpha",   PinDir::Output, PinType::Float);
    ed->setNodeHeader(colorRamp, Color(100, 100, 100, 255));

    int mixRGB    = ed->addNode("Mix RGB",            150, 60);
    ed->addPin(mixRGB, "Fac",    PinDir::Input,  PinType::Float);
    ed->addPin(mixRGB, "Color1", PinDir::Input,  PinType::Color);
    ed->addPin(mixRGB, "Color2", PinDir::Input,  PinType::Color);
    ed->addPin(mixRGB, "Color",  PinDir::Output, PinType::Color);
    ed->setNodeHeader(mixRGB, Color(100, 100, 100, 255));

    int bump      = ed->addNode("Bump",              150, 240);
    ed->addPin(bump, "Strength",  PinDir::Input,  PinType::Float);
    ed->addPin(bump, "Distance",  PinDir::Input,  PinType::Float);
    ed->addPin(bump, "Height",    PinDir::Input,  PinType::Float);
    ed->addPin(bump, "Normal",    PinDir::Input,  PinType::Vec3);
    ed->addPin(bump, "Normal",    PinDir::Output, PinType::Vec3);
    ed->setNodeHeader(bump, Color(90, 60, 120, 255));

    int fresnel   = ed->addNode("Fresnel",           150, 400);
    ed->addPin(fresnel, "IOR",     PinDir::Input,  PinType::Float);
    ed->addPin(fresnel, "Normal",  PinDir::Input,  PinType::Vec3);
    ed->addPin(fresnel, "Fac",     PinDir::Output, PinType::Float);
    ed->setNodeHeader(fresnel, Color(80, 130, 80, 255));

    int bsdf      = ed->addNode("Principled BSDF",   400, 0);
    ed->addPin(bsdf, "Base Color",   PinDir::Input, PinType::Color);
    ed->addPin(bsdf, "Metallic",     PinDir::Input, PinType::Float);
    ed->addPin(bsdf, "Roughness",    PinDir::Input, PinType::Float);
    ed->addPin(bsdf, "IOR",          PinDir::Input, PinType::Float);
    ed->addPin(bsdf, "Normal",       PinDir::Input, PinType::Vec3);
    ed->addPin(bsdf, "Clearcoat",    PinDir::Input, PinType::Float);
    ed->addPin(bsdf, "Emission",     PinDir::Input, PinType::Color);
    ed->addPin(bsdf, "Alpha",        PinDir::Input, PinType::Float);
    ed->addPin(bsdf, "BSDF",         PinDir::Output, PinType::Any);
    ed->setNodeHeader(bsdf, Color(80, 130, 80, 255));

    int matOutput = ed->addNode("Material Output",   650, 50);
    ed->addPin(matOutput, "Surface",     PinDir::Input, PinType::Any);
    ed->addPin(matOutput, "Volume",      PinDir::Input, PinType::Any);
    ed->addPin(matOutput, "Displacement", PinDir::Input, PinType::Float);
    ed->setNodeHeader(matOutput, Color(140, 60, 60, 255));

    int mathAdd   = ed->addNode("Math (Add)",        400, 300);
    ed->addPin(mathAdd, "Value",  PinDir::Input,  PinType::Float);
    ed->addPin(mathAdd, "Value",  PinDir::Input,  PinType::Float);
    ed->addPin(mathAdd, "Value",  PinDir::Output, PinType::Float);
    ed->setNodeHeader(mathAdd, Color(100, 100, 100, 255));

    int separate  = ed->addNode("Separate XYZ",      -300, 200);
    ed->addPin(separate, "Vector", PinDir::Input,  PinType::Vec3);
    ed->addPin(separate, "X",      PinDir::Output, PinType::Float);
    ed->addPin(separate, "Y",      PinDir::Output, PinType::Float);
    ed->addPin(separate, "Z",      PinDir::Output, PinType::Float);
    ed->setNodeHeader(separate, Color(90, 60, 120, 255));

    // Connect the graph
    ed->addLink(texCoord, 1, mapping, 0);       // UV -> Mapping.Vector
    ed->addLink(mapping, 4, noiseTex, 0);       // Mapping.Vector -> Noise.Vector
    ed->addLink(mapping, 4, imgTex, 0);         // Mapping.Vector -> Image.Vector
    ed->addLink(noiseTex, 3, colorRamp, 0);     // Noise.Fac -> ColorRamp.Fac
    ed->addLink(colorRamp, 1, mixRGB, 0);       // ColorRamp.Color -> MixRGB.Fac ... close enough
    ed->addLink(imgTex, 1, mixRGB, 1);          // Image.Color -> MixRGB.Color1
    ed->addLink(normalMap, 2, bump, 3);         // NormalMap.Normal -> Bump.Normal
    ed->addLink(mixRGB, 3, bsdf, 0);            // MixRGB.Color -> BSDF.BaseColor
    ed->addLink(bump, 4, bsdf, 4);              // Bump.Normal -> BSDF.Normal
    ed->addLink(fresnel, 2, bsdf, 5);           // Fresnel.Fac -> BSDF.Clearcoat
    ed->addLink(bsdf, 8, matOutput, 0);          // BSDF.BSDF -> Output.Surface
}

// Build animation timeline (~8 tracks, keyframes + clips)
static void buildTimeline(Timeline* tl)
{
    tl->setTimeRange(0.f, 10.f);
    tl->setPlayhead(2.4f);

    int t0 = tl->addTrack("Camera", Color(120, 180, 120, 255));
    tl->addKeyframe(t0, 0.0f);
    tl->addKeyframe(t0, 2.5f);
    tl->addKeyframe(t0, 5.0f);
    tl->addKeyframe(t0, 8.0f);
    tl->addClip(t0, 0.0f, 3.0f, "Intro Pan", Color(80, 150, 80, 200));

    int t1 = tl->addTrack("Player.Location", Color(180, 120, 120, 255));
    tl->addKeyframe(t1, 0.0f);
    tl->addKeyframe(t1, 1.0f);
    tl->addKeyframe(t1, 2.0f);
    tl->addKeyframe(t1, 3.5f);
    tl->addKeyframe(t1, 5.0f);
    tl->addKeyframe(t1, 7.5f);
    tl->addKeyframe(t1, 10.0f);

    int t2 = tl->addTrack("Player.Rotation", Color(180, 160, 100, 255));
    tl->addKeyframe(t2, 0.0f);
    tl->addKeyframe(t2, 2.0f);
    tl->addKeyframe(t2, 4.0f);
    tl->addKeyframe(t2, 6.0f);
    tl->addKeyframe(t2, 8.0f);

    int t3 = tl->addTrack("Player.Scale", Color(100, 160, 180, 255));
    tl->addKeyframe(t3, 0.0f);
    tl->addKeyframe(t3, 10.0f);

    int t4 = tl->addTrack("Sun.Intensity", Color(220, 200, 100, 255));
    tl->addKeyframe(t4, 0.0f);
    tl->addKeyframe(t4, 5.0f);
    tl->addKeyframe(t4, 10.0f);
    tl->addClip(t4, 3.0f, 7.0f, "Sunset", Color(200, 150, 60, 200));

    int t5 = tl->addTrack("Fog.Density", Color(150, 150, 180, 255));
    tl->addKeyframe(t5, 0.0f);
    tl->addKeyframe(t5, 3.0f);
    tl->addKeyframe(t5, 6.0f);
    tl->addKeyframe(t5, 10.0f);

    int t6 = tl->addTrack("Audio.Master", Color(160, 120, 180, 255));
    tl->addClip(t6, 0.0f, 10.0f, "BGM_Forest", Color(120, 80, 160, 200));

    int t7 = tl->addTrack("Audio.SFX", Color(180, 100, 140, 255));
    tl->addClip(t7, 1.0f, 2.0f, "Footstep", Color(160, 80, 120, 200));
    tl->addClip(t7, 3.5f, 4.2f, "Sword", Color(160, 80, 120, 200));
    tl->addClip(t7, 5.0f, 5.5f, "Impact", Color(160, 80, 120, 200));
    tl->addClip(t7, 7.0f, 8.5f, "Ambient", Color(160, 80, 120, 200));
}

void registerFakeBlenderStage(WidgetApp& app)
{
    auto* root = app.addStage("fakeblender");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setStretch(1);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Menu Bar ─────────────────────────────────────────────────────────
    auto* menuBar = outer->createChild<MenuBar>();
    {
        // Submenus (static to survive frame-to-frame)
        static Menu* recentMenu = nullptr;
        static Menu* importMenu = nullptr;
        static Menu* exportMenu = nullptr;
        static Menu* meshMenu = nullptr;
        static Menu* curveMenu = nullptr;
        static Menu* lightMenu = nullptr;
        static Menu* emptyMenu = nullptr;
        static Menu* forceMenu = nullptr;
        static Menu* engineMenu = nullptr;

        if (!recentMenu) {
            recentMenu = new Menu();
            recentMenu->addAction("forest_scene.blend");
            recentMenu->addAction("character_v3.blend");
            recentMenu->addAction("lighting_test.blend");
            recentMenu->addAction("untitled.blend");
            recentMenu->addSeparator();
            recentMenu->addAction("Clear Recent Files");
        }
        if (!importMenu) {
            importMenu = new Menu();
            importMenu->addAction("FBX (.fbx)");
            importMenu->addAction("OBJ (.obj)");
            importMenu->addAction("glTF (.gltf/.glb)");
            importMenu->addAction("Alembic (.abc)");
            importMenu->addAction("USD (.usd/.usdc)");
            importMenu->addAction("Collada (.dae)");
        }
        if (!exportMenu) {
            exportMenu = new Menu();
            exportMenu->addAction("FBX (.fbx)");
            exportMenu->addAction("OBJ (.obj)");
            exportMenu->addAction("glTF (.gltf/.glb)");
            exportMenu->addAction("Alembic (.abc)");
            exportMenu->addAction("USD (.usd/.usdc)");
        }
        if (!meshMenu) {
            meshMenu = new Menu();
            meshMenu->addAction("Plane"); meshMenu->addAction("Cube");
            meshMenu->addAction("Circle"); meshMenu->addAction("UV Sphere");
            meshMenu->addAction("Ico Sphere"); meshMenu->addAction("Cylinder");
            meshMenu->addAction("Cone"); meshMenu->addAction("Torus");
            meshMenu->addAction("Monkey");
        }
        if (!curveMenu) {
            curveMenu = new Menu();
            curveMenu->addAction("Bezier"); curveMenu->addAction("Circle");
            curveMenu->addAction("Nurbs Path");
        }
        if (!lightMenu) {
            lightMenu = new Menu();
            lightMenu->addAction("Point"); lightMenu->addAction("Sun");
            lightMenu->addAction("Spot"); lightMenu->addAction("Area");
        }
        if (!emptyMenu) {
            emptyMenu = new Menu();
            emptyMenu->addAction("Plain Axes"); emptyMenu->addAction("Arrows");
            emptyMenu->addAction("Cube"); emptyMenu->addAction("Sphere");
        }
        if (!forceMenu) {
            forceMenu = new Menu();
            forceMenu->addAction("Wind"); forceMenu->addAction("Vortex");
            forceMenu->addAction("Turbulence");
        }
        if (!engineMenu) {
            engineMenu = new Menu();
            engineMenu->addCheckable("Eevee", true);
            engineMenu->addCheckable("Cycles");
            engineMenu->addCheckable("Workbench");
        }

        auto* fileMenu = menuBar->addMenu("File");
        fileMenu->addAction("New")->setShortcut("Ctrl+N");
        fileMenu->addAction("Open...")->setShortcut("Ctrl+O");
        fileMenu->addAction("Open Recent")->setSubmenu(recentMenu);
        fileMenu->addSeparator();
        fileMenu->addAction("Save")->setShortcut("Ctrl+S");
        fileMenu->addAction("Save As...")->setShortcut("Ctrl+Shift+S");
        fileMenu->addAction("Save Copy...");
        fileMenu->addSeparator();
        fileMenu->addAction("Import")->setSubmenu(importMenu);
        fileMenu->addAction("Export")->setSubmenu(exportMenu);
        fileMenu->addSeparator();
        fileMenu->addAction("Quit")->setShortcut("Ctrl+Q");

        auto* editMenu = menuBar->addMenu("Edit");
        editMenu->addAction("Undo")->setShortcut("Ctrl+Z");
        editMenu->addAction("Redo")->setShortcut("Ctrl+Shift+Z");
        editMenu->addSeparator();
        editMenu->addAction("Cut")->setShortcut("Ctrl+X");
        editMenu->addAction("Copy")->setShortcut("Ctrl+C");
        editMenu->addAction("Paste")->setShortcut("Ctrl+V");
        editMenu->addAction("Duplicate")->setShortcut("Shift+D");
        editMenu->addAction("Delete")->setShortcut("X");
        editMenu->addSeparator();
        editMenu->addAction("Preferences...");

        auto* viewMenu = menuBar->addMenu("View");
        viewMenu->addAction("Toggle Sidebar")->setShortcut("N");
        viewMenu->addAction("Toggle Header");
        viewMenu->addSeparator();
        viewMenu->addAction("Perspective/Ortho")->setShortcut("Numpad 5");
        viewMenu->addAction("Front")->setShortcut("Numpad 1");
        viewMenu->addAction("Right")->setShortcut("Numpad 3");
        viewMenu->addAction("Top")->setShortcut("Numpad 7");
        viewMenu->addSeparator();
        viewMenu->addAction("Frame All")->setShortcut("Home");
        viewMenu->addAction("Frame Selected")->setShortcut("Numpad .");
        viewMenu->addAction("Toggle Fullscreen")->setShortcut("F11");

        auto* addMenu = menuBar->addMenu("Add");
        addMenu->addAction("Mesh")->setSubmenu(meshMenu);
        addMenu->addAction("Curve")->setSubmenu(curveMenu);
        addMenu->addAction("Light")->setSubmenu(lightMenu);
        addMenu->addAction("Camera");
        addMenu->addAction("Empty")->setSubmenu(emptyMenu);
        addMenu->addAction("Force Field")->setSubmenu(forceMenu);
        addMenu->addAction("Text");
        addMenu->addAction("Armature");

        auto* renderMenu = menuBar->addMenu("Render");
        renderMenu->addAction("Render Image")->setShortcut("F12");
        renderMenu->addAction("Render Animation")->setShortcut("Ctrl+F12");
        renderMenu->addSeparator();
        renderMenu->addAction("Render Engine")->setSubmenu(engineMenu);
        renderMenu->addAction("View Render")->setShortcut("F11");

        auto* windowMenu = menuBar->addMenu("Window");
        windowMenu->addAction("New Main Window");
        windowMenu->addAction("Toggle System Console");
        windowMenu->addSeparator();
        windowMenu->addAction("Save Screenshot");

        auto* helpMenu = menuBar->addMenu("Help");
        helpMenu->addAction("Manual");
        helpMenu->addAction("Tutorials");
        helpMenu->addSeparator();
        helpMenu->addAction("Report a Bug");
        helpMenu->addAction("About Blender (fake)");
    }

    // ── Toolbar ──────────────────────────────────────────────────────────
    auto* toolRow = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    toolRow->setSize(0, 32);
    toolRow->setPadding(Edges(4, 4, 4, 4));
    toolRow->setSpacing(4);
    {
        auto* back = toolRow->createChild<Button>("\u2190 Menu");
        back->setSize(80, 24);
        back->clicked.connect([&app]() {
            app.setStage("menu", TransitionType::CoverRight);
        });
        toolRow->createChild<Line>();
        toolRow->createChild<Label>("Fake Blender — BuGUI Stress Test");
        toolRow->createChild<Spacer>(0.f)->setStretch(1);
        toolRow->createChild<Label>("124,563 verts | 62,281 faces | Eevee");
    }

    // ── DockPanel — the main event ───────────────────────────────────────
    auto* dock = outer->createChild<DockPanel>();
    dock->setStretch(1);

    // Scene Outliner (deep tree)
    {
        auto* tree = dock->addPanel<TreeView>("Outliner");
        buildSceneTree(tree);
    }

    // Viewport placeholder
    {
        auto* vp = dock->addPanel<BoxLayout>("Viewport", LayoutDir::Vertical);
        vp->setPadding(20);
        vp->setSpacing(10);
        auto* lbl = vp->createChild<Label>("3D Viewport");
        lbl->setColor(Color(100, 170, 240, 255));
        vp->createChild<Label>("(placeholder — imagine a 3D scene here)");
        vp->createChild<Label>("Pan: MMB  |  Orbit: MMB+Shift  |  Zoom: Scroll");

        // Fake stats
        auto* stats = vp->createChild<BoxLayout>(LayoutDir::Vertical);
        stats->setSpacing(2);
        stats->createChild<Label>("Objects: 156  |  Memory: 1.2 GB");
        stats->createChild<Label>("Render: 16.7ms (60 FPS)  |  Sync: 0.3ms");
    }

    // Properties (heavy)
    {
        auto* pg = dock->addPanel<PropertyGrid>("Properties");
        buildProperties(pg);
    }

    // Shader editor (TextEdit)
    {
        auto* ed = dock->addPanel<TextEdit>("Shader");
        ed->setText(
            "// ═══════════════════════════════════════════\n"
            "// PBR Fragment Shader (Principled BSDF)\n"
            "// ═══════════════════════════════════════════\n"
            "#version 330 core\n\n"
            "in vec3 FragPos;\n"
            "in vec3 Normal;\n"
            "in vec2 TexCoord;\n"
            "in mat3 TBN;\n\n"
            "out vec4 FragColor;\n\n"
            "uniform sampler2D albedoMap;\n"
            "uniform sampler2D normalMap;\n"
            "uniform sampler2D roughnessMap;\n"
            "uniform sampler2D metallicMap;\n"
            "uniform sampler2D aoMap;\n"
            "uniform samplerCube irradianceMap;\n"
            "uniform samplerCube prefilterMap;\n"
            "uniform sampler2D brdfLUT;\n\n"
            "uniform vec3  lightPositions[4];\n"
            "uniform vec3  lightColors[4];\n"
            "uniform vec3  camPos;\n"
            "uniform float exposure;\n\n"
            "const float PI = 3.14159265359;\n\n"
            "// Normal Distribution (GGX/Trowbridge-Reitz)\n"
            "float DistributionGGX(vec3 N, vec3 H, float r) {\n"
            "    float a  = r * r;\n"
            "    float a2 = a * a;\n"
            "    float NdH = max(dot(N, H), 0.0);\n"
            "    float d  = (NdH * NdH * (a2 - 1.0) + 1.0);\n"
            "    return a2 / (PI * d * d);\n"
            "}\n\n"
            "// Geometry (Smith GGX)\n"
            "float GeometrySmith(vec3 N, vec3 V, vec3 L, float r) {\n"
            "    float k  = (r + 1.0) * (r + 1.0) / 8.0;\n"
            "    float NdV = max(dot(N, V), 0.0);\n"
            "    float NdL = max(dot(N, L), 0.0);\n"
            "    float g1  = NdV / (NdV * (1.0 - k) + k);\n"
            "    float g2  = NdL / (NdL * (1.0 - k) + k);\n"
            "    return g1 * g2;\n"
            "}\n\n"
            "// Fresnel (Schlick)\n"
            "vec3 FresnelSchlick(float cosTheta, vec3 F0) {\n"
            "    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);\n"
            "}\n\n"
            "void main() {\n"
            "    vec3 albedo   = pow(texture(albedoMap, TexCoord).rgb, vec3(2.2));\n"
            "    float metal   = texture(metallicMap, TexCoord).r;\n"
            "    float rough   = texture(roughnessMap, TexCoord).r;\n"
            "    float ao      = texture(aoMap, TexCoord).r;\n"
            "    vec3  N       = normalize(TBN * (texture(normalMap, TexCoord).rgb * 2.0 - 1.0));\n"
            "    vec3  V       = normalize(camPos - FragPos);\n"
            "    vec3  F0      = mix(vec3(0.04), albedo, metal);\n\n"
            "    vec3 Lo = vec3(0.0);\n"
            "    for (int i = 0; i < 4; ++i) {\n"
            "        vec3 L = normalize(lightPositions[i] - FragPos);\n"
            "        vec3 H = normalize(V + L);\n"
            "        float dist = length(lightPositions[i] - FragPos);\n"
            "        float atten = 1.0 / (dist * dist);\n"
            "        vec3 radiance = lightColors[i] * atten;\n\n"
            "        float NDF = DistributionGGX(N, H, rough);\n"
            "        float G   = GeometrySmith(N, V, L, rough);\n"
            "        vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);\n\n"
            "        vec3 num  = NDF * G * F;\n"
            "        float den = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;\n"
            "        vec3 spec = num / den;\n"
            "        vec3 kD   = (vec3(1.0) - F) * (1.0 - metal);\n"
            "        Lo += (kD * albedo / PI + spec) * radiance * max(dot(N, L), 0.0);\n"
            "    }\n\n"
            "    // IBL ambient\n"
            "    vec3 kS = FresnelSchlick(max(dot(N, V), 0.0), F0);\n"
            "    vec3 kD = (1.0 - kS) * (1.0 - metal);\n"
            "    vec3 irradiance = texture(irradianceMap, N).rgb;\n"
            "    vec3 diffuse = irradiance * albedo;\n"
            "    const float MAX_LOD = 4.0;\n"
            "    vec3 R = reflect(-V, N);\n"
            "    vec3 prefiltered = textureLod(prefilterMap, R, rough * MAX_LOD).rgb;\n"
            "    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), rough)).rg;\n"
            "    vec3 specular = prefiltered * (kS * brdf.x + brdf.y);\n"
            "    vec3 ambient = (kD * diffuse + specular) * ao;\n\n"
            "    vec3 color = ambient + Lo;\n"
            "    color = vec3(1.0) - exp(-color * exposure);  // tone mapping\n"
            "    color = pow(color, vec3(1.0/2.2));           // gamma\n"
            "    FragColor = vec4(color, 1.0);\n"
            "}\n"
        );
    }

    // Node Editor (shader graph)
    {
        auto* ne = dock->addPanel<NodeEditor>("Shader Nodes");
        ne->setGridSnap(20.f);
        buildNodeGraph(ne);
    }

    // Timeline
    {
        auto* tl = dock->addPanel<Timeline>("Timeline");
        buildTimeline(tl);
    }

    // Console
    {
        auto* con = dock->addPanel<ConsoleWidget>("Console");
        con->log(LogLevel::Info,  "Blender (fake) 4.1.0");
        con->log(LogLevel::Info,  "Scene: forest_scene.blend loaded in 0.87s");
        con->log(LogLevel::Info,  "Objects: 156 | Meshes: 89 | Materials: 34");
        con->log(LogLevel::Info,  "Memory: 1.2 GB (peak 1.8 GB)");
        con->log(LogLevel::Warn,  "Mesh 'Tree_Pine.003' has degenerate faces (12)");
        con->log(LogLevel::Warn,  "Texture 'bark_normal.png' resolution 8192x8192 — consider downscaling");
        con->log(LogLevel::Info,  "Eevee: Compiled 34 shaders in 0.42s");
        con->log(LogLevel::Error, "Script 'player_controller.py' line 87: AttributeError 'NoneType'");
        con->log(LogLevel::Info,  "Physics world initialised (Bullet 2.89)");
        con->log(LogLevel::Info,  "Armature 'Player' has 142 bones, 23 constraints");
        con->log(LogLevel::Warn,  "Bone 'finger_index_L.003' has zero-length — auto-fix applied");
        con->log(LogLevel::Info,  "Animation cache built: 7 actions, 1,247 keyframes");
        con->log(LogLevel::Info,  "Ready.");
    }

    // ── Initial dock layout (Blender-like) ───────────────────────────────
    dock->splitOff("Outliner",     DockSide::Left,    0.18f);
    dock->splitOff("Properties",   DockSide::Right,   0.22f);
    dock->splitOff("Timeline",     DockSide::Bottom,  0.22f);
    dock->splitOff("Console",      DockSide::Bottom,  0.35f);  // below timeline
    dock->splitOff("Shader Nodes", DockSide::Bottom,  0.45f);  // tab with timeline
}
