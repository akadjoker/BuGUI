#include "Widgets.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <algorithm>

// ═════════════════════════════════════════════════════════════════════════════
//  ScrollBar
// ═════════════════════════════════════════════════════════════════════════════

ScrollBar::ScrollBar(LayoutDir orientation) : orientation_(orientation)
{
    acceptsFocus_ = true;
}

float ScrollBar::maxValue() const
{
    return (contentSize_ > visibleSize_) ? contentSize_ - visibleSize_ : 0.0f;
}

void ScrollBar::clampValue()
{
    float mv = maxValue();
    if (value_ > mv) value_ = mv;
    if (value_ < 0)  value_ = 0;
}

void ScrollBar::setValue(float v)
{
    v = Clamp(v, 0.0f, maxValue());
    if (v != value_)
    {
        value_ = v;
        onValueChanged.emit(value_);
        WidgetApp::instance().fireEvent("scroll", this);
        markDirty();
    }
}

float ScrollBar::trackLength() const
{
    return (orientation_ == LayoutDir::Vertical) ? rect_.h : rect_.w;
}

float ScrollBar::thumbLength() const
{
    if (contentSize_ <= 0) return trackLength();
    float ratio = visibleSize_ / contentSize_;
    if (ratio >= 1.0f) return trackLength();
    float len = trackLength() * ratio;
    return (len < 16.0f) ? 16.0f : len;  // min thumb size
}

float ScrollBar::thumbPos() const
{
    float mv = maxValue();
    if (mv <= 0) return 0;
    float available = trackLength() - thumbLength();
    return (value_ / mv) * available;
}

Widget::Vec2f ScrollBar::sizeHint() const
{
    const auto& t = Theme::instance();
    if (orientation_ == LayoutDir::Vertical)
        return {t.scrollbarWidth, 100.0f};
    return {100.0f, t.scrollbarWidth};
}

void ScrollBar::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    bool vert = (orientation_ == LayoutDir::Vertical);

    // Track background
    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        Color trackC = hovered_ ? t.sliderTrackHover : t.sliderTrack;
        ctx.fill.SetColor(trackC.r, trackC.g, trackC.b, trackC.a);
        ctx.fill.RoundedRectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                                   static_cast<int>(clipped.w), static_cast<int>(clipped.h),
                                   t.borderRadius, 6, true);
    }

    // Don't draw thumb if content fits
    if (contentSize_ <= visibleSize_) return;

    // Thumb
    float tPos = thumbPos();
    float tLen = thumbLength();
    Rect thumbRect;
    if (vert)
        thumbRect = {abs.x + 1, abs.y + tPos, abs.w - 2, tLen};
    else
        thumbRect = {abs.x + tPos, abs.y + 1, tLen, abs.h - 2};

    Rect clippedThumb;
    if (ctx.clipRectIntersect(thumbRect, clippedThumb))
    {
        Color thumbC = dragging_ ? t.sliderThumbPressed : (hovered_ ? t.sliderThumbHover : t.sliderThumb);
        ctx.fill.SetColor(thumbC.r, thumbC.g, thumbC.b, thumbC.a);
        ctx.fill.RoundedRectangle(static_cast<int>(clippedThumb.x), static_cast<int>(clippedThumb.y),
                                   static_cast<int>(clippedThumb.w), static_cast<int>(clippedThumb.h),
                                   t.borderRadius, 6, true);
    }

    Widget::paint(ctx);
}

void ScrollBar::onMousePress(MouseEvent& e)
{
    if (!enabled_ || contentSize_ <= visibleSize_) return;

    bool vert = (orientation_ == LayoutDir::Vertical);
    float mousePos = vert ? e.localY : e.localX;
    float tPos = thumbPos();
    float tLen = thumbLength();

    if (mousePos >= tPos && mousePos <= tPos + tLen)
    {
        // Clicked on thumb → start drag
        dragging_ = true;
        dragOffset_ = mousePos - tPos;
    }
    else
    {
        // Clicked on track → page jump
        float pageSize = visibleSize_ * 0.9f;
        if (mousePos < tPos)
            setValue(value_ - pageSize);
        else
            setValue(value_ + pageSize);
    }
    e.consumed = true;
    Widget::onMousePress(e);
}

void ScrollBar::onMouseRelease(MouseEvent& e)
{
    dragging_ = false;
    e.consumed = true;
    Widget::onMouseRelease(e);
}

void ScrollBar::onMouseMove(MouseEvent& e)
{
    if (dragging_)
    {
        bool vert = (orientation_ == LayoutDir::Vertical);
        float mousePos = vert ? e.localY : e.localX;
        float newThumbPos = mousePos - dragOffset_;
        float available = trackLength() - thumbLength();
        if (available > 0)
        {
            float ratio = Clamp(newThumbPos / available, 0.0f, 1.0f);
            setValue(ratio * maxValue());
        }
        e.consumed = true;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  ScrollView
// ═════════════════════════════════════════════════════════════════════════════

ScrollView::ScrollView()
{
    // Create scroll bars as children - they live in the widget tree
    vBar_ = createChild<ScrollBar>(LayoutDir::Vertical);
    hBar_ = createChild<ScrollBar>(LayoutDir::Horizontal);
    hBar_->setVisible(false);
    vBar_->setVisible(false);

    // ScrollBars are NOT affected by parent's scroll offset
    vBar_->setScrollable(false);
    hBar_->setScrollable(false);

    vBar_->onValueChanged.connect([this](float v) {
        scrollY_ = v;
        markDirty();
    });
    hBar_->onValueChanged.connect([this](float v) {
        scrollX_ = v;
        markDirty();
    });
}

void ScrollView::setContentWidget(Widget* w)
{
    if (content_ && content_->parent() == this)
        removeChild(content_);
    if (w && w->parent() && w->parent() != this)
    {
        content_ = nullptr;
        return;
    }
    content_ = w;
    if (w)
    {
        // Insert content before the scrollbars (index 0)
        addChild(w);
        // Move it to the front (before vBar_ and hBar_)
        auto& ch = children_;
        auto it = std::find(ch.begin(), ch.end(), w);
        if (it != ch.end()) {
            ch.erase(it);
            ch.insert(ch.begin(), w);
        }
        markDirty();
    }
}

void ScrollView::setScrollX(float v)
{
    float maxX = (content_ ? content_->rect().w - rect_.w : 0);
    if (hBar_->isVisible()) maxX += 0; // already accounted
    v = Clamp(v, 0.0f, (maxX > 0 ? maxX : 0));
    if (v != scrollX_) { scrollX_ = v; hBar_->setValue(v); markDirty(); }
}

void ScrollView::setScrollY(float v)
{
    float maxY = (content_ ? content_->rect().h - rect_.h : 0);
    v = Clamp(v, 0.0f, (maxY > 0 ? maxY : 0));
    if (v != scrollY_) { scrollY_ = v; vBar_->setValue(v); markDirty(); }
}

void ScrollView::layout()
{
    if (!content_) return;

    // Pass 1: give content the full viewport width so it can compute
    // its natural height at that width (like a vertical flow).
    float vpW1 = rect_.w;
    float vpH1 = rect_.h;

    content_->setRect({0, 0, vpW1, vpH1});
    content_->layout();

    // Now ask for the natural size - height may exceed viewport
    auto hint = content_->sizeHint();
    float contentH = (hint.y > 0) ? hint.y : vpH1;

    // Determine if we need vertical scrollbar
    bool needV = contentH > rect_.h;

    // With V bar, viewport width shrinks - re-layout content at real width
    float vpW = rect_.w - (needV ? barWidth_ : 0);
    float vpH = rect_.h;

    // Re-layout content at the real available width
    content_->setRect({0, 0, vpW, vpH});
    content_->layout();

    // Re-check height after width change
    hint = content_->sizeHint();
    contentH = (hint.y > 0) ? hint.y : vpH;
    needV = contentH > vpH;

    // Now check horizontal - only if content's min width exceeds viewport
    float contentW = (hint.x > 0) ? hint.x : vpW;
    bool needH = contentW > vpW;

    if (needH) vpH = rect_.h - barWidth_;
    if (!needV && contentH > vpH) { needV = true; vpW = rect_.w - barWidth_; }

    vBar_->setVisible(needV);
    hBar_->setVisible(needH);

    // Final content size
    float cw = needH ? contentW : vpW;
    float ch = needV ? contentH : vpH;
    content_->setRect({0, 0, cw, ch});
    content_->layout();

    // Position scrollbars
    if (needV)
    {
        vBar_->setRect({vpW, 0, barWidth_, vpH});
        vBar_->setContentSize(ch);
        vBar_->setVisibleSize(vpH);
    }
    if (needH)
    {
        hBar_->setRect({0, vpH, vpW, barWidth_});
        hBar_->setContentSize(cw);
        hBar_->setVisibleSize(vpW);
    }

    syncBars();
}

void ScrollView::syncBars()
{
    if (vBar_->isVisible()) vBar_->setValue(scrollY_);
    else                     scrollY_ = 0;

    if (hBar_->isVisible()) hBar_->setValue(scrollX_);
    else                     scrollX_ = 0;
}

void ScrollView::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    // Background
    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.fill.SetColor(t.bgColor.r, t.bgColor.g, t.bgColor.b, t.bgColor.a);
        ctx.fill.Rectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                           static_cast<int>(clipped.w), static_cast<int>(clipped.h), true);
    }

    // Viewport clip area (exclude scrollbar space)
    float vpW = rect_.w - (vBar_->isVisible() ? barWidth_ : 0);
    float vpH = rect_.h - (hBar_->isVisible() ? barWidth_ : 0);
    Rect viewport = {abs.x, abs.y, vpW, vpH};

    // Paint content with scroll offset (absoluteRect handles the offset)
    if (content_)
    {
        ctx.pushClip(viewport);
        content_->paint(ctx);
        ctx.popClip();
    }

    // Paint scrollbars (outside viewport clip)
    if (vBar_->isVisible()) vBar_->paint(ctx);
    if (hBar_->isVisible()) hBar_->paint(ctx);

    // Border
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
        ctx.line.Rectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                           static_cast<int>(clipped.w), static_cast<int>(clipped.h), false);
    }
}

void ScrollView::onMouseScroll(MouseEvent& e)
{
    if (e.consumed) return;

    if (vBar_->isVisible() && e.scrollY != 0)
    {
        setScrollY(scrollY_ - e.scrollY * scrollSpeed_);
        e.consumed = true;
    }
    if (hBar_->isVisible() && e.scrollX != 0)
    {
        setScrollX(scrollX_ - e.scrollX * scrollSpeed_);
        e.consumed = true;
    }
}
