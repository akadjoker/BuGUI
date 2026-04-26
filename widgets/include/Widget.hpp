#pragma once

#include "Signal.hpp"
#include "Color.hpp"
#include "IconAtlas.hpp"
#include <string>
#include <vector>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <any>

class RenderBatch;
class Font;
class Menu;

// ═════════════════════════════════════════════════════════════════════════════
//  Rect - axis-aligned rectangle (float)
// ═════════════════════════════════════════════════════════════════════════════

struct Rect
{
    float x = 0, y = 0, w = 0, h = 0;

    float right()  const { return x + w; }
    float bottom() const { return y + h; }

    bool contains(float px, float py) const
    {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    Rect shrunk(float pad) const
    {
        return {x + pad, y + pad, w - pad * 2, h - pad * 2};
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  PaintContext - 3 batches for 3-pass rendering + clip stack
//    fill:  filled shapes (backgrounds, rects, circles)
//    line:  outlines, borders (unfilled shapes, lines)
//    text:  font quads (text rendering via Font)
// ═════════════════════════════════════════════════════════════════════════════

struct PaintContext
{
    RenderBatch& fill;
    RenderBatch& line;
    RenderBatch& text;
    Font&        font;
    IconAtlas*   icons = nullptr;   // optional icon atlas (set by WidgetApp)

    PaintContext(RenderBatch& f, RenderBatch& l, RenderBatch& t, Font& fn,
                 IconAtlas* ico = nullptr);

    // ── Clip stack ────────────────────────────────────────────────────────
    void pushClip(const Rect& r);
    void popClip();

    const Rect& clipRect() const;
    bool hasClip() const;

    bool isClipped(const Rect& r) const;
    bool clipRectIntersect(const Rect& in, Rect& out) const;

    void fontClip(float out[4]) const;

    // ── Clip-aware drawing helpers ────────────────────────────────────────
    // These clip primitives to the current clipRect before submitting.
    // Use these instead of raw fill./line. calls for proper clipping.
    void fillRect(float x, float y, float w, float h);
    void lineRect(float x, float y, float w, float h);
    void fillRoundedRect(float x, float y, float w, float h, float r, int seg = 8);
    void lineRoundedRect(float x, float y, float w, float h, float r, int seg = 8);
    void fillCircle(float cx, float cy, float radius);
    void lineCircle(float cx, float cy, float radius);
    void drawLine(float x1, float y1, float x2, float y2);
    void fillTriangle(float x1, float y1, float x2, float y2, float x3, float y3);

    // ── Icon drawing ──────────────────────────────────────────────────────
    // Draw an icon from the atlas at (x,y) with the given pixel size.
    // Uses the fill batch with the atlas texture.
    void drawIcon(IconId id, float x, float y, float size);
    // Tinted variant
    void drawIcon(IconId id, float x, float y, float size, const Color& tint);

private:
    mutable Rect cachedClip_ = {0, 0, 99999, 99999};
};

// ═════════════════════════════════════════════════════════════════════════════
//  MouseEvent / KeyEvent
// ═════════════════════════════════════════════════════════════════════════════

struct MouseEvent
{
    float x = 0, y = 0;       // absolute screen coords
    float localX = 0, localY = 0; // relative to widget
    int   button = 0;          // 0=left, 1=right, 2=middle
    float scrollX = 0, scrollY = 0;
    bool  consumed = false;
};

struct KeyEvent
{
    int  key      = 0;
    int  scancode = 0;
    bool shift    = false;
    bool ctrl     = false;
    bool alt      = false;
    bool consumed = false;
    char text[32] = {};
};

// ═════════════════════════════════════════════════════════════════════════════
//  Layout enums & structs
// ═════════════════════════════════════════════════════════════════════════════

enum class Align    { Start, Center, End, Fill };
enum class MainAlign { Start, Center, End, SpaceBetween, SpaceEvenly };

struct Edges
{
    float top = 0, right = 0, bottom = 0, left = 0;

    Edges() = default;
    Edges(float all) : top(all), right(all), bottom(all), left(all) {}
    Edges(float tb, float lr) : top(tb), right(lr), bottom(tb), left(lr) {}
    Edges(float t, float r, float b, float l) : top(t), right(r), bottom(b), left(l) {}

    float horizontal() const { return left + right; }
    float vertical()   const { return top + bottom; }
};

// ═════════════════════════════════════════════════════════════════════════════
//  Cursor types (maps to SDL system cursors)
// ═════════════════════════════════════════════════════════════════════════════

enum class CursorType
{
    Arrow,          // default pointer
    Hand,           // clickable (buttons, links)
    IBeam,          // text input
    Crosshair,      // precision select
    SizeH,          // horizontal resize  ↔
    SizeV,          // vertical resize    ↕
    SizeAll,        // move / drag all directions
    No,             // not allowed
};

// ═════════════════════════════════════════════════════════════════════════════
//  Widget - base class for all UI elements (retained mode)
// ═════════════════════════════════════════════════════════════════════════════

class Widget
{
    friend class WidgetApp;   // manages hover state
    friend class UIApp;
public:
    Widget() = default;
    virtual ~Widget();

    // non-copyable, non-movable (tree node)
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    // ── Tree ──────────────────────────────────────────────────────────────
    // Add an existing widget as child (takes ownership). Returns same ptr.
    template <typename T>
    T* addChild(T* child)
    {
        if (!child) return nullptr;
        if (child->parent_ == this) return child;
        if (child->parent_ != nullptr)
        {
            return child;
        }
        child->parent_ = this;
        children_.push_back(child);
        markDirty();
        return child;
    }

    // Convenience: construct in-place
    template <typename T, typename... Args>
    T* createChild(Args&&... args)
    {
        return addChild(new T(std::forward<Args>(args)...));
    }

    void removeChild(Widget* child);
    Widget* parent() const { return parent_; }
    const std::vector<Widget*>& children() const { return children_; }

    // ── Geometry ──────────────────────────────────────────────────────────
    void setPosition(float x, float y) { rect_.x = x; rect_.y = y; markDirty(); }
    void setSize(float w, float h)     { rect_.w = w; rect_.h = h; markDirty(); }
    void setRect(const Rect& r)        { rect_ = r; markDirty(); }
    const Rect& rect() const           { return rect_; }

    // Absolute rect in screen space (accumulated from parent chain)
    Rect absoluteRect() const;

    struct Vec2f { float x = 0, y = 0; };

    // ── Minimum / preferred size ──────────────────────────────────────────
    virtual Vec2f sizeHint() const { return {rect_.w, rect_.h}; }

    // ── Visibility / Enabled ──────────────────────────────────────────────
    void setVisible(bool v) { visible_ = v; }
    bool isVisible() const  { return visible_; }
    void setEnabled(bool e) { enabled_ = e; }
    bool isEnabled() const  { return enabled_; }

    // ── Focus ─────────────────────────────────────────────────────────────
    bool isFocused() const  { return focused_; }
    void setFocused(bool f) { focused_ = f; }
    bool acceptsFocus() const { return acceptsFocus_; }

    // ── Dirty (needs repaint) ─────────────────────────────────────────────
    bool isDirty() const { return dirty_; }
    void markDirty()     { dirty_ = true; }
    void clearDirty()    { dirty_ = false; }

    // ── Layout ────────────────────────────────────────────────────────────
    virtual void layout();

    // ── Scroll offset (overridden by ScrollView) ──────────────────────────
    // Returns how much this widget's children are scrolled (dx, dy).
    virtual Vec2f scrollOffset() const { return {0, 0}; }

    // ── Popup hit test (overridden by Menu for submenu support) ──────────
    // Returns true if (x,y) is inside this popup or any sub-popup it owns.
    virtual bool popupContains(float x, float y) const
    { return absoluteRect().contains(x, y); }

    // Called when popup is closed - override to reset internal state.
    virtual void resetPopupState() {}

    // Whether this widget is affected by parent's scroll offset
    void setScrollable(bool s) { scrollable_ = s; }
    bool isScrollable() const  { return scrollable_; }

    // ── Paint ─────────────────────────────────────────────────────────────
    // Single pass: widget draws into ctx.fill, ctx.line, ctx.text as needed.
    // WidgetApp renders the 3 batches in order: fill → line → text.
    virtual void paint(PaintContext& ctx);

    // ── Events (override in subclasses) ───────────────────────────────────
    virtual void onMousePress(MouseEvent& e);
    virtual void onMouseRelease(MouseEvent& e);
    virtual void onMouseMove(MouseEvent& e);
    virtual void onMouseScroll(MouseEvent& e)  { (void)e; }
    virtual void onMouseEnter();
    virtual void onMouseLeave();
    virtual void onKeyPress(KeyEvent& e)       { (void)e; }
    virtual void onKeyRelease(KeyEvent& e)     { (void)e; }
    virtual void onTextInput(KeyEvent& e)      { (void)e; }

    // ── Signals (connect from user code) ──────────────────────────────────
    Signal<int>        pressed;       // mouse button down (button id)
    Signal<int>        released;      // mouse button up   (button id)
    Signal<>           clicked;       // press + release inside widget (left)
    Signal<>           hoverEnter;    // mouse entered widget area
    Signal<>           hoverLeave;    // mouse left widget area
    Signal<float,float> moved;        // mouse moved (localX, localY)

    // ── Hover state (set by WidgetApp) ────────────────────────────────────
    bool isHovered() const { return hovered_; }

    // ── ID (optional, for lookups) ────────────────────────────────────────
    void setId(const std::string& id) { id_ = id; }
    const std::string& id() const     { return id_; }

    // Find a descendant widget by ID (recursive). Returns nullptr if not found.
    Widget* findById(const std::string& id);

    // Find by ID and cast to a specific type. Returns nullptr on miss or wrong type.
    template <typename T>
    T* findById(const std::string& id)
    {
        return dynamic_cast<T*>(findById(id));
    }

    // ── Tags (for grouping/filtering) ─────────────────────────────────────
    void addTag(const std::string& tag)    { tags_.insert(tag); }
    void removeTag(const std::string& tag) { tags_.erase(tag); }
    bool hasTag(const std::string& tag) const { return tags_.count(tag) > 0; }
    const std::unordered_set<std::string>& tags() const { return tags_; }

    // Find all descendants with a given tag (recursive, appends to result)
    void findByTag(const std::string& tag, std::vector<Widget*>& result);

    // Find all descendants with a tag, cast to type T (skips non-matching types)
    template <typename T>
    std::vector<T*> findByTag(const std::string& tag)
    {
        std::vector<Widget*> all;
        findByTag(tag, all);
        std::vector<T*> typed;
        for (auto* w : all)
            if (auto* t = dynamic_cast<T*>(w))
                typed.push_back(t);
        return typed;
    }

    // ── User data (arbitrary key-value storage) ───────────────────────────
    void setData(const std::string& key, std::any value) { data_[key] = std::move(value); }
    bool hasData(const std::string& key) const { return data_.count(key) > 0; }
    void removeData(const std::string& key)    { data_.erase(key); }

    // ── Tooltip ───────────────────────────────────────────────────────────
    void setTooltip(const std::string& tip) { tooltip_ = tip; }
    const std::string& tooltip() const      { return tooltip_; }
    void setTooltipDelay(float seconds)     { tooltipDelay_ = seconds; }
    float tooltipDelay() const              { return tooltipDelay_; }

    // ── Cursor ────────────────────────────────────────────────────────────
    void setCursor(CursorType c)  { cursor_ = c; }
    CursorType cursor() const     { return cursor_; }

    // ── Context menu (right-click) ────────────────────────────────────────
    void setContextMenu(Menu* m)  { contextMenu_ = m; }
    Menu* contextMenu() const     { return contextMenu_; }

    template <typename T>
    T data(const std::string& key) const
    {
        auto it = data_.find(key);
        if (it != data_.end())
            return std::any_cast<T>(it->second);
        return T{};
    }

    template <typename T>
    T data(const std::string& key, const T& fallback) const
    {
        auto it = data_.find(key);
        if (it != data_.end()) {
            try { return std::any_cast<T>(it->second); }
            catch (...) {}
        }
        return fallback;
    }

    // ── Layout properties (used by parent BoxLayout) ──────────────────────
    void setMargins(const Edges& m) { margins_ = m; markDirty(); }
    void setMargins(float all)      { margins_ = Edges(all); markDirty(); }
    void setMargins(float tb, float lr) { margins_ = Edges(tb, lr); markDirty(); }
    void setMargins(float t, float r, float b, float l) { margins_ = Edges(t,r,b,l); markDirty(); }
    const Edges& margins() const { return margins_; }

    void setLayoutAlign(Align a)    { layoutAlign_ = a; markDirty(); }
    Align layoutAlign() const       { return layoutAlign_; }

    void setStretch(float s)  { stretch_ = s; markDirty(); }
    float stretch() const     { return stretch_; }

protected:
    Rect rect_;
    Widget* parent_ = nullptr;
    std::vector<Widget*> children_;

    bool visible_      = true;
    bool enabled_      = true;
    bool focused_      = false;
    bool hovered_      = false;
    bool pressed_      = false;
    bool acceptsFocus_ = false;
    bool dirty_        = true;
    bool scrollable_   = true;   // affected by parent's scroll offset

    // Layout
    Edges margins_;
    Align layoutAlign_ = Align::Fill;
    float stretch_     = 0.0f;

    std::string id_;
    std::unordered_set<std::string> tags_;
    std::unordered_map<std::string, std::any> data_;
    std::string tooltip_;
    float tooltipDelay_ = 0.6f;  // seconds before tooltip appears
    CursorType cursor_ = CursorType::Arrow;
    Menu* contextMenu_ = nullptr;  // not owned - user manages lifetime
};

// ═════════════════════════════════════════════════════════════════════════════
//  Layouts
// ═════════════════════════════════════════════════════════════════════════════

enum class LayoutDir { Vertical, Horizontal };

class BoxLayout : public Widget
{
public:
    explicit BoxLayout(LayoutDir dir) : dir_(dir) {}

    void setSpacing(float s) { spacing_ = s; }
    float spacing() const    { return spacing_; }
    LayoutDir dir() const    { return dir_; }

    void setPadding(const Edges& e)                          { padding_ = e; }
    void setPadding(float all)                              { padding_ = Edges(all); }
    void setPadding(float tb, float lr)                     { padding_ = Edges(tb, lr); }
    void setPadding(float t, float r, float b, float l)     { padding_ = Edges(t, r, b, l); }
    const Edges& padding() const { return padding_; }

    void setMainAlign(MainAlign a) { mainAlign_ = a; }
    MainAlign mainAlign() const    { return mainAlign_; }

    void layout() override;
    Vec2f sizeHint() const override;

private:
    LayoutDir dir_;
    float spacing_      = 4.0f;
    Edges padding_      = Edges(4.0f);
    MainAlign mainAlign_ = MainAlign::Start;
};

using VBoxLayout = BoxLayout;   // construct with LayoutDir::Vertical
using HBoxLayout = BoxLayout;   // construct with LayoutDir::Horizontal
