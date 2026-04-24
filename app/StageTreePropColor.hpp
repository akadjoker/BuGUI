#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  TreeView + PropertyGrid + ColorPicker demo stage
// ═════════════════════════════════════════════════════════════════════════════

inline void buildTreePropColorStage(WidgetApp& app)
{
    Widget* root = app.addStage("treeprops");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Navigation ───────────────────────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    nav->setPadding(16, 16, 0, 16);
    auto* back = nav->createChild<Button>("<  Demo");
    back->clicked.connect([]{ WidgetApp::instance().setStage("demo", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("Tree / Props / Color")->setColor(Clr::accent);
    auto* navGrid = nav->createChild<Button>("DataGrid >");
    navGrid->clicked.connect([]{ WidgetApp::instance().setStage("datagrid", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── Main content: 3-column splitter ──────────────────────────────────
    auto* content = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    content->setStretch(1);
    content->setSpacing(4);
    content->setPadding(4);

    // ── COLUMN 1 — TreeView ──────────────────────────────────────────────
    auto* col1 = content->createChild<BoxLayout>(LayoutDir::Vertical);
    col1->setSize(240, 0);
    col1->setStretch(0);
    col1->setSpacing(4);
    col1->setPadding(0);

    col1->createChild<Label>("TreeView")->setColor(Clr::section);

    auto* tree = col1->createChild<TreeView>();
    tree->setStretch(1);

    // Populate tree
    auto* project = tree->addRoot("Project");
    project->setExpanded(true);
    project->setIcon("folder");

    auto* src = project->addChild("src");
    src->setExpanded(true);
    src->setIcon("folder");
    src->addChild("main.cpp")->setIcon("file");
    src->addChild("Widget.cpp")->setIcon("file");
    src->addChild("Batch.cpp")->setIcon("file");

    auto* inc = project->addChild("include");
    inc->setIcon("folder");
    inc->addChild("Widget.hpp")->setIcon("file");
    inc->addChild("Batch.hpp")->setIcon("file");
    inc->addChild("Font.hpp")->setIcon("file");

    auto* assets = project->addChild("assets");
    assets->setIcon("folder");
    assets->addChild("font.ttf")->setIcon("file");
    assets->addChild("icon.png")->setIcon("file");

    auto* build = project->addChild("build");
    build->setIcon("folder");
    build->addChild("CMakeCache.txt")->setIcon("file");

    auto* docs = tree->addRoot("Documentation");
    docs->setIcon("book");
    docs->addChild("README.md");
    docs->addChild("API.md");
    docs->addChild("CHANGELOG.md");

    // Selection label
    auto* selLabel = col1->createChild<Label>("(none selected)");
    selLabel->setColor(Clr::dim);

    tree->selectionChanged.connect([selLabel](TreeNode* n) {
        if (n)
            selLabel->setText("Selected: " + n->text());
        else
            selLabel->setText("(none selected)");
    });

    // ── COLUMN 2 — PropertyGrid ──────────────────────────────────────────
    auto* col2 = content->createChild<BoxLayout>(LayoutDir::Vertical);
    col2->setStretch(1);
    col2->setSpacing(4);
    col2->setPadding(0);

    col2->createChild<Label>("PropertyGrid")->setColor(Clr::section);

    auto* props = col2->createChild<PropertyGrid>();
    props->setStretch(1);

    // Populate properties
    props->addSection("Transform");
    props->addVec2("Position", 100.0f, 200.0f, 0, 1920, nullptr, "XY position in pixels");
    props->addVec2("Size", 300.0f, 150.0f, 10, 2000, nullptr, "Width and height");
    props->addFloat("Rotation", 0.0f, -360, 360, nullptr, "Rotation angle in degrees");
    props->addVec3("Scale", 1.0f, 1.0f, 1.0f, 0, 10, nullptr, "Scale factor per axis");

    props->addSection("Appearance");
    props->addColor("Background", Color(45, 45, 48, 255));
    props->addColor("Border", Color(70, 70, 70, 255));
    props->addFloat("Opacity", 1.0f, 0, 1, nullptr, "Transparency (0=invisible, 1=opaque)");
    props->addInt("Border Radius", 4, 0, 50, nullptr, "Corner roundness in pixels");
    props->addBool("Visible", true);
    props->addBool("Enabled", true);
    props->addSeparator();
    props->addVec4("Margin", 0, 0, 0, 0, 0, 100, nullptr, "Top, Right, Bottom, Left");
    props->addVec4("Padding", 6, 6, 6, 6, 0, 100, nullptr, "Top, Right, Bottom, Left");

    props->addSection("Text");
    props->addString("Content", "Hello World");
    props->addFloat("Font Size", 16.0f, 8, 72);
    props->addCombo("Align", {"Left", "Center", "Right"}, 0);
    props->addColor("Text Color", Color(220, 220, 220, 255));
    props->addBool("Bold", false);
    props->addBool("Italic", false);
    props->addRange("Letter Spacing", -2.0f, 5.0f, -10, 20, nullptr, "Min/max letter spacing");

    props->addSection("Layout");
    props->addCombo("Direction", {"Horizontal", "Vertical"}, 1);
    props->addFloat("Spacing", 4.0f, 0, 50);
    props->addFloat("Padding", 6.0f, 0, 50);
    props->addCombo("Alignment", {"Start", "Center", "End", "Fill"}, 3);
    props->addRange("Width Limits", 100.0f, 800.0f, 0, 1920, nullptr, "Min/max width constraints");
    props->addSeparator();
    props->addButton("Reset Layout", []{});

    // Change label
    auto* changeLabel = col2->createChild<Label>("");
    changeLabel->setColor(Clr::dim);

    props->propertyChanged.connect([changeLabel](int idx) {
        char buf[48];
        snprintf(buf, sizeof(buf), "Changed row %d", idx);
        changeLabel->setText(buf);
    });

    // ── COLUMN 3 — ColorPicker ───────────────────────────────────────────
    auto* col3 = content->createChild<BoxLayout>(LayoutDir::Vertical);
    col3->setSize(230, 0);
    col3->setStretch(0);
    col3->setSpacing(4);
    col3->setPadding(0);

    col3->createChild<Label>("ColorPicker")->setColor(Clr::section);

    auto* picker = col3->createChild<ColorPicker>();
    picker->setColor(Color(100, 160, 220, 255));
    picker->setSize(220, 260);

    // Preview swatch
    auto* previewPanel = col3->createChild<Panel>();
    previewPanel->setSize(0, 40);
    previewPanel->setBgColor(Color(100, 160, 220, 255));

    auto* hexLabel = col3->createChild<Label>("#6490DC");
    hexLabel->setColor(Clr::dim);

    picker->colorChanged.connect([previewPanel, hexLabel](const Color& c) {
        previewPanel->setBgColor(c);
        char buf[20];
        snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", c.r, c.g, c.b, c.a);
        hexLabel->setText(buf);
    });

    // Second picker without alpha
    col3->createChild<Spacer>(10);
    col3->createChild<Label>("No Alpha")->setColor(Clr::dim);
    auto* picker2 = col3->createChild<ColorPicker>();
    picker2->setColor(Color(220, 80, 80, 255));
    picker2->setShowAlpha(false);
    picker2->setSize(220, 220);
}
