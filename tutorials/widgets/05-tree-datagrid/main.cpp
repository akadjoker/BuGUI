#include "WidgetApp.hpp"
#include "Widgets.hpp"

#include <string>
#include <vector>

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 05 - Tree & DataGrid", 1240, 760))
        return 1;

    Widget* root = app.addStage("main");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(14, 14, 14, 14);
    outer->setSpacing(10);

    auto* title = outer->createChild<Label>("Tutorial 05: TreeView + DataGrid");
    title->setColor(Color(120, 190, 255, 255));
    auto* status = outer->createChild<Label>("Select a tree node to filter the table.");

    auto* row = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    row->setSpacing(10);
    row->setStretch(1);

    auto* left = row->createChild<Panel>();
    left->setStretch(1);
    auto* leftCol = left->createChild<BoxLayout>(LayoutDir::Vertical);
    leftCol->setPadding(8, 8, 8, 8);
    leftCol->setSpacing(6);
    leftCol->createChild<Label>("Project Tree");

    auto* tree = leftCol->createChild<TreeView>();
    tree->setStretch(1);
    auto* nProject = tree->addRoot("Project");
    auto* nSrc = nProject->addChild("src");
    auto* nArt = nProject->addChild("assets");
    auto* nDoc = nProject->addChild("docs");
    nSrc->addChild("main.cpp");
    nSrc->addChild("editor.cpp");
    nArt->addChild("tileset.png");
    nArt->addChild("player.png");
    nDoc->addChild("README.md");
    nProject->setExpanded(true);
    nSrc->setExpanded(true);
    nArt->setExpanded(true);
    nDoc->setExpanded(true);

    auto* right = row->createChild<Panel>();
    right->setStretch(2);
    auto* rightCol = right->createChild<BoxLayout>(LayoutDir::Vertical);
    rightCol->setPadding(8, 8, 8, 8);
    rightCol->setSpacing(6);
    rightCol->createChild<Label>("DataGrid");

    auto* grid = rightCol->createChild<DataGrid>();
    grid->setStretch(1);
    grid->setMultiSelect(true);
    grid->addColumn("Name", 240);
    grid->addColumn("Type", 120);
    grid->addColumn("Size", 100);
    grid->addColumn("Scope", 160);

    using Row = std::vector<std::string>;
    const std::vector<Row> allRows = {
        {"main.cpp", "C++", "6 KB", "src"},
        {"editor.cpp", "C++", "18 KB", "src"},
        {"tileset.png", "Image", "192 KB", "assets"},
        {"player.png", "Image", "88 KB", "assets"},
        {"README.md", "Markdown", "4 KB", "docs"},
        {"GUIDE.md", "Markdown", "17 KB", "docs"},
    };

    auto refillGrid = [grid](const std::vector<Row>& rows) {
        grid->clearRows();
        for (const auto& r : rows) grid->addRow(r);
    };
    refillGrid(allRows);

    tree->selectionChanged.connect([status, tree, allRows, refillGrid](TreeNode* node) {
        if (!node) return;
        const std::string scope = node->text();
        if (scope == "src" || scope == "assets" || scope == "docs")
        {
            std::vector<Row> filtered;
            for (const auto& r : allRows)
                if (r[3] == scope) filtered.push_back(r);
            refillGrid(filtered);
            status->setText("Filtered scope: " + scope);
        }
        else if (scope == "Project")
        {
            refillGrid(allRows);
            status->setText("Showing all rows");
        }
        else
        {
            status->setText("Selected leaf: " + scope);
        }
    });

    grid->selectionChanged.connect([status](int rowIdx) {
        status->setText("DataGrid selected row: " + std::to_string(rowIdx));
    });

    app.setStage("main");
    return app.run();
}
