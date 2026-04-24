#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include <functional>

struct Texture;

// ═════════════════════════════════════════════════════════════════════════════
//  GridLayout — uniform grid with N columns, rows auto-calculated
//    Children flow left-to-right, top-to-bottom. Each child = 1 cell.
//    Cell sizes are equal: cellW = (width - padding - gaps) / cols
//    Children can use setStretch() for row-span (like BoxLayout vertical).
// ═════════════════════════════════════════════════════════════════════════════

class GridLayout : public Widget
{
public:
    explicit GridLayout(int cols = 2);

    void setCols(int c) { cols_ = (c > 0) ? c : 1; markDirty(); }
    int  cols() const   { return cols_; }

    void setSpacing(float s)  { spacingH_ = spacingV_ = s; }
    void setSpacing(float h, float v) { spacingH_ = h; spacingV_ = v; }
    float spacingH() const    { return spacingH_; }
    float spacingV() const    { return spacingV_; }

    void setPadding(const Edges& e)                      { padding_ = e; }
    void setPadding(float all)                            { padding_ = Edges(all); }
    void setPadding(float tb, float lr)                   { padding_ = Edges(tb, lr); }
    void setPadding(float t, float r, float b, float l)   { padding_ = Edges(t, r, b, l); }
    const Edges& padding() const { return padding_; }

    int rows() const;  // computed from child count

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    int   cols_     = 2;
    float spacingH_ = 4.0f;
    float spacingV_ = 4.0f;
    Edges padding_  = Edges(4.0f);
};

// ═════════════════════════════════════════════════════════════════════════════
//  BorderLayout — 5-region layout: Top, Bottom, Left, Right, Center
//    Top/Bottom span full width. Left/Right fill remaining height.
//    Center gets whatever space is left.
//    Each region holds at most one widget. Unset regions take 0 space.
//
//  Region sizes:
//    - Positive value (e.g. 200)  → fixed pixels
//    - Negative value (e.g. -0.2) → percentage of total (abs value, 0..1)
//    - Zero (default)             → use widget's sizeHint
//    Center region ignores size — always fills remaining space.
// ═════════════════════════════════════════════════════════════════════════════

enum class BorderRegion { Top, Bottom, Left, Right, Center };

class BorderLayout : public Widget
{
public:
    BorderLayout() = default;

    // Place a widget in a region. Creates the widget and returns it.
    template <typename T, typename... Args>
    T* set(BorderRegion region, Args&&... args)
    {
        auto* w = new T(std::forward<Args>(args)...);
        setWidget(region, w);
        return w;
    }

    // Place an existing widget in a region (takes ownership)
    void setWidget(BorderRegion region, Widget* w);

    // Get widget in a region (nullptr if empty)
    Widget* get(BorderRegion region) const;

    // Set region size: positive = fixed px, negative = percentage (e.g. -0.2 = 20%)
    // Zero = use sizeHint. Center ignores this.
    void setRegionSize(BorderRegion region, float size);
    float regionSize(BorderRegion region) const;

    void setSpacing(float s) { spacing_ = s; markDirty(); }
    float spacing() const    { return spacing_; }

    void setPadding(const Edges& e)                      { padding_ = e; }
    void setPadding(float all)                            { padding_ = Edges(all); }
    void setPadding(float tb, float lr)                   { padding_ = Edges(tb, lr); }
    void setPadding(float t, float r, float b, float l)   { padding_ = Edges(t, r, b, l); }
    const Edges& padding() const { return padding_; }

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    Widget* regions_[5]     = {nullptr, nullptr, nullptr, nullptr, nullptr};
    float   regionSizes_[5] = {0, 0, 0, 0, 0};  // per-region size config
    float   spacing_        = 0.0f;
    Edges   padding_        = Edges(0.0f);

    // Resolve a region size: positive→px, negative→% of totalDim, zero→hint
    float resolveSize(BorderRegion region, float totalDim, float hint) const;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Collapsible — clickable header that shows/hides content
//  Draws a triangle indicator (▶ collapsed / ▼ expanded) via fillBatch.
// ═════════════════════════════════════════════════════════════════════════════

class Collapsible : public Widget
{
public:
    explicit Collapsible(const std::string& title = "", bool expanded = true);

    void setTitle(const std::string& t) { title_ = t; markDirty(); }
    const std::string& title() const    { return title_; }

    void setExpanded(bool e);
    bool isExpanded() const { return expanded_; }

    void setHeaderHeight(float h) { headerH_ = h; markDirty(); }
    float headerHeight() const    { return headerH_; }

    void setHeaderColor(const Color& c) { headerColor_ = c; }
    const Color& headerColor() const    { return headerColor_; }

    void setContentPadding(float p) { contentPad_ = p; markDirty(); }
    float contentPadding() const    { return contentPad_; }

    void setContentSpacing(float s) { contentSpacing_ = s; markDirty(); }
    float contentSpacing() const    { return contentSpacing_; }

    Signal<bool> expandedChanged;

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;

private:
    std::string title_;
    bool  expanded_       = true;
    float headerH_        = 26.0f;
    float contentPad_     = 4.0f;
    float contentSpacing_ = 2.0f;
    Color headerColor_    = Color(0, 0, 0, 0);  // transparent = use theme
};

// ═════════════════════════════════════════════════════════════════════════════
//  StatusBar — horizontal bar (typically at bottom) with auto resize grip
//  Shows resize grip triangle when window is resizable.
//  Children are laid out horizontally (left-aligned).
// ═════════════════════════════════════════════════════════════════════════════

class StatusBar : public Widget
{
public:
    StatusBar() = default;

    void setBgColor(const Color& c)  { bgColor_ = c; }
    const Color& bgColor() const     { return bgColor_; }

    void setSpacing(float s) { spacing_ = s; markDirty(); }
    float spacing() const    { return spacing_; }

    // Set a text shown on the left of the bar
    void setText(const std::string& t) { text_ = t; markDirty(); }
    const std::string& text() const    { return text_; }

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    Color bgColor_ = Color(35, 36, 40, 255);
    float spacing_  = 6.0f;
    std::string text_;
};

// ═════════════════════════════════════════════════════════════════════════════
//  StackLayout — shows one child at a time (like QStackedLayout)
//  All children occupy the full area; only the active one is visible/painted.
// ═════════════════════════════════════════════════════════════════════════════

class StackLayout : public Widget
{
public:
    StackLayout() = default;

    // Set active child by index (0-based). -1 = none visible.
    void setCurrentIndex(int idx);
    int  currentIndex() const { return currentIndex_; }

    // Get current visible widget (nullptr if none)
    Widget* currentWidget() const;

    Signal<int> currentChanged;

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    int currentIndex_ = 0;
};

// ═════════════════════════════════════════════════════════════════════════════
//  FormLayout — two-column label:widget layout (like QFormLayout)
//  Add rows with addRow(label, widget). Labels auto-right-aligned.
// ═════════════════════════════════════════════════════════════════════════════

class FormLayout : public Widget
{
public:
    FormLayout() = default;

    // Add a row: creates a Label + places widget side by side
    template <typename T, typename... Args>
    T* addRow(const std::string& label, Args&&... args)
    {
        createChild<Label>(label);
        return createChild<T>(std::forward<Args>(args)...);
    }

    // Label column width: positive = fixed px, 0 = auto (widest label)
    void setLabelWidth(float w) { labelWidth_ = w; markDirty(); }
    float labelWidth() const    { return labelWidth_; }

    void setSpacing(float h, float v) { spacingH_ = h; spacingV_ = v; markDirty(); }
    void setSpacing(float s)          { spacingH_ = s; spacingV_ = s; markDirty(); }
    float spacingH() const { return spacingH_; }
    float spacingV() const { return spacingV_; }

    void setPadding(const Edges& e)                      { padding_ = e; }
    void setPadding(float all)                            { padding_ = Edges(all); }
    void setPadding(float tb, float lr)                   { padding_ = Edges(tb, lr); }
    void setPadding(float t, float r, float b, float l)   { padding_ = Edges(t, r, b, l); }
    const Edges& padding() const { return padding_; }

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    float labelWidth_ = 0;     // 0 = auto
    float spacingH_   = 8.0f;
    float spacingV_   = 4.0f;
    Edges padding_;
};

// ═════════════════════════════════════════════════════════════════════════════
//  FlowLayout — wraps children like text (CSS flex-wrap)
// ═════════════════════════════════════════════════════════════════════════════

class FlowLayout : public Widget
{
public:
    FlowLayout() = default;

    void setSpacing(float h, float v) { spacingH_ = h; spacingV_ = v; markDirty(); }
    void setSpacing(float s)          { spacingH_ = s; spacingV_ = s; markDirty(); }
    float spacingH() const { return spacingH_; }
    float spacingV() const { return spacingV_; }

    void setPadding(const Edges& e)                      { padding_ = e; }
    void setPadding(float all)                            { padding_ = Edges(all); }
    void setPadding(float tb, float lr)                   { padding_ = Edges(tb, lr); }
    void setPadding(float t, float r, float b, float l)   { padding_ = Edges(t, r, b, l); }
    const Edges& padding() const { return padding_; }

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    float spacingH_ = 4.0f;
    float spacingV_ = 4.0f;
    Edges padding_;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Overlay — children stacked in Z-order, all occupy full area
//  First child = bottom, last = top. For popups, floating panels.
// ═════════════════════════════════════════════════════════════════════════════

class Overlay : public Widget
{
public:
    Overlay() = default;

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Splitter — two children with draggable divider (like QSplitter/GtkPaned)
//  Horizontal: left | right.  Vertical: top | bottom.
// ═════════════════════════════════════════════════════════════════════════════

class Splitter : public Widget
{
public:
    explicit Splitter(LayoutDir dir = LayoutDir::Horizontal);

    LayoutDir dir() const { return dir_; }

    // Split ratio: 0.0..1.0, how much the first child gets (default 0.5)
    void  setRatio(float r);
    float ratio() const { return ratio_; }

    // Clamp ratio to [min, max] range (default 0.0 .. 1.0)
    void  setMinRatio(float r) { minRatio_ = r; setRatio(ratio_); }
    void  setMaxRatio(float r) { maxRatio_ = r; setRatio(ratio_); }
    float minRatio() const     { return minRatio_; }
    float maxRatio() const     { return maxRatio_; }

    // Divider thickness in px
    void  setHandleSize(float s) { handleSize_ = s; markDirty(); }
    float handleSize() const     { return handleSize_; }

    // Min size for each panel (prevents collapsing to zero)
    void  setMinSize(float s) { minSize_ = s; }
    float minSize() const     { return minSize_; }

    Signal<float> ratioChanged;

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseLeave() override;

private:
    LayoutDir dir_;
    float ratio_      = 0.5f;
    float minRatio_   = 0.0f;
    float maxRatio_   = 1.0f;
    float handleSize_ = 5.0f;
    float minSize_    = 30.0f;
    bool  dragging_      = false;
    bool  handleHovered_ = false;
};

// ═════════════════════════════════════════════════════════════════════════════
//  TabLayout — tabbed container (like QTabWidget)
//  Tab bar can be Top, Bottom, Left or Right.
//  Each tab has a label and optional close button (X).
//
//    auto* tabs = parent->createChild<TabLayout>();
//    tabs->addTab("Scene",   scenePanel);
//    tabs->addTab("Assets",  assetsPanel);
//    tabs->setTabsClosable(true);       // shows X on each tab
//    tabs->setTabPosition(TabPosition::Bottom);
// ═════════════════════════════════════════════════════════════════════════════

enum class TabPosition { Top, Bottom, Left, Right };

class TabLayout : public Widget
{
public:
    TabLayout() = default;

    // Add a tab with a label. Creates widget of type T as content. Returns it.
    template <typename T, typename... Args>
    T* addTab(const std::string& label, Args&&... args)
    {
        auto* w = new T(std::forward<Args>(args)...);
        addTabWidget(label, w);
        return w;
    }

    // Add an existing widget as a tab page (takes ownership)
    void addTabWidget(const std::string& label, Widget* content);

    // Remove a tab by index (deletes content widget)
    void removeTab(int index);

    // Tab count
    int tabCount() const { return static_cast<int>(tabs_.size()); }

    // Current tab
    void setCurrentIndex(int idx);
    int  currentIndex() const { return currentIndex_; }
    Widget* currentWidget() const;

    // Tab label
    void setTabLabel(int index, const std::string& label);
    const std::string& tabLabel(int index) const;

    // Tab bar position
    void setTabPosition(TabPosition pos);
    TabPosition tabPosition() const      { return tabPosition_; }

    // Closable tabs (shows X button)
    void setTabsClosable(bool c) { closable_ = c; markDirty(); }
    bool tabsClosable() const    { return closable_; }

    // Tab bar height/width
    void setTabBarSize(float s) { tabBarSize_ = s; markDirty(); }
    float tabBarSize() const    { return tabBarSize_; }

    Signal<int> currentChanged;   // index of new active tab
    Signal<int> tabClosed;        // index of tab that was closed

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;

private:
    struct TabInfo {
        std::string label;
        Widget*     content = nullptr;
    };

    std::vector<TabInfo> tabs_;
    int currentIndex_     = 0;
    TabPosition tabPosition_ = TabPosition::Top;
    bool closable_        = false;
    float tabBarSize_     = 28.0f;

    // Compute tab bar rect and content rect from current geometry
    void computeRects(Rect& barRect, Rect& contentRect) const;

    // Get rect of a specific tab button within the bar
    Rect tabRect(int index, const Rect& barRect) const;

    // Get rect of close button within a tab rect
    Rect closeRect(const Rect& tabR) const;
};

// ═════════════════════════════════════════════════════════════════════════════
//  AnchorLayout — children positioned by anchors relative to parent
//  Each child's position/size is defined by anchor rules.
//  Anchors are 0..1 fractions of parent size + pixel offsets.
//
//    auto* anchor = parent->createChild<AnchorLayout>();
//    auto* btn = anchor->createChild<Button>("OK");
//    AnchorLayout::setAnchors(btn, {
//        .left = 0.5f, .right = 0.5f,      // centered horizontally
//        .bottom = 1.0f, .top = 1.0f,      // anchored to bottom
//        .leftOff = -50, .rightOff = 50,    // 100px wide centered
//        .topOff = -40, .bottomOff = -8     // 32px tall, 8px from bottom
//    });
// ═════════════════════════════════════════════════════════════════════════════

struct AnchorRule
{
    // Anchor fractions (0.0 = parent left/top, 1.0 = parent right/bottom)
    float left   = 0.0f;
    float right  = 1.0f;
    float top    = 0.0f;
    float bottom = 1.0f;

    // Pixel offsets added after anchor fraction
    float leftOff   = 0.0f;
    float rightOff  = 0.0f;
    float topOff    = 0.0f;
    float bottomOff = 0.0f;
};

class AnchorLayout : public Widget
{
public:
    AnchorLayout() = default;

    // Set anchors for a child widget (must be a child of this layout)
    static void setAnchors(Widget* child, const AnchorRule& rule);
    static AnchorRule anchors(const Widget* child);
    static bool hasAnchors(const Widget* child);

    void setPadding(const Edges& e)                      { padding_ = e; }
    void setPadding(float all)                            { padding_ = Edges(all); }
    void setPadding(float tb, float lr)                   { padding_ = Edges(tb, lr); }
    void setPadding(float t, float r, float b, float l)   { padding_ = Edges(t, r, b, l); }
    const Edges& padding() const { return padding_; }

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    Edges padding_;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Carousel — animated page viewer with arrows, dots, auto-play
//
//    auto* car = parent->createChild<Carousel>();
//    car->addPage<Panel>();  // page 0
//    car->addPage<Panel>();  // page 1
//    car->setAutoPlay(true);
//    car->setLoop(true);
// ═════════════════════════════════════════════════════════════════════════════

class Carousel : public Widget
{
public:
    Carousel();

    // Add a page (creates widget of type T as content, returns it)
    template <typename T, typename... Args>
    T* addPage(Args&&... args)
    {
        auto* w = new T(std::forward<Args>(args)...);
        addPageWidget(w);
        return w;
    }

    void addPageWidget(Widget* page);
    void removePage(int index);
    int  pageCount() const { return static_cast<int>(pages_.size()); }

    // Navigation
    void setCurrentPage(int idx);
    int  currentPage() const { return currentPage_; }
    void next();
    void prev();

    // Auto-play (advances automatically)
    void setAutoPlay(bool on)            { autoPlay_ = on; autoTimer_ = 0; }
    bool autoPlay() const                { return autoPlay_; }
    void setAutoPlayInterval(float sec)  { autoInterval_ = sec; }
    float autoPlayInterval() const       { return autoInterval_; }

    // Loop (wrap around at ends)
    void setLoop(bool l)   { loop_ = l; }
    bool loop() const      { return loop_; }

    // Show/hide controls
    void setShowArrows(bool s) { showArrows_ = s; markDirty(); }
    bool showArrows() const    { return showArrows_; }
    void setShowDots(bool s)   { showDots_ = s; markDirty(); }
    bool showDots() const      { return showDots_; }

    // Animation duration (seconds)
    void  setAnimDuration(float sec) { animDuration_ = sec; }
    float animDuration() const       { return animDuration_; }

    Signal<int> pageChanged;

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseLeave() override;

private:
    std::vector<Widget*> pages_;
    int   currentPage_   = 0;

    bool  autoPlay_      = false;
    float autoInterval_  = 3.0f;
    float autoTimer_     = 0.0f;

    bool  loop_          = true;
    bool  showArrows_    = true;
    bool  showDots_      = true;

    float animDuration_  = 0.3f;
    float animProgress_  = 0.0f;   // 0..1
    int   animFrom_      = -1;     // page sliding out
    int   animDir_       = 0;      // -1=slide right, +1=slide left
    bool  animating_     = false;

    int   hoveredArrow_  = 0;      // 0=none, -1=left, +1=right

    float dotsHeight() const { return 24.0f; }
    Rect contentRect() const;
    Rect arrowRect(bool rightSide) const;
    void startAnim(int from, int to, int dir);

    void ensureArrowTexture();
    static unsigned int arrowTexId_;
    static constexpr int kIconSize = 32;
};

