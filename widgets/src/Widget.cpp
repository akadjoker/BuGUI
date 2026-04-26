#include "Widget.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <algorithm>
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
//  PaintContext — clip stack
// ═════════════════════════════════════════════════════════════════════════════

PaintContext::PaintContext(RenderBatch& f, RenderBatch& l, RenderBatch& t, Font& fn,
                           IconAtlas* ico)
    : fill(f), line(l), text(t), font(fn), icons(ico)
{
    fill.ClearClipRects();
    line.ClearClipRects();
    text.ClearClipRects();
}

void PaintContext::pushClip(const Rect& r)
{
    fill.PushClipRect(r.x, r.y, r.w, r.h);
    line.PushClipRect(r.x, r.y, r.w, r.h);
    text.PushClipRect(r.x, r.y, r.w, r.h);
}

void PaintContext::popClip()
{
    fill.PopClipRect();
    line.PopClipRect();
    text.PopClipRect();
}

const Rect& PaintContext::clipRect() const
{
    float c[4];
    fill.GetClipRect(c);
    cachedClip_ = {c[0], c[1], c[2], c[3]};
    return cachedClip_;
}

bool PaintContext::hasClip() const
{
    return fill.HasClipRect();
}

bool PaintContext::isClipped(const Rect& r) const
{
    return fill.IsRectOutsideClip(r.x, r.y, r.w, r.h);
}

bool PaintContext::clipRectIntersect(const Rect& in, Rect& out) const
{
    float c[4];
    if (!fill.IntersectClipRect(in.x, in.y, in.w, in.h, c)) return false;
    out = {c[0], c[1], c[2], c[3]};
    return true;
}

void PaintContext::fontClip(float out[4]) const
{
    fill.GetClipRect(out);
}

// ── Clip-aware drawing helpers ────────────────────────────────────────────

void PaintContext::fillRect(float x, float y, float w, float h)
{
    fill.RectangleClipped(x, y, w, h, true);
}

void PaintContext::lineRect(float x, float y, float w, float h)
{
    line.RectangleClipped(x, y, w, h, false);
}

void PaintContext::fillRoundedRect(float x, float y, float w, float h, float r, int seg)
{
    fill.RoundedRectangleClipped(x, y, w, h, r, seg, true);
}

void PaintContext::lineRoundedRect(float x, float y, float w, float h, float r, int seg)
{
    line.RoundedRectangleClipped(x, y, w, h, r, seg, false);
}

void PaintContext::fillCircle(float cx, float cy, float radius)
{
    fill.CircleClipped(cx, cy, radius, true);
}

void PaintContext::lineCircle(float cx, float cy, float radius)
{
    line.CircleClipped(cx, cy, radius, false);
}

void PaintContext::drawLine(float x1, float y1, float x2, float y2)
{
    line.Line2DClipped(x1, y1, x2, y2);
}

void PaintContext::fillTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
    fill.TriangleClipped(x1, y1, x2, y2, x3, y3);
}

// ── Icon drawing via atlas texture ───────────────────────────────────────

void PaintContext::drawIcon(IconId id, float x, float y, float size)
{
    if (!icons || !icons->ready() || id == IconId::None) return;

    Rect bb{x, y, size, size};
    if (isClipped(bb)) return;

    FloatRect src = icons->srcRect(id);
    const Rect& c = clipRect();
    float clip4[4] = {c.x, c.y, c.w, c.h};
    fill.SetColor(255, 255, 255, 255);
    fill.DrawImageRegion(icons->texture(),
                         src.x, src.y, src.width, src.height,
                         x, y, size, size,
                         0.0f, 0.0f, 0.0f,
                         &clip4[0]);
}

void PaintContext::drawIcon(IconId id, float x, float y, float size, const Color& tint)
{
    if (!icons || !icons->ready() || id == IconId::None) return;

    Rect bb{x, y, size, size};
    if (isClipped(bb)) return;

    FloatRect src = icons->srcRect(id);
    const Rect& c = clipRect();
    float clip4[4] = {c.x, c.y, c.w, c.h};
    fill.SetColor(tint.r, tint.g, tint.b, tint.a);
    fill.DrawImageRegion(icons->texture(),
                         src.x, src.y, src.width, src.height,
                         x, y, size, size,
                         0.0f, 0.0f, 0.0f,
                         &clip4[0]);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Widget — tree / geometry / paint
// ═════════════════════════════════════════════════════════════════════════════

Widget::~Widget()
{
    for (Widget* c : children_)
        delete c;
    children_.clear();
}

void Widget::removeChild(Widget* child)
{
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end())
    {
        WidgetApp::instance().notifyWidgetRemoved(child);
        delete *it;
        children_.erase(it);
    }
    markDirty();
}

Rect Widget::absoluteRect() const
{
    Rect r = rect_;
    const Widget* child = this;
    const Widget* p = parent_;
    while (p)
    {
        r.x += p->rect_.x;
        r.y += p->rect_.y;
        // Apply scroll offset only if this child is scrollable
        if (child->scrollable_)
        {
            auto so = p->scrollOffset();
            r.x -= so.x;
            r.y -= so.y;
        }
        child = p;
        p = p->parent_;
    }
    return r;
}

Widget* Widget::findById(const std::string& searchId)
{
    if (id_ == searchId) return this;
    for (auto* c : children_) {
        Widget* found = c->findById(searchId);
        if (found) return found;
    }
    return nullptr;
}

void Widget::findByTag(const std::string& tag, std::vector<Widget*>& result)
{
    if (hasTag(tag))
        result.push_back(this);
    for (auto* c : children_)
        c->findByTag(tag, result);
}

void Widget::layout()
{
    for (auto& c : children_)
        c->layout();
}

void Widget::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    ctx.pushClip(abs);
    for (auto& c : children_)
        c->paint(ctx);
    ctx.popClip();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Widget — base event handlers (emit signals)
// ═════════════════════════════════════════════════════════════════════════════

void Widget::onMousePress(MouseEvent& e)
{
    if (!enabled_) return;
    pressed_ = true;
    pressed.emit(e.button);
    WidgetApp::instance().fireEvent("press", this);
    if (acceptsFocus_) e.consumed = true;
    markDirty();
}

void Widget::onMouseRelease(MouseEvent& e)
{
    if (!enabled_) return;
    bool wasPressed = pressed_;
    pressed_ = false;
    released.emit(e.button);
    WidgetApp::instance().fireEvent("release", this);
    if (acceptsFocus_) e.consumed = true;
    markDirty();

    // clicked = press + release with left button while still inside
    if (wasPressed && e.button == 0)
    {
        Rect abs = absoluteRect();
        if (abs.contains(e.x, e.y))
        {
            clicked.emit();
            WidgetApp::instance().fireEvent("click", this);
        }
    }
}

void Widget::onMouseMove(MouseEvent& e)
{
    moved.emit(e.localX, e.localY);
    WidgetApp::instance().fireEvent("move", this);
}

void Widget::onMouseEnter()
{
    hoverEnter.emit();
    WidgetApp::instance().fireEvent("hover", this);
    markDirty();
}

void Widget::onMouseLeave()
{
    pressed_ = false;
    hoverLeave.emit();
    WidgetApp::instance().fireEvent("leave", this);
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  BoxLayout
// ═════════════════════════════════════════════════════════════════════════════

void BoxLayout::layout()
{
    const bool vert = (dir_ == LayoutDir::Vertical);

    // Available space inside padding
    float areaX = padding_.left;
    float areaY = padding_.top;
    float areaW = rect_.w - padding_.left - padding_.right;
    float areaH = rect_.h - padding_.top  - padding_.bottom;

    // ── Pass 1: collect visible children, measure fixed + stretch ────────
    struct Item {
        Widget* w;
        Vec2f   hint;
        float   mainHint;   // size on main axis (from sizeHint)
        float   mainMargin; // margin on main axis (before + after)
        float   crossMargin;
    };

    std::vector<Item> items;
    items.reserve(children_.size());

    float totalFixed   = 0;
    float totalStretch = 0;
    int   count        = 0;

    for (auto* child : children_)
    {
        if (!child->isVisible()) continue;

        // Pre-set the cross-axis dimension so children like FlowLayout
        // can compute a width-dependent sizeHint (e.g. row wrapping).
        // We only update the single dimension without triggering markDirty.
        const Edges& m = child->margins();
        if (vert)
        {
            float cw = areaW - m.left - m.right;
            if (cw > 0 && child->rect().w != cw)
            {
                Rect r = child->rect();
                r.w = cw;
                child->setRect(r);
            }
        }
        else
        {
            float ch = areaH - m.top - m.bottom;
            if (ch > 0 && child->rect().h != ch)
            {
                Rect r = child->rect();
                r.h = ch;
                child->setRect(r);
            }
        }

        const Vec2f hint = child->sizeHint();
        float mainMargin  = vert ? m.top + m.bottom : m.left + m.right;
        float crossMargin = vert ? m.left + m.right : m.top + m.bottom;

        float mainH = vert ? hint.y : hint.x;

        items.push_back({child, hint, mainH, mainMargin, crossMargin});

        if (child->stretch() > 0)
        {
            totalStretch += child->stretch();
            totalFixed += mainMargin;   // margins still consume fixed space
        }
        else
            totalFixed += mainH + mainMargin;

        count++;
    }

    if (count == 0) return;

    // Total spacing between items
    float totalSpacing = (count > 1) ? spacing_ * (count - 1) : 0;

    // Available for main axis (after padding, spacing, fixed items)
    float mainAvail = (vert ? areaH : areaW) - totalSpacing - totalFixed;
    if (mainAvail < 0) mainAvail = 0;

    // ── Pass 2: compute main-axis sizes ──────────────────────────────────
    struct Placed { float pos; float size; };
    std::vector<Placed> mainPlaced(count);

    float usedMain = 0;
    for (int i = 0; i < count; ++i)
    {
        auto& it = items[i];
        if (it.w->stretch() > 0 && totalStretch > 0)
            mainPlaced[i].size = (it.w->stretch() / totalStretch) * mainAvail;
        else
            mainPlaced[i].size = it.mainHint;
        usedMain += mainPlaced[i].size + it.mainMargin;
    }
    usedMain += totalSpacing;

    // ── Pass 3: main-axis positioning (mainAlign) ────────────────────────
    float extraMain = (vert ? areaH : areaW) - usedMain;
    if (extraMain < 0) extraMain = 0;

    float gap     = 0;
    float startOff = 0;

    switch (mainAlign_)
    {
    case MainAlign::Start:
        break;
    case MainAlign::Center:
        startOff = extraMain * 0.5f;
        break;
    case MainAlign::End:
        startOff = extraMain;
        break;
    case MainAlign::SpaceBetween:
        if (count > 1) gap = extraMain / (count - 1);
        break;
    case MainAlign::SpaceEvenly:
        gap = extraMain / (count + 1);
        startOff = gap;
        break;
    }

    float cursor = (vert ? areaY : areaX) + startOff;

    for (int i = 0; i < count; ++i)
    {
        auto& it = items[i];
        const Edges& m = it.w->margins();
        float mainBefore = vert ? m.top  : m.left;
        float crossBefore = vert ? m.left : m.top;

        cursor += mainBefore;
        mainPlaced[i].pos = cursor;
        cursor += mainPlaced[i].size;
        cursor += (vert ? m.bottom : m.right);
        cursor += spacing_ + gap;
    }

    // ── Pass 4: cross-axis alignment + set rects ─────────────────────────
    for (int i = 0; i < count; ++i)
    {
        auto& it = items[i];
        const Edges& m = it.w->margins();
        Rect cr;

        float crossTotal = vert ? areaW : areaH;
        float crossHint  = vert ? it.hint.x : it.hint.y;
        float crossMBefore = vert ? m.left : m.top;
        float crossMAfter  = vert ? m.right : m.bottom;
        float crossAvail = crossTotal - crossMBefore - crossMAfter;

        float crossSize, crossPos;
        Align al = it.w->layoutAlign();
        switch (al)
        {
        case Align::Start:
            crossSize = std::min(crossHint, crossAvail);
            crossPos  = (vert ? areaX : areaY) + crossMBefore;
            break;
        case Align::Center:
            crossSize = std::min(crossHint, crossAvail);
            crossPos  = (vert ? areaX : areaY) + crossMBefore + (crossAvail - crossSize) * 0.5f;
            break;
        case Align::End:
            crossSize = std::min(crossHint, crossAvail);
            crossPos  = (vert ? areaX : areaY) + crossMBefore + crossAvail - crossSize;
            break;
        case Align::Fill:
        default:
            crossSize = crossAvail;
            crossPos  = (vert ? areaX : areaY) + crossMBefore;
            break;
        }

        if (vert)
        {
            cr.x = crossPos;
            cr.y = mainPlaced[i].pos;
            cr.w = crossSize;
            cr.h = mainPlaced[i].size;
        }
        else
        {
            cr.x = mainPlaced[i].pos;
            cr.y = crossPos;
            cr.w = mainPlaced[i].size;
            cr.h = crossSize;
        }

        it.w->setRect(cr);
        it.w->layout();
    }
}

Widget::Vec2f BoxLayout::sizeHint() const
{
    float mainAxis  = padding_.top + padding_.bottom;
    float crossAxis = 0;
    int count = 0;

    if (dir_ == LayoutDir::Horizontal)
        mainAxis = padding_.left + padding_.right;

    for (const auto& child : children_)
    {
        if (!child->isVisible()) continue;
        const Vec2f hint = child->sizeHint();
        const Edges& m   = child->margins();

        if (dir_ == LayoutDir::Vertical)
        {
            mainAxis  += hint.y + m.top + m.bottom;
            crossAxis  = std::max(crossAxis, hint.x + m.left + m.right);
        }
        else
        {
            mainAxis  += hint.x + m.left + m.right;
            crossAxis  = std::max(crossAxis, hint.y + m.top + m.bottom);
        }
        count++;
    }

    if (count > 1) mainAxis += spacing_ * (count - 1);

    if (dir_ == LayoutDir::Vertical)
    {
        crossAxis += padding_.left + padding_.right;
        return {crossAxis, mainAxis};
    }
    else
    {
        crossAxis += padding_.top + padding_.bottom;
        return {mainAxis, crossAxis};
    }
}
