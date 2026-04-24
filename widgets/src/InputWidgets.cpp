#include "Widgets.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include "Utf8.hpp"


#include <SDL2/SDL.h>
#include <algorithm>
#include <cstring>
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
//  Slider
// ═════════════════════════════════════════════════════════════════════════════

Slider::Slider(float minVal, float maxVal, float value)
    : min_(minVal), max_(maxVal), value_(value) 
{
    acceptsFocus_ = true;
    cursor_ = CursorType::Hand;
}

void Slider::setRange(float minVal, float maxVal) { min_ = minVal; max_ = maxVal; }

void Slider::setValue(float v)
{
    v = Clamp(v, min_, max_);
    if (v != value_)
    {
        value_ = v;
        onValueChanged.emit(value_);
        WidgetApp::instance().fireEvent("change", this);
        markDirty();
    }
}

Widget::Vec2f Slider::sizeHint() const
{
    const auto& t = Theme::instance();
    if (orientation_ == LayoutDir::Vertical)
        return {t.sliderHeight, 120.0f};
    return {120.0f, t.sliderHeight};
}

void Slider::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    float ratio = (max_ > min_) ? (value_ - min_) / (max_ - min_) : 0.0f;
    bool vert = (orientation_ == LayoutDir::Vertical);

    if (vert)
    {
        // ── Vertical slider ──────────────────────────────────────────────
        float trackW = 4.0f;
        float trackX = abs.x + (abs.w - trackW) * 0.5f;

        // Track
        Rect trackRect = {trackX, abs.y, trackW, abs.h};
        Rect clippedTrack;
        if (ctx.clipRectIntersect(trackRect, clippedTrack))
        {
            Color trackC = focused_ ? t.sliderTrackFocused : (hovered_ ? t.sliderTrackHover : t.sliderTrack);
            ctx.fill.SetColor(trackC.r, trackC.g, trackC.b, trackC.a);
            ctx.fill.RoundedRectangle(static_cast<int>(clippedTrack.x), static_cast<int>(clippedTrack.y),
                                       static_cast<int>(clippedTrack.w), static_cast<int>(clippedTrack.h),
                                       2.0f, 4, true);
        }

        // Fill (from bottom up)
        float fillH = abs.h * ratio;
        float fillY = abs.y + abs.h - fillH;
        Rect fillRect = {trackX, fillY, trackW, fillH};
        Rect clippedFill;
        if (ctx.clipRectIntersect(fillRect, clippedFill))
        {
            Color fillC = hovered_ ? t.sliderFillHover : t.sliderFill;
            ctx.fill.SetColor(fillC.r, fillC.g, fillC.b, fillC.a);
            ctx.fill.RoundedRectangle(static_cast<int>(clippedFill.x), static_cast<int>(clippedFill.y),
                                       static_cast<int>(clippedFill.w), static_cast<int>(clippedFill.h),
                                       2.0f, 4, true);
        }

        // Thumb
        float thumbR = std::min(abs.w, 24.0f) * 0.4f;
        float thumbX = abs.x + abs.w * 0.5f;
        float thumbY = fillY;
        Rect thumbBounds = {thumbX - thumbR, thumbY - thumbR, thumbR * 2, thumbR * 2};
        if (!ctx.isClipped(thumbBounds))
        {
            Color thumbC = pressed_ ? t.sliderThumbPressed : (hovered_ ? t.sliderThumbHover : t.sliderThumb);
            ctx.fill.SetColor(thumbC.r, thumbC.g, thumbC.b, thumbC.a);
            ctx.fillCircle(thumbX, thumbY, thumbR);
        }
    }
    else
    {
        // ── Horizontal slider ────────────────────────────────────────────
        float trackH = 4.0f;
        float trackY = abs.y + (abs.h - trackH) * 0.5f;

        Rect trackRect = {abs.x, trackY, abs.w, trackH};
        Rect clippedTrack;
        if (ctx.clipRectIntersect(trackRect, clippedTrack))
        {
            Color trackC = focused_ ? t.sliderTrackFocused : (hovered_ ? t.sliderTrackHover : t.sliderTrack);
            ctx.fill.SetColor(trackC.r, trackC.g, trackC.b, trackC.a);
            ctx.fill.RoundedRectangle(static_cast<int>(clippedTrack.x), static_cast<int>(clippedTrack.y),
                                       static_cast<int>(clippedTrack.w), static_cast<int>(clippedTrack.h),
                                       2.0f, 4, true);
        }

        float fillW = abs.w * ratio;
        Rect fillRect = {abs.x, trackY, fillW, trackH};
        Rect clippedFill;
        if (ctx.clipRectIntersect(fillRect, clippedFill))
        {
            Color fillC = hovered_ ? t.sliderFillHover : t.sliderFill;
            ctx.fill.SetColor(fillC.r, fillC.g, fillC.b, fillC.a);
            ctx.fill.RoundedRectangle(static_cast<int>(clippedFill.x), static_cast<int>(clippedFill.y),
                                       static_cast<int>(clippedFill.w), static_cast<int>(clippedFill.h),
                                       2.0f, 4, true);
        }

        float thumbR = abs.h * 0.4f;
        float thumbX = abs.x + fillW;
        float thumbY = abs.y + abs.h * 0.5f;
        Rect thumbBounds = {thumbX - thumbR, thumbY - thumbR, thumbR * 2, thumbR * 2};
        if (!ctx.isClipped(thumbBounds))
        {
            Color thumbC = pressed_ ? t.sliderThumbPressed : (hovered_ ? t.sliderThumbHover : t.sliderThumb);
            ctx.fill.SetColor(thumbC.r, thumbC.g, thumbC.b, thumbC.a);
            ctx.fillCircle(thumbX, thumbY, thumbR);
        }
    }

    // Focus ring
    if (focused_)
    {
        Rect ringRect = {abs.x - 1, abs.y - 1, abs.w + 2, abs.h + 2};
        Rect clippedRing;
        if (ctx.clipRectIntersect(ringRect, clippedRing))
        {
            ctx.line.SetColor(t.focusColor.r, t.focusColor.g, t.focusColor.b, 80);
            ctx.line.RoundedRectangle(static_cast<int>(clippedRing.x), static_cast<int>(clippedRing.y),
                                       static_cast<int>(clippedRing.w), static_cast<int>(clippedRing.h),
                                       3.0f, 6, false);
        }
    }

    Widget::paint(ctx);
}

void Slider::onMousePress(MouseEvent& e)
{
    if (!enabled_) return;
    dragging_ = true;
    updateFromMouse(e.localX, e.localY);
    e.consumed = true;
    Widget::onMousePress(e);
}

void Slider::onMouseRelease(MouseEvent& e)
{
    dragging_ = false;
    e.consumed = true;
    Widget::onMouseRelease(e);
}

void Slider::onMouseMove(MouseEvent& e)
{
    if (dragging_)
    {
        updateFromMouse(e.localX, e.localY);
        e.consumed = true;
    }
}

void Slider::updateFromMouse(float localX, float localY)
{
    float ratio;
    if (orientation_ == LayoutDir::Vertical)
    {
        if (rect_.h <= 0.0f) return;
        ratio = 1.0f - Clamp(localY / rect_.h, 0.0f, 1.0f);  // bottom=0, top=1
    }
    else
    {
        if (rect_.w <= 0.0f) return;
        ratio = Clamp(localX / rect_.w, 0.0f, 1.0f);
    }
    setValue(min_ + ratio * (max_ - min_));
}

// ═════════════════════════════════════════════════════════════════════════════
//  ProgressBar
// ═════════════════════════════════════════════════════════════════════════════

ProgressBar::ProgressBar(float minVal, float maxVal, float value)
    : min_(minVal), max_(maxVal), value_(value) {}

void ProgressBar::setValue(float v)
{
    v = Clamp(v, min_, max_);
    if (v != value_)
    {
        value_ = v;
        onValueChanged.emit(value_);
        WidgetApp::instance().fireEvent("change", this);
        markDirty();
    }
}

void ProgressBar::setRange(float minVal, float maxVal)
{
    min_ = minVal;
    max_ = maxVal;
    markDirty();
}

Widget::Vec2f ProgressBar::sizeHint() const
{
    if (orientation_ == LayoutDir::Vertical)
        return {24.0f, 120.0f};
    return {120.0f, 20.0f};
}

void ProgressBar::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    float ratio = (max_ > min_) ? (value_ - min_) / (max_ - min_) : 0.0f;
    bool vert = (orientation_ == LayoutDir::Vertical);

    // Track background
    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.fill.SetColor(t.sliderTrack.r, t.sliderTrack.g, t.sliderTrack.b, t.sliderTrack.a);
        ctx.fill.RoundedRectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                                   static_cast<int>(clipped.w), static_cast<int>(clipped.h),
                                   3.0f, 6, true);
    }

    // Fill bar
    if (vert)
    {
        float fillH = abs.h * ratio;
        float fillY = abs.y + abs.h - fillH;
        Rect fillRect = {abs.x, fillY, abs.w, fillH};
        Rect clippedFill;
        if (ctx.clipRectIntersect(fillRect, clippedFill))
        {
            ctx.fill.SetColor(barColor_.r, barColor_.g, barColor_.b, barColor_.a);
            ctx.fill.RoundedRectangle(static_cast<int>(clippedFill.x), static_cast<int>(clippedFill.y),
                                       static_cast<int>(clippedFill.w), static_cast<int>(clippedFill.h),
                                       3.0f, 6, true);
        }
    }
    else
    {
        float fillW = abs.w * ratio;
        Rect fillRect = {abs.x, abs.y, fillW, abs.h};
        Rect clippedFill;
        if (ctx.clipRectIntersect(fillRect, clippedFill))
        {
            ctx.fill.SetColor(barColor_.r, barColor_.g, barColor_.b, barColor_.a);
            ctx.fill.RoundedRectangle(static_cast<int>(clippedFill.x), static_cast<int>(clippedFill.y),
                                       static_cast<int>(clippedFill.w), static_cast<int>(clippedFill.h),
                                       3.0f, 6, true);
        }
    }

    // Border
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
        ctx.line.RoundedRectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                                   static_cast<int>(clipped.w), static_cast<int>(clipped.h),
                                   3.0f, 6, false);
    }

    // Status text (horizontal only)
    if (!text_.empty() && !vert)
    {
        ctx.font.SetFontSize(t.fontSize * 0.8f);
        ctx.font.SetColor(textColor_);
        ctx.font.SetBatch(&ctx.text);

        float textW = ctx.font.GetTextWidth(text_.c_str());
        float textH = ctx.font.GetFontSize();
        float tx, ty = abs.y + (abs.h - textH) * 0.5f;

        switch (textAlign_)
        {
        case TextAlign::LEFT:
            tx = abs.x + 4.0f;
            break;
        case TextAlign::RIGHT:
            tx = abs.x + abs.w - textW - 4.0f;
            break;
        case TextAlign::CENTER:
        default:
            tx = abs.x + (abs.w - textW) * 0.5f;
            break;
        }

        float fc[4];
        ctx.fontClip(fc);
        Font::ClipRect clip{fc[0], fc[1], fc[2], fc[3]};
        ctx.font.Print(text_.c_str(), tx, ty, &clip);
    }

    Widget::paint(ctx);
}

// ═════════════════════════════════════════════════════════════════════════════
//  ListBox — scrollable list of selectable items
// ═════════════════════════════════════════════════════════════════════════════

static const std::string kEmptyItem;

ListBox::ListBox()
{
    acceptsFocus_ = true;
    scrollable_   = true;
    cursor_       = CursorType::Hand;
}

// ── Scrollbar helpers ────────────────────────────────────────────────────
bool  ListBox::needsScrollBar() const { return totalHeight() > rect_.h; }
float ListBox::totalHeight()    const { return items_.size() * rowHeight_; }
float ListBox::maxScroll()      const { float m = totalHeight() - rect_.h; return m > 0 ? m : 0; }

float ListBox::thumbLength(float trackH) const
{
    float total = totalHeight();
    if (total <= 0) return trackH;
    float len = (rect_.h / total) * trackH;
    return len < 16.0f ? 16.0f : len;
}

float ListBox::thumbPos(float trackH) const
{
    float mx = maxScroll();
    if (mx <= 0) return 0;
    float tLen = thumbLength(trackH);
    return (scrollOffset_ / mx) * (trackH - tLen);
}

void ListBox::addItem(const std::string& text)
{
    items_.push_back(text);
    markDirty();
}

void ListBox::insertItem(int index, const std::string& text)
{
    if (index < 0) index = 0;
    if (index > static_cast<int>(items_.size())) index = static_cast<int>(items_.size());
    items_.insert(items_.begin() + index, text);
    if (selectedIndex_ >= index) ++selectedIndex_;
    markDirty();
}

void ListBox::removeItem(int index)
{
    if (index < 0 || index >= static_cast<int>(items_.size())) return;
    items_.erase(items_.begin() + index);
    if (selectedIndex_ == index)
    {
        selectedIndex_ = -1;
        selectionChanged.emit(-1);
    }
    else if (selectedIndex_ > index)
        --selectedIndex_;
    markDirty();
}

void ListBox::clear()
{
    items_.clear();
    selectedIndex_ = -1;
    scrollOffset_  = 0;
    markDirty();
}

const std::string& ListBox::itemText(int index) const
{
    if (index >= 0 && index < static_cast<int>(items_.size()))
        return items_[index];
    return kEmptyItem;
}

void ListBox::setItemText(int index, const std::string& text)
{
    if (index >= 0 && index < static_cast<int>(items_.size()))
    {
        items_[index] = text;
        markDirty();
    }
}

void ListBox::setSelectedIndex(int idx)
{
    if (idx < -1) idx = -1;
    if (idx >= static_cast<int>(items_.size())) idx = static_cast<int>(items_.size()) - 1;
    if (selectedIndex_ != idx)
    {
        selectedIndex_ = idx;
        selectionChanged.emit(idx);
        markDirty();
    }
}

Widget::Vec2f ListBox::sizeHint() const
{
    const auto& t = Theme::instance();
    float w = 120.0f;
    for (auto& item : items_)
    {
        float tw = item.size() * t.fontSize * 0.6f + t.padding * 2;
        if (tw > w) w = tw;
    }
    float h = std::min(static_cast<int>(items_.size()), 8) * rowHeight_ + 2;
    return {w, h};
}

void ListBox::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    int n = static_cast<int>(items_.size());
    bool sb = needsScrollBar();
    float contentW = sb ? abs.w - t.scrollbarWidth : abs.w;

    // Background
    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, t.inputBg.a);
        ctx.fill.Rectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                           static_cast<int>(clipped.w), static_cast<int>(clipped.h), true);

        // Border (clipped to viewport)
        Color brd = focused_ ? t.focusColor : (hovered_ ? t.inputBorderHover : t.inputBorder);
        ctx.line.SetColor(brd.r, brd.g, brd.b, brd.a);
        ctx.line.Rectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                           static_cast<int>(clipped.w), static_cast<int>(clipped.h), false);
    }

    // Clip items (leave space for scrollbar)
    Rect itemClip = {abs.x, abs.y, contentW, abs.h};
    ctx.pushClip(itemClip);

    int firstVisible = static_cast<int>(scrollOffset_ / rowHeight_);
    int lastVisible  = static_cast<int>((scrollOffset_ + abs.h) / rowHeight_) + 1;
    if (firstVisible < 0) firstVisible = 0;
    if (lastVisible > n)  lastVisible  = n;

    for (int i = firstVisible; i < lastVisible; ++i)
    {
        float ry = abs.y + i * rowHeight_ - scrollOffset_;
        Rect rowR = {abs.x, ry, contentW, rowHeight_};

        if (ctx.isClipped(rowR)) continue;

        // Selection highlight
        if (i == selectedIndex_)
        {
            Rect selClip;
            if (ctx.clipRectIntersect(rowR, selClip))
            {
                ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g,
                                  t.selectionColor.b, t.selectionColor.a);
                ctx.fill.Rectangle(static_cast<int>(selClip.x), static_cast<int>(selClip.y),
                                   static_cast<int>(selClip.w), static_cast<int>(selClip.h), true);
            }
        }

        // Item text
        ctx.font.SetFontSize(t.fontSize * 0.9f);
        ctx.font.SetColor(i == selectedIndex_ ? t.textColor : t.textDisabled);
        ctx.font.SetBatch(&ctx.text);
        float tx = abs.x + t.padding;
        float ty = ry + (rowHeight_ - t.fontSize * 0.9f) * 0.5f;

        float fc[4];
        ctx.fontClip(fc);
        Font::ClipRect clip{fc[0], fc[1], fc[2], fc[3]};
        ctx.font.Print(items_[i].c_str(), tx, ty, &clip);
    }

    ctx.popClip();

    // ── Scrollbar ────────────────────────────────────────────────────────
    if (sb)
    {
        float sbX = abs.x + contentW;
        Rect trackRect = {sbX, abs.y, t.scrollbarWidth, abs.h};

        // Track
        Rect trackClip;
        if (ctx.clipRectIntersect(trackRect, trackClip))
        {
            ctx.fill.SetColor(t.sliderTrack.r, t.sliderTrack.g, t.sliderTrack.b, t.sliderTrack.a);
            ctx.fill.RoundedRectangle(static_cast<int>(trackClip.x), static_cast<int>(trackClip.y),
                                       static_cast<int>(trackClip.w), static_cast<int>(trackClip.h),
                                       3.0f, 4, true);
        }

        // Thumb
        float tLen = thumbLength(abs.h);
        float tPos = thumbPos(abs.h);
        Rect thumbRect = {sbX + 1, abs.y + tPos, t.scrollbarWidth - 2, tLen};

        Rect thumbClip;
        if (ctx.clipRectIntersect(thumbRect, thumbClip))
        {
            Color thumbC = draggingThumb_ ? t.sliderThumbPressed : (hovered_ ? t.sliderThumbHover : t.sliderThumb);
            ctx.fill.SetColor(thumbC.r, thumbC.g, thumbC.b, thumbC.a);
            ctx.fill.RoundedRectangle(static_cast<int>(thumbClip.x), static_cast<int>(thumbClip.y),
                                       static_cast<int>(thumbClip.w), static_cast<int>(thumbClip.h),
                                       3.0f, 4, true);
        }
    }
}

void ListBox::onMousePress(MouseEvent& e)
{
    if (!enabled_) return;

    const auto& t = Theme::instance();
    Rect abs = absoluteRect();

    // Click on scrollbar?
    if (needsScrollBar())
    {
        float sbX = abs.x + abs.w - t.scrollbarWidth;
        if (e.x >= sbX)
        {
            float localY = e.y - abs.y;
            float tPos = thumbPos(abs.h);
            float tLen = thumbLength(abs.h);

            if (localY >= tPos && localY <= tPos + tLen)
            {
                draggingThumb_ = true;
                dragOffset_ = localY - tPos;
            }
            else
            {
                // Page jump
                float pageSize = rect_.h * 0.8f;
                if (localY < tPos)
                    scrollOffset_ -= pageSize;
                else
                    scrollOffset_ += pageSize;
                float mx = maxScroll();
                if (scrollOffset_ < 0) scrollOffset_ = 0;
                if (scrollOffset_ > mx) scrollOffset_ = mx;
                markDirty();
            }
            e.consumed = true;
            return;
        }
    }

    float localY = e.y - abs.y + scrollOffset_;
    int idx = static_cast<int>(localY / rowHeight_);

    if (idx >= 0 && idx < static_cast<int>(items_.size()))
        setSelectedIndex(idx);

    e.consumed = true;
}

void ListBox::onMouseScroll(MouseEvent& e)
{
    float totalH = items_.size() * rowHeight_;
    float viewH  = rect_.h;
    if (totalH <= viewH) { scrollOffset_ = 0; return; }

    scrollOffset_ -= e.scrollY * rowHeight_ * 0.5f;
    if (scrollOffset_ < 0) scrollOffset_ = 0;
    if (scrollOffset_ > totalH - viewH) scrollOffset_ = totalH - viewH;

    e.consumed = true;
    markDirty();
}

void ListBox::onMouseRelease(MouseEvent& e)
{
    if (draggingThumb_)
    {
        draggingThumb_ = false;
        markDirty();
        e.consumed = true;
    }
}

void ListBox::onMouseMove(MouseEvent& e)
{
    if (draggingThumb_)
    {
        Rect abs = absoluteRect();
        float localY = e.y - abs.y;
        float tLen = thumbLength(abs.h);
        float trackSpace = abs.h - tLen;
        if (trackSpace > 0)
        {
            float ratio = (localY - dragOffset_) / trackSpace;
            if (ratio < 0) ratio = 0;
            if (ratio > 1) ratio = 1;
            scrollOffset_ = ratio * maxScroll();
        }
        e.consumed = true;
        markDirty();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  ComboBox — dropdown selector
// ═════════════════════════════════════════════════════════════════════════════

// ── Internal popup widget ────────────────────────────────────────────────
class ComboPopup_ : public Widget
{
public:
    ComboPopup_(ComboBox* owner, const std::vector<std::string>& items,
                int selected, int maxVis, float rowH)
        : owner_(owner), items_(items), selected_(selected), rowHeight_(rowH)
    {
        acceptsFocus_ = true;
        scrollable_   = true;
        int vis = std::min(static_cast<int>(items.size()), maxVis);
        float h = vis * rowH + 2;
        // rect_ is set by caller in absolute coords
        (void)h; // height set externally
    }

    void paint(PaintContext& ctx) override
    {
        Rect abs = absoluteRect();
        const auto& t = Theme::instance();

        // Background + border
        ctx.fill.SetColor(t.panelColor.r, t.panelColor.g, t.panelColor.b, 255);
        ctx.fill.RoundedRectangle(static_cast<int>(abs.x), static_cast<int>(abs.y),
                                   static_cast<int>(abs.w), static_cast<int>(abs.h),
                                   t.borderRadius, 6, true);
        ctx.line.SetColor(t.inputBorderHover.r, t.inputBorderHover.g, t.inputBorderHover.b, 255);
        ctx.line.Rectangle(static_cast<int>(abs.x), static_cast<int>(abs.y),
                           static_cast<int>(abs.w), static_cast<int>(abs.h), false);

        // Items
        ctx.pushClip(abs);
        int n = static_cast<int>(items_.size());
        int first = static_cast<int>(scrollOff_ / rowHeight_);
        int last  = static_cast<int>((scrollOff_ + abs.h) / rowHeight_) + 1;
        if (first < 0) first = 0;
        if (last > n)  last  = n;

        for (int i = first; i < last; ++i)
        {
            float ry = abs.y + i * rowHeight_ - scrollOff_;
            Rect rowR = {abs.x, ry, abs.w, rowHeight_};
            if (ctx.isClipped(rowR)) continue;

            // Hover / selected highlight
            bool isHov = (i == hovered_);
            bool isSel = (i == selected_);
            if (isHov || isSel)
            {
                Rect rc;
                if (ctx.clipRectIntersect(rowR, rc))
                {
                    Color c = isSel ? t.selectionColor : Color(t.buttonHover.r, t.buttonHover.g, t.buttonHover.b, 120);
                    ctx.fill.SetColor(c.r, c.g, c.b, c.a);
                    ctx.fill.Rectangle(static_cast<int>(rc.x), static_cast<int>(rc.y),
                                       static_cast<int>(rc.w), static_cast<int>(rc.h), true);
                }
            }

            ctx.font.SetFontSize(t.fontSize);
            ctx.font.SetColor(t.textColor);
            ctx.font.SetBatch(&ctx.text);
            float fc[4]; ctx.fontClip(fc);
            Font::ClipRect clip{fc[0], fc[1], fc[2], fc[3]};
            ctx.font.Print(items_[i].c_str(), abs.x + t.padding, ry + (rowHeight_ - t.fontSize) * 0.5f, &clip);
        }
        ctx.popClip();
    }

    void onMousePress(MouseEvent& e) override
    {
        Rect abs = absoluteRect();
        float localY = e.y - abs.y + scrollOff_;
        int idx = static_cast<int>(localY / rowHeight_);
        if (idx >= 0 && idx < static_cast<int>(items_.size()))
        {
            owner_->setSelectedIndex(idx);
        }
        owner_->closeDropdown();
        e.consumed = true;
    }

    void onMouseMove(MouseEvent& e) override
    {
        Rect abs = absoluteRect();
        float localY = e.y - abs.y + scrollOff_;
        int idx = static_cast<int>(localY / rowHeight_);
        if (idx < 0 || idx >= static_cast<int>(items_.size())) idx = -1;
        if (idx != hovered_)
        {
            hovered_ = idx;
            markDirty();
        }
    }

    void onMouseScroll(MouseEvent& e) override
    {
        float totalH = items_.size() * rowHeight_;
        float viewH  = rect_.h;
        if (totalH <= viewH) return;
        scrollOff_ -= e.scrollY * rowHeight_ * 0.5f;
        if (scrollOff_ < 0) scrollOff_ = 0;
        float mx = totalH - viewH;
        if (scrollOff_ > mx) scrollOff_ = mx;
        e.consumed = true;
        markDirty();
    }

    void onMouseLeave() override
    {
        hovered_ = -1;
        markDirty();
    }

private:
    ComboBox* owner_;
    std::vector<std::string> items_;
    int   selected_ = -1;
    int   hovered_  = -1;
    float rowHeight_ = 24.0f;
    float scrollOff_ = 0.0f;
};

// ── ComboBox implementation ──────────────────────────────────────────────

static const std::string kEmptyCombo;

ComboBox::ComboBox()
{
    acceptsFocus_ = true;
    cursor_ = CursorType::Hand;
}

void ComboBox::addItem(const std::string& text)     { items_.push_back(text); markDirty(); }
void ComboBox::insertItem(int index, const std::string& text)
{
    if (index < 0) index = 0;
    if (index > static_cast<int>(items_.size())) index = static_cast<int>(items_.size());
    items_.insert(items_.begin() + index, text);
    if (selectedIndex_ >= index) ++selectedIndex_;
    markDirty();
}
void ComboBox::removeItem(int index)
{
    if (index < 0 || index >= static_cast<int>(items_.size())) return;
    items_.erase(items_.begin() + index);
    if (selectedIndex_ == index) { selectedIndex_ = -1; selectionChanged.emit(-1); }
    else if (selectedIndex_ > index) --selectedIndex_;
    markDirty();
}
void ComboBox::clear()                { items_.clear(); selectedIndex_ = -1; markDirty(); }

const std::string& ComboBox::itemText(int index) const
{
    if (index >= 0 && index < static_cast<int>(items_.size())) return items_[index];
    return kEmptyCombo;
}

void ComboBox::setItemText(int index, const std::string& text)
{
    if (index >= 0 && index < static_cast<int>(items_.size()))
    { items_[index] = text; markDirty(); }
}

const std::string& ComboBox::currentText() const
{
    return itemText(selectedIndex_);
}

void ComboBox::setSelectedIndex(int idx)
{
    if (idx < -1) idx = -1;
    if (idx >= static_cast<int>(items_.size())) idx = static_cast<int>(items_.size()) - 1;
    if (selectedIndex_ != idx)
    {
        selectedIndex_ = idx;
        selectionChanged.emit(idx);
        markDirty();
    }
}

Widget::Vec2f ComboBox::sizeHint() const
{
    const auto& t = Theme::instance();
    float w = 100.0f;
    for (auto& it : items_)
    {
        float tw = it.size() * t.fontSize * 0.6f + t.padding * 3 + 16; // +16 for arrow
        if (tw > w) w = tw;
    }
    return {w, t.fontSize + t.padding * 2};
}

void ComboBox::paint(PaintContext& ctx)
{
    if (!visible_) return;
    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    // Background + Border (clipped to viewport)
    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        Color bg = pressed_ ? t.buttonPressed : (hovered_ ? t.buttonHover : t.buttonNormal);
        ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
        ctx.fill.RoundedRectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                                   static_cast<int>(clipped.w), static_cast<int>(clipped.h),
                                   t.borderRadius, 6, true);

        Color brd = focused_ ? t.focusColor : (hovered_ ? t.inputBorderHover : t.buttonBorder);
        ctx.line.SetColor(brd.r, brd.g, brd.b, brd.a);
        ctx.line.Rectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                           static_cast<int>(clipped.w), static_cast<int>(clipped.h), false);
    }

    // Text
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size()))
    {
        ctx.font.SetFontSize(t.fontSize);
        ctx.font.SetColor(t.textColor);
        ctx.font.SetBatch(&ctx.text);
        float ty = abs.y + (abs.h - t.fontSize) * 0.5f;
        float fc[4]; ctx.fontClip(fc);
        Font::ClipRect clip{fc[0], fc[1], fc[2], fc[3]};
        ctx.font.Print(items_[selectedIndex_].c_str(), abs.x + t.padding, ty, &clip);
    }

    // Arrow (chevron ▼)
    float arrowSz = 6.0f;
    float ax = abs.x + abs.w - t.padding - arrowSz * 2;
    float ay = abs.y + abs.h * 0.5f - arrowSz * 0.3f;
    ctx.line.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, t.textColor.a);
    ctx.line.Line2D(static_cast<int>(ax), static_cast<int>(ay),
                    static_cast<int>(ax + arrowSz), static_cast<int>(ay + arrowSz));
    ctx.line.Line2D(static_cast<int>(ax + arrowSz), static_cast<int>(ay + arrowSz),
                    static_cast<int>(ax + arrowSz * 2), static_cast<int>(ay));
}

void ComboBox::onMousePress(MouseEvent& e)
{
    if (!enabled_) return;
    if (open_)
        closeDropdown();
    else
        openDropdown();
    e.consumed = true;
}

void ComboBox::openDropdown()
{
    if (open_ || items_.empty()) return;
    open_ = true;

    Rect abs = absoluteRect();
    float rowH = 24.0f;
    int vis = std::min(static_cast<int>(items_.size()), maxVisible_);
    float popH = vis * rowH + 2;

    auto* popup = new ComboPopup_(this, items_, selectedIndex_, maxVisible_, rowH);
    popup->setRect({abs.x, abs.y + abs.h, abs.w, popH});

    WidgetApp::instance().showPopup(popup, this);
    markDirty();
}

void ComboBox::closeDropdown()
{
    if (!open_) return;
    open_ = false;
    WidgetApp::instance().closePopup();
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  ListWidget — scrollable list with widget rows
// ═════════════════════════════════════════════════════════════════════════════

ListWidget::ListWidget()
{
    acceptsFocus_ = true;
    scrollable_   = true;
}

void ListWidget::addRowWidget(Widget* w)
{
    w->setSize(0, rowHeight_);
    addChild(w);
}

void ListWidget::removeRow(int index)
{
    if (index < 0 || index >= static_cast<int>(children_.size())) return;
    Widget* w = children_[index];
    removeChild(w);  // removeChild already deletes
    if (selectedIndex_ == index) { selectedIndex_ = -1; selectionChanged.emit(-1); }
    else if (selectedIndex_ > index) --selectedIndex_;
    markDirty();
}

void ListWidget::clearRows()
{
    while (!children_.empty())
        removeChild(children_.back());  // removeChild already deletes
    selectedIndex_ = -1;
    scrollOffset_  = 0;
    markDirty();
}

Widget* ListWidget::row(int index) const
{
    if (index >= 0 && index < static_cast<int>(children_.size())) return children_[index];
    return nullptr;
}

void ListWidget::setSelectedIndex(int idx)
{
    if (idx < -1) idx = -1;
    if (idx >= static_cast<int>(children_.size())) idx = static_cast<int>(children_.size()) - 1;
    if (selectedIndex_ != idx)
    {
        selectedIndex_ = idx;
        selectionChanged.emit(idx);
        markDirty();
    }
}

bool  ListWidget::needsScrollBar() const { return totalHeight() > rect_.h; }
float ListWidget::totalHeight()    const { return children_.size() * rowHeight_; }
float ListWidget::maxScroll()      const { float m = totalHeight() - rect_.h; return m > 0 ? m : 0; }

float ListWidget::thumbLength(float trackH) const
{
    float total = totalHeight();
    if (total <= 0) return trackH;
    float len = (rect_.h / total) * trackH;
    return len < 16.0f ? 16.0f : len;
}

float ListWidget::thumbPos(float trackH) const
{
    float mx = maxScroll();
    if (mx <= 0) return 0;
    float tLen = thumbLength(trackH);
    return (scrollOffset_ / mx) * (trackH - tLen);
}

void ListWidget::layout()
{
    const auto& t = Theme::instance();
    bool sb = needsScrollBar();
    float contentW = sb ? rect_.w - t.scrollbarWidth : rect_.w;

    for (int i = 0; i < static_cast<int>(children_.size()); ++i)
    {
        Widget* w = children_[i];
        float ry = i * rowHeight_ - scrollOffset_;
        w->setRect({0, ry, contentW, rowHeight_});
        w->layout();
    }
}

Widget::Vec2f ListWidget::sizeHint() const
{
    float h = std::min(static_cast<int>(children_.size()), 8) * rowHeight_ + 2;
    return {200.0f, h};
}

void ListWidget::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    int n = static_cast<int>(children_.size());
    bool sb = needsScrollBar();
    float contentW = sb ? abs.w - t.scrollbarWidth : abs.w;

    // Background
    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, t.inputBg.a);
        ctx.fill.Rectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                           static_cast<int>(clipped.w), static_cast<int>(clipped.h), true);

        Color brd = focused_ ? t.focusColor : (hovered_ ? t.inputBorderHover : t.inputBorder);
        ctx.line.SetColor(brd.r, brd.g, brd.b, brd.a);
        ctx.line.Rectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                           static_cast<int>(clipped.w), static_cast<int>(clipped.h), false);
    }

    // Clip items
    Rect itemClip = {abs.x, abs.y, contentW, abs.h};
    ctx.pushClip(itemClip);

    int firstVisible = static_cast<int>(scrollOffset_ / rowHeight_);
    int lastVisible  = static_cast<int>((scrollOffset_ + abs.h) / rowHeight_) + 1;
    if (firstVisible < 0) firstVisible = 0;
    if (lastVisible > n)  lastVisible  = n;

    for (int i = firstVisible; i < lastVisible; ++i)
    {
        float ry = abs.y + i * rowHeight_ - scrollOffset_;
        Rect rowR = {abs.x, ry, contentW, rowHeight_};

        if (ctx.isClipped(rowR)) continue;

        // Selection highlight
        if (i == selectedIndex_)
        {
            Rect selClip;
            if (ctx.clipRectIntersect(rowR, selClip))
            {
                ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g,
                                  t.selectionColor.b, t.selectionColor.a);
                ctx.fill.Rectangle(static_cast<int>(selClip.x), static_cast<int>(selClip.y),
                                   static_cast<int>(selClip.w), static_cast<int>(selClip.h), true);
            }
        }

        // Paint row widget
        children_[i]->paint(ctx);
    }

    ctx.popClip();

    // Scrollbar
    if (sb)
    {
        float sbX = abs.x + contentW;
        Rect trackRect = {sbX, abs.y, t.scrollbarWidth, abs.h};
        Rect trackClip;
        if (ctx.clipRectIntersect(trackRect, trackClip))
        {
            ctx.fill.SetColor(t.sliderTrack.r, t.sliderTrack.g, t.sliderTrack.b, t.sliderTrack.a);
            ctx.fill.RoundedRectangle(static_cast<int>(trackClip.x), static_cast<int>(trackClip.y),
                                       static_cast<int>(trackClip.w), static_cast<int>(trackClip.h),
                                       3.0f, 4, true);
        }
        float tLen = thumbLength(abs.h);
        float tPos = thumbPos(abs.h);
        Rect thumbRect = {sbX + 1, abs.y + tPos, t.scrollbarWidth - 2, tLen};
        Rect thumbClip;
        if (ctx.clipRectIntersect(thumbRect, thumbClip))
        {
            Color thumbC = draggingThumb_ ? t.sliderThumbPressed : (hovered_ ? t.sliderThumbHover : t.sliderThumb);
            ctx.fill.SetColor(thumbC.r, thumbC.g, thumbC.b, thumbC.a);
            ctx.fill.RoundedRectangle(static_cast<int>(thumbClip.x), static_cast<int>(thumbClip.y),
                                       static_cast<int>(thumbClip.w), static_cast<int>(thumbClip.h),
                                       3.0f, 4, true);
        }
    }
}

void ListWidget::onMousePress(MouseEvent& e)
{
    if (!enabled_) return;

    const auto& t = Theme::instance();
    Rect abs = absoluteRect();

    // Click on scrollbar?
    if (needsScrollBar())
    {
        float sbX = abs.x + abs.w - t.scrollbarWidth;
        if (e.x >= sbX)
        {
            float localY = e.y - abs.y;
            float tPos = thumbPos(abs.h);
            float tLen = thumbLength(abs.h);
            if (localY >= tPos && localY <= tPos + tLen)
            {
                draggingThumb_ = true;
                dragOffset_ = localY - tPos;
            }
            else
            {
                float pageSize = rect_.h * 0.8f;
                if (localY < tPos) scrollOffset_ -= pageSize;
                else               scrollOffset_ += pageSize;
                float mx = maxScroll();
                if (scrollOffset_ < 0)  scrollOffset_ = 0;
                if (scrollOffset_ > mx) scrollOffset_ = mx;
                markDirty();
            }
            e.consumed = true;
            return;
        }
    }

    float localY = e.y - abs.y + scrollOffset_;
    int idx = static_cast<int>(localY / rowHeight_);
    if (idx >= 0 && idx < static_cast<int>(children_.size()))
        setSelectedIndex(idx);
    e.consumed = true;
}

void ListWidget::onMouseScroll(MouseEvent& e)
{
    float viewH = rect_.h;
    if (totalHeight() <= viewH) { scrollOffset_ = 0; return; }
    scrollOffset_ -= e.scrollY * rowHeight_ * 0.5f;
    if (scrollOffset_ < 0) scrollOffset_ = 0;
    float mx = maxScroll();
    if (scrollOffset_ > mx) scrollOffset_ = mx;
    e.consumed = true;
    markDirty();
}

void ListWidget::onMouseRelease(MouseEvent& e)
{
    if (draggingThumb_)
    {
        draggingThumb_ = false;
        markDirty();
        e.consumed = true;
    }
}

void ListWidget::onMouseMove(MouseEvent& e)
{
    if (draggingThumb_)
    {
        Rect abs = absoluteRect();
        float localY = e.y - abs.y;
        float tLen = thumbLength(abs.h);
        float trackSpace = abs.h - tLen;
        if (trackSpace > 0)
        {
            float ratio = (localY - dragOffset_) / trackSpace;
            if (ratio < 0) ratio = 0;
            if (ratio > 1) ratio = 1;
            scrollOffset_ = ratio * maxScroll();
        }
        e.consumed = true;
        markDirty();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  TextInput — single-line text field
// ═════════════════════════════════════════════════════════════════════════════

TextInput::TextInput(const std::string& text, Mode mode)
    : text_(text), mode_(mode)
{
    acceptsFocus_ = true;
    setCursor(CursorType::IBeam);
    cursor_ = utf8Length(text_);
    selStart_ = selEnd_ = cursor_;
}

void TextInput::setText(const std::string& t)
{
    if (text_ == t) return;
    text_ = t;
    int len = utf8Length(text_);
    if (cursor_ > len) cursor_ = len;
    selStart_ = selEnd_ = cursor_;
    scrollX_ = 0;
    markDirty();
    textChanged.emit(text_);
}

void TextInput::selectAll()
{
    selStart_ = 0;
    selEnd_ = utf8Length(text_);
    cursor_ = selEnd_;
    markDirty();
}

void TextInput::clearSelection()
{
    selStart_ = selEnd_ = cursor_;
    markDirty();
}

std::string TextInput::selectedText() const
{
    if (selStart_ == selEnd_) return {};
    int lo = std::min(selStart_, selEnd_);
    int hi = std::max(selStart_, selEnd_);
    return utf8Substr(text_, lo, hi);
}

void TextInput::setCursorPos(int pos)
{
    int len = utf8Length(text_);
    cursor_ = std::clamp(pos, 0, len);
    selStart_ = selEnd_ = cursor_;
    markDirty();
}

Widget::Vec2f TextInput::sizeHint() const
{
    auto& t = Theme::instance();
    return {120.0f, t.inputHeight};
}

std::string TextInput::displayText() const
{
    if (mode_ == Mode::Password)
    {
        int len = utf8Length(text_);
        std::string result;
        result.reserve(len * 3);
        for (int i = 0; i < len; ++i)
            result += "\xe2\x80\xa2";
        return result;
    }
    return text_;
}

int TextInput::hitTestChar(PaintContext& ctx, float localX) const
{
    std::string disp = displayText();
    int len = utf8Length(text_);
    float pad = Theme::instance().padding;
    float x = pad - scrollX_;

    ctx.font.SetFontSize(Theme::instance().fontSize);

    for (int i = 0; i < len; ++i)
    {
        std::string ch = utf8Substr(disp, i, i + 1);
        float cw = ctx.font.GetTextWidth(ch.c_str());
        if (localX < x + cw * 0.5f)
            return i;
        x += cw;
    }
    return len;
}

float TextInput::cursorXOffset(PaintContext& ctx, int pos) const
{
    std::string disp = displayText();
    std::string sub = utf8Substr(disp, 0, pos);
    ctx.font.SetFontSize(Theme::instance().fontSize);
    return ctx.font.GetTextWidth(sub.c_str());
}

void TextInput::deleteSelection()
{
    if (selStart_ == selEnd_) return;
    int lo = std::min(selStart_, selEnd_);
    int hi = std::max(selStart_, selEnd_);
    size_t byteStart = utf8ByteOffset(text_, lo);
    size_t byteEnd   = utf8ByteOffset(text_, hi);
    text_.erase(byteStart, byteEnd - byteStart);
    cursor_ = lo;
    selStart_ = selEnd_ = cursor_;
}

void TextInput::insertText(const std::string& t)
{
    if (mode_ == Mode::ReadOnly) return;

    if (mode_ == Mode::NumberOnly)
    {
        for (char c : t)
            if (!isNumberChar(c)) return;
    }

    deleteSelection();
    size_t bytePos = utf8ByteOffset(text_, cursor_);
    text_.insert(bytePos, t);
    cursor_ += utf8Length(t);
    selStart_ = selEnd_ = cursor_;
    markDirty();
    textChanged.emit(text_);
}

void TextInput::ensureCursorVisible(PaintContext& ctx)
{
    float pad = Theme::instance().padding;
    float fieldW = rect_.w - pad * 2;
    float cx = cursorXOffset(ctx, cursor_);

    if (cx - scrollX_ < 0)
        scrollX_ = cx;
    else if (cx - scrollX_ > fieldW)
        scrollX_ = cx - fieldW;
}

void TextInput::clampCursor()
{
    int len = utf8Length(text_);
    if (cursor_ < 0) cursor_ = 0;
    if (cursor_ > len) cursor_ = len;
}

bool TextInput::isNumberChar(char c) const
{
    return (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+';
}

void TextInput::paint(PaintContext& ctx)
{
    auto& t = Theme::instance();
    Rect abs = absoluteRect();
    Rect clipped;
    float pad = t.padding;

    blinkTimer_ += WidgetApp::instance().deltaTime();
    if (blinkTimer_ > 1.0f) blinkTimer_ -= 1.0f;

    // Background
    Color bg = isFocused() ? t.inputBgHover : (isHovered() ? t.inputBgHover : t.inputBg);
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
        ctx.fill.RoundedRectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                                   static_cast<int>(clipped.w), static_cast<int>(clipped.h),
                                   t.borderRadius, 6, true);
    }

    // Border
    Color bc = isFocused() ? t.focusColor : (isHovered() ? t.inputBorderHover : t.inputBorder);
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.line.SetColor(bc.r, bc.g, bc.b, bc.a);
        ctx.line.RoundedRectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                                   static_cast<int>(clipped.w), static_cast<int>(clipped.h),
                                   t.borderRadius, 6, false);
    }

    // Text area
    Rect textArea = {abs.x + pad, abs.y, abs.w - pad * 2, abs.h};
    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetBatch(&ctx.text);

    ensureCursorVisible(ctx);

    float textX = textArea.x - scrollX_;
    float textY = abs.y + (abs.h - t.fontSize) * 0.5f;

    Font::ClipRect clip{textArea.x, textArea.y, textArea.w, textArea.h};

    if (text_.empty() && !isFocused() && !placeholder_.empty())
    {
        ctx.font.SetColor(Color(t.textDisabled.r, t.textDisabled.g, t.textDisabled.b, 128));
        ctx.font.Print(placeholder_.c_str(), textX, textY, &clip);
    }
    else
    {
        // Selection highlight
        if (hasSelection() && isFocused())
        {
            int lo = std::min(selStart_, selEnd_);
            int hi = std::max(selStart_, selEnd_);
            float selX1 = textArea.x + cursorXOffset(ctx, lo) - scrollX_;
            float selX2 = textArea.x + cursorXOffset(ctx, hi) - scrollX_;
            float sx1 = std::max(selX1, textArea.x);
            float sx2 = std::min(selX2, textArea.x + textArea.w);
            if (sx1 < sx2)
            {
                ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g, t.selectionColor.b, t.selectionColor.a);
                ctx.fill.Rectangle(static_cast<int>(sx1), static_cast<int>(abs.y + 2),
                                    static_cast<int>(sx2 - sx1), static_cast<int>(abs.h - 4), true);
            }
        }

        // Text
        std::string disp = displayText();
        ctx.font.SetColor(t.textColor);
        ctx.font.Print(disp.c_str(), textX, textY, &clip);

        // Cursor
        if (isFocused() && blinkTimer_ < 0.5f && mode_ != Mode::ReadOnly)
        {
            float cx = textArea.x + cursorXOffset(ctx, cursor_) - scrollX_;
            if (cx >= textArea.x && cx <= textArea.x + textArea.w)
            {
                ctx.line.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, t.textColor.a);
                ctx.line.Line2D(static_cast<int>(cx), static_cast<int>(abs.y + 3),
                                static_cast<int>(cx), static_cast<int>(abs.y + abs.h - 3));
            }
        }
    }
}

void TextInput::onMousePress(MouseEvent& e)
{
    Rect abs = absoluteRect();
    if (e.localX < 0 || e.localX > abs.w || e.localY < 0 || e.localY > abs.h)
        return;

    auto& app = WidgetApp::instance();
    PaintContext ctx{app.fillBatch(), app.lineBatch(), app.textBatch(), app.font()};

    ctx.font.SetFontSize(Theme::instance().fontSize);
    cursor_ = hitTestChar(ctx, e.localX);
    selStart_ = selEnd_ = cursor_;
    dragging_ = true;
    blinkTimer_ = 0;
    markDirty();
    e.consumed = true;
}

void TextInput::onMouseRelease(MouseEvent& e)
{
    if (dragging_)
    {
        dragging_ = false;
        e.consumed = true;
    }
}

void TextInput::onMouseMove(MouseEvent& e)
{
    if (dragging_)
    {
        auto& app = WidgetApp::instance();
        PaintContext ctx{app.fillBatch(), app.lineBatch(), app.textBatch(), app.font()};

        ctx.font.SetFontSize(Theme::instance().fontSize);
        cursor_ = hitTestChar(ctx, e.localX);
        selEnd_ = cursor_;
        blinkTimer_ = 0;
        markDirty();
        e.consumed = true;
    }
}

void TextInput::onKeyPress(KeyEvent& e)
{
    if (mode_ == Mode::ReadOnly && e.key != SDLK_c)
    {
        if (!e.ctrl) return;
    }

    int len = utf8Length(text_);
    blinkTimer_ = 0;

    if (e.ctrl)
    {
        switch (e.key)
        {
        case SDLK_a:
            selectAll();
            e.consumed = true;
            return;
        case SDLK_c:
        {
            std::string sel = selectedText();
            if (!sel.empty())
                SDL_SetClipboardText(sel.c_str());
            e.consumed = true;
            return;
        }
        case SDLK_x:
        {
            if (mode_ == Mode::ReadOnly) { e.consumed = true; return; }
            std::string sel = selectedText();
            if (!sel.empty())
            {
                SDL_SetClipboardText(sel.c_str());
                deleteSelection();
                markDirty();
                textChanged.emit(text_);
            }
            e.consumed = true;
            return;
        }
        case SDLK_v:
        {
            if (mode_ == Mode::ReadOnly) { e.consumed = true; return; }
            char* clip = SDL_GetClipboardText();
            if (clip && clip[0])
            {
                std::string paste(clip);
                paste.erase(std::remove(paste.begin(), paste.end(), '\n'), paste.end());
                paste.erase(std::remove(paste.begin(), paste.end(), '\r'), paste.end());
                insertText(paste);
            }
            SDL_free(clip);
            e.consumed = true;
            return;
        }
        default: break;
        }
    }

    switch (e.key)
    {
    case SDLK_LEFT:
        if (cursor_ > 0)
        {
            if (e.shift)
            {
                cursor_--;
                selEnd_ = cursor_;
            }
            else
            {
                if (hasSelection())
                    cursor_ = std::min(selStart_, selEnd_);
                else
                    cursor_--;
                selStart_ = selEnd_ = cursor_;
            }
        }
        else if (!e.shift)
            selStart_ = selEnd_ = cursor_;
        markDirty();
        e.consumed = true;
        break;

    case SDLK_RIGHT:
        if (cursor_ < len)
        {
            if (e.shift)
            {
                cursor_++;
                selEnd_ = cursor_;
            }
            else
            {
                if (hasSelection())
                    cursor_ = std::max(selStart_, selEnd_);
                else
                    cursor_++;
                selStart_ = selEnd_ = cursor_;
            }
        }
        else if (!e.shift)
            selStart_ = selEnd_ = cursor_;
        markDirty();
        e.consumed = true;
        break;

    case SDLK_HOME:
        cursor_ = 0;
        if (e.shift) selEnd_ = cursor_;
        else selStart_ = selEnd_ = cursor_;
        markDirty();
        e.consumed = true;
        break;

    case SDLK_END:
        cursor_ = len;
        if (e.shift) selEnd_ = cursor_;
        else selStart_ = selEnd_ = cursor_;
        markDirty();
        e.consumed = true;
        break;

    case SDLK_BACKSPACE:
        if (mode_ == Mode::ReadOnly) { e.consumed = true; break; }
        if (hasSelection())
        {
            deleteSelection();
        }
        else if (cursor_ > 0)
        {
            size_t byteEnd = utf8ByteOffset(text_, cursor_);
            cursor_--;
            size_t byteStart = utf8ByteOffset(text_, cursor_);
            text_.erase(byteStart, byteEnd - byteStart);
            selStart_ = selEnd_ = cursor_;
        }
        markDirty();
        textChanged.emit(text_);
        e.consumed = true;
        break;

    case SDLK_DELETE:
        if (mode_ == Mode::ReadOnly) { e.consumed = true; break; }
        if (hasSelection())
        {
            deleteSelection();
        }
        else if (cursor_ < len)
        {
            size_t byteStart = utf8ByteOffset(text_, cursor_);
            size_t byteEnd = utf8ByteOffset(text_, cursor_ + 1);
            text_.erase(byteStart, byteEnd - byteStart);
            selStart_ = selEnd_ = cursor_;
        }
        markDirty();
        textChanged.emit(text_);
        e.consumed = true;
        break;

    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        submitted.emit(text_);
        e.consumed = true;
        break;

    case SDLK_ESCAPE:
        clearSelection();
        e.consumed = true;
        break;

    default:
        break;
    }
}

void TextInput::onTextInput(KeyEvent& e)
{
    if (mode_ == Mode::ReadOnly) { e.consumed = true; return; }

    std::string t(e.text);
    if (t.empty()) return;

    insertText(t);
    blinkTimer_ = 0;
    e.consumed = true;
}
