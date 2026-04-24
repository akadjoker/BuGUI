#include "Widgets.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include "Texture.hpp"
#include "Pixmap.hpp"
#include <cmath>
#include <algorithm>

// Static arrow icon atlas texture (64x32 RGBA, left arrow | right arrow)
unsigned int Carousel::arrowTexId_ = 0;

// ═════════════════════════════════════════════════════════════════════════════
//  Canvas
// ═════════════════════════════════════════════════════════════════════════════

void Canvas::layout()
{
    for (auto* c : children_)
        c->layout();
}

void Canvas::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    // Background
    Rect clipped;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
        ctx.fill.Rectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                           static_cast<int>(clipped.w), static_cast<int>(clipped.h), true);

        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
        ctx.line.Rectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                           static_cast<int>(clipped.w), static_cast<int>(clipped.h), false);
    }

    // Clip children + custom paint
    ctx.pushClip(abs);

    // User custom paint callback
    if (onPaint_)
        onPaint_(ctx, abs);

    // Children
    for (auto& c : children_)
        c->paint(ctx);

    ctx.popClip();
}

// ═════════════════════════════════════════════════════════════════════════════
//  ImageView
// ═════════════════════════════════════════════════════════════════════════════

void ImageView::setTexture(Texture* tex)
{
    texture_ = tex;
    if (tex && rect_.w == 0 && rect_.h == 0)
        setSize(static_cast<float>(tex->width), static_cast<float>(tex->height));
}

void ImageView::paint(PaintContext& ctx)
{
    if (!visible_ || !texture_) return;

    Rect abs = absoluteRect();
    // Apply offset (drag position)
    float dx = abs.x + offsetX_;
    float dy = abs.y + offsetY_;
    float dw = static_cast<float>(texture_->width);
    float dh = static_cast<float>(texture_->height);

    // Get clip from PaintContext
    const Rect& cr = ctx.clipRect();
    float clip[4] = {cr.x, cr.y, cr.w, cr.h};

    // Draw using fillBatch (TRIANGLES mode, soft-clipped)
    ctx.fill.DrawImageEx(texture_, dx, dy, dw, dh, angle_, dw * 0.5f, dh * 0.5f, clip);

    Widget::paint(ctx);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Carousel
// ═════════════════════════════════════════════════════════════════════════════

Carousel::Carousel()
{
    cursor_ = CursorType::Hand;
}

void Carousel::ensureArrowTexture()
{
    if (arrowTexId_ != 0) return;

    // Create a 64x32 RGBA atlas: left arrow [0..31] | right arrow [32..63]
    constexpr int S = kIconSize;  // 32
    Pixmap pix(S * 2, S, 4);
    pix.Fill(0, 0, 0, 0); // transparent

    // Semi-transparent dark circle backgrounds
    Color bg(0, 0, 0, 140);
    pix.DrawCircle(S / 2, S / 2, S / 2 - 1, bg, true);
    pix.DrawCircle(S + S / 2, S / 2, S / 2 - 1, bg, true);

    // White chevron lines
    Color fg(230, 230, 235, 240);
    // Left chevron: tip at (10, 16), top at (20, 7), bottom at (20, 25)
    pix.DrawLine(20, 7,  10, 16, fg);
    pix.DrawLine(10, 16, 20, 25, fg);
    // Thicken: offset by 1 pixel
    pix.DrawLine(21, 7,  11, 16, fg);
    pix.DrawLine(11, 16, 21, 25, fg);

    // Right chevron: tip at (S+22, 16), top at (S+12, 7), bottom at (S+12, 25)
    pix.DrawLine(S + 12, 7,  S + 22, 16, fg);
    pix.DrawLine(S + 22, 16, S + 12, 25, fg);
    // Thicken
    pix.DrawLine(S + 11, 7,  S + 21, 16, fg);
    pix.DrawLine(S + 21, 16, S + 11, 25, fg);

    Texture* tex = CreateTextureFromPixmap("__carousel_arrows__", pix);
    if (tex) {
        arrowTexId_ = tex->id;
        tex->id = 0; // prevent Texture destructor from deleting the GL texture
        delete tex;
    }
}

void Carousel::addPageWidget(Widget* page)
{
    pages_.push_back(page);
    addChild(page);
    if (pages_.size() == 1)
        currentPage_ = 0;
    markDirty();
    WidgetApp::instance().requestLayout();
}

void Carousel::removePage(int index)
{
    if (index < 0 || index >= static_cast<int>(pages_.size())) return;
    removeChild(pages_[index]);
    pages_.erase(pages_.begin() + index);
    if (currentPage_ >= static_cast<int>(pages_.size()))
        currentPage_ = std::max(0, static_cast<int>(pages_.size()) - 1);
    markDirty();
    WidgetApp::instance().requestLayout();
}

void Carousel::startAnim(int from, int to, int dir)
{
    animFrom_     = from;
    animDir_      = dir;
    animProgress_ = 0.0f;
    animating_    = true;
    currentPage_  = to;
    autoTimer_    = 0.0f;
    pageChanged.emit(to);
    WidgetApp::instance().requestLayout();
}

void Carousel::setCurrentPage(int idx)
{
    if (idx < 0 || idx >= static_cast<int>(pages_.size())) return;
    if (idx == currentPage_ && !animating_) return;

    int dir = (idx > currentPage_) ? 1 : -1;
    startAnim(currentPage_, idx, dir);
}

void Carousel::next()
{
    if (pages_.empty()) return;
    int n = static_cast<int>(pages_.size());
    if (currentPage_ < n - 1)
        startAnim(currentPage_, currentPage_ + 1, 1);
    else if (loop_)
        startAnim(currentPage_, 0, 1);
}

void Carousel::prev()
{
    if (pages_.empty()) return;
    int n = static_cast<int>(pages_.size());
    if (currentPage_ > 0)
        startAnim(currentPage_, currentPage_ - 1, -1);
    else if (loop_)
        startAnim(currentPage_, n - 1, -1);
}

Rect Carousel::contentRect() const
{
    float dh = showDots_ ? dotsHeight() : 0;
    return {0, 0, rect_.w, rect_.h - dh};
}

Rect Carousel::arrowRect(bool rightSide) const
{
    Rect cr = contentRect();
    float aw = 36.0f;
    float ah = 60.0f;
    float y = cr.y + (cr.h - ah) * 0.5f;
    if (rightSide)
        return {cr.x + cr.w - aw - 4.0f, y, aw, ah};
    else
        return {cr.x + 4.0f, y, aw, ah};
}

Widget::Vec2f Carousel::sizeHint() const
{
    float w = 200, h = 150;
    for (auto* p : pages_)
    {
        auto hint = p->sizeHint();
        if (hint.x > w) w = hint.x;
        if (hint.y > h) h = hint.y;
    }
    if (showDots_) h += dotsHeight();
    return {w, h};
}

void Carousel::layout()
{
    Rect cr = contentRect();
    int n = static_cast<int>(pages_.size());

    for (int i = 0; i < n; ++i)
    {
        if (animating_)
        {
            // Eased progress
            float t = animProgress_;
            t = t * t * (3.0f - 2.0f * t);  // smoothstep

            if (i == currentPage_)
            {
                // New page sliding in
                float off = animDir_ * (1.0f - t) * cr.w;
                pages_[i]->setVisible(true);
                pages_[i]->setRect({cr.x + off, cr.y, cr.w, cr.h});
                pages_[i]->layout();
            }
            else if (i == animFrom_)
            {
                // Old page sliding out
                float off = -animDir_ * t * cr.w;
                pages_[i]->setVisible(true);
                pages_[i]->setRect({cr.x + off, cr.y, cr.w, cr.h});
                pages_[i]->layout();
            }
            else
            {
                pages_[i]->setVisible(false);
            }
        }
        else
        {
            if (i == currentPage_)
            {
                pages_[i]->setVisible(true);
                pages_[i]->setRect(cr);
                pages_[i]->layout();
            }
            else
            {
                pages_[i]->setVisible(false);
            }
        }
    }
}

void Carousel::paint(PaintContext& ctx)
{
    if (!visible_) return;
    ensureArrowTexture();

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    float dt = WidgetApp::instance().deltaTime();
    int n = static_cast<int>(pages_.size());

    // ── Advance animation ────────────────────────────────────────────────
    if (animating_)
    {
        animProgress_ += dt / animDuration_;
        if (animProgress_ >= 1.0f)
        {
            animProgress_ = 1.0f;
            animating_    = false;
            animFrom_     = -1;
            // Re-layout to settle pages
            WidgetApp::instance().requestLayout();
        }
        else
        {
            // Keep laying out during animation
            layout();
        }
    }

    // ── Auto-play ────────────────────────────────────────────────────────
    if (autoPlay_ && !animating_ && n > 1)
    {
        autoTimer_ += dt;
        if (autoTimer_ >= autoInterval_)
        {
            autoTimer_ = 0;
            next();
        }
    }

    // ── Content area (clip pages) ────────────────────────────────────────
    Rect cr = contentRect();
    Rect absCr = {abs.x + cr.x, abs.y + cr.y, cr.w, cr.h};
    ctx.pushClip(absCr);

    for (auto* p : pages_)
        if (p->isVisible()) p->paint(ctx);

    ctx.popClip();

    // ── Arrow buttons (drawn via text batch so they always overlay) ──────
    if (showArrows_ && n > 1 && arrowTexId_ != 0)
    {
        auto paintArrow = [&](bool rightSide) {
            Rect ar = arrowRect(rightSide);
            float ax = abs.x + ar.x;
            float ay = abs.y + ar.y;

            // UV coords: left icon = [0..0.5], right icon = [0.5..1]
            float u0 = rightSide ? 0.5f : 0.0f;
            float u1 = rightSide ? 1.0f : 0.5f;

            bool hov = (rightSide && hoveredArrow_ == 1) ||
                       (!rightSide && hoveredArrow_ == -1);
            u8 alpha = hov ? 255 : 140;

            ctx.text.SetColor(255, 255, 255, alpha);
            ctx.text.TexturedQuad(
                {ax,          ay},
                {ax + ar.w,   ay},
                {ax + ar.w,   ay + ar.h},
                {ax,          ay + ar.h},
                {u0, 0.0f}, {u1, 0.0f}, {u1, 1.0f}, {u0, 1.0f},
                arrowTexId_);
        };

        bool canPrev = loop_ || currentPage_ > 0;
        bool canNext = loop_ || currentPage_ < n - 1;
        if (canPrev) paintArrow(false);
        if (canNext) paintArrow(true);
    }

    // ── Dot indicators (text batch so they overlay page content) ─────
    if (showDots_ && n > 1)
    {
        float dh = dotsHeight();
        float dotR  = 4.0f;
        float gap   = 12.0f;
        float totalW = n * dotR * 2 + (n - 1) * gap;
        float startX = abs.x + (abs.w - totalW) * 0.5f;
        float dotY   = abs.y + rect_.h - dh + (dh - dotR * 2) * 0.5f;

        for (int i = 0; i < n; ++i)
        {
            float dx = startX + i * (dotR * 2 + gap);
            bool active = (i == currentPage_);

            if (active)
                ctx.text.SetColor(t.carouselDotActive.r, t.carouselDotActive.g, t.carouselDotActive.b, t.carouselDotActive.a);
            else
                ctx.text.SetColor(t.carouselDotInactive.r, t.carouselDotInactive.g, t.carouselDotInactive.b, t.carouselDotInactive.a);

            ctx.text.Circle(
                static_cast<int>(dx + dotR), static_cast<int>(dotY + dotR),
                dotR, true);
        }
    }
}

void Carousel::onMousePress(MouseEvent& e)
{
    if (!enabled_) return;
    if (animating_) return;

    Rect abs = absoluteRect();
    int n = static_cast<int>(pages_.size());

    // Check arrow clicks
    if (showArrows_ && n > 1)
    {
        // Left arrow
        Rect la = arrowRect(false);
        Rect absLa = {abs.x + la.x, abs.y + la.y, la.w, la.h};
        if (absLa.contains(e.x, e.y))
        {
            prev();
            e.consumed = true;
            return;
        }

        // Right arrow
        Rect ra = arrowRect(true);
        Rect absRa = {abs.x + ra.x, abs.y + ra.y, ra.w, ra.h};
        if (absRa.contains(e.x, e.y))
        {
            next();
            e.consumed = true;
            return;
        }
    }

    // Check dot clicks
    if (showDots_ && n > 1)
    {
        float dh = dotsHeight();
        float dotR = 4.0f;
        float gap  = 12.0f;
        float totalW = n * dotR * 2 + (n - 1) * gap;
        float startX = abs.x + (abs.w - totalW) * 0.5f;
        float dotY   = abs.y + rect_.h - dh;

        for (int i = 0; i < n; ++i)
        {
            float dx = startX + i * (dotR * 2 + gap);
            Rect dr = {dx - 4, dotY, dotR * 2 + 8, dh};
            if (dr.contains(e.x, e.y))
            {
                setCurrentPage(i);
                e.consumed = true;
                return;
            }
        }
    }
}

void Carousel::onMouseMove(MouseEvent& e)
{
    Rect abs = absoluteRect();
    int n = static_cast<int>(pages_.size());
    int old = hoveredArrow_;
    hoveredArrow_ = 0;

    if (showArrows_ && n > 1)
    {
        Rect la = arrowRect(false);
        Rect absLa = {abs.x + la.x, abs.y + la.y, la.w, la.h};
        if (absLa.contains(e.x, e.y)) { hoveredArrow_ = -1; }

        Rect ra = arrowRect(true);
        Rect absRa = {abs.x + ra.x, abs.y + ra.y, ra.w, ra.h};
        if (absRa.contains(e.x, e.y)) { hoveredArrow_ = 1; }
    }

    if (hoveredArrow_ != old) markDirty();
    Widget::onMouseMove(e);
}

void Carousel::onMouseLeave()
{
    if (hoveredArrow_ != 0) { hoveredArrow_ = 0; markDirty(); }
    Widget::onMouseLeave();
}

// ═════════════════════════════════════════════════════════════════════════════
//  FloatWindow — draggable floating panel
// ═════════════════════════════════════════════════════════════════════════════

FloatWindow::FloatWindow(const std::string& title)
    : title_(title)
{
}

FloatWindow::~FloatWindow()
{
    // content_ is in children_, Widget::~Widget() handles deletion
}

void FloatWindow::setContentWidget(Widget* w)
{
    if (content_)
        removeChild(content_);   // removeChild deletes it
    content_ = w;
    if (content_) addChild(content_);
    markDirty();
}

void FloatWindow::setMinimized(bool m)
{
    if (minimized_ != m)
    {
        minimized_ = m;
        minimizedChanged.emit(m);
        markDirty();
    }
}

// ── Geometry helpers ─────────────────────────────────────────────────────────

Rect FloatWindow::titleBarRect() const
{
    return {floatX_, floatY_, floatW_, titleBarH_};
}

Rect FloatWindow::contentRect() const
{
    return {floatX_, floatY_ + titleBarH_, floatW_, floatH_ - titleBarH_};
}

Rect FloatWindow::closeButtonRect() const
{
    float btnW = titleBarH_;
    return {floatX_ + floatW_ - btnW, floatY_, btnW, titleBarH_};
}

Rect FloatWindow::minimizeButtonRect() const
{
    float btnW = titleBarH_;
    float offset = closable_ ? btnW : 0.0f;
    return {floatX_ + floatW_ - offset - btnW, floatY_, btnW, titleBarH_};
}

Rect FloatWindow::resizeGripRect() const
{
    return {floatX_ + floatW_ - kResizeGrip,
            floatY_ + floatH_ - kResizeGrip,
            kResizeGrip, kResizeGrip};
}

Widget::Vec2f FloatWindow::sizeHint() const
{
    return {floatW_, floatH_};
}

// ── Layout ───────────────────────────────────────────────────────────────────

void FloatWindow::layout()
{
    // Set own rect to absolute float position
    setRect({floatX_, floatY_, floatW_, minimized_ ? titleBarH_ : floatH_});

    if (content_ && !minimized_)
    {
        // Content rect is LOCAL to the FloatWindow (not absolute),
        // because absoluteRect() will add parent's position automatically.
        float pad = 2.0f;
        content_->setRect({pad, titleBarH_ + pad,
                           floatW_ - pad * 2, floatH_ - titleBarH_ - pad * 2});
        content_->layout();
    }
}

// ── Paint ────────────────────────────────────────────────────────────────────

void FloatWindow::paint(PaintContext& ctx)
{
    const auto& t = Theme::instance();
    float fh = minimized_ ? titleBarH_ : floatH_;

    // Window background + border
    ctx.fill.SetColor(t.floatBg.r, t.floatBg.g, t.floatBg.b, t.floatBg.a);
    ctx.fill.RoundedRectangle(
        static_cast<int>(floatX_), static_cast<int>(floatY_),
        static_cast<int>(floatW_), static_cast<int>(fh),
        t.borderRadius, 4, true);

    ctx.line.SetColor(t.floatBorder.r, t.floatBorder.g, t.floatBorder.b, t.floatBorder.a);
    ctx.line.RoundedRectangle(
        static_cast<int>(floatX_), static_cast<int>(floatY_),
        static_cast<int>(floatW_), static_cast<int>(fh),
        t.borderRadius, 4, false);

    // Title bar background
    ctx.fill.SetColor(t.floatTitleBg.r, t.floatTitleBg.g, t.floatTitleBg.b, t.floatTitleBg.a);
    ctx.fill.RoundedRectangle(
        static_cast<int>(floatX_), static_cast<int>(floatY_),
        static_cast<int>(floatW_), static_cast<int>(titleBarH_),
        t.borderRadius, 4, true);

    // Title text
    ctx.font.SetFontSize(t.fontSize * 0.9f);
    ctx.font.SetColor(t.floatTitleText);
    ctx.font.SetBatch(&ctx.text);
    float textY = floatY_ + (titleBarH_ - t.fontSize * 0.9f) * 0.5f;
    ctx.font.Print(title_.c_str(), floatX_ + 8.0f, textY);

    // Close button
    if (closable_)
    {
        Rect cb = closeButtonRect();
        if (hoveredBtn_ == 1)
        {
            ctx.fill.SetColor(t.floatCloseHover.r, t.floatCloseHover.g,
                              t.floatCloseHover.b, t.floatCloseHover.a);
            ctx.fill.Rectangle(static_cast<int>(cb.x), static_cast<int>(cb.y),
                               static_cast<int>(cb.w), static_cast<int>(cb.h), true);
        }
        // X icon
        float cx = cb.x + cb.w * 0.5f, cy = cb.y + cb.h * 0.5f;
        float s = 5.0f;
        ctx.line.SetColor(t.floatTitleText.r, t.floatTitleText.g,
                          t.floatTitleText.b, t.floatTitleText.a);
        ctx.line.Line2D(cx - s, cy - s, cx + s, cy + s);
        ctx.line.Line2D(cx + s, cy - s, cx - s, cy + s);
    }

    // Minimize button
    if (minimizable_)
    {
        Rect mb = minimizeButtonRect();
        if (hoveredBtn_ == 2)
        {
            ctx.fill.SetColor(t.floatBtnHover.r, t.floatBtnHover.g,
                              t.floatBtnHover.b, t.floatBtnHover.a);
            ctx.fill.Rectangle(static_cast<int>(mb.x), static_cast<int>(mb.y),
                               static_cast<int>(mb.w), static_cast<int>(mb.h), true);
        }
        // — icon (minimize) or ▢ icon (restore)
        float mx = mb.x + mb.w * 0.5f, my = mb.y + mb.h * 0.5f;
        float s = 5.0f;
        ctx.line.SetColor(t.floatTitleText.r, t.floatTitleText.g,
                          t.floatTitleText.b, t.floatTitleText.a);
        if (minimized_)
        {
            // ▢ restore icon
            ctx.line.Rectangle(static_cast<int>(mx - s), static_cast<int>(my - s),
                               static_cast<int>(s * 2), static_cast<int>(s * 2), false);
        }
        else
        {
            // — minimize icon
            ctx.line.Line2D(mx - s, my, mx + s, my);
        }
    }

    // Content — flush before+after so float chrome and content
    // are in separate draw calls (correct Z-order)
    if (content_ && !minimized_)
    {
        ctx.fill.Render();
        ctx.line.Render();
        ctx.text.Render();

        Rect cr = contentRect();
        ctx.pushClip(cr);
        content_->paint(ctx);
        ctx.popClip();

        ctx.fill.Render();
        ctx.line.Render();
        ctx.text.Render();
    }

    // Resize grip (bottom-right corner)
    if (resizable_ && !minimized_)
    {
        float gx = floatX_ + floatW_ - 2.0f;
        float gy = floatY_ + floatH_ - 2.0f;
        ctx.line.SetColor(t.floatBorder.r, t.floatBorder.g,
                          t.floatBorder.b, t.floatBorder.a);
        ctx.line.Line2D(gx - 10, gy, gx, gy - 10);
        ctx.line.Line2D(gx - 6, gy, gx, gy - 6);
        ctx.line.Line2D(gx - 2, gy, gx, gy - 2);
    }
}

// ── Mouse handling ───────────────────────────────────────────────────────────

void FloatWindow::onMousePress(MouseEvent& e)
{
    if (e.button != 0) return;

    // Close button
    if (closable_ && closeButtonRect().contains(e.x, e.y))
    {
        hoveredBtn_ = 0;
        setVisible(false);
        closed.emit();
        e.consumed = true;
        return;
    }

    // Minimize button
    if (minimizable_ && minimizeButtonRect().contains(e.x, e.y))
    {
        setMinimized(!minimized_);
        e.consumed = true;
        return;
    }

    // Resize grip
    if (resizable_ && !minimized_ && resizeGripRect().contains(e.x, e.y))
    {
        resizing_ = true;
        resizeOffX_ = e.x;
        resizeOffY_ = e.y;
        resizeStartW_ = floatW_;
        resizeStartH_ = floatH_;
        e.consumed = true;
        return;
    }

    // Title bar drag
    if (titleBarRect().contains(e.x, e.y))
    {
        dragging_ = true;
        dragOffX_ = e.x - floatX_;
        dragOffY_ = e.y - floatY_;
        e.consumed = true;
        return;
    }

    // Forward to content
    if (content_ && !minimized_)
        content_->onMousePress(e);
}

void FloatWindow::onMouseRelease(MouseEvent& e)
{
    if (dragging_)  { dragging_ = false; e.consumed = true; return; }
    if (resizing_)  { resizing_ = false; e.consumed = true; return; }
    if (content_ && !minimized_)
        content_->onMouseRelease(e);
}

void FloatWindow::onMouseMove(MouseEvent& e)
{
    if (dragging_)
    {
        floatX_ = e.x - dragOffX_;
        floatY_ = e.y - dragOffY_;
        layout();
        markDirty();
        e.consumed = true;
        return;
    }

    if (resizing_)
    {
        floatW_ = std::max(120.0f, resizeStartW_ + (e.x - resizeOffX_));
        floatH_ = std::max(titleBarH_ + 20.0f, resizeStartH_ + (e.y - resizeOffY_));
        layout();
        markDirty();
        e.consumed = true;
        return;
    }

    // Button hover tracking
    int oldBtn = hoveredBtn_;
    hoveredBtn_ = 0;
    if (closable_ && closeButtonRect().contains(e.x, e.y))
        hoveredBtn_ = 1;
    else if (minimizable_ && minimizeButtonRect().contains(e.x, e.y))
        hoveredBtn_ = 2;
    if (hoveredBtn_ != oldBtn) markDirty();

    if (content_ && !minimized_)
        content_->onMouseMove(e);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SidePanel — slide-in drawer
// ═════════════════════════════════════════════════════════════════════════════

SidePanel::SidePanel(Side side, float panelWidth)
    : side_(side), panelWidth_(panelWidth)
{
    setVisible(false);  // start closed, invisible to hitTest
}

void SidePanel::setContentWidget(Widget* w)
{
    if (content_)
        removeChild(content_);
    content_ = w;
    if (w) addChild(w);
    markDirty();
}

void SidePanel::open()
{
    if (open_ && !animating_) return;
    open_ = true;
    animating_ = true;
    animDir_ = 1;
    setVisible(true);  // make hittable
    openChanged.emit(true);
}

void SidePanel::close()
{
    if (!open_ && !animating_) return;
    open_ = false;
    animating_ = true;
    animDir_ = -1;
    openChanged.emit(false);
}

void SidePanel::toggle()
{
    if (open_) close(); else open();
}

void SidePanel::update(float dt)
{
    if (!animating_) return;

    float speed = 1.0f / animDuration_;
    animProgress_ += animDir_ * speed * dt;

    if (animProgress_ >= 1.0f) { animProgress_ = 1.0f; animating_ = false; }
    if (animProgress_ <= 0.0f) { animProgress_ = 0.0f; animating_ = false; setVisible(false); }
    markDirty();
}

float SidePanel::currentOffset() const
{
    // Ease out cubic
    float t = animProgress_;
    float eased = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    return eased * panelWidth_;
}

Widget::Vec2f SidePanel::sizeHint() const
{
    return {0, 0};  // overlays parent, no size contribution
}

void SidePanel::layout()
{
    if (!content_) return;

    Rect abs = absoluteRect();
    float offset = currentOffset();

    if (side_ == Left)
        content_->setRect({abs.x - panelWidth_ + offset, abs.y, panelWidth_, abs.h});
    else
        content_->setRect({abs.x + abs.w - offset, abs.y, panelWidth_, abs.h});

    content_->layout();
}

void SidePanel::paint(PaintContext& ctx)
{
    // Advance animation
    update(WidgetApp::instance().deltaTime());

    if (animProgress_ <= 0.0f) return;  // fully closed, skip

    // Re-layout content with current animation offset
    layout();

    // Flush previous batches so the drawer renders on top
    ctx.fill.Render();
    ctx.line.Render();
    ctx.text.Render();

    Rect abs = absoluteRect();
    const auto& t = Theme::instance();
    float offset = currentOffset();

    // Scrim (dark overlay behind drawer)
    float scrimAlpha = scrimColor_.a * animProgress_;
    ctx.fill.SetColor(scrimColor_.r, scrimColor_.g, scrimColor_.b,
                      static_cast<unsigned char>(scrimAlpha));
    ctx.fill.Rectangle(static_cast<int>(abs.x), static_cast<int>(abs.y),
                       static_cast<int>(abs.w), static_cast<int>(abs.h), true);

    // Panel background
    float px = (side_ == Left)
        ? abs.x - panelWidth_ + offset
        : abs.x + abs.w - offset;

    ctx.fill.SetColor(t.drawerBg.r, t.drawerBg.g, t.drawerBg.b, t.drawerBg.a);
    ctx.fill.Rectangle(static_cast<int>(px), static_cast<int>(abs.y),
                       static_cast<int>(panelWidth_), static_cast<int>(abs.h), true);

    // Border
    float bx = (side_ == Left) ? px + panelWidth_ : px;
    ctx.line.SetColor(t.drawerBorder.r, t.drawerBorder.g, t.drawerBorder.b, t.drawerBorder.a);
    ctx.line.Line2D(bx, abs.y, bx, abs.y + abs.h);

    // Content
    if (content_)
    {
        Rect clip = {px, abs.y, panelWidth_, abs.h};
        ctx.pushClip(clip);
        content_->paint(ctx);
        ctx.popClip();
    }

    // Flush drawer so it's a separate draw call on top
    ctx.fill.Render();
    ctx.line.Render();
    ctx.text.Render();
}

void SidePanel::onMousePress(MouseEvent& e)
{
    if (animProgress_ <= 0.0f) return;

    Rect abs = absoluteRect();
    float offset = currentOffset();
    float px = (side_ == Left)
        ? abs.x - panelWidth_ + offset
        : abs.x + abs.w - offset;

    Rect panelRect = {px, abs.y, panelWidth_, abs.h};

    if (panelRect.contains(e.x, e.y))
    {
        // Inside panel — forward to content
        if (content_) content_->onMousePress(e);
        e.consumed = true;
    }
    else
    {
        // Click on scrim — close
        close();
        e.consumed = true;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  PageView — shows one child ("page") at a time
// ═════════════════════════════════════════════════════════════════════════════

void PageView::addPageImpl(Widget* w)
{
    addChild(w);
    w->setVisible(false);  // hidden until selected
    pages_.push_back(w);

    // Auto-select first page
    if (pages_.size() == 1)
    {
        currentIdx_ = 0;
        w->setVisible(true);
    }
}

Widget* PageView::page(int idx) const
{
    if (idx < 0 || idx >= static_cast<int>(pages_.size())) return nullptr;
    return pages_[idx];
}

void PageView::setPage(int idx, Transition tr)
{
    if (idx < 0 || idx >= static_cast<int>(pages_.size())) return;
    if (idx == currentIdx_ && transProgress_ >= 1.0f) return;

    prevIdx_ = currentIdx_;
    currentIdx_ = idx;

    // Determine transition type
    if (tr == SlideAuto)
    {
        if (prevIdx_ < 0)
            tr = None;
        else
            tr = (idx > prevIdx_) ? SlideLeft : SlideRight;
    }
    transType_ = tr;
    transProgress_ = (tr == None) ? 1.0f : 0.0f;

    // Show new page
    if (currentIdx_ >= 0 && currentIdx_ < static_cast<int>(pages_.size()))
        pages_[currentIdx_]->setVisible(true);

    // During transition, old page stays visible
    // After transition completes, old page is hidden (in paint)

    pageChanged.emit(currentIdx_);
    markDirty();
    WidgetApp::instance().requestLayout();
}

void PageView::setPage(Widget* pg, Transition tr)
{
    for (int i = 0; i < static_cast<int>(pages_.size()); i++)
    {
        if (pages_[i] == pg) { setPage(i, tr); return; }
    }
}

void PageView::layout()
{
    // All pages get the full rect
    for (auto* p : pages_)
    {
        p->setRect({0, 0, rect_.w, rect_.h});
        if (p->isVisible())
            p->layout();
    }
}

void PageView::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    float dt = WidgetApp::instance().deltaTime();

    // Advance transition
    if (transProgress_ < 1.0f)
    {
        float speed = (transDuration_ > 0.0f) ? (1.0f / transDuration_) : 10.0f;
        transProgress_ += speed * dt;
        if (transProgress_ >= 1.0f)
        {
            transProgress_ = 1.0f;
            // Hide old page
            if (prevIdx_ >= 0 && prevIdx_ < static_cast<int>(pages_.size()) && prevIdx_ != currentIdx_)
                pages_[prevIdx_]->setVisible(false);
            prevIdx_ = -1;
        }
        markDirty();
    }

    ctx.pushClip(abs);

    bool animating = (transProgress_ < 1.0f && prevIdx_ >= 0);

    if (!animating)
    {
        // Just paint current page
        if (currentIdx_ >= 0 && currentIdx_ < static_cast<int>(pages_.size()))
            pages_[currentIdx_]->paint(ctx);
    }
    else
    {
        // Ease out cubic
        float t = transProgress_;
        float eased = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

        Widget* oldPage = pages_[prevIdx_];
        Widget* newPage = pages_[currentIdx_];

        switch (transType_)
        {
        case SlideLeft:
        {
            // Old slides out left, new slides in from right
            float oldX = -abs.w * eased;
            float newX = abs.w * (1.0f - eased);

            // Temporarily offset for painting
            Rect oldR = oldPage->rect();
            Rect newR = newPage->rect();
            oldPage->setRect({oldR.x + oldX, oldR.y, oldR.w, oldR.h});
            newPage->setRect({newR.x + newX, newR.y, newR.w, newR.h});

            oldPage->paint(ctx);
            newPage->paint(ctx);

            // Restore
            oldPage->setRect(oldR);
            newPage->setRect(newR);
            break;
        }
        case SlideRight:
        {
            float oldX = abs.w * eased;
            float newX = -abs.w * (1.0f - eased);

            Rect oldR = oldPage->rect();
            Rect newR = newPage->rect();
            oldPage->setRect({oldR.x + oldX, oldR.y, oldR.w, oldR.h});
            newPage->setRect({newR.x + newX, newR.y, newR.w, newR.h});

            oldPage->paint(ctx);
            newPage->paint(ctx);

            oldPage->setRect(oldR);
            newPage->setRect(newR);
            break;
        }
        case Fade:
        {
            // Paint old, then new on top with increasing alpha
            oldPage->paint(ctx);

            // Flush old, draw scrim for fade effect
            ctx.fill.Render();
            ctx.line.Render();
            ctx.text.Render();

            // Dark overlay fading in over old content
            unsigned char alpha = static_cast<unsigned char>(eased * 180);
            ctx.fill.SetColor(0, 0, 0, alpha);
            ctx.fill.Rectangle(static_cast<int>(abs.x), static_cast<int>(abs.y),
                               static_cast<int>(abs.w), static_cast<int>(abs.h), true);
            ctx.fill.Render();

            newPage->paint(ctx);
            break;
        }
        default:
            newPage->paint(ctx);
            break;
        }
    }

    ctx.popClip();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Dialog — modal dialog shown via the popup layer
// ═════════════════════════════════════════════════════════════════════════════

Dialog::Dialog(const std::string& title, float width, float height)
    : title_(title), dialogW_(width), dialogH_(height)
{
}

void Dialog::setContentWidget(Widget* w)
{
    if (content_) removeChild(content_);
    content_ = w;
    if (content_) addChild(content_);
    markDirty();
}

void Dialog::addButton(const std::string& label, ButtonStyle style)
{
    buttons_.push_back({label, style, {}});
    markDirty();
}

float Dialog::autoHeight() const
{
    float h = kTitleBarH + kBtnRowH;
    if (content_)
    {
        auto sh = content_->sizeHint();
        h += std::max(sh.y, 40.0f) + kMsgPad * 2;
    }
    else if (!message_.empty())
    {
        float lineH = 20.0f;
        float avail = dialogW_ - kPad * 2;
        float charsPerLine = avail / 7.0f;
        int lines = 1;
        if (charsPerLine > 0)
            lines = std::max(1, static_cast<int>(message_.size() / static_cast<size_t>(charsPerLine)) + 1);
        h += lines * lineH + kMsgPad * 2;
    }
    else
    {
        h += 40.0f;
    }
    return h;
}

Rect Dialog::dialogRect() const
{
    float h = (dialogH_ > 0) ? dialogH_ : autoHeight();
    float x = (rect_.w - dialogW_) * 0.5f;
    float y = (rect_.h - h) * 0.5f;
    return {rect_.x + x, rect_.y + y, dialogW_, h};
}

Rect Dialog::titleBarRect() const
{
    Rect dr = dialogRect();
    return {dr.x, dr.y, dr.w, kTitleBarH};
}

Rect Dialog::contentAreaRect() const
{
    Rect dr = dialogRect();
    return {dr.x + kPad, dr.y + kTitleBarH + kMsgPad,
            dr.w - kPad * 2, dr.h - kTitleBarH - kBtnRowH - kMsgPad * 2};
}

Rect Dialog::buttonRowRect() const
{
    Rect dr = dialogRect();
    return {dr.x + kPad, dr.y + dr.h - kBtnRowH,
            dr.w - kPad * 2, kBtnRowH};
}

void Dialog::computeButtonRects()
{
    if (buttons_.empty()) return;
    Rect row = buttonRowRect();
    float btnW = 90.0f;
    float gap = 8.0f;
    float totalW = buttons_.size() * btnW + (buttons_.size() - 1) * gap;
    float startX = row.x + row.w - totalW;
    float btnY = row.y + (row.h - kBtnH) * 0.5f;

    for (size_t i = 0; i < buttons_.size(); i++)
    {
        buttons_[i].rect = {startX + i * (btnW + gap), btnY, btnW, kBtnH};
    }
}

void Dialog::layout()
{
    auto& app = WidgetApp::instance();
    setRect({0, 0, static_cast<float>(app.width()), static_cast<float>(app.height())});

    computeButtonRects();

    if (content_)
    {
        Rect ca = contentAreaRect();
        content_->setRect({ca.x, ca.y, ca.w, ca.h});
        content_->layout();
    }
}

void Dialog::paint(PaintContext& ctx)
{
    layout();

    const auto& t = Theme::instance();

    // Scrim
    ctx.fill.SetColor(t.dialogScrim.r, t.dialogScrim.g, t.dialogScrim.b, t.dialogScrim.a);
    ctx.fill.Rectangle(static_cast<int>(rect_.x), static_cast<int>(rect_.y),
                        static_cast<int>(rect_.w), static_cast<int>(rect_.h), true);

    Rect dr = dialogRect();

    // Card background
    ctx.fill.SetColor(t.dialogBg.r, t.dialogBg.g, t.dialogBg.b, t.dialogBg.a);
    ctx.fill.RoundedRectangle(static_cast<int>(dr.x), static_cast<int>(dr.y),
                               static_cast<int>(dr.w), static_cast<int>(dr.h),
                               t.borderRadius + 2, 6, true);

    // Card border
    ctx.line.SetColor(t.dialogBorder.r, t.dialogBorder.g, t.dialogBorder.b, t.dialogBorder.a);
    ctx.line.RoundedRectangle(static_cast<int>(dr.x), static_cast<int>(dr.y),
                               static_cast<int>(dr.w), static_cast<int>(dr.h),
                               t.borderRadius + 2, 6, false);

    // Title bar separator
    float sepY = dr.y + kTitleBarH;
    ctx.line.SetColor(t.dialogBorder.r, t.dialogBorder.g, t.dialogBorder.b, 100);
    ctx.line.Line2D(dr.x + 8, sepY, dr.x + dr.w - 8, sepY);

    // Title text
    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetColor(t.dialogTitleText);
    ctx.font.SetBatch(&ctx.text);
    float titleY = dr.y + (kTitleBarH - t.fontSize) * 0.5f;
    ctx.font.Print(title_.c_str(), dr.x + kPad, titleY);

    // Content area
    if (content_)
    {
        Rect ca = contentAreaRect();
        ctx.pushClip(ca);
        content_->paint(ctx);
        ctx.popClip();
    }
    else if (!message_.empty())
    {
        Rect ca = contentAreaRect();
        ctx.font.SetFontSize(t.fontSize * 0.9f);
        ctx.font.SetColor(t.dialogText);
        ctx.font.SetBatch(&ctx.text);

        // Word-wrap rendering
        float lineH = t.fontSize * 1.3f;
        float x = ca.x;
        float maxW = ca.w;
        std::string word;
        float curX = x;
        float curY = ca.y;

        for (size_t i = 0; i <= message_.size(); i++)
        {
            char ch = (i < message_.size()) ? message_[i] : ' ';
            if (ch == '\n')
            {
                if (!word.empty())
                {
                    ctx.font.Print(word.c_str(), curX, curY);
                    word.clear();
                }
                curX = x;
                curY += lineH;
            }
            else if (ch == ' ' || i == message_.size())
            {
                if (!word.empty())
                {
                    float wordW = ctx.font.GetTextWidth(word.c_str());
                    if (curX + wordW > x + maxW && curX > x)
                    {
                        curX = x;
                        curY += lineH;
                    }
                    ctx.font.Print(word.c_str(), curX, curY);
                    curX += wordW + ctx.font.GetTextWidth(" ");
                    word.clear();
                }
            }
            else
            {
                word += ch;
            }
        }
    }

    // Button row separator
    Rect br = buttonRowRect();
    float bsepY = br.y - 4;
    ctx.line.SetColor(t.dialogBorder.r, t.dialogBorder.g, t.dialogBorder.b, 80);
    ctx.line.Line2D(dr.x + 8, bsepY, dr.x + dr.w - 8, bsepY);

    // Buttons
    for (int i = 0; i < static_cast<int>(buttons_.size()); i++)
    {
        auto& btn = buttons_[i];
        Color bg, bgH;
        Color txtC = t.textColor;

        switch (btn.style)
        {
        case Primary:
            bg  = t.dialogBtnPrimary;
            bgH = t.dialogBtnPrimaryHover;
            txtC = Color(255, 255, 255, 255);
            break;
        case Danger:
            bg  = t.dialogBtnDanger;
            bgH = t.dialogBtnDangerHover;
            txtC = Color(255, 255, 255, 255);
            break;
        default:
            bg  = t.dialogBtnBg;
            bgH = t.dialogBtnHover;
            break;
        }

        bool hov = (i == hoveredBtn_);
        Color c = hov ? bgH : bg;
        ctx.fill.SetColor(c.r, c.g, c.b, c.a);
        ctx.fill.RoundedRectangle(static_cast<int>(btn.rect.x), static_cast<int>(btn.rect.y),
                                   static_cast<int>(btn.rect.w), static_cast<int>(btn.rect.h),
                                   t.borderRadius, 4, true);

        ctx.font.SetFontSize(t.fontSize * 0.9f);
        ctx.font.SetColor(txtC);
        ctx.font.SetBatch(&ctx.text);
        float tw = ctx.font.GetTextWidth(btn.label.c_str());
        float tx = btn.rect.x + (btn.rect.w - tw) * 0.5f;
        float ty = btn.rect.y + (btn.rect.h - t.fontSize * 0.9f) * 0.5f;
        ctx.font.Print(btn.label.c_str(), tx, ty);
    }
}

void Dialog::onMousePress(MouseEvent& e)
{
    for (int i = 0; i < static_cast<int>(buttons_.size()); i++)
    {
        if (buttons_[i].rect.contains(e.x, e.y))
        {
            buttonClicked.emit(i);
            WidgetApp::instance().closePopup();
            e.consumed = true;
            return;
        }
    }

    // Click on scrim closes dialog
    Rect dr = dialogRect();
    if (!dr.contains(e.x, e.y))
    {
        WidgetApp::instance().closePopup();
        e.consumed = true;
    }
}

void Dialog::onMouseMove(MouseEvent& e)
{
    hoveredBtn_ = -1;
    for (int i = 0; i < static_cast<int>(buttons_.size()); i++)
    {
        if (buttons_[i].rect.contains(e.x, e.y))
        {
            hoveredBtn_ = i;
            break;
        }
    }
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  AlertDialog
// ═════════════════════════════════════════════════════════════════════════════

AlertDialog::AlertDialog(const std::string& title,
                         const std::string& message,
                         const std::string& okText)
    : Dialog(title, 340.0f)
{
    setMessage(message);
    addButton(okText, Primary);
}

// ═════════════════════════════════════════════════════════════════════════════
//  ConfirmDialog
// ═════════════════════════════════════════════════════════════════════════════

ConfirmDialog::ConfirmDialog(const std::string& title,
                             const std::string& message,
                             const std::string& okText,
                             const std::string& cancelText)
    : Dialog(title, 380.0f)
{
    setMessage(message);
    addButton(cancelText, Default);
    addButton(okText, Primary);
    buttonClicked.connect([this](int idx){
        if (idx == 1) confirmed.emit();
        else          cancelled.emit();
    });
}
