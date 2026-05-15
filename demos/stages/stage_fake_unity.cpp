#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "DockPanel.hpp"
#include "TreePropertyColorWidgets.hpp"
#include "TextInputWidgets.hpp"
#include "ConsoleWidget.hpp"
#include "MenuWidgets.hpp"
#include "DataWidgets.hpp"
#include "ChartWidgets.hpp"
#include "InputWidgets.hpp"
#include "ScrollWidgets.hpp"
#include "AssetBrowser.hpp"
#include "Timeline.hpp"
#include "GizmoWidgets.hpp"
#include <cstdio>
using namespace BuGUI;

// ─────────────────────────────────────────────────────────────────────────────
//  Stage — Fake Unity 3D
//
//  A realistic Unity-style editor with:
//  - MenuBar (File, Edit, Assets, GameObject, Component, Window, Help)
//  - Toolbar (Play/Pause/Step, transform tool buttons, layer/layout dropdowns)
//  - DockPanel with:
//      Hierarchy     — deep GameObject tree (~130 nodes)
//      Scene View    — viewport placeholder with Gizmo2D
//      Game View     — preview placeholder
//      Inspector     — heavy PropertyGrid (~70 props: Transform, MeshRenderer,
//                      Rigidbody, Collider, Script, Animation, Audio, NavMesh)
//      Project       — AssetBrowser with ~60 assets
//      Console       — log with typical Unity messages
//      Animation     — Timeline widget with anim tracks
// ─────────────────────────────────────────────────────────────────────────────

// ═════════════════════════════════════════════════════════════════════════════
//  Hierarchy — deep GameObject tree
// ═════════════════════════════════════════════════════════════════════════════
static void buildHierarchy(TreeView* tree)
{
    auto* root = tree->addRoot("SampleScene");
    root->setExpanded(true);

    // Cameras & Lighting (always at top in Unity)
    root->addChild("Main Camera");
    root->addChild("Directional Light");

    // EventSystem
    root->addChild("EventSystem");

    // ── Environment ──────────────────────────────────────────────────────
    auto* env = root->addChild("--- Environment ---");
    env->setExpanded(true);

    auto* terrain = env->addChild("Terrain");
    terrain->addChild("TerrainData");
    terrain->addChild("WindZone");
    terrain->addChild("Water Plane");

    auto* sky = env->addChild("Sky & Fog");
    sky->addChild("Skybox Volume");
    sky->addChild("Fog Volume (Global)");
    sky->addChild("Post-Process Volume");

    auto* buildings = env->addChild("Buildings");
    buildings->setExpanded(true);
    const char* buildNames[] = {"House_A", "House_B", "Tower", "Church",
                                "Warehouse", "Bridge", "Wall_Segment"};
    for (auto& b : buildNames) {
        auto* go = buildings->addChild(b);
        go->addChild("MeshFilter");
        go->addChild("MeshRenderer");
        go->addChild("BoxCollider");
    }

    auto* vegetation = env->addChild("Vegetation");
    const char* trees[] = {"Tree_Oak_01", "Tree_Oak_02", "Tree_Pine_01",
                           "Tree_Pine_02", "Tree_Birch_01", "Bush_01", "Bush_02"};
    for (auto& t : trees) vegetation->addChild(t);

    auto* props = env->addChild("Props");
    const char* propNames[] = {"Barrel_01", "Barrel_02", "Crate_Wood", "Crate_Metal",
                               "Lamp_Street", "Bench_Park", "Fence_01", "Cart"};
    for (auto& p : propNames) props->addChild(p);

    // ── Characters ───────────────────────────────────────────────────────
    auto* chars = root->addChild("--- Characters ---");
    chars->setExpanded(true);

    auto makeCharacter = [](TreeNode* parent, const char* name) {
        auto* go = parent->addChild(name);
        go->setExpanded(true);
        go->addChild("Model");
        auto* armature = go->addChild("Armature");
        armature->addChild("Hips");
        armature->addChild("Spine");
        armature->addChild("Head");
        armature->addChild("LeftHand");
        armature->addChild("RightHand");
        go->addChild("Animator");
        go->addChild("CapsuleCollider");
        go->addChild("Rigidbody");
        go->addChild("NavMeshAgent");
        return go;
    };

    auto* player = makeCharacter(chars, "Player");
    player->addChild("PlayerController (Script)");
    player->addChild("InventoryManager (Script)");
    player->addChild("AudioSource");

    auto* enemies = chars->addChild("Enemies");
    enemies->setExpanded(true);
    for (int i = 0; i < 5; ++i) {
        char buf[32]; snprintf(buf, sizeof(buf), "Enemy_%02d", i);
        auto* e = makeCharacter(enemies, buf);
        e->addChild("AIBehaviour (Script)");
        e->addChild("HealthBar (Canvas)");
    }

    auto* npcs = chars->addChild("NPCs");
    const char* npcNames[] = {"Merchant", "Guard_01", "Guard_02", "Villager_01",
                               "Villager_02", "Blacksmith"};
    for (auto& n : npcNames) {
        makeCharacter(npcs, n);
    }

    // ── UI ───────────────────────────────────────────────────────────────
    auto* ui = root->addChild("--- UI ---");
    ui->setExpanded(true);

    auto* canvas = ui->addChild("Canvas");
    canvas->setExpanded(true);
    auto* hud = canvas->addChild("HUD");
    hud->addChild("HealthBar");
    hud->addChild("ManaBar");
    hud->addChild("StaminaBar");
    hud->addChild("Minimap");
    hud->addChild("Crosshair");
    hud->addChild("CompassBar");

    auto* inventory = canvas->addChild("InventoryPanel");
    inventory->addChild("GridContainer");
    inventory->addChild("ItemTooltip");
    inventory->addChild("DragGhost");

    auto* dialog = canvas->addChild("DialoguePanel");
    dialog->addChild("SpeakerName");
    dialog->addChild("DialogueText");
    dialog->addChild("ChoiceButtons");

    canvas->addChild("PauseMenu");
    canvas->addChild("SettingsPanel");
    canvas->addChild("LoadingScreen");

    // ── Systems ──────────────────────────────────────────────────────────
    auto* sys = root->addChild("--- Game Systems ---");
    sys->addChild("GameManager");
    sys->addChild("AudioManager");
    sys->addChild("QuestManager");
    sys->addChild("SaveManager");
    sys->addChild("SpawnManager");
    sys->addChild("ObjectPool");
    sys->addChild("NavMesh Surface");

    // ── VFX ──────────────────────────────────────────────────────────────
    auto* vfx = root->addChild("--- VFX ---");
    vfx->addChild("ParticleSystem_Fire");
    vfx->addChild("ParticleSystem_Smoke");
    vfx->addChild("ParticleSystem_Rain");
    vfx->addChild("ParticleSystem_Dust");
    vfx->addChild("ParticleSystem_MagicTrail");
    vfx->addChild("ParticleSystem_BloodSplat");
    vfx->addChild("VFX_Graph_Fireflies");
    vfx->addChild("VFX_Graph_Lightning");
}

// ═════════════════════════════════════════════════════════════════════════════
//  Inspector — heavy PropertyGrid (~70 properties)
// ═════════════════════════════════════════════════════════════════════════════
static void buildInspector(PropertyGrid* pg)
{

    // --- GameObject header ---
    pg->addSection("GameObject");
    pg->addString("Name", "Player");
    pg->addString("Tag", "Player");
    pg->addCombo("Layer", {"Default", "TransparentFX", "Water", "UI", "Player", "Enemy"}, 4);
    pg->addBool("Static", false);

    // --- Transform ---
    pg->addSection("Transform");
    pg->addVec3("Position", 12.5f, 0.0f, -3.2f, -1000.f, 1000.f);
    pg->addVec3("Rotation", 0.0f, 135.0f, 0.0f, -360.f, 360.f);
    pg->addVec3("Scale", 1.0f, 1.0f, 1.0f, 0.001f, 100.f);

    // --- Mesh Filter ---
    pg->addSection("Mesh Filter");
    pg->addString("Mesh", "player_mesh (Mesh)");

    // --- Mesh Renderer ---
    pg->addSection("Mesh Renderer");
    pg->addBool("Enabled", true);
    pg->addCombo("Cast Shadows", {"Off", "On", "Two Sided", "Shadows Only"}, 1);
    pg->addBool("Receive Shadows", true);
    pg->addCombo("Light Probes", {"Off", "Blend Probes", "Use Proxy Volume"}, 1);
    pg->addSeparator();
    pg->addString("Material 0", "Player_Body (Material)");
    pg->addString("Material 1", "Player_Armor (Material)");
    pg->addString("Material 2", "Player_Eyes (Material)");

    // --- Rigidbody ---
    pg->addSection("Rigidbody");
    pg->addFloat("Mass", 75.0f, 0.01f, 1000.0f);
    pg->addFloat("Drag", 0.0f, 0.0f, 100.0f);
    pg->addFloat("Angular Drag", 0.05f, 0.0f, 100.0f);
    pg->addBool("Use Gravity", true);
    pg->addBool("Is Kinematic", false);
    pg->addCombo("Interpolate", {"None", "Interpolate", "Extrapolate"}, 1);
    pg->addCombo("Collision Detection", {"Discrete", "Continuous", "ContinuousDynamic", "ContinuousSpeculative"}, 0);

    // --- Capsule Collider ---
    pg->addSection("Capsule Collider");
    pg->addBool("Is Trigger", false);
    pg->addString("Material", "Player_Physics (Physics Material)");
    pg->addVec3("Center", 0.0f, 0.9f, 0.0f, -10.f, 10.f);
    pg->addFloat("Radius", 0.35f, 0.01f, 10.0f);
    pg->addFloat("Height", 1.8f, 0.01f, 20.0f);
    pg->addCombo("Direction", {"X-Axis", "Y-Axis", "Z-Axis"}, 1);

    // --- Animator ---
    pg->addSection("Animator");
    pg->addString("Controller", "PlayerAnimController (RuntimeAnimatorController)");
    pg->addString("Avatar", "player_meshAvatar (Avatar)");
    pg->addBool("Apply Root Motion", true);
    pg->addCombo("Update Mode", {"Normal", "Animate Physics", "Unscaled Time"}, 0);
    pg->addCombo("Culling Mode", {"Always Animate", "Cull Update Transforms", "Cull Completely"}, 0);

    // --- PlayerController (Script) ---
    pg->addSection("PlayerController (Script)");
    pg->addFloat("Move Speed", 5.0f, 0.0f, 50.0f);
    pg->addFloat("Sprint Multiplier", 1.6f, 1.0f, 5.0f);
    pg->addFloat("Jump Force", 8.0f, 0.0f, 50.0f);
    pg->addFloat("Gravity", -20.0f, -100.0f, 0.0f);
    pg->addFloat("Mouse Sensitivity", 2.0f, 0.1f, 10.0f);
    pg->addBool("Invert Y", false);
    pg->addFloat("Ground Check Radius", 0.2f, 0.01f, 1.0f);
    pg->addString("Ground Layer Mask", "Default, Ground");
    pg->addString("Camera Pivot", "CameraPivot (Transform)");

    // --- InventoryManager (Script) ---
    pg->addSection("InventoryManager (Script)");
    pg->addInt("Max Slots", 28, 1, 100);
    pg->addFloat("Max Weight", 150.0f, 10.0f, 1000.0f);
    pg->addBool("Auto Sort", true);
    pg->addCombo("Sort Mode", {"ByName", "ByType", "ByValue", "ByWeight"}, 0);
    pg->addString("Tooltip Prefab", "ItemTooltip (GameObject)");

    // --- Audio Source ---
    pg->addSection("Audio Source");
    pg->addString("AudioClip", "footstep_grass (AudioClip)");
    pg->addBool("Play On Awake", false);
    pg->addBool("Loop", false);
    pg->addFloat("Volume", 0.8f, 0.0f, 1.0f);
    pg->addFloat("Pitch", 1.0f, -3.0f, 3.0f);
    pg->addFloat("Spatial Blend", 1.0f, 0.0f, 1.0f);
    pg->addFloat("Min Distance", 1.0f, 0.0f, 500.0f);
    pg->addFloat("Max Distance", 30.0f, 0.0f, 500.0f);

    // --- Nav Mesh Agent ---
    pg->addSection("Nav Mesh Agent");
    pg->addFloat("Speed", 3.5f, 0.0f, 50.0f);
    pg->addFloat("Angular Speed", 120.0f, 0.0f, 1000.0f);
    pg->addFloat("Acceleration", 8.0f, 0.0f, 100.0f);
    pg->addFloat("Stopping Distance", 0.5f, 0.0f, 50.0f);
    pg->addFloat("Agent Radius", 0.35f, 0.01f, 5.0f);
    pg->addFloat("Agent Height", 1.8f, 0.01f, 10.0f);
    pg->addFloat("Base Offset", 0.0f, -5.0f, 5.0f);
    pg->addCombo("Obstacle Avoidance", {"None", "Low", "Medium", "High", "Very High"}, 2);
    pg->addInt("Priority", 50, 0, 99);

    pg->addButton("Add Component");
}

// ═════════════════════════════════════════════════════════════════════════════
//  Project — AssetBrowser with ~60 items
// ═════════════════════════════════════════════════════════════════════════════
static void populateProjectBrowser(AssetBrowser* ab)
{
    ab->setPath("Assets");
    ab->setThumbSize(64);

    // Folders
    ab->addItem({"Animations",  AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"Audio",       AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"Materials",   AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"Models",      AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"Plugins",     AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"Prefabs",     AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"Resources",   AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"Scenes",      AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"Scripts",     AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"Shaders",     AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"Textures",    AssetType::Folder, Color(200, 160,  60, 255)});
    ab->addItem({"UI",          AssetType::Folder, Color(200, 160,  60, 255)});

    // Scripts
    ab->addItem({"PlayerController.cs",   AssetType::Script,   Color( 80, 180, 100, 255)});
    ab->addItem({"AIBehaviour.cs",        AssetType::Script,   Color( 80, 180, 100, 255)});
    ab->addItem({"InventoryManager.cs",   AssetType::Script,   Color( 80, 180, 100, 255)});
    ab->addItem({"QuestManager.cs",       AssetType::Script,   Color( 80, 180, 100, 255)});
    ab->addItem({"GameManager.cs",        AssetType::Script,   Color( 80, 180, 100, 255)});
    ab->addItem({"SaveManager.cs",        AssetType::Script,   Color( 80, 180, 100, 255)});
    ab->addItem({"SpawnManager.cs",       AssetType::Script,   Color( 80, 180, 100, 255)});
    ab->addItem({"DialogueSystem.cs",     AssetType::Script,   Color( 80, 180, 100, 255)});
    ab->addItem({"CameraController.cs",   AssetType::Script,   Color( 80, 180, 100, 255)});

    // Materials
    ab->addItem({"Player_Body.mat",       AssetType::Material, Color(120, 100, 200, 255)});
    ab->addItem({"Player_Armor.mat",      AssetType::Material, Color(120, 100, 200, 255)});
    ab->addItem({"Terrain_Ground.mat",    AssetType::Material, Color(120, 100, 200, 255)});
    ab->addItem({"Water_Surface.mat",     AssetType::Material, Color(100, 150, 220, 255)});
    ab->addItem({"URP_Lit_Default.mat",   AssetType::Material, Color(120, 100, 200, 255)});
    ab->addItem({"Skybox_Procedural.mat", AssetType::Material, Color(100, 130, 200, 255)});

    // Textures
    ab->addItem({"ground_albedo.png",      AssetType::Image, Color(140, 110,  70, 255)});
    ab->addItem({"ground_normal.png",      AssetType::Image, Color(128, 128, 255, 255)});
    ab->addItem({"bark_albedo.png",        AssetType::Image, Color( 90,  70,  50, 255)});
    ab->addItem({"armor_metallic.png",     AssetType::Image, Color(180, 180, 180, 255)});
    ab->addItem({"skin_albedo.png",        AssetType::Image, Color(200, 160, 130, 255)});
    ab->addItem({"water_normal.png",       AssetType::Image, Color(100, 130, 200, 255)});
    ab->addItem({"noise_perlin_256.png",   AssetType::Image, Color(128, 128, 128, 255)});

    // Meshes
    ab->addItem({"player_mesh.fbx",       AssetType::Mesh,  Color(100, 200, 200, 255)});
    ab->addItem({"enemy_mesh.fbx",        AssetType::Mesh,  Color(100, 200, 200, 255)});
    ab->addItem({"house_A.fbx",           AssetType::Mesh,  Color(100, 200, 200, 255)});
    ab->addItem({"house_B.fbx",           AssetType::Mesh,  Color(100, 200, 200, 255)});
    ab->addItem({"tree_oak.fbx",          AssetType::Mesh,  Color(100, 200, 200, 255)});
    ab->addItem({"tree_pine.fbx",         AssetType::Mesh,  Color(100, 200, 200, 255)});
    ab->addItem({"barrel.fbx",            AssetType::Mesh,  Color(100, 200, 200, 255)});
    ab->addItem({"crate.fbx",             AssetType::Mesh,  Color(100, 200, 200, 255)});

    // Audio
    ab->addItem({"footstep_grass.wav",    AssetType::Audio, Color(220, 160,  80, 255)});
    ab->addItem({"sword_swing.wav",       AssetType::Audio, Color(220, 160,  80, 255)});
    ab->addItem({"ambient_forest.ogg",    AssetType::Audio, Color(220, 160,  80, 255)});
    ab->addItem({"ui_click.wav",          AssetType::Audio, Color(220, 160,  80, 255)});
    ab->addItem({"music_explore.ogg",     AssetType::Audio, Color(220, 160,  80, 255)});
    ab->addItem({"music_combat.ogg",      AssetType::Audio, Color(220, 160,  80, 255)});
    ab->addItem({"explosion.wav",         AssetType::Audio, Color(220, 160,  80, 255)});

    // Scenes
    ab->addItem({"MainMenu.unity",        AssetType::Scene, Color(180, 100, 220, 255)});
    ab->addItem({"SampleScene.unity",     AssetType::Scene, Color(180, 100, 220, 255)});
    ab->addItem({"TestLevel.unity",       AssetType::Scene, Color(180, 100, 220, 255)});
}

// ═════════════════════════════════════════════════════════════════════════════
//  Animation tracks
// ═════════════════════════════════════════════════════════════════════════════
static void buildAnimationTracks(Timeline* tl)
{
    tl->setTimeRange(0.0f, 120.0f);
    tl->setPlayhead(24.0f);

    int t0 = tl->addTrack("Position",       Color(220,  80,  80, 255));
    int t1 = tl->addTrack("Rotation",       Color( 80, 180,  80, 255));
    int t2 = tl->addTrack("Scale",          Color( 80, 100, 220, 255));
    int t3 = tl->addTrack("Blend Shape",    Color(200, 160,  60, 255));
    int t4 = tl->addTrack("IK Left Hand",   Color(180, 100, 200, 255));
    int t5 = tl->addTrack("IK Right Hand",  Color(200, 120, 180, 255));

    // Position keyframes
    float posKeys[] = {0, 10, 24, 40, 55, 72, 90, 110, 120};
    for (float k : posKeys) tl->addKeyframe(t0, k);

    // Rotation keyframes
    float rotKeys[] = {0, 15, 30, 50, 70, 90, 120};
    for (float k : rotKeys) tl->addKeyframe(t1, k);

    // Scale keyframes
    tl->addKeyframe(t2, 0); tl->addKeyframe(t2, 60); tl->addKeyframe(t2, 120);

    // Blend shape
    float bsKeys[] = {0, 20, 24, 28, 50, 55, 80, 100, 120};
    for (float k : bsKeys) tl->addKeyframe(t3, k);

    // IK clips
    tl->addClip(t4, 10, 40, "Grab Sword");
    tl->addClip(t4, 60, 85, "Shield Block");
    tl->addClip(t5, 15, 35, "Swing Attack");
    tl->addClip(t5, 50, 75, "Cast Spell");
    tl->addClip(t5, 90, 115, "Throw Item");
}

// ═════════════════════════════════════════════════════════════════════════════
//  Registration
// ═════════════════════════════════════════════════════════════════════════════
void registerFakeUnityStage(WidgetApp& app)
{
    auto* root = app.addStage("fakeunity");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setStretch(1);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Menu Bar ─────────────────────────────────────────────────────────
    auto* menuBar = outer->createChild<MenuBar>();
    {
        // Static submenus
        static Menu* recentMenu   = nullptr;
        static Menu* buildMenu    = nullptr;
        static Menu* create3dMenu = nullptr;
        static Menu* create2dMenu = nullptr;
        static Menu* lightMenu    = nullptr;
        static Menu* audioMenu    = nullptr;
        static Menu* uiMenu       = nullptr;
        static Menu* physicsMenu  = nullptr;
        static Menu* renderMenu   = nullptr;
        static Menu* layoutMenu   = nullptr;

        if (!recentMenu) {
            recentMenu = new Menu();
            recentMenu->addAction("SampleScene.unity");
            recentMenu->addAction("MainMenu.unity");
            recentMenu->addAction("TestLevel.unity");
            recentMenu->addSeparator();
            recentMenu->addAction("Clear Recent");
        }
        if (!buildMenu) {
            buildMenu = new Menu();
            buildMenu->addAction("PC, Mac & Linux Standalone");
            buildMenu->addAction("WebGL");
            buildMenu->addAction("Android");
            buildMenu->addAction("iOS");
            buildMenu->addAction("PS5");
            buildMenu->addAction("Xbox Series X");
        }
        if (!create3dMenu) {
            create3dMenu = new Menu();
            create3dMenu->addAction("Cube");
            create3dMenu->addAction("Sphere");
            create3dMenu->addAction("Capsule");
            create3dMenu->addAction("Cylinder");
            create3dMenu->addAction("Plane");
            create3dMenu->addAction("Quad");
            create3dMenu->addAction("Terrain");
            create3dMenu->addAction("Wind Zone");
            create3dMenu->addAction("Tree");
        }
        if (!create2dMenu) {
            create2dMenu = new Menu();
            create2dMenu->addAction("Sprite");
            create2dMenu->addAction("Sprite Mask");
            create2dMenu->addAction("Tilemap");
            create2dMenu->addAction("Sprite Shape");
        }
        if (!lightMenu) {
            lightMenu = new Menu();
            lightMenu->addAction("Directional Light");
            lightMenu->addAction("Point Light");
            lightMenu->addAction("Spot Light");
            lightMenu->addAction("Area Light");
            lightMenu->addAction("Reflection Probe");
            lightMenu->addAction("Light Probe Group");
        }
        if (!audioMenu) {
            audioMenu = new Menu();
            audioMenu->addAction("Audio Source");
            audioMenu->addAction("Audio Listener");
            audioMenu->addAction("Audio Reverb Zone");
        }
        if (!uiMenu) {
            uiMenu = new Menu();
            uiMenu->addAction("Canvas");
            uiMenu->addAction("Panel");
            uiMenu->addAction("Button");
            uiMenu->addAction("Text - TextMeshPro");
            uiMenu->addAction("Image");
            uiMenu->addAction("Slider");
            uiMenu->addAction("Scrollbar");
            uiMenu->addAction("Input Field");
            uiMenu->addAction("Dropdown");
            uiMenu->addAction("Toggle");
        }
        if (!physicsMenu) {
            physicsMenu = new Menu();
            physicsMenu->addAction("Rigidbody");
            physicsMenu->addAction("Character Controller");
            physicsMenu->addSeparator();
            physicsMenu->addAction("Box Collider");
            physicsMenu->addAction("Sphere Collider");
            physicsMenu->addAction("Capsule Collider");
            physicsMenu->addAction("Mesh Collider");
            physicsMenu->addSeparator();
            physicsMenu->addAction("Cloth");
            physicsMenu->addAction("Joint (Hinge)");
        }
        if (!renderMenu) {
            renderMenu = new Menu();
            renderMenu->addAction("Camera");
            renderMenu->addAction("Skybox");
            renderMenu->addAction("Flare Layer");
            renderMenu->addAction("LOD Group");
            renderMenu->addAction("Occlusion Area");
        }
        if (!layoutMenu) {
            layoutMenu = new Menu();
            layoutMenu->addAction("Default");
            layoutMenu->addAction("2 by 3");
            layoutMenu->addAction("4 Split");
            layoutMenu->addAction("Tall");
            layoutMenu->addAction("Wide");
        }

        // File
        auto* fileMenu = menuBar->addMenu("File");
        fileMenu->addAction("New Scene")->setShortcut("Ctrl+N");
        fileMenu->addAction("Open Scene")->setShortcut("Ctrl+O");
        fileMenu->addAction("Open Recent")->setSubmenu(recentMenu);
        fileMenu->addSeparator();
        fileMenu->addAction("Save")->setShortcut("Ctrl+S");
        fileMenu->addAction("Save As...")->setShortcut("Ctrl+Shift+S");
        fileMenu->addSeparator();
        fileMenu->addAction("Build Settings...")->setShortcut("Ctrl+Shift+B");
        fileMenu->addAction("Build And Run")->setShortcut("Ctrl+B");
        fileMenu->addAction("Build")->setSubmenu(buildMenu);
        fileMenu->addSeparator();
        fileMenu->addAction("Exit");

        // Edit
        auto* editMenu = menuBar->addMenu("Edit");
        editMenu->addAction("Undo")->setShortcut("Ctrl+Z");
        editMenu->addAction("Redo")->setShortcut("Ctrl+Y");
        editMenu->addSeparator();
        editMenu->addAction("Select All")->setShortcut("Ctrl+A");
        editMenu->addAction("Deselect All")->setShortcut("Shift+D");
        editMenu->addAction("Invert Selection");
        editMenu->addSeparator();
        editMenu->addAction("Cut")->setShortcut("Ctrl+X");
        editMenu->addAction("Copy")->setShortcut("Ctrl+C");
        editMenu->addAction("Paste")->setShortcut("Ctrl+V");
        editMenu->addAction("Duplicate")->setShortcut("Ctrl+D");
        editMenu->addAction("Delete")->setShortcut("Delete");
        editMenu->addSeparator();
        editMenu->addAction("Play")->setShortcut("Ctrl+P");
        editMenu->addAction("Pause")->setShortcut("Ctrl+Shift+P");
        editMenu->addAction("Step")->setShortcut("Ctrl+Alt+P");
        editMenu->addSeparator();
        editMenu->addAction("Project Settings...");
        editMenu->addAction("Preferences...");

        // Assets
        auto* assetsMenu = menuBar->addMenu("Assets");
        assetsMenu->addAction("Create")->setSubmenu(create3dMenu);
        assetsMenu->addSeparator();
        assetsMenu->addAction("Import New Asset...");
        assetsMenu->addAction("Import Package");
        assetsMenu->addAction("Export Package...");
        assetsMenu->addSeparator();
        assetsMenu->addAction("Refresh")->setShortcut("Ctrl+R");
        assetsMenu->addAction("Reimport All");

        // GameObject
        auto* goMenu = menuBar->addMenu("GameObject");
        goMenu->addAction("Create Empty")->setShortcut("Ctrl+Shift+N");
        goMenu->addAction("Create Empty Child")->setShortcut("Alt+Shift+N");
        goMenu->addSeparator();
        goMenu->addAction("3D Object")->setSubmenu(create3dMenu);
        goMenu->addAction("2D Object")->setSubmenu(create2dMenu);
        goMenu->addAction("Light")->setSubmenu(lightMenu);
        goMenu->addAction("Audio")->setSubmenu(audioMenu);
        goMenu->addAction("UI")->setSubmenu(uiMenu);
        goMenu->addAction("Camera");
        goMenu->addAction("Particle System");
        goMenu->addSeparator();
        goMenu->addAction("Align With View")->setShortcut("Ctrl+Shift+F");
        goMenu->addAction("Move To View")->setShortcut("Ctrl+Alt+F");

        // Component
        auto* compMenu = menuBar->addMenu("Component");
        compMenu->addAction("Physics")->setSubmenu(physicsMenu);
        compMenu->addAction("Rendering")->setSubmenu(renderMenu);
        compMenu->addAction("Audio")->setSubmenu(audioMenu);
        compMenu->addAction("UI")->setSubmenu(uiMenu);
        compMenu->addSeparator();
        compMenu->addAction("New Script...");
        compMenu->addAction("Add...");

        // Window
        auto* winMenu = menuBar->addMenu("Window");
        winMenu->addAction("General")->setSubmenu(layoutMenu);
        winMenu->addSeparator();
        winMenu->addAction("Scene")->setShortcut("Ctrl+1");
        winMenu->addAction("Game")->setShortcut("Ctrl+2");
        winMenu->addAction("Inspector")->setShortcut("Ctrl+3");
        winMenu->addAction("Hierarchy")->setShortcut("Ctrl+4");
        winMenu->addAction("Project")->setShortcut("Ctrl+5");
        winMenu->addAction("Console")->setShortcut("Ctrl+Shift+C");
        winMenu->addAction("Animation")->setShortcut("Ctrl+6");
        winMenu->addAction("Profiler")->setShortcut("Ctrl+7");
        winMenu->addAction("Package Manager");

        // Help
        auto* helpMenu = menuBar->addMenu("Help");
        helpMenu->addAction("Unity Manual");
        helpMenu->addAction("Scripting Reference");
        helpMenu->addAction("Unity Forum");
        helpMenu->addSeparator();
        helpMenu->addAction("Check for Updates");
        helpMenu->addAction("Release Notes");
        helpMenu->addAction("About Unity (fake)");
    }

    // ── Toolbar (Play/Pause/Step + transform tools) ─────────────────────
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

        // Transform tools
        toolRow->createChild<Button>("Q Hand");
        toolRow->createChild<Button>("W Move");
        toolRow->createChild<Button>("E Rotate");
        toolRow->createChild<Button>("R Scale");
        toolRow->createChild<Button>("T Rect");
        toolRow->createChild<Button>("Y Multi");

        toolRow->createChild<Line>();

        // Pivot / Local-Global toggles
        toolRow->createChild<Button>("Pivot");
        toolRow->createChild<Button>("Local");

        toolRow->createChild<Spacer>(0.f)->setStretch(1);

        // Play controls (centered)
        auto* playBtn  = toolRow->createChild<Button>("Play");
        playBtn->setIconId(IconId::Play);
        auto* pauseBtn = toolRow->createChild<Button>("Pause");
        pauseBtn->setIconId(IconId::Pause);
        auto* stepBtn  = toolRow->createChild<Button>("Step");
        stepBtn->setIconId(IconId::StepForward);

        toolRow->createChild<Spacer>(0.f)->setStretch(1);

        // Right side
        toolRow->createChild<Label>("Layers: Everything");
        toolRow->createChild<Line>();
        toolRow->createChild<Label>("Layout: Default");
    }

    // ── DockPanel — main editor area ─────────────────────────────────────
    auto* dock = outer->createChild<DockPanel>();
    dock->setStretch(1);

    // Hierarchy
    {
        auto* tree = dock->addPanel<TreeView>("Hierarchy");
        buildHierarchy(tree);
    }

    // Scene View
    {
        auto* sceneBox = dock->addPanel<BoxLayout>("Scene", LayoutDir::Vertical);
        sceneBox->setPadding(8);
        sceneBox->setSpacing(8);

        // Scene toolbar
        auto* sceneToolbar = sceneBox->createChild<BoxLayout>(LayoutDir::Horizontal);
        sceneToolbar->setSpacing(6);
        sceneToolbar->createChild<Button>("Shaded");
        sceneToolbar->createChild<Button>("2D");
        sceneToolbar->createChild<Button>("Lighting");
        sceneToolbar->createChild<Button>("Audio");
        sceneToolbar->createChild<Button>("FX");
        sceneToolbar->createChild<Spacer>(0.f)->setStretch(1);
        sceneToolbar->createChild<Button>("Gizmos");

        // Viewport area with gizmo
        auto* viewport = sceneBox->createChild<Panel>();
        viewport->setStretch(1);
        viewport->setBgColor(Color(45, 45, 50, 255));
        auto* gizmo = viewport->createChild<Gizmo2D>();
        gizmo->setPosition(300, 200);
        gizmo->setMode(GizmoMode::Translate);

        // Fake info overlay
        auto* infoOverlay = sceneBox->createChild<BoxLayout>(LayoutDir::Horizontal);
        infoOverlay->setSpacing(16);
        infoOverlay->createChild<Label>("FPS: 120.4");
        infoOverlay->createChild<Label>("Tris: 1.2M");
        infoOverlay->createChild<Label>("Verts: 682K");
        infoOverlay->createChild<Label>("SetPass: 87");
        infoOverlay->createChild<Label>("Batches: 234");
        infoOverlay->createChild<Label>("Shadow Casters: 42");
    }

    // Game View
    {
        auto* gameBox = dock->addPanel<BoxLayout>("Game", LayoutDir::Vertical);
        gameBox->setPadding(12);
        gameBox->setSpacing(8);

        auto* gameToolbar = gameBox->createChild<BoxLayout>(LayoutDir::Horizontal);
        gameToolbar->setSpacing(6);
        gameToolbar->createChild<Label>("Display 1");
        gameToolbar->createChild<Line>();
        gameToolbar->createChild<Label>("Free Aspect");
        gameToolbar->createChild<Line>();
        gameToolbar->createChild<Label>("Scale 1.0x");
        gameToolbar->createChild<Spacer>(0.f)->setStretch(1);
        gameToolbar->createChild<Button>("Maximize");
        gameToolbar->createChild<Button>("Mute Audio");
        gameToolbar->createChild<Button>("Stats");

        auto* gameArea = gameBox->createChild<Panel>();
        gameArea->setStretch(1);
        gameArea->setBgColor(Color(30, 30, 35, 255));
        auto* centerLabel = gameArea->createChild<Label>("Game View (not playing)");
        centerLabel->setColor(Color(100, 100, 120, 255));
    }

    // Inspector
    {
        auto* pg = dock->addPanel<PropertyGrid>("Inspector");
        buildInspector(pg);
    }

    // Project (Asset Browser)
    {
        auto* projBox = dock->addPanel<BoxLayout>("Project", LayoutDir::Vertical);
        projBox->setPadding(4);
        projBox->setSpacing(4);

        auto* projToolbar = projBox->createChild<BoxLayout>(LayoutDir::Horizontal);
        projToolbar->setSpacing(4);
        projToolbar->createChild<Button>("Create");
        projToolbar->createChild<Spacer>(0.f)->setStretch(1);
        auto* searchInput = projToolbar->createChild<TextInput>();
        searchInput->setPlaceholder("Search...");
        searchInput->setSize(180, 22);

        auto* ab = projBox->createChild<AssetBrowser>();
        ab->setStretch(1);
        populateProjectBrowser(ab);
    }

    // Console
    {
        auto* con = dock->addPanel<ConsoleWidget>("Console");
        con->log(LogLevel::Info,  "[Unity] Built-in packages loaded (42 packages)");
        con->log(LogLevel::Info,  "[Shader] Compiled 'URP/Lit' (1 variants) in 0.12s");
        con->log(LogLevel::Info,  "[Shader] Compiled 'URP/Unlit' (1 variants) in 0.04s");
        con->log(LogLevel::Info,  "[AssetDatabase] Refresh completed: 287 assets (0.63s)");
        con->log(LogLevel::Warn,  "[Physics] Rigidbody 'Barrel_02' has non-convex MeshCollider with Is Trigger enabled");
        con->log(LogLevel::Info,  "[Scene] 'SampleScene' loaded in 1.24s (168 GameObjects)");
        con->log(LogLevel::Warn,  "[Animation] 'Enemy_03' Animator has missing transition condition");
        con->log(LogLevel::Error, "[Script] NullReferenceException: Object reference not set in AIBehaviour.cs:142");
        con->log(LogLevel::Error, "[Script]   at AIBehaviour.FindNearestTarget() in AIBehaviour.cs:142");
        con->log(LogLevel::Error, "[Script]   at AIBehaviour.Update() in AIBehaviour.cs:87");
        con->log(LogLevel::Info,  "[NavMesh] Surface baked: 12,847 polygons (0.38s)");
        con->log(LogLevel::Warn,  "[Texture] 'ground_albedo.png' is 4096x4096 — consider using streaming");
        con->log(LogLevel::Info,  "[Audio] 'ambient_forest.ogg' loaded (48000Hz, Stereo, 3:42)");
        con->log(LogLevel::Info,  "[Build] Target: StandaloneWindows64 | Scripting: IL2CPP | Config: Release");
        con->log(LogLevel::Info,  "[GC] Heap: 128 MB | Used: 67 MB | Collections: 12 (last frame: 0)");
        con->log(LogLevel::Warn,  "[Performance] Frame time spike: 42ms (target 16.6ms) at frame 8,412");
        con->log(LogLevel::Info,  "[Player] Position: (12.5, 0.0, -3.2) | Velocity: (2.1, 0.0, 1.8)");
        con->log(LogLevel::Info,  "[QuestManager] Active quests: 3 | Completed: 7");
        con->log(LogLevel::Info,  "Ready — Unity (fake) 2024.1.0f1");
    }

    // Animation
    {
        auto* tl = dock->addPanel<Timeline>("Animation");
        buildAnimationTracks(tl);
    }

    // ── Initial dock layout (Unity-like) ─────────────────────────────────
    // Unity default: Hierarchy left, Inspector right, Scene+Game center, Project+Console bottom
    dock->splitOff("Hierarchy",  DockSide::Left,    0.18f);
    dock->splitOff("Inspector",  DockSide::Right,   0.24f);
    dock->splitOff("Project",    DockSide::Bottom,  0.28f);
    dock->splitOff("Console",    DockSide::Bottom,  0.40f);
    dock->splitOff("Animation",  DockSide::Bottom,  0.35f);

    // ── Status Bar ───────────────────────────────────────────────────────
    auto* status = outer->createChild<StatusBar>();
    status->createChild<Label>("Unity (fake) 2024.1.0f1  |  URP 16.0.6  |  Scripting: IL2CPP  |  Platform: StandaloneWindows64");
}
