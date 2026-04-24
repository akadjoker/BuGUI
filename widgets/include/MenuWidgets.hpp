#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include <functional>

class Menu;

// ═════════════════════════════════════════════════════════════════════════════

class Menu : public Widget
{
public:
    Menu();
    ~Menu() override;

    void addAction(const std::string& label, std::function<void()> cb,
                   const std::string& shortcut = "");
    void addAction(const std::string& label, std::function<void()> cb,
                   bool enabled, const std::string& shortcut = "");
    void addCheckable(const std::string& label, bool checked,
                      std::function<void(bool)> cb);
    Menu* addSubmenu(const std::string& label);
    void addSeparator();
    void clear();
    void resetState();  // reset hovered/submenu (call when menu is closed/re-shown)

    const std::vector<MenuAction>& actions() const { return actions_; }

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseLeave() override;
    bool popupContains(float x, float y) const override;
    void resetPopupState() override { resetState(); }

    Signal<> closed;

private:
    std::vector<MenuAction> actions_;
    std::vector<Menu*> ownedSubmenus_;  // submenus owned by this menu
    int hovered_ = -1;   // index of hovered item
    int openSub_ = -1;   // index of open submenu (-1 = none)
    float computeWidth() const;
    bool hasCheckable() const;
    void openSubmenu(int index);
    void closeSubmenu();
};

// ═════════════════════════════════════════════════════════════════════════════
//  MenuBar — horizontal bar of menu buttons (File, Edit, View, ...)
//    Each button opens a Menu popup below it.
// ═════════════════════════════════════════════════════════════════════════════

class MenuBar : public Widget
{
public:
    MenuBar();
    ~MenuBar() override;

    // Add a top-level menu and return it for adding actions
    Menu* addMenu(const std::string& title);

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;

private:
    struct MenuEntry {
        std::string title;
        Menu*       menu = nullptr;
        float       x = 0;     // computed position
        float       w = 0;     // computed width
    };
    std::vector<MenuEntry> entries_;
    int  activeMenu_ = -1;     // currently open menu index (-1 = none)
    int  hoveredItem_ = -1;    // hovered bar item
    bool armed_ = false;       // track if menu bar is in "hover-opens" mode

    void openMenu(int index);
    void closeMenu();
    float computeItemWidth(const std::string& title) const;
};

// ═════════════════════════════════════════════════════════════════════════════
//  ContextMenu — right-click popup menu
//    Attach to any widget: widget->setContextMenu(menu);
//    Or show programmatically: ContextMenu::show(menu, x, y);
// ═════════════════════════════════════════════════════════════════════════════

class ContextMenu
{
public:
    // Show a menu at screen position (x, y). Takes ownership via popup system.
    static void show(Menu* menu, float x, float y, Widget* owner = nullptr);
};

// ═════════════════════════════════════════════════════════════════════════════
//  TreeView — hierarchical tree with expand/collapse nodes
//    auto* tree = parent->createChild<TreeView>();
//    auto* root = tree->addRoot("Project");
//    auto* src  = root->addChild("src");
//    src->addChild("main.cpp");
//    tree->selectionChanged.connect([](TreeNode* n){ ... });
// ═════════════════════════════════════════════════════════════════════════════

class TreeNode
{
public:
    explicit TreeNode(const std::string& text = "");
    ~TreeNode();

    TreeNode* addChild(const std::string& text);
    void removeChild(int index);
    void clear();

    const std::string& text() const { return text_; }
    void setText(const std::string& t) { text_ = t; }

    bool isExpanded() const { return expanded_; }
    void setExpanded(bool e) { expanded_ = e; }

    TreeNode* parent() const { return parent_; }
    const std::vector<TreeNode*>& children() const { return children_; }
    bool hasChildren() const { return !children_.empty(); }

    // User data
    void setUserData(void* d) { userData_ = d; }
    void* userData() const { return userData_; }

    // Icon from atlas
    void setIcon(IconId id) { iconId_ = id; }
    IconId iconId() const { return iconId_; }

    // Legacy string icon (for backward compat — mapped to IconId internally)
    void setIcon(const std::string& name);
    const std::string& icon() const { return iconStr_; }

private:
    friend class TreeView;
    std::string text_;
    std::string iconStr_;
    IconId iconId_ = IconId::None;
    bool expanded_ = false;
    TreeNode* parent_ = nullptr;
    std::vector<TreeNode*> children_;
    void* userData_ = nullptr;
};

class TreeView : public Widget
{
