#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "TreePropertyColorWidgets.hpp"
#include <cstdio>
using namespace BuGUI;

// ─────────────────────────────────────────────────────────────────────────────
//  Stage 09 — TreeView + PropertyGrid + ColorPicker
// ─────────────────────────────────────────────────────────────────────────────

static Label*      s_tpcStatus    = nullptr;
static ColorPicker* s_colorPicker = nullptr;

void registerTreePropColorStage(WidgetApp& app)
{
    s_tpcStatus   = nullptr;
    s_colorPicker = nullptr;

    auto* root = app.addStage("treepropcolor");
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(0.0f);
    vbox->setPadding(0.0f);

    // ── Top bar ────────────────────────────────────────────────────────────
    auto* topBar = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSpacing(8.0f);
    topBar->setPadding(Edges(8.0f, 12.0f));

    auto* back = topBar->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
    topBar->createChild<Spacer>(0.0f)->setStretch(1.0f);
    auto* titleLbl = topBar->createChild<Label>("09 \u2014 TreeView / PropertyGrid / ColorPicker");
    titleLbl->setAlign(TextAlign::RIGHT);

    vbox->createChild<Line>();

    // ── Main content: 3 columns ────────────────────────────────────────────
    auto* row = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    row->setSpacing(8.0f);
    row->setPadding(12.0f);
    row->setStretch(1.0f);

    // ── Left: TreeView ──────────────────────────────────────────────────────
    auto* leftCol = row->createChild<BoxLayout>(LayoutDir::Vertical);
    leftCol->setSpacing(6.0f);
    leftCol->setPadding(0.0f);

    leftCol->createChild<Label>("TreeView");
    leftCol->createChild<Line>();

    auto* tree = leftCol->createChild<TreeView>();
    tree->setStretch(1.0f);

    // Build sample tree
    auto* srcNode = tree->addRoot("Source");
    srcNode->setIcon("folder");
    srcNode->setExpanded(true);
    {
        auto* appNode = srcNode->addChild("app");
        appNode->setIcon("folder");
        auto* mainFile = appNode->addChild("main.cpp");
        mainFile->setIcon("file_code");
        auto* cfgFile = appNode->addChild("Config.hpp");
        cfgFile->setIcon("file_code");

        auto* coreNode = srcNode->addChild("core");
        coreNode->setIcon("folder");
        auto* rendFile = coreNode->addChild("Renderer.cpp");
        rendFile->setIcon("file_code");
        auto* texFile = coreNode->addChild("Texture.cpp");
        texFile->setIcon("file_code");
        auto* shFile = coreNode->addChild("Shader.cpp");
        shFile->setIcon("file_code");

        auto* uiNode = srcNode->addChild("widgets");
        uiNode->setIcon("folder");
        uiNode->setExpanded(false);
        uiNode->addChild("Button.cpp")->setIcon("file_code");
        uiNode->addChild("Label.cpp")->setIcon("file_code");
    }

    auto* assetsNode = tree->addRoot("Assets");
    assetsNode->setIcon("folder");
    assetsNode->setExpanded(false);
    assetsNode->addChild("logo.png")->setIcon("file");
    assetsNode->addChild("font.ttf")->setIcon("file");
    assetsNode->addChild("shader.glsl")->setIcon("file_code");

    auto* docsNode = tree->addRoot("Docs");
    docsNode->setIcon("book");
    docsNode->addChild("README.md")->setIcon("file");
    docsNode->addChild("API.md")->setIcon("file");

    tree->selectionChanged.connect([](TreeNode* n) {
        if (s_tpcStatus && n)
        {
            static char buf[128];
            snprintf(buf, sizeof(buf), "Selected: %s", n->text().c_str());
            s_tpcStatus->setText(buf);
        }
    });

    // ── Center: PropertyGrid ───────────────────────────────────────────────
    auto* midCol = row->createChild<BoxLayout>(LayoutDir::Vertical);
    midCol->setSpacing(6.0f);
    midCol->setPadding(0.0f);
    midCol->setStretch(1.0f);

    midCol->createChild<Label>("PropertyGrid");
    midCol->createChild<Line>();

    auto* grid = midCol->createChild<PropertyGrid>();
    grid->setDescHeight(48.0f);
    grid->setStretch(1.0f);

    grid->addSection("Transform");
    grid->addVec3("Position", 0.0f, 1.5f, -3.0f, -100.0f, 100.0f,
                  [](float x, float y, float z) {
                      if (s_tpcStatus) {
                          static char b[80];
                          snprintf(b, sizeof(b), "Pos: %.2f, %.2f, %.2f", x, y, z);
                          s_tpcStatus->setText(b);
                      }
                  }, "World-space position");
    grid->addVec3("Rotation", 0.0f, 0.0f, 0.0f, -360.0f, 360.0f,
                  nullptr, "Euler angles (degrees)");
    grid->addVec3("Scale",    1.0f, 1.0f, 1.0f, 0.001f, 100.0f,
                  nullptr, "Uniform scale");

    grid->addSection("Appearance");
    grid->addColor("Albedo",  Color(220, 180, 120, 255),
                   [](const Color& c) {
                       if (s_tpcStatus) {
                           static char b[64];
                           snprintf(b, sizeof(b), "Color: #%02X%02X%02X", c.r, c.g, c.b);
                           s_tpcStatus->setText(b);
                       }
                       if (s_colorPicker) s_colorPicker->setColor(c);
                   }, "Surface base color");
    grid->addFloat("Roughness", 0.4f, 0.0f, 1.0f,
                   [](float v) {
                       if (s_tpcStatus) {
                           static char b[48];
                           snprintf(b, sizeof(b), "Roughness: %.3f", v);
                           s_tpcStatus->setText(b);
                       }
                   }, "PBR roughness (0=mirror, 1=diffuse)");
    grid->addFloat("Metallic",  0.0f, 0.0f, 1.0f, nullptr, "PBR metallic factor");
    grid->addBool("Cast Shadow", true, nullptr, "Object casts shadows");
    grid->addBool("Receive Shadow", true, nullptr, "Object receives shadows");

    grid->addSection("Physics");
    grid->addCombo("Body Type", {"Static", "Dynamic", "Kinematic"}, 1,
                   [](int idx) {
                       const char* names[] = {"Static", "Dynamic", "Kinematic"};
                       if (s_tpcStatus && idx < 3) {
                           static char b[64];
                           snprintf(b, sizeof(b), "Physics: %s", names[idx]);
                           s_tpcStatus->setText(b);
                       }
                   }, "Rigid body type");
    grid->addFloat("Mass", 1.0f, 0.0f, 1000.0f, nullptr, "Mass in kg");
    grid->addFloat("Friction", 0.5f, 0.0f, 1.0f, nullptr, "Surface friction coefficient");
    grid->addRange("Speed Limit", 0.0f, 50.0f, 0.0f, 200.0f, nullptr, "Min/max speed");

    grid->addSection("Debug");
    grid->addString("Tag", "Player", nullptr, "Object tag string");
    grid->addInt("Layer", 0, 0, 31, nullptr, "Render/collision layer");
    grid->addSeparator();
    grid->addButton("Reset Transform", []() {
        if (s_tpcStatus) s_tpcStatus->setText("Transform reset.");
    }, "Reset to default transform");

    grid->propertyChanged.connect([](int row) {
        (void)row;
    });

    // ── Right: ColorPicker ─────────────────────────────────────────────────
    auto* rightCol = row->createChild<BoxLayout>(LayoutDir::Vertical);
    rightCol->setSpacing(6.0f);
    rightCol->setPadding(0.0f);

    rightCol->createChild<Label>("ColorPicker");
    rightCol->createChild<Line>();

    auto* picker = rightCol->createChild<ColorPicker>();
    picker->setShowAlpha(true);
    picker->setColor(Color(220, 180, 120, 255));
    s_colorPicker = picker;

    picker->colorChanged.connect([grid](const Color& c) {
        // sync back to the Albedo row (index varies; find it by name)
        // albedo was added after section + vec3 + vec3 + vec3 = row indices 0,1,2,3,4
        // Section=0, Vec3=1,2,3, Color=4
        grid->setColor(4, c);
        if (s_tpcStatus) {
            static char b[64];
            snprintf(b, sizeof(b), "Color: #%02X%02X%02X", c.r, c.g, c.b);
            s_tpcStatus->setText(b);
        }
    });

    // ── Status bar ─────────────────────────────────────────────────────────
    vbox->createChild<Line>();
    auto* statusBar = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    statusBar->setSpacing(8.0f);
    statusBar->setPadding(Edges(8.0f, 6.0f));

    s_tpcStatus = statusBar->createChild<Label>("Click a tree node or property to begin.");
}
