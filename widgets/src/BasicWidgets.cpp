#include "Widgets.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include "Texture.hpp"
#include <algorithm>

namespace {
    inline Font::ClipRect toFontClip(const Rect& r)
    { return {r.x, r.y, r.w, r.h}; }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Label
// ═════════════════════════════════════════════════════════════════════════════

Label::Label(const std::string& text) : text_(text) {}

void Label::setText(const std::string& t) { text_ = t; markDirty(); }
void Label::setColor(const Color& c)      { color_ = c; }
void Label::setAlign(TextAlign a)         { align_ = a; markDirty(); }

Widget::Vec2f Label::sizeHint() const
{
    const auto& t = Theme::instance();
    float w = text_.size() * t.fontSize * 0.6f;
    return {w + t.padding * 2, t.fontSize + t.padding * 2};
}

void Label::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    const Color& c = enabled_ ? color_ : t.textDisabled;

    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetColor(c);
    ctx.font.SetBatch(&ctx.text);

    float textW = ctx.font.GetTextWidth(text_.c_str());
    float tx;
    switch (align_)
    {
    case TextAlign::CENTER: tx = abs.x + (abs.w - textW) * 0.5f; break;
    case TextAlign::RIGHT:  tx = abs.x + abs.w - textW - t.padding; break;
    default:                tx = abs.x + t.padding; break;
    }
    float ty = abs.y + (abs.h - t.fontSize) * 0.5f;

    // Clip text: only print if within clip region
    Rect textRect = {tx, ty, textW, t.fontSize};
    if (!ctx.isClipped(textRect))
    {
        auto fc = toFontClip(ctx.clipRect());
        ctx.font.Print(text_.c_str(), tx, ty, &fc);
    }

    Widget::paint(ctx);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Spacer
// ═════════════════════════════════════════════════════════════════════════════

Spacer::Spacer(float size) : size_(size) {}

Widget::Vec2f Spacer::sizeHint() const
{
    return {size_, size_};
}

// ═════════════════════════════════════════════════════════════════════════════
//  Line (separator)
// ═════════════════════════════════════════════════════════════════════════════

Line::Line(float thickness) : thickness_(thickness) {}

Widget::Vec2f Line::sizeHint() const
{
    auto* p = dynamic_cast<BoxLayout*>(parent());
    if (p && p->dir() == LayoutDir::Horizontal)
        return {thickness_, 0.0f};  // vertical line
    return {0.0f, thickness_};      // horizontal line
}

void Line::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    auto* p = dynamic_cast<BoxLayout*>(parent());
    bool vertical = p && p->dir() == LayoutDir::Horizontal;

    ctx.fill.SetColor(color_.r, color_.g, color_.b, color_.a);

    if (vertical)
    {
        // Vertical line centered in widget width
        float lx = abs.x + (abs.w - thickness_) * 0.5f;
        ctx.fill.Rectangle(static_cast<int>(lx), static_cast<int>(abs.y),
                           static_cast<int>(thickness_), static_cast<int>(abs.h), true);
    }
    else
    {
        // Horizontal line centered in widget height
        float ly = abs.y + (abs.h - thickness_) * 0.5f;
        ctx.fill.Rectangle(static_cast<int>(abs.x), static_cast<int>(ly),
                           static_cast<int>(abs.w), static_cast<int>(thickness_), true);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Button
// ═════════════════════════════════════════════════════════════════════════════

Button::Button(const std::string& text) : text_(text)
{
    acceptsFocus_ = true;
    cursor_ = CursorType::Hand;
}

void Button::setText(const std::string& t) { text_ = t; markDirty(); }
void Button::setAlign(TextAlign a)         { align_ = a; markDirty(); }

Widget::Vec2f Button::sizeHint() const
{
    const auto& t = Theme::instance();
    float textW = text_.size() * t.fontSize * 0.6f;
    return {textW + t.padding * 4, t.buttonHeight};
}

void Button::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    // Clip the button rect
    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        // Background (fill)
        Color bg;
        if (bgColor_.a > 0)
        {
            // Custom color with press/hover dimming
            bg = bgColor_;
            if (pressed_)     { bg.r = bg.r * 3/4; bg.g = bg.g * 3/4; bg.b = bg.b * 3/4; }
            else if (hovered_){ bg.r = std::min(255, bg.r + 20); bg.g = std::min(255, bg.g + 20); bg.b = std::min(255, bg.b + 20); }
        }
        else
        {
            bg = pressed_ ? t.buttonPressed : (hovered_ ? t.buttonHover : t.buttonNormal);
        }
        if (!enabled_) bg = t.panelColor;

        ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
        ctx.fill.RoundedRectangle(
            static_cast<int>(clipped.x), static_cast<int>(clipped.y),
            static_cast<int>(clipped.w), static_cast<int>(clipped.h),
            t.borderRadius, 6, true);

        // Border (line)
        Color bc = focused_ ? t.focusColor : t.buttonBorder;
        ctx.line.SetColor(bc.r, bc.g, bc.b, bc.a);
        ctx.line.RoundedRectangle(
            static_cast<int>(clipped.x), static_cast<int>(clipped.y),
            static_cast<int>(clipped.w), static_cast<int>(clipped.h),
            t.borderRadius, 6, false);
    }

    // Text
    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetColor(enabled_ ? t.textColor : t.textDisabled);
    ctx.font.SetBatch(&ctx.text);
    float textW = ctx.font.GetTextWidth(text_.c_str());
    float tx;
    switch (align_)
    {
    case TextAlign::LEFT:   tx = abs.x + t.padding * 2; break;
    case TextAlign::RIGHT:  tx = abs.x + abs.w - textW - t.padding * 2; break;
    default:                tx = abs.x + (abs.w - textW) * 0.5f; break;
    }
    float ty = abs.y + (abs.h - t.fontSize) * 0.5f;

    Rect textRect = {tx, ty, textW, t.fontSize};
    if (!ctx.isClipped(textRect))
    {
        auto fc = toFontClip(ctx.clipRect());
        ctx.font.Print(text_.c_str(), tx, ty, &fc);
    }

    Widget::paint(ctx);
}

// ═════════════════════════════════════════════════════════════════════════════
//  ImageButton
// ═════════════════════════════════════════════════════════════════════════════

Widget::Vec2f ImageButton::sizeHint() const
{
    float w = rect_.w > 0 ? rect_.w : (srcRect_.width + iconPad_ * 2);
    float h = rect_.h > 0 ? rect_.h : (srcRect_.height + iconPad_ * 2);
    return {w, h};
}

void ImageButton::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        // Background
        Color bg;
        if (bgColor_.a > 0)
        {
            bg = bgColor_;
            if (pressed_)     { bg.r = bg.r * 3/4; bg.g = bg.g * 3/4; bg.b = bg.b * 3/4; }
            else if (hovered_){ bg.r = std::min(255, bg.r + 20); bg.g = std::min(255, bg.g + 20); bg.b = std::min(255, bg.b + 20); }
        }
        else
        {
            bg = pressed_ ? t.buttonPressed : (hovered_ ? t.buttonHover : t.buttonNormal);
        }
        if (!enabled_) bg = t.panelColor;

        ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
        ctx.fill.RoundedRectangle(
            static_cast<int>(clipped.x), static_cast<int>(clipped.y),
            static_cast<int>(clipped.w), static_cast<int>(clipped.h),
            t.borderRadius, 6, true);

        // Border
        Color bc = focused_ ? t.focusColor : t.buttonBorder;
        ctx.line.SetColor(bc.r, bc.g, bc.b, bc.a);
        ctx.line.RoundedRectangle(
            static_cast<int>(clipped.x), static_cast<int>(clipped.y),
            static_cast<int>(clipped.w), static_cast<int>(clipped.h),
            t.borderRadius, 6, false);
    }

    // Draw textured icon centered in the button
    if (tex_ && tex_->id && srcRect_.width > 0 && srcRect_.height > 0)
    {
        float dstW = abs.w - iconPad_ * 2;
        float dstH = abs.h - iconPad_ * 2;
        // Fit icon maintaining aspect ratio
        float scale = std::min(dstW / srcRect_.width, dstH / srcRect_.height);
        float iw = srcRect_.width * scale;
        float ih = srcRect_.height * scale;
        float ix = abs.x + (abs.w - iw) * 0.5f;
        float iy = abs.y + (abs.h - ih) * 0.5f;

        ctx.fill.SetColor(tint_.r, tint_.g, tint_.b, tint_.a);
        ctx.fill.DrawImageRegion(tex_,
            srcRect_.x, srcRect_.y, srcRect_.width, srcRect_.height,
            ix, iy, iw, ih,
            0, 0, 0, nullptr);
    }

    Widget::paint(ctx);
}

// ═════════════════════════════════════════════════════════════════════════════
//  IconButton
// ═════════════════════════════════════════════════════════════════════════════

IconButton::IconButton(Icon icon)
    : icon_(icon), targetIcon_(icon)
{
    acceptsFocus_ = true;
    cursor_ = CursorType::Hand;
}

void IconButton::setIcon(Icon icon)
{
    if (icon == targetIcon_) return;
    if (animated_)
    {
        icon_ = targetIcon_;   // current becomes "from"
        targetIcon_ = icon;
        animProgress_ = 0.0f;
    }
    else
    {
        icon_ = icon;
        targetIcon_ = icon;
        animProgress_ = 1.0f;
    }
    markDirty();
}

Widget::Vec2f IconButton::sizeHint() const
{
    return {32.0f, Theme::instance().buttonHeight};
}

void IconButton::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    // Animate
    if (animated_ && animProgress_ < 1.0f)
    {
        float dt = WidgetApp::instance().deltaTime();
        animProgress_ += dt * 4.0f;  // ~0.25s transition
        if (animProgress_ >= 1.0f)
        {
            animProgress_ = 1.0f;
            icon_ = targetIcon_;
        }
        markDirty();
    }

    // Background (same as Button)
    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        Color bg = pressed_ ? t.buttonPressed : (hovered_ ? t.buttonHover : t.buttonNormal);
        if (!enabled_) bg = t.panelColor;
        ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
        ctx.fill.RoundedRectangle(
            static_cast<int>(clipped.x), static_cast<int>(clipped.y),
            static_cast<int>(clipped.w), static_cast<int>(clipped.h),
            t.borderRadius, 6, true);

        Color bc = focused_ ? t.focusColor : t.buttonBorder;
        ctx.line.SetColor(bc.r, bc.g, bc.b, bc.a);
        ctx.line.RoundedRectangle(
            static_cast<int>(clipped.x), static_cast<int>(clipped.y),
            static_cast<int>(clipped.w), static_cast<int>(clipped.h),
            t.borderRadius, 6, false);
    }

    // Draw icon
    float progress = animProgress_;
    if (animated_ && progress < 1.0f)
    {
        // Cross-fade: draw old fading out, new fading in
        Color oldColor = iconColor_;
        oldColor.a = static_cast<unsigned char>(iconColor_.a * (1.0f - progress));
        Color newColor = iconColor_;
        newColor.a = static_cast<unsigned char>(iconColor_.a * progress);

        ctx.line.SetColor(oldColor.r, oldColor.g, oldColor.b, oldColor.a);
        drawIcon(ctx, abs, icon_, 1.0f - progress);
        ctx.line.SetColor(newColor.r, newColor.g, newColor.b, newColor.a);
        drawIcon(ctx, abs, targetIcon_, progress);
    }
    else
    {
        ctx.line.SetColor(iconColor_.r, iconColor_.g, iconColor_.b, iconColor_.a);
        drawIcon(ctx, abs, targetIcon_, 1.0f);
    }

    Widget::paint(ctx);
}

void IconButton::drawIcon(PaintContext& ctx, const Rect& abs, Icon icon, float t)
{
    // Icon area: centered in button, 14x14 px
    float iconSize = 14.0f;
    float cx = abs.x + abs.w * 0.5f;
    float cy = abs.y + abs.h * 0.5f;
    float hs = iconSize * 0.5f;  // half size

    float lw = lineWidth_;

    switch (icon)
    {
    case Hamburger:
    {
        // Three horizontal lines (≡)
        float left  = cx - hs;
        float right = cx + hs;
        float y0 = cy - hs + 1;
        float y1 = cy;
        float y2 = cy + hs - 1;
        ctx.fill.SetColor(iconColor_.r, iconColor_.g, iconColor_.b,
                          static_cast<unsigned char>(iconColor_.a * t));
        ctx.fill.Rectangle(static_cast<int>(left), static_cast<int>(y0 - lw * 0.5f),
                           static_cast<int>(iconSize), static_cast<int>(lw), true);
        ctx.fill.Rectangle(static_cast<int>(left), static_cast<int>(y1 - lw * 0.5f),
                           static_cast<int>(iconSize), static_cast<int>(lw), true);
        ctx.fill.Rectangle(static_cast<int>(left), static_cast<int>(y2 - lw * 0.5f),
                           static_cast<int>(iconSize), static_cast<int>(lw), true);
        break;
    }
    case Close:
    {
        // X mark
        float left  = cx - hs;
        float right = cx + hs;
        float top   = cy - hs;
        float bot   = cy + hs;
        ctx.line.Line2D(left, top, right, bot);
        ctx.line.Line2D(right, top, left, bot);
        break;
    }
    case ArrowLeft:
    {
        float left = cx - hs * 0.6f;
        float right = cx + hs * 0.6f;
        ctx.line.Line2D(right, cy - hs, left, cy);
        ctx.line.Line2D(left, cy, right, cy + hs);
        break;
    }
    case ArrowRight:
    {
        float left = cx - hs * 0.6f;
        float right = cx + hs * 0.6f;
        ctx.line.Line2D(left, cy - hs, right, cy);
        ctx.line.Line2D(right, cy, left, cy + hs);
        break;
    }
    case ArrowUp:
    {
        float top = cy - hs * 0.6f;
        float bot = cy + hs * 0.6f;
        ctx.line.Line2D(cx - hs, bot, cx, top);
        ctx.line.Line2D(cx, top, cx + hs, bot);
        break;
    }
    case ArrowDown:
    {
        float top = cy - hs * 0.6f;
        float bot = cy + hs * 0.6f;
        ctx.line.Line2D(cx - hs, top, cx, bot);
        ctx.line.Line2D(cx, bot, cx + hs, top);
        break;
    }
    case Plus:
    {
        ctx.line.Line2D(cx - hs, cy, cx + hs, cy);
        ctx.line.Line2D(cx, cy - hs, cx, cy + hs);
        break;
    }
    case Minus:
    {
        ctx.line.Line2D(cx - hs, cy, cx + hs, cy);
        break;
    }
    case Check:
    {
        // Checkmark ✓
        float x0 = cx - hs;
        float x1 = cx - hs * 0.3f;
        float x2 = cx + hs;
        float y0 = cy;
        float y1 = cy + hs * 0.6f;
        float y2 = cy - hs * 0.5f;
        ctx.line.Line2D(x0, y0, x1, y1);
        ctx.line.Line2D(x1, y1, x2, y2);
        break;
    }
    case Chevron:
    {
        float left = cx - hs * 0.4f;
        float right = cx + hs * 0.4f;
        ctx.line.Line2D(left, cy - hs, right, cy);
        ctx.line.Line2D(right, cy, left, cy + hs);
        break;
    }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Panel
// ═════════════════════════════════════════════════════════════════════════════

void Panel::setBgColor(const Color& c) { bgColor_ = c; }

void Panel::layout()
{
    // Fill all children to the panel's area
    for (auto* c : children_)
    {
        c->setRect({0, 0, rect_.w, rect_.h});
        c->layout();
    }
}

void Panel::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();

    // Cull if fully outside clip
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    // Clip panel drawing to current clip
    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        // Background (fill) — draw only the clipped portion
        ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
        ctx.fill.RoundedRectangle(
            static_cast<int>(clipped.x), static_cast<int>(clipped.y),
            static_cast<int>(clipped.w), static_cast<int>(clipped.h),
            t.borderRadius, 6, true);

        // Border (line)
        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
        ctx.line.RoundedRectangle(
            static_cast<int>(clipped.x), static_cast<int>(clipped.y),
            static_cast<int>(clipped.w), static_cast<int>(clipped.h),
            t.borderRadius, 6, false);
    }

    // Push clip rect for children — they must stay inside panel bounds
    ctx.pushClip(abs);
    for (auto& c : children_)
        c->paint(ctx);
    ctx.popClip();
}

// ═════════════════════════════════════════════════════════════════════════════
//  CheckBox
// ═════════════════════════════════════════════════════════════════════════════

CheckBox::CheckBox(const std::string& text) : text_(text)
{
    acceptsFocus_ = true;
    cursor_ = CursorType::Hand;
    // Toggle on click (uses base Widget::clicked signal)
    clicked.connect([this]() {
        checked_ = !checked_;
        toggled.emit(checked_);
        WidgetApp::instance().fireEvent("toggle", this);
        markDirty();
    });
}

void CheckBox::setText(const std::string& t) { text_ = t; markDirty(); }
void CheckBox::setChecked(bool c)            { checked_ = c; markDirty(); }

Widget::Vec2f CheckBox::sizeHint() const
{
    const auto& t = Theme::instance();
    float boxSize = t.fontSize;
    float textW = text_.size() * t.fontSize * 0.6f;
    return {boxSize + t.padding * 2 + textW, t.fontSize + t.padding * 2};
}

void CheckBox::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    float boxSize = t.fontSize;
    float bx = abs.x + t.padding;
    float by = abs.y + (abs.h - boxSize) * 0.5f;

    Rect boxRect = {bx, by, boxSize, boxSize};
    Rect clippedBox;
    if (ctx.clipRectIntersect(boxRect, clippedBox))
    {
        // Box background (fill) — hover-aware
        Color bg = hovered_ ? t.inputBgHover : t.inputBg;
        ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
        ctx.fill.Rectangle(static_cast<int>(clippedBox.x), static_cast<int>(clippedBox.y),
                            static_cast<int>(clippedBox.w), static_cast<int>(clippedBox.h), true);

        // Box border (line) — hover/focus-aware
        Color border = focused_ ? t.focusColor : (hovered_ ? t.inputBorderHover : t.inputBorder);
        ctx.line.SetColor(border.r, border.g, border.b, border.a);
        ctx.line.Rectangle(static_cast<int>(clippedBox.x), static_cast<int>(clippedBox.y),
                            static_cast<int>(clippedBox.w), static_cast<int>(clippedBox.h), false);

        // Check mark (fill)
        if (checked_)
        {
            float m = 3.0f;
            Rect markRect = {bx + m, by + m, boxSize - m * 2, boxSize - m * 2};
            Rect clippedMark;
            if (ctx.clipRectIntersect(markRect, clippedMark))
            {
                ctx.fill.SetColor(t.checkMark.r, t.checkMark.g, t.checkMark.b, t.checkMark.a);
                ctx.fill.Rectangle(static_cast<int>(clippedMark.x), static_cast<int>(clippedMark.y),
                                    static_cast<int>(clippedMark.w), static_cast<int>(clippedMark.h), true);
            }
        }
    }

    // Text
    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetColor(enabled_ ? t.textColor : t.textDisabled);
    ctx.font.SetBatch(&ctx.text);
    float tx = bx + boxSize + t.padding;
    float ty = abs.y + (abs.h - t.fontSize) * 0.5f;

    Rect textRect = {tx, ty, abs.w, t.fontSize};
    if (!ctx.isClipped(textRect))
    {
        auto fc = toFontClip(ctx.clipRect());
        ctx.font.Print(text_.c_str(), tx, ty, &fc);
    }

    Widget::paint(ctx);
}

// ═════════════════════════════════════════════════════════════════════════════
//  RadioGroup
// ═════════════════════════════════════════════════════════════════════════════

RadioGroup::~RadioGroup()
{
    // Detach all buttons so their destructors don't access this dead group
    for (auto* rb : buttons_)
        rb->group_ = nullptr;
}

void RadioGroup::add(RadioButton* rb)
{
    buttons_.push_back(rb);
}

void RadioGroup::remove(RadioButton* rb)
{
    buttons_.erase(std::remove(buttons_.begin(), buttons_.end(), rb), buttons_.end());
    if (selected_ == rb) selected_ = nullptr;
}

int RadioGroup::selectedIndex() const
{
    for (int i = 0; i < static_cast<int>(buttons_.size()); ++i)
        if (buttons_[i] == selected_) return i;
    return -1;
}

void RadioGroup::select(RadioButton* rb)
{
    if (selected_ == rb) return;

    // Deselect previous
    if (selected_)
    {
        selected_->selected_ = false;
        selected_->markDirty();
    }

    selected_ = rb;
    if (rb)
    {
        rb->selected_ = true;
        rb->markDirty();
        rb->toggled.emit(true);
    }

    selectionChanged.emit(selectedIndex());
}

// ═════════════════════════════════════════════════════════════════════════════
//  RadioButton
// ═════════════════════════════════════════════════════════════════════════════

RadioButton::RadioButton(const std::string& text, RadioGroup* group)
    : text_(text), group_(group)
{
    acceptsFocus_ = true;
    cursor_ = CursorType::Hand;

    if (group_) group_->add(this);

    clicked.connect([this]() {
        if (group_)
            group_->select(this);
        else
        {
            selected_ = true;
            toggled.emit(true);
            markDirty();
        }
    });
}

RadioButton::~RadioButton()
{
    if (group_) group_->remove(this);
}

void RadioButton::setSelected(bool s)
{
    if (s && group_)
        group_->select(this);
    else if (!s && selected_)
    {
        selected_ = false;
        toggled.emit(false);
        markDirty();
    }
}

void RadioButton::setGroup(RadioGroup* g)
{
    if (group_) group_->remove(this);
    group_ = g;
    if (group_) group_->add(this);
}

Widget::Vec2f RadioButton::sizeHint() const
{
    const auto& t = Theme::instance();
    float circSize = t.fontSize;
    float textW = text_.size() * t.fontSize * 0.6f;
    return {circSize + t.padding * 2 + textW, t.fontSize + t.padding * 2};
}

void RadioButton::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    float r = t.fontSize * 0.5f;
    float cx = abs.x + t.padding + r;
    float cy = abs.y + abs.h * 0.5f;

    // Outer circle — border
    Color border = focused_ ? t.focusColor : (hovered_ ? t.inputBorderHover : t.inputBorder);
    ctx.line.SetColor(border.r, border.g, border.b, border.a);
    ctx.lineCircle(cx, cy, r);

    // Inner fill
    Color bg = hovered_ ? t.inputBgHover : t.inputBg;
    ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
    ctx.fillCircle(cx, cy, r - 1.0f);

    // Selected dot
    if (selected_)
    {
        ctx.fill.SetColor(t.checkMark.r, t.checkMark.g, t.checkMark.b, t.checkMark.a);
        ctx.fillCircle(cx, cy, r * 0.45f);
    }

    // Text
    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetColor(enabled_ ? t.textColor : t.textDisabled);
    ctx.font.SetBatch(&ctx.text);
    float rtx = abs.x + t.padding + t.fontSize + t.padding;
    float rty = abs.y + (abs.h - t.fontSize) * 0.5f;

    Rect trRect = {rtx, rty, abs.w, t.fontSize};
    if (!ctx.isClipped(trRect))
    {
        auto fc = toFontClip(ctx.clipRect());
        ctx.font.Print(text_.c_str(), rtx, rty, &fc);
    }

    Widget::paint(ctx);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Switch — on/off toggle
// ═════════════════════════════════════════════════════════════════════════════

Switch::Switch(const std::string& text) : text_(text)
{
    acceptsFocus_ = true;
    cursor_ = CursorType::Hand;

    clicked.connect([this]() {
        on_ = !on_;
        toggled.emit(on_);
        markDirty();
    });
}

void Switch::setOn(bool v)
{
    if (on_ != v) { on_ = v; toggled.emit(on_); markDirty(); }
}

Widget::Vec2f Switch::sizeHint() const
{
    const auto& t = Theme::instance();
    float trackW = 36.0f;
    float trackH = 18.0f;
    float textW  = text_.size() * t.fontSize * 0.6f;
    float w = trackW + (text_.empty() ? 0 : t.padding + textW);
    return {w + t.padding * 2, std::max(trackH, t.fontSize) + t.padding * 2};
}

void Switch::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    float trackW = 36.0f;
    float trackH = 18.0f;
    float swx = abs.x + t.padding;
    float swy = abs.y + (abs.h - trackH) * 0.5f;
    float thumbR = trackH * 0.5f - 2.0f;

    // Track background (rounded rect)
    Color trackCol = on_ ? onColor_ : offColor_;
    if (hovered_)
    {
        trackCol.r = static_cast<u8>(std::min(255, trackCol.r + 15));
        trackCol.g = static_cast<u8>(std::min(255, trackCol.g + 15));
        trackCol.b = static_cast<u8>(std::min(255, trackCol.b + 15));
    }
    ctx.fill.SetColor(trackCol.r, trackCol.g, trackCol.b, trackCol.a);
    ctx.fillRoundedRect(swx, swy, trackW, trackH, trackH * 0.5f, 8);

    // Track border
    Color brd = focused_ ? t.focusColor : (hovered_ ? t.inputBorderHover : t.inputBorder);
    ctx.line.SetColor(brd.r, brd.g, brd.b, brd.a);
    ctx.lineRoundedRect(swx, swy, trackW, trackH, trackH * 0.5f, 8);

    // Thumb circle
    float thumbX = on_ ? (swx + trackW - trackH * 0.5f) : (swx + trackH * 0.5f);
    float thumbY = swy + trackH * 0.5f;
    ctx.fill.SetColor(t.switchThumb.r, t.switchThumb.g, t.switchThumb.b, t.switchThumb.a);
    ctx.fillCircle(thumbX, thumbY, thumbR);

    // Text label
    if (!text_.empty())
    {
        ctx.font.SetFontSize(t.fontSize);
        ctx.font.SetColor(enabled_ ? t.textColor : t.textDisabled);
        ctx.font.SetBatch(&ctx.text);
        float lx = swx + trackW + t.padding;
        float ly = abs.y + (abs.h - t.fontSize) * 0.5f;

        Rect textRect = {lx, ly, abs.w, t.fontSize};
        if (!ctx.isClipped(textRect))
        {
            auto fc = toFontClip(ctx.clipRect());
            ctx.font.Print(text_.c_str(), lx, ly, &fc);
        }
    }

    Widget::paint(ctx);
}
