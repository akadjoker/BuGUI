#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "ScrollWidgets.hpp"
#include "AppWidgets.hpp"
#include "TreePropertyColorWidgets.hpp"

void registerAppWidgetsStage(WidgetApp& app)
{
    auto* root = app.addStage("appwidgets");

    auto* sv = root->createChild<ScrollView>();
    sv->setStretch(1);

    auto* vbox = sv->setContent<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(12.0f);
    vbox->setPadding(16.0f);

    auto* back = vbox->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
    vbox->createChild<Line>();

    // ── Breadcrumbs ──────────────────────────────────────────────────────
    vbox->createChild<Label>("Breadcrumbs")->setColor(Color(100, 180, 220, 255));
    auto* bc = vbox->createChild<Breadcrumbs>();
    bc->setPath({"Home", "Projects", "BuGUI", "widgets", "src"});
    auto* bcLabel = vbox->createChild<Label>("Click a segment to navigate");
    bc->itemClicked.connect([bcLabel](int idx) {
        bcLabel->setText("Navigated to segment " + std::to_string(idx));
    });

    vbox->createChild<Spacer>(8);
    vbox->createChild<Line>();

    // ── SearchBar ────────────────────────────────────────────────────────
    vbox->createChild<Label>("SearchBar")->setColor(Color(100, 180, 220, 255));
    auto* sb = vbox->createChild<SearchBar>();
    sb->setPlaceholder("Type to search...");
    auto* searchResult = vbox->createChild<Label>("");
    sb->onTextChanged.connect([searchResult](const std::string& q) {
        searchResult->setText("Searching: " + q);
    });
    sb->onSearch.connect([searchResult](const std::string& q) {
        searchResult->setText("Search submitted: " + q);
    });

    vbox->createChild<Spacer>(8);
    vbox->createChild<Line>();

    // ── SplitButton ──────────────────────────────────────────────────────
    vbox->createChild<Label>("SplitButton")->setColor(Color(100, 180, 220, 255));
    auto* splitLabel = vbox->createChild<Label>("Click primary or dropdown");
    auto* split = vbox->createChild<SplitButton>("Save");
    split->addAction("Save As...", [splitLabel]() { splitLabel->setText("Save As clicked"); });
    split->addAction("Save All",  [splitLabel]() { splitLabel->setText("Save All clicked"); });
    split->addSeparator();
    split->addAction("Export...", [splitLabel]() { splitLabel->setText("Export clicked"); });
    split->clicked.connect([splitLabel]() { splitLabel->setText("Primary Save clicked"); });

    vbox->createChild<Spacer>(8);
    vbox->createChild<Line>();

    // ── ContextMenuBuilder ───────────────────────────────────────────────
    vbox->createChild<Label>("ContextMenuBuilder (right-click the panel below)")->setColor(Color(100, 180, 220, 255));
    auto* ctxPanel = vbox->createChild<Panel>();
    ctxPanel->setSize(300, 60);
    ctxPanel->setBgColor(Color(50, 50, 55, 255));
    auto* ctxLabel = vbox->createChild<Label>("Right-click result: none");
    ContextMenuBuilder::build(ctxPanel, {
        {"Cut",    [ctxLabel]() { ctxLabel->setText("Right-click result: Cut"); },   "Ctrl+X"},
        {"Copy",   [ctxLabel]() { ctxLabel->setText("Right-click result: Copy"); },  "Ctrl+C"},
        {"-"},
        {"Paste",  [ctxLabel]() { ctxLabel->setText("Right-click result: Paste"); }, "Ctrl+V"},
        {"Delete", [ctxLabel]() { ctxLabel->setText("Right-click result: Delete"); }},
    });

    vbox->createChild<Spacer>(8);
    vbox->createChild<Line>();

    // ── Outliner ─────────────────────────────────────────────────────────
    vbox->createChild<Label>("Outliner (Scene Tree)")->setColor(Color(100, 180, 220, 255));
    auto* outliner = vbox->createChild<Outliner>();
    outliner->setSize(400, 200);
    auto* scene = outliner->addRoot("Scene");
    scene->setIcon("Folder");
    auto* cam = scene->addChild("Camera");
    cam->setIcon("Eye");
    auto* cube = scene->addChild("Cube");
    cube->setIcon("File");
    auto* light = scene->addChild("DirectionalLight");
    light->setIcon("Star");
    auto* group = scene->addChild("Group");
    group->setIcon("FolderOpen");
    group->addChild("Sphere");
    group->addChild("Cylinder");
    group->addChild("Plane");

    outliner->setNodeVisible(light, false);
    outliner->setNodeLocked(cube, true);

    auto* olLabel = vbox->createChild<Label>("Selected: none");
    outliner->selectionChanged.connect([olLabel](TreeNode* n) {
        olLabel->setText("Selected: " + (n ? n->text() : "none"));
    });

    vbox->createChild<Spacer>(8);
    vbox->createChild<Line>();

    // ── RichText ─────────────────────────────────────────────────────────
    vbox->createChild<Label>("RichText / Markdown Viewer")->setColor(Color(100, 180, 220, 255));
    auto* rt = vbox->createChild<RichText>();
    rt->setSize(500, 250);
    rt->setMarkdown(
        "# BuGUI Widget Library\n"
        "\n"
        "A **retained-mode** UI toolkit for C++ with zero dependencies.\n"
        "\n"
        "## Features\n"
        "\n"
        "- **80+ widgets** including `CodeEditor`, `NodeEditor`, `Timeline`\n"
        "- *Docking* layout system\n"
        "- `Model/View` architecture\n"
        "- Full [documentation](https://bugui.dev/docs)\n"
        "\n"
        "### Code Example\n"
        "\n"
        "```\n"
        "auto* btn = parent->createChild<Button>(\"Click me\");\n"
        "btn->clicked.connect([]() { printf(\"Hello!\\n\"); });\n"
        "```\n"
        "\n"
        "Built with care.\n"
    );

    vbox->createChild<Spacer>(8);
    vbox->createChild<Line>();

    // ── Viewport3D ───────────────────────────────────────────────────────
    vbox->createChild<Label>("Viewport3D (no FBO attached — shows grid)")->setColor(Color(100, 180, 220, 255));
    auto* vp = vbox->createChild<Viewport3D>();
    vp->setSize(500, 300);
    auto* vpLabel = vbox->createChild<Label>("Orbit: yaw=0 pitch=0");
    vp->onOrbit.connect([vpLabel](float yaw, float pitch) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Orbit: yaw=%.1f pitch=%.1f", yaw, pitch);
        vpLabel->setText(buf);
    });
    vp->onZoom.connect([vpLabel](float delta) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Zoom: delta=%.1f", delta);
        vpLabel->setText(buf);
    });
    vp->onPan.connect([vpLabel](float dx, float dy) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Pan: dx=%.1f dy=%.1f", dx, dy);
        vpLabel->setText(buf);
    });

    vbox->createChild<Spacer>(20);
}
