#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include <functional>

// ═════════════════════════════════════════════════════════════════════════════
//  ListBox - scrollable list of selectable text items
//    auto* lb = parent->createChild<ListBox>();
//    lb->addItem("Alpha"); lb->addItem("Beta");
//    lb->selectionChanged.connect([](int idx) { ... });
// ═════════════════════════════════════════════════════════════════════════════

class ListBox : public Widget
{
public:
    ListBox() ;

    void addItem(const std::string& text);
    void insertItem(int index, const std::string& text);
    void removeItem(int index);
    void clear();

    int  itemCount() const { return static_cast<int>(items_.size()); }
    const std::string& itemText(int index) const;
    void setItemText(int index, const std::string& text);

    int  selectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(int idx);

    // Row height
    void  setRowHeight(float h) { rowHeight_ = h; markDirty(); }
    float rowHeight() const     { return rowHeight_; }

    Signal<int> selectionChanged;  // emits new selected index
    Signal<int> itemActivated;     // emits on double-click (future)

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;

private:
    std::vector<std::string> items_;
    int   selectedIndex_ = -1;
    float rowHeight_     = 24.0f;
    float scrollOffset_  = 0.0f;   // vertical pixel offset (>=0)

    // Scrollbar
    bool  draggingThumb_ = false;
    float dragOffset_    = 0.0f;
    bool  needsScrollBar() const;
    float totalHeight()   const;
    float maxScroll()     const;
    float thumbLength(float trackH) const;
    float thumbPos(float trackH)    const;
};

// ═════════════════════════════════════════════════════════════════════════════
//  ComboBox - dropdown selector (like HTML <select>)
//    auto* cb = parent->createChild<ComboBox>();
//    cb->addItem("Alpha"); cb->addItem("Beta");
//    cb->selectionChanged.connect([](int idx) { ... });
// ═════════════════════════════════════════════════════════════════════════════

class ComboBox : public Widget
{
public:
    ComboBox();

    void addItem(const std::string& text);
    void insertItem(int index, const std::string& text);
    void removeItem(int index);
    void clear();

    int  itemCount() const { return static_cast<int>(items_.size()); }
    const std::string& itemText(int index) const;
    void setItemText(int index, const std::string& text);

    int  selectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(int idx);

    const std::string& currentText() const;

    // Max visible items in dropdown (rest scrollable)
    void setMaxVisible(int n) { maxVisible_ = n; }
    int  maxVisible() const   { return maxVisible_; }

    Signal<int> selectionChanged;

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;

private:
    std::vector<std::string> items_;
    int   selectedIndex_ = -1;
    int   maxVisible_    = 8;
    bool  open_          = false;

    void openDropdown();
    void closeDropdown();

    friend class ComboPopup_;
};

// ═════════════════════════════════════════════════════════════════════════════
//  ListWidget - scrollable list where each row is an arbitrary widget
//    auto* lw = parent->createChild<ListWidget>();
//    lw->addRow<Label>("Row 1");
//    auto* row = lw->addRow<BoxLayout>(LayoutDir::Horizontal);
//    row->createChild<Label>("Name"); row->createChild<Button>("Edit");
//    lw->selectionChanged.connect([](int idx) { ... });
// ═════════════════════════════════════════════════════════════════════════════

class ListWidget : public Widget
{
public:
    ListWidget();

    // Add a row widget. Returns the created widget.
    template <typename T, typename... Args>
    T* addRow(Args&&... args)
    {
        auto* w = new T(std::forward<Args>(args)...);
        addRowWidget(w);
        return w;
    }

    void addRowWidget(Widget* w);
    void removeRow(int index);
    void clearRows();

    int rowCount() const { return static_cast<int>(children_.size()); }
    Widget* row(int index) const;

    int  selectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(int idx);

    // Row height (uniform)
    void  setRowHeight(float h) { rowHeight_ = h; markDirty(); }
    float rowHeight() const     { return rowHeight_; }

    Signal<int> selectionChanged;

    void layout() override;
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;

private:
    int   selectedIndex_ = -1;
    float rowHeight_     = 28.0f;
    float scrollOffset_  = 0.0f;

    // Scrollbar
    bool  draggingThumb_ = false;
    float dragOffset_    = 0.0f;
    bool  needsScrollBar() const;
    float totalHeight()   const;
    float maxScroll()     const;
    float thumbLength(float trackH) const;
    float thumbPos(float trackH)    const;

    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
};

// ═════════════════════════════════════════════════════════════════════════════
//  ScrollBar - draggable scroll indicator with proportional thumb
//    - Thumb size = (visibleSize / contentSize) * trackLength
//    - Click on track = page scroll
//    - Drag thumb = continuous scroll
// ═════════════════════════════════════════════════════════════════════════════

class ScrollBar : public Widget
{
public:
    explicit ScrollBar(LayoutDir orientation = LayoutDir::Vertical);

    // Content geometry: how much is visible vs total
    void setContentSize(float total)   { contentSize_ = total; clampValue(); markDirty(); }
    void setVisibleSize(float visible) { visibleSize_ = visible; clampValue(); markDirty(); }
    float contentSize()  const { return contentSize_; }
    float visibleSize()  const { return visibleSize_; }

    // Current scroll position (0 .. contentSize - visibleSize)
    void  setValue(float v);
    float value() const { return value_; }

    void setOrientation(LayoutDir dir) { orientation_ = dir; markDirty(); }
    LayoutDir orientation() const      { return orientation_; }

    Signal<float> onValueChanged;

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;

private:
    LayoutDir orientation_;
    float contentSize_  = 100.0f;
    float visibleSize_  = 100.0f;
    float value_        = 0.0f;
    bool  dragging_     = false;
    float dragOffset_   = 0.0f;   // offset from thumb top/left to mouse

    float maxValue() const;
    void  clampValue();

    // Thumb geometry (in local coords)
    float thumbLength() const;
    float thumbPos()    const;
    float trackLength() const;
};

// ═════════════════════════════════════════════════════════════════════════════
//  ScrollView - clipping container with automatic ScrollBars
//    Wrap a single content widget. Scrolls it if larger than the viewport.
// ═════════════════════════════════════════════════════════════════════════════

class ScrollView : public Widget
{
public:
    ScrollView();

    // The content widget (the one that scrolls). Takes ownership.
    template <typename T, typename... Args>
    T* setContent(Args&&... args)
    {
        auto* w = new T(std::forward<Args>(args)...);
        setContentWidget(w);
        return w;
    }

    void setContentWidget(Widget* w);
    Widget* content() const { return content_; }

    // Scroll position
    float scrollX() const { return scrollX_; }
    float scrollY() const { return scrollY_; }
    void  setScrollX(float v);
    void  setScrollY(float v);

    // Policy
    void setScrollBarWidth(float w) { barWidth_ = w; markDirty(); }

    void layout() override;
    void paint(PaintContext& ctx) override;
    void onMouseScroll(MouseEvent& e) override;
    Vec2f scrollOffset() const override { return {scrollX_, scrollY_}; }

private:
    Widget*    content_  = nullptr;
    ScrollBar* hBar_     = nullptr;
    ScrollBar* vBar_     = nullptr;
    float scrollX_ = 0, scrollY_ = 0;
    float barWidth_ = 12.0f;
    float scrollSpeed_ = 30.0f;

    void syncBars();
};


