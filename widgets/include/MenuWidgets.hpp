#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include <functional>

class Menu;

struct MenuAction;

// ═════════════════════════════════════════════════════════════════════════════
//  Menu — popup list of MenuActions (shown via popup overlay)
//    Used by MenuBar and ContextMenu.

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
//  MenuItem — a single entry in a Menu
//    Can be a normal item, separator, or submenu header.
// ═════════════════════════════════════════════════════════════════════════════

struct MenuAction
{
    std::string label;
    std::string shortcut;    // display text, e.g. "Ctrl+S"
    bool        enabled = true;
    bool        separator = false;  // if true, draws a horizontal line
    bool        checkable = false;  // if true, draws a check mark
    bool        checked = false;    // state of check mark
    Menu*       submenu = nullptr;  // if non-null, opens submenu on hover
    std::function<void()> callback;

    // Convenience constructors
    static MenuAction Separator() { MenuAction m; m.separator = true; return m; }
};

