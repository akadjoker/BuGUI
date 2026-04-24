#include "Widgets.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <algorithm>
#include <cmath>

namespace {
    inline Font::ClipRect toFontClip(const Rect& r)
    { return {r.x, r.y, r.w, r.h}; }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Menu — popup dropdown list
// ═════════════════════════════════════════════════════════════════════════════

Menu::Menu()
{
    acceptsFocus_ = false;
}

Menu::~Menu()
{
    for (auto* sub : ownedSubmenus_)
        delete sub;
    ownedSubmenus_.clear();
}

void Menu::addAction(const std::string& label, std::function<void()> cb,
                     const std::string& shortcut)
{
    MenuAction a;
    a.label = label; a.shortcut = shortcut; a.enabled = true;
    a.callback = std::move(cb);
    actions_.push_back(std::move(a));
    markDirty();
}

void Menu::addAction(const std::string& label, std::function<void()> cb,
                     bool enabled, const std::string& shortcut)
{
    MenuAction a;
    a.label = label; a.shortcut = shortcut; a.enabled = enabled;
    a.callback = std::move(cb);
    actions_.push_back(std::move(a));
    markDirty();
}

void Menu::addCheckable(const std::string& label, bool checked,
                        std::function<void(bool)> cb)
{
    MenuAction a;
    a.label = label; a.enabled = true;
    a.checkable = true; a.checked = checked;
    // Wrap the bool callback so it toggles the checked state
    int idx = static_cast<int>(actions_.size());
    a.callback = [this, idx, cb = std::move(cb)]() {
        if (idx < static_cast<int>(actions_.size())) {
            actions_[idx].checked = !actions_[idx].checked;
            if (cb) cb(actions_[idx].checked);
        }
    };
    actions_.push_back(std::move(a));
    markDirty();
}

Menu* Menu::addSubmenu(const std::string& label)
{
    auto* sub = new Menu();
    ownedSubmenus_.push_back(sub);
    MenuAction a;
    a.label = label; a.enabled = true;
    a.submenu = sub;
    actions_.push_back(std::move(a));
    markDirty();
    return sub;
}

void Menu::addSeparator()
{
    actions_.push_back(MenuAction::Separator());
    markDirty();
}

void Menu::clear()
{
    for (auto* sub : ownedSubmenus_)
        delete sub;
    ownedSubmenus_.clear();
    actions_.clear();
    hovered_ = -1;
    openSub_ = -1;
    markDirty();
}

void Menu::resetState()
{
    closeSubmenu();
    hovered_ = -1;
    // Recursively reset all submenus
    for (auto& a : actions_)
        if (a.submenu) a.submenu->resetState();
    markDirty();
}

bool Menu::hasCheckable() const
{
    for (auto& a : actions_)
        if (a.checkable) return true;
    return false;
}

float Menu::computeWidth() const
{
    const auto& t = Theme::instance();
    auto& app = WidgetApp::instance();
    auto& font = app.font();
    font.SetFontSize(t.fontSize);

    float maxLabel = 0;
    float maxShortcut = 0;
    bool hasSubmenu = false;
    bool hasCheck = hasCheckable();

    for (auto& a : actions_)
    {
        if (a.separator) continue;
        float lw = font.GetTextWidth(a.label.c_str());
        maxLabel = std::max(maxLabel, lw);
        if (!a.shortcut.empty())
        {
            float sw = font.GetTextWidth(a.shortcut.c_str());
            maxShortcut = std::max(maxShortcut, sw);
        }
        if (a.submenu) hasSubmenu = true;
    }

    // checkIndent + padX + label + padX + shortcut/arrow + padX
    float checkIndent = hasCheck ? t.fontSize + 4.0f : 0;
    float w = checkIndent + t.menuItemPadX + maxLabel;
    if (maxShortcut > 0)
        w += t.menuItemPadX + maxShortcut;
    if (hasSubmenu)
        w += t.menuItemPadX + 8.0f;  // arrow space
    w += t.menuItemPadX;

    return std::max(w, t.menuMinWidth);
}

Widget::Vec2f Menu::sizeHint() const
{
    const auto& t = Theme::instance();
    float h = 0;
    for (auto& a : actions_)
        h += a.separator ? (t.padding + 1.0f) : t.menuItemHeight;
    h += t.padding; // top + bottom padding (half each)

    float w = const_cast<Menu*>(this)->computeWidth();
    return {w, h};
}

void Menu::layout()
{
    Vec2f hint = sizeHint();
    setSize(hint.x, hint.y);
}

void Menu::openSubmenu(int index)
{
    if (openSub_ == index) return;
    closeSubmenu();
    if (index < 0 || index >= static_cast<int>(actions_.size())) return;
    auto& a = actions_[index];
    if (!a.submenu) return;

    Rect abs = absoluteRect();
    const auto& t = Theme::instance();

    // Compute Y position of item
    float y = abs.y + t.padding * 0.5f;
    for (int i = 0; i < index; ++i)
        y += actions_[i].separator ? (t.padding + 1.0f) : t.menuItemHeight;

    a.submenu->layout();
    Widget::Vec2f hint = a.submenu->sizeHint();

    // Position to the right of this menu
    float sx = abs.x + abs.w - 2;
    float sy = y;

    // Clamp to window
    auto& app = WidgetApp::instance();
    if (sx + hint.x > app.width())
        sx = abs.x - hint.x + 2;  // open to the left
    if (sy + hint.y > app.height())
        sy = app.height() - hint.y;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;

    a.submenu->setPosition(sx, sy);
    a.submenu->layout();
    openSub_ = index;
}

void Menu::closeSubmenu()
{
    if (openSub_ >= 0)
    {
        openSub_ = -1;
    }
}

void Menu::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    const auto& t = Theme::instance();

    // Shadow
    if (t.menuShadowSize > 0)
    {
        float s = t.menuShadowSize;
        ctx.fill.SetColor(0, 0, 0, 40);
        ctx.fill.Rectangle(
            static_cast<int>(abs.x + s), static_cast<int>(abs.y + s),
            static_cast<int>(abs.w), static_cast<int>(abs.h), true);
    }

    // Background
    ctx.fill.SetColor(t.menuBg.r, t.menuBg.g, t.menuBg.b, t.menuBg.a);
    ctx.fill.Rectangle(
        static_cast<int>(abs.x), static_cast<int>(abs.y),
        static_cast<int>(abs.w), static_cast<int>(abs.h), true);

    // Border
    ctx.line.SetColor(t.menuBorder.r, t.menuBorder.g, t.menuBorder.b, t.menuBorder.a);
    ctx.line.Rectangle(
        static_cast<int>(abs.x), static_cast<int>(abs.y),
        static_cast<int>(abs.w), static_cast<int>(abs.h), false);

    // Items
    float y = abs.y + t.padding * 0.5f;
    bool hasCheck = hasCheckable();
    float checkIndent = hasCheck ? t.fontSize + 4.0f : 0;

    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetBatch(&ctx.text);

    for (int i = 0; i < static_cast<int>(actions_.size()); ++i)
    {
        auto& a = actions_[i];

        if (a.separator)
        {
            float sy = y + t.padding * 0.5f;
            ctx.line.SetColor(t.menuSeparator.r, t.menuSeparator.g, t.menuSeparator.b, t.menuSeparator.a);
            ctx.line.Line2D(
                static_cast<int>(abs.x + t.padding),
                static_cast<int>(sy),
                static_cast<int>(abs.x + abs.w - t.padding),
                static_cast<int>(sy));
            y += t.padding + 1.0f;
            continue;
        }

        Rect itemRect = {abs.x, y, abs.w, t.menuItemHeight};
        bool isHov = (i == hovered_ && a.enabled);

        // Hover highlight
        if (isHov)
        {
            ctx.fill.SetColor(t.menuItemHover.r, t.menuItemHover.g, t.menuItemHover.b, t.menuItemHover.a);
            ctx.fill.Rectangle(
                static_cast<int>(itemRect.x + 2), static_cast<int>(itemRect.y),
                static_cast<int>(itemRect.w - 4), static_cast<int>(itemRect.h), true);
        }

        float textY = itemRect.y + (t.menuItemHeight - t.fontSize) * 0.5f;
        float labelX = itemRect.x + checkIndent + t.menuItemPadX;
        auto fc = toFontClip({abs.x, abs.y, abs.w, abs.h});

        // Check box (same style as CheckBox widget)
        if (a.checkable)
        {
            float boxSz = t.fontSize - 2.0f;
            float bx = itemRect.x + t.menuItemPadX * 0.5f + (t.fontSize - boxSz) * 0.5f;
            float by = itemRect.y + (t.menuItemHeight - boxSz) * 0.5f;

            // Box background
            Color bg = isHov ? t.inputBgHover : t.inputBg;
            ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
            ctx.fill.Rectangle(
                static_cast<int>(bx), static_cast<int>(by),
                static_cast<int>(boxSz), static_cast<int>(boxSz), true);

            // Box border
            Color border = isHov ? t.inputBorderHover : t.inputBorder;
            ctx.line.SetColor(border.r, border.g, border.b, border.a);
            ctx.line.Rectangle(
                static_cast<int>(bx), static_cast<int>(by),
                static_cast<int>(boxSz), static_cast<int>(boxSz), false);

            // Inner fill when checked
            if (a.checked)
            {
                Color ck = isHov ? t.menuItemTextHover : t.menuCheckMark;
                float m = 3.0f;
                ctx.fill.SetColor(ck.r, ck.g, ck.b, ck.a);
                ctx.fill.Rectangle(
                    static_cast<int>(bx + m), static_cast<int>(by + m),
                    static_cast<int>(boxSz - m * 2), static_cast<int>(boxSz - m * 2), true);
            }
        }

        // Label text
        Color textCol = a.enabled ? t.menuItemText : t.menuItemDisabled;
        if (isHov)
            textCol = t.menuItemTextHover;
        ctx.font.SetColor(textCol);
        ctx.font.Print(a.label.c_str(), labelX, textY, &fc);

        // Submenu arrow > (chevron, drawn with lines)
        if (a.submenu)
        {
            Color arrowCol = isHov ? t.menuItemTextHover : t.menuSubmenuArrow;
            float arrowX = itemRect.x + itemRect.w - t.menuItemPadX + 2;
            float arrowCY = itemRect.y + t.menuItemHeight * 0.5f;
            float hs = 4.0f;
            ctx.line.SetColor(arrowCol.r, arrowCol.g, arrowCol.b, arrowCol.a);
            ctx.line.Line2D(arrowX - hs * 0.4f, arrowCY - hs,
                           arrowX + hs * 0.4f, arrowCY);
            ctx.line.Line2D(arrowX + hs * 0.4f, arrowCY,
                           arrowX - hs * 0.4f, arrowCY + hs);
        }
        // Shortcut text
        else if (!a.shortcut.empty())
        {
            Color scCol = a.enabled ? t.menuShortcutText : t.menuItemDisabled;
            if (isHov)
                scCol = t.menuShortcutHover;
            ctx.font.SetColor(scCol);
            float sw = ctx.font.GetTextWidth(a.shortcut.c_str());
            ctx.font.Print(a.shortcut.c_str(),
                          itemRect.x + itemRect.w - sw - t.menuItemPadX,
                          textY, &fc);
        }

        y += t.menuItemHeight;
    }

    // Paint open submenu (drawn in same popup pass)
    if (openSub_ >= 0 && openSub_ < static_cast<int>(actions_.size()))
    {
        auto* sub = actions_[openSub_].submenu;
        if (sub) sub->paint(ctx);
    }
}

void Menu::onMousePress(MouseEvent& e)
{
    if (!visible_) return;

    // Check if click is in open submenu
    if (openSub_ >= 0 && openSub_ < static_cast<int>(actions_.size()))
    {
        auto* sub = actions_[openSub_].submenu;
        if (sub)
        {
            Rect subAbs = sub->absoluteRect();
            if (subAbs.contains(e.x, e.y))
            {
                sub->onMousePress(e);
                return;
            }
        }
    }

    Rect abs = absoluteRect();
    const auto& t = Theme::instance();

    float y = abs.y + t.padding * 0.5f;
    for (int i = 0; i < static_cast<int>(actions_.size()); ++i)
    {
        auto& a = actions_[i];
        if (a.separator) { y += t.padding + 1.0f; continue; }

        Rect itemRect = {abs.x, y, abs.w, t.menuItemHeight};
        if (itemRect.contains(e.x, e.y) && a.enabled)
        {
            if (a.submenu)
            {
                // Click on submenu item — just open/keep the submenu
                openSubmenu(i);
                e.consumed = true;
                return;
            }
            if (a.callback) a.callback();
            // Checkable items stay open so user can toggle several
            if (!a.checkable)
                WidgetApp::instance().closePopup();
            else
                markDirty();  // redraw to show toggled state
            e.consumed = true;
            return;
        }
        y += t.menuItemHeight;
    }
    e.consumed = true;
}

void Menu::onMouseMove(MouseEvent& e)
{
    if (!visible_) return;

    // Check if mouse is in open submenu
    if (openSub_ >= 0 && openSub_ < static_cast<int>(actions_.size()))
    {
        auto* sub = actions_[openSub_].submenu;
        if (sub)
        {
            Rect subAbs = sub->absoluteRect();
            if (subAbs.contains(e.x, e.y))
            {
                sub->onMouseMove(e);
                return;
            }
        }
    }

    Rect abs = absoluteRect();
    const auto& t = Theme::instance();

    int oldHovered = hovered_;
    hovered_ = -1;

    float y = abs.y + t.padding * 0.5f;
    for (int i = 0; i < static_cast<int>(actions_.size()); ++i)
    {
        auto& a = actions_[i];
        if (a.separator) { y += t.padding + 1.0f; continue; }

        Rect itemRect = {abs.x, y, abs.w, t.menuItemHeight};
        if (itemRect.contains(e.x, e.y))
        {
            hovered_ = i;

            // Open submenu on hover
            if (a.submenu && a.enabled)
                openSubmenu(i);
            else if (openSub_ >= 0 && openSub_ != i)
                closeSubmenu();

            break;
        }
        y += t.menuItemHeight;
    }

    if (hovered_ != oldHovered)
        markDirty();
}

void Menu::onMouseLeave()
{
    // Don't close submenu if mouse moved into it
    if (openSub_ >= 0 && openSub_ < static_cast<int>(actions_.size()))
    {
        auto* sub = actions_[openSub_].submenu;
        if (sub)
        {
            auto& app = WidgetApp::instance();
            float mx = app.mouseX();
            float my = app.mouseY();
            if (sub->popupContains(mx, my))
                return;  // mouse is in submenu, keep it open
        }
    }
    closeSubmenu();
    hovered_ = -1;
    markDirty();
}

bool Menu::popupContains(float x, float y) const
{
    if (absoluteRect().contains(x, y)) return true;
    if (openSub_ >= 0 && openSub_ < static_cast<int>(actions_.size()))
    {
        auto* sub = actions_[openSub_].submenu;
        if (sub && sub->popupContains(x, y)) return true;
    }
    return false;
}


// ═════════════════════════════════════════════════════════════════════════════
//  MenuBar — horizontal bar of menu titles
// ═════════════════════════════════════════════════════════════════════════════

MenuBar::MenuBar()
{
    const auto& t = Theme::instance();
    setSize(0, t.menuBarHeight);
}

MenuBar::~MenuBar()
{
    for (auto& e : entries_)
        delete e.menu;
    entries_.clear();
}

float MenuBar::computeItemWidth(const std::string& title) const
{
    const auto& t = Theme::instance();
    auto& app = WidgetApp::instance();
    auto& font = app.font();
    font.SetFontSize(t.fontSize);
    float tw = font.GetTextWidth(title.c_str());
    return tw + t.menuItemPadX * 2;
}

Menu* MenuBar::addMenu(const std::string& title)
{
    auto* menu = new Menu();
    entries_.push_back({title, menu, 0, 0});
    markDirty();
    return menu;
}

void MenuBar::layout()
{
    const auto& t = Theme::instance();
    Rect abs = absoluteRect();

    float x = abs.x;
    for (auto& e : entries_)
    {
        e.w = computeItemWidth(e.title);
        e.x = x;
        x += e.w;
    }

    setSize(abs.w, t.menuBarHeight);
}

Widget::Vec2f MenuBar::sizeHint() const
{
    const auto& t = Theme::instance();
    float w = 0;
    for (auto& e : entries_)
        w += const_cast<MenuBar*>(this)->computeItemWidth(e.title);
    return {w, t.menuBarHeight};
}

void MenuBar::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    // Bar background
    ctx.fill.SetColor(t.menuBarBg.r, t.menuBarBg.g, t.menuBarBg.b, t.menuBarBg.a);
    ctx.fill.Rectangle(
        static_cast<int>(abs.x), static_cast<int>(abs.y),
        static_cast<int>(abs.w), static_cast<int>(abs.h), true);

    // Border bottom
    ctx.line.SetColor(t.menuBorder.r, t.menuBorder.g, t.menuBorder.b, t.menuBorder.a);
    ctx.line.Line2D(
        static_cast<int>(abs.x), static_cast<int>(abs.y + abs.h - 1),
        static_cast<int>(abs.x + abs.w), static_cast<int>(abs.y + abs.h - 1));

    // Menu titles
    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetBatch(&ctx.text);
    auto fc = toFontClip(ctx.clipRect());

    for (int i = 0; i < static_cast<int>(entries_.size()); ++i)
    {
        auto& e = entries_[i];
        Rect itemRect = {e.x, abs.y, e.w, abs.h};

        // Hover/active highlight
        if (i == activeMenu_ || i == hoveredItem_)
        {
            Color bg = i == activeMenu_ ? t.menuItemHover : t.menuBarItemHover;
            ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
            ctx.fill.Rectangle(
                static_cast<int>(itemRect.x), static_cast<int>(itemRect.y),
                static_cast<int>(itemRect.w), static_cast<int>(itemRect.h), true);
        }

        // Title text
        Color textCol = t.menuItemText;
        if (i == activeMenu_)
            textCol = t.menuItemTextHover;
        ctx.font.SetColor(textCol);
        float tw = ctx.font.GetTextWidth(e.title.c_str());
        float tx = e.x + (e.w - tw) * 0.5f;
        float ty = abs.y + (abs.h - t.fontSize) * 0.5f;
        ctx.font.Print(e.title.c_str(), tx, ty, &fc);
    }
}

void MenuBar::openMenu(int index)
{
    if (index < 0 || index >= static_cast<int>(entries_.size())) return;

    auto& e = entries_[index];
    if (!e.menu || e.menu->actions().empty()) return;

    Rect abs = absoluteRect();
    const auto& t = Theme::instance();

    // Position menu below the bar item
    e.menu->setPosition(e.x, abs.y + t.menuBarHeight);
    e.menu->layout();

    // Clamp to window bounds
    auto& app = WidgetApp::instance();
    Widget::Vec2f hint = e.menu->sizeHint();
    float menuRight = e.x + hint.x;
    float menuBottom = abs.y + t.menuBarHeight + hint.y;

    float mx = e.x;
    if (menuRight > app.width())
        mx = app.width() - hint.x;
    if (mx < 0) mx = 0;

    float my = abs.y + t.menuBarHeight;
    if (menuBottom > app.height())
        my = abs.y - hint.y;  // open above if no space below

    e.menu->setPosition(mx, my);
    e.menu->layout();

    activeMenu_ = index;
    armed_ = true;
    app.showPopup(e.menu, this, false);  // not owned — MenuBar manages lifetime
}

void MenuBar::closeMenu()
{
    if (activeMenu_ >= 0)
    {
        entries_[activeMenu_].menu->resetState();
        WidgetApp::instance().closePopup();
        activeMenu_ = -1;
        armed_ = false;
    }
}

void MenuBar::onMousePress(MouseEvent& e)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y)) return;

    for (int i = 0; i < static_cast<int>(entries_.size()); ++i)
    {
        Rect itemRect = {entries_[i].x, abs.y, entries_[i].w, abs.h};
        if (itemRect.contains(e.x, e.y))
        {
            if (activeMenu_ == i)
            {
                closeMenu();
            }
            else
            {
                closeMenu();
                openMenu(i);
            }
            e.consumed = true;
            return;
        }
    }
}

void MenuBar::onMouseMove(MouseEvent& e)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y))
    {
        if (hoveredItem_ >= 0) { hoveredItem_ = -1; markDirty(); }
        return;
    }

    int oldHovered = hoveredItem_;
    hoveredItem_ = -1;

    for (int i = 0; i < static_cast<int>(entries_.size()); ++i)
    {
        Rect itemRect = {entries_[i].x, abs.y, entries_[i].w, abs.h};
        if (itemRect.contains(e.x, e.y))
        {
            hoveredItem_ = i;

            // If a menu is already open, hovering a different title opens it
            if (armed_ && activeMenu_ >= 0 && activeMenu_ != i)
            {
                closeMenu();
                openMenu(i);
            }
            break;
        }
    }

    if (hoveredItem_ != oldHovered)
        markDirty();
}


// ═════════════════════════════════════════════════════════════════════════════
//  ContextMenu — show a Menu at an arbitrary position
// ═════════════════════════════════════════════════════════════════════════════

void ContextMenu::show(Menu* menu, float x, float y, Widget* owner)
{
    if (!menu) return;

    menu->resetState();
    auto& app = WidgetApp::instance();
    menu->layout();

    Widget::Vec2f hint = menu->sizeHint();

    // Clamp to window
    if (x + hint.x > app.width())
        x = app.width() - hint.x;
    if (y + hint.y > app.height())
        y = app.height() - hint.y;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    menu->setPosition(x, y);
    menu->layout();

    app.showPopup(menu, nullptr, false);  // no owner — left-click anywhere outside closes
}
