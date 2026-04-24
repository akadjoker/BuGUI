#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include <functional>

// ═════════════════════════════════════════════════════════════════════════════
//  FloatWindow — draggable floating panel with title bar, close & minimize
//    auto* fw = new FloatWindow("Inspector");
//    fw->setContent<BoxLayout>(BoxLayout::Vertical);
//    fw->setFloatPos(100, 100);
//    fw->setFloatSize(300, 200);
//    app.addFloat(fw);
// ═════════════════════════════════════════════════════════════════════════════

class FloatWindow : public Widget
{
public:
    explicit FloatWindow(const std::string& title = "Window");
    ~FloatWindow() override;

    // Title bar
    void setTitle(const std::string& t) { title_ = t; markDirty(); }
    const std::string& title() const { return title_; }

    // Floating position (absolute screen coords)
    void  setFloatPos(float x, float y) { floatX_ = x; floatY_ = y; markDirty(); }
    float floatX() const { return floatX_; }
    float floatY() const { return floatY_; }

    // Floating size
    void  setFloatSize(float w, float h) { floatW_ = w; floatH_ = h; markDirty(); }
    float floatW() const { return floatW_; }
    float floatH() const { return floatH_; }

    // Content (takes ownership, like ScrollView)
    template <typename T, typename... Args>
    T* setContent(Args&&... args)
    {
        auto* w = new T(std::forward<Args>(args)...);
        setContentWidget(w);
        return w;
    }
    void setContentWidget(Widget* w);
    Widget* content() const { return content_; }

    // Features
    void setClosable(bool c)    { closable_ = c; }
    void setMinimizable(bool m) { minimizable_ = m; }
    void setResizable(bool r)   { resizable_ = r; }
    bool isMinimized() const    { return minimized_; }
    void setMinimized(bool m);

    // Title bar height
    void  setTitleBarHeight(float h) { titleBarH_ = h; markDirty(); }
    float titleBarHeight() const { return titleBarH_; }

    // Signals
    Signal<> closed;     // emitted when close button clicked
    Signal<bool> minimizedChanged;

    // Overrides — called by WidgetApp float layer
    void layout() override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    Vec2f sizeHint() const override;

private:
    std::string title_;
    Widget* content_ = nullptr;
    float floatX_ = 50, floatY_ = 50;
    float floatW_ = 300, floatH_ = 200;
    float titleBarH_ = 28.0f;
    bool closable_    = true;
    bool minimizable_ = true;
    bool resizable_   = true;
    bool minimized_   = false;

    // Drag state
    bool  dragging_   = false;
    float dragOffX_   = 0, dragOffY_ = 0;

    // Resize state
    bool  resizing_     = false;
    float resizeOffX_   = 0, resizeOffY_ = 0;
    float resizeStartW_ = 0, resizeStartH_ = 0;
    static constexpr float kResizeGrip = 8.0f;

    // Button hover
    int hoveredBtn_ = 0;  // 0=none, 1=close, 2=minimize

    // Geometry helpers
    Rect titleBarRect() const;
    Rect contentRect() const;
    Rect closeButtonRect() const;
    Rect minimizeButtonRect() const;
    Rect resizeGripRect() const;
};

// ═════════════════════════════════════════════════════════════════════════════
//  SidePanel — slide-in drawer (hamburger menu pattern)
//    auto* sp = parent->createChild<SidePanel>();
//    auto* content = sp->setContent<BoxLayout>(BoxLayout::Vertical);
//    sp->toggle();   // open/close with animation
// ═════════════════════════════════════════════════════════════════════════════

class SidePanel : public Widget
{
public:
    enum Side { Left, Right };

    explicit SidePanel(Side side = Left, float panelWidth = 260.0f);

    // Content inside the drawer (takes ownership)
    template <typename T, typename... Args>
    T* setContent(Args&&... args)
    {
        auto* w = new T(std::forward<Args>(args)...);
        setContentWidget(w);
        return w;
    }
    void setContentWidget(Widget* w);
    Widget* content() const { return content_; }

    // Open / close
    void open();
    void close();
    void toggle();
    bool isOpen() const { return open_; }

    // Settings
    void  setPanelWidth(float w) { panelWidth_ = w; markDirty(); }
    float panelWidth() const { return panelWidth_; }
    void  setAnimDuration(float sec) { animDuration_ = sec; }
    void  setScrimColor(const Color& c) { scrimColor_ = c; }

    Signal<bool> openChanged;

    void layout() override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    Vec2f sizeHint() const override;

    // Called by WidgetApp idle to animate
    void update(float dt);

private:
    Side    side_          = Left;
    float   panelWidth_    = 260.0f;
    bool    open_          = false;

    // Animation
    float   animProgress_  = 0.0f;   // 0 = closed, 1 = fully open
    float   animDuration_  = 0.25f;
    bool    animating_     = false;
    int     animDir_       = 0;      // +1 opening, -1 closing

    Color   scrimColor_    = Color(0, 0, 0, 120);
    Widget* content_       = nullptr;

    float currentOffset() const;  // panel X offset based on progress
};

// ═════════════════════════════════════════════════════════════════════════════
//  PageView — shows one child ("page") at a time, with animated transitions
//    Like Qt's QStackedWidget or Android's ViewPager.
//
//    auto* pv = parent->createChild<PageView>();
//    auto* page0 = pv->addPage<BoxLayout>(LayoutDir::Vertical);  // index 0
//    auto* page1 = pv->addPage<BoxLayout>(LayoutDir::Vertical);  // index 1
//    pv->setPage(1);  // switch to page 1 with animation
//    pv->setPage(0, PageView::Fade);  // explicit transition
// ═════════════════════════════════════════════════════════════════════════════

class PageView : public Widget
{
public:
    enum Transition { None, Fade, SlideLeft, SlideRight, SlideAuto };

    PageView() = default;

    // ── Page management ──────────────────────────────────────────────────
    template <typename T, typename... Args>
    T* addPage(Args&&... args)
    {
        auto* w = new T(std::forward<Args>(args)...);
        addPageImpl(w);
        pageNames_.emplace_back();
        return w;
    }

    void setPageName(int idx, const std::string& name)
    {
        if (idx >= 0 && idx < static_cast<int>(pageNames_.size()))
            pageNames_[idx] = name;
    }

    int  pageCount() const   { return static_cast<int>(pages_.size()); }
    int  currentPage() const { return currentIdx_; }
    Widget* page(int idx) const;

    const std::string& pageName(int idx) const
    {
        static const std::string empty;
        if (idx < 0 || idx >= static_cast<int>(pageNames_.size())) return empty;
        return pageNames_[idx];
    }

    // Switch to page by index, with optional transition override
    void setPage(int idx, Transition tr = SlideAuto);

    // Switch to page by pointer
    void setPage(Widget* page, Transition tr = SlideAuto);

    // Default transition used when SlideAuto picks direction
    void setTransitionDuration(float secs) { transDuration_ = secs; }
    float transitionDuration() const       { return transDuration_; }

    Signal<int> pageChanged;  // emits new page index

    // ── Overrides ────────────────────────────────────────────────────────
    void layout() override;
    void paint(PaintContext& ctx) override;

private:
    void addPageImpl(Widget* w);

    std::vector<Widget*> pages_;
    std::vector<std::string> pageNames_;
    int   currentIdx_    = -1;
    int   prevIdx_       = -1;
    float transProgress_ = 1.0f;  // 1.0 = done
    float transDuration_ = 0.3f;
    Transition transType_ = None;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Dialog — modal dialog shown via the popup layer
//    Draws a scrim + centered card with title, content area, and button row.
//    Shown via WidgetApp::instance().showPopup(dialog).
//
//    auto* dlg = new Dialog("Save Changes?", 360, 180);
//    dlg->setMessage("Do you want to save before closing?");
//    dlg->addButton("Cancel", Dialog::Default);
//    dlg->addButton("Save",   Dialog::Primary);
//    dlg->buttonClicked.connect([](int idx){ ... });
//    WidgetApp::instance().showPopup(dlg);
// ═════════════════════════════════════════════════════════════════════════════

