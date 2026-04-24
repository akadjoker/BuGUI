#include "Widget.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <algorithm>
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
//  Sutherland-Hodgman polygon clipper (anonymous namespace)
// ═════════════════════════════════════════════════════════════════════════════

namespace {

static constexpr float kPI = 3.14159265358979323846f;

struct Vec2 { float x, y; };

// Clip polygon against one axis-aligned edge.
// edge: 0=left(>=val), 1=right(<=val), 2=top(>=val), 3=bottom(<=val)
static int clipEdge(const Vec2* in, int inCount, Vec2* out, int edge, float val)
{
    if (inCount == 0) return 0;
    int outCount = 0;

    auto inside = [&](const Vec2& v) -> bool {
        switch (edge) {
            case 0: return v.x >= val;
            case 1: return v.x <= val;
            case 2: return v.y >= val;
            case 3: return v.y <= val;
        }
        return true;
    };

    auto intersect = [&](const Vec2& a, const Vec2& b) -> Vec2 {
        float t;
        if (edge < 2) {
            float dx = b.x - a.x;
            t = (dx != 0.0f) ? (val - a.x) / dx : 0.0f;
        } else {
            float dy = b.y - a.y;
            t = (dy != 0.0f) ? (val - a.y) / dy : 0.0f;
        }
        return { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
    };

    const Vec2* prev = &in[inCount - 1];
    for (int i = 0; i < inCount; ++i) {
        const Vec2* cur = &in[i];
        bool curIn  = inside(*cur);
        bool prevIn = inside(*prev);
        if (curIn) {
            if (!prevIn) out[outCount++] = intersect(*prev, *cur);
            out[outCount++] = *cur;
        } else if (prevIn) {
            out[outCount++] = intersect(*prev, *cur);
        }
        prev = cur;
    }
    return outCount;
}

// Clip a convex polygon against an axis-aligned rect.
// Returns clipped vertex count (0 = fully outside). Max output = inCount + 4.
static int clipPoly(const Vec2* verts, int count,
                    float cx, float cy, float cw, float ch,
                    Vec2* result)
{
    // Stack buffers: a triangle clipped against 4 edges produces max 7 verts.
    // For larger polygons (rounded rect fan slice), max = count + 4.
    Vec2 buf1[12], buf2[12];
    int n;

    for (int i = 0; i < count && i < 12; ++i) buf1[i] = verts[i];
    n = count;

    n = clipEdge(buf1, n, buf2, 0, cx);           // left
    n = clipEdge(buf2, n, buf1, 1, cx + cw);      // right
    n = clipEdge(buf1, n, buf2, 2, cy);            // top
    n = clipEdge(buf2, n, result, 3, cy + ch);     // bottom

    return n;
}

// Check if bounding box is fully contained inside clip rect (fast path)
static bool isContained(const Rect& bb, const Rect& clip)
{
    return bb.x >= clip.x && bb.y >= clip.y &&
           bb.x + bb.w <= clip.x + clip.w &&
           bb.y + bb.h <= clip.y + clip.h;
}

// Emit a clipped convex polygon as a triangle fan into the batch
static void emitClippedFan(RenderBatch& batch, const Vec2* verts, int count)
{
    if (count < 3) return;
    batch.SetTexture(0u);
    batch.SetMode(0x0004); // TRIANGLES
    for (int i = 1; i < count - 1; ++i) {
        batch.Vertex2f(verts[0].x, verts[0].y);
        batch.Vertex2f(verts[i].x, verts[i].y);
        batch.Vertex2f(verts[i+1].x, verts[i+1].y);
    }
}

} // anonymous namespace

// ═════════════════════════════════════════════════════════════════════════════
//  PaintContext — clip stack
// ═════════════════════════════════════════════════════════════════════════════

void PaintContext::pushClip(const Rect& r)
{
    // Intersect with current clip
    const Rect& cur = clipRect();
    float x0 = std::max(r.x, cur.x);
    float y0 = std::max(r.y, cur.y);
    float x1 = std::min(r.x + r.w, cur.x + cur.w);
    float y1 = std::min(r.y + r.h, cur.y + cur.h);
    float w  = std::max(0.0f, x1 - x0);
    float h  = std::max(0.0f, y1 - y0);
    clipStack_.push_back({x0, y0, w, h});
}

void PaintContext::popClip()
{
    if (!clipStack_.empty())
        clipStack_.pop_back();
}

const Rect& PaintContext::clipRect() const
{
    return clipStack_.empty() ? fullScreen_ : clipStack_.back();
}

bool PaintContext::isClipped(const Rect& r) const
{
    const Rect& c = clipRect();
    return (r.x >= c.x + c.w || r.x + r.w <= c.x ||
            r.y >= c.y + c.h || r.y + r.h <= c.y);
}

bool PaintContext::clipRectIntersect(const Rect& in, Rect& out) const
{
    const Rect& c = clipRect();
    float x0 = std::max(in.x, c.x);
    float y0 = std::max(in.y, c.y);
    float x1 = std::min(in.x + in.w, c.x + c.w);
    float y1 = std::min(in.y + in.h, c.y + c.h);
    if (x1 <= x0 || y1 <= y0) return false;
    out = {x0, y0, x1 - x0, y1 - y0};
    return true;
}

void PaintContext::fontClip(float out[4]) const
{
    const Rect& c = clipRect();
    out[0] = c.x; out[1] = c.y; out[2] = c.w; out[3] = c.h;
}

// ── Clip-aware drawing helpers ────────────────────────────────────────────

void PaintContext::fillRect(float x, float y, float w, float h)
{
    Rect in{x, y, w, h};
    Rect out;
    if (clipRectIntersect(in, out))
        fill.Rectangle(static_cast<int>(out.x), static_cast<int>(out.y),
                        static_cast<int>(out.w), static_cast<int>(out.h), true);
}

void PaintContext::lineRect(float x, float y, float w, float h)
{
    Rect in{x, y, w, h};
    if (isClipped(in)) return;
    const Rect& c = clipRect();
    if (isContained(in, c)) {
        line.Rectangle(static_cast<int>(x), static_cast<int>(y),
                        static_cast<int>(w), static_cast<int>(h), false);
        return;
    }
    // Soft clip each edge
    drawLine(x, y, x + w, y);
    drawLine(x + w, y, x + w, y + h);
    drawLine(x + w, y + h, x, y + h);
    drawLine(x, y + h, x, y);
}

void PaintContext::fillRoundedRect(float x, float y, float w, float h, float r, int seg)
{
    Rect bb{x, y, w, h};
    if (isClipped(bb)) return;
    const Rect& c = clipRect();
    // Fast path: fully inside clip → delegate to batch
    if (isContained(bb, c)) {
        fill.RoundedRectangle(static_cast<int>(x), static_cast<int>(y),
                               static_cast<int>(w), static_cast<int>(h), r, seg, true);
        return;
    }
    // Soft clip: generate rounded rect triangles and clip each
    if (r <= 0.0f) { fillRect(x, y, w, h); return; }
    r = std::min(r, w * 0.5f);
    r = std::min(r, h * 0.5f);
    seg = std::max(2, std::min(seg, 32));

    const float cxTL = x + r,     cyTL = y + r;
    const float cxTR = x + w - r, cyTR = y + r;
    const float cxBR = x + w - r, cyBR = y + h - r;
    const float cxBL = x + r,     cyBL = y + h - r;

    // Generate perimeter points (4 corners × seg points)
    const int totalPts = seg * 4;
    Vec2 pts[128]; // seg max 32 → 128 points max

    auto fillCorner = [&](int base, float ccx, float ccy, float startDeg, float endDeg) {
        const float startRad = startDeg * (kPI / 180.0f);
        const float step = (endDeg - startDeg) * (kPI / 180.0f) / (float)seg;
        for (int i = 0; i < seg; ++i) {
            const float a = startRad + step * (float)i;
            pts[base + i] = { ccx + cosf(a) * r, ccy + sinf(a) * r };
        }
    };

    fillCorner(0 * seg, cxTL, cyTL, 180.0f, 270.0f);
    fillCorner(1 * seg, cxTR, cyTR, 270.0f, 360.0f);
    fillCorner(2 * seg, cxBR, cyBR,   0.0f,  90.0f);
    fillCorner(3 * seg, cxBL, cyBL,  90.0f, 180.0f);

    // Triangle fan from center
    const float mx = x + w * 0.5f, my = y + h * 0.5f;
    fill.SetTexture(0u);
    fill.SetMode(0x0004); // TRIANGLES
    for (int i = 0; i < totalPts; ++i) {
        const Vec2& a = pts[i];
        const Vec2& b = pts[(i + 1) % totalPts];
        Vec2 tri[3] = { {mx, my}, {a.x, a.y}, {b.x, b.y} };
        Vec2 clipped[12];
        int n = clipPoly(tri, 3, c.x, c.y, c.w, c.h, clipped);
        for (int j = 1; j < n - 1; ++j) {
            fill.Vertex2f(clipped[0].x, clipped[0].y);
            fill.Vertex2f(clipped[j].x, clipped[j].y);
            fill.Vertex2f(clipped[j+1].x, clipped[j+1].y);
        }
    }
}

void PaintContext::lineRoundedRect(float x, float y, float w, float h, float r, int seg)
{
    Rect bb{x, y, w, h};
    if (isClipped(bb)) return;
    const Rect& c = clipRect();
    if (isContained(bb, c)) {
        line.RoundedRectangle(static_cast<int>(x), static_cast<int>(y),
                               static_cast<int>(w), static_cast<int>(h), r, seg, false);
        return;
    }
    // Soft clip: generate outline segments and clip each via drawLine
    if (r <= 0.0f) { lineRect(x, y, w, h); return; }
    r = std::min(r, w * 0.5f);
    r = std::min(r, h * 0.5f);
    seg = std::max(2, std::min(seg, 32));

    const float cxTL = x + r,     cyTL = y + r;
    const float cxTR = x + w - r, cyTR = y + r;
    const float cxBR = x + w - r, cyBR = y + h - r;
    const float cxBL = x + r,     cyBL = y + h - r;

    const int totalPts = seg * 4;
    Vec2 pts[128];

    auto fillCorner = [&](int base, float ccx, float ccy, float startDeg, float endDeg) {
        const float startRad = startDeg * (kPI / 180.0f);
        const float step = (endDeg - startDeg) * (kPI / 180.0f) / (float)seg;
        for (int i = 0; i < seg; ++i) {
            const float a = startRad + step * (float)i;
            pts[base + i] = { ccx + cosf(a) * r, ccy + sinf(a) * r };
        }
    };

    fillCorner(0 * seg, cxTL, cyTL, 180.0f, 270.0f);
    fillCorner(1 * seg, cxTR, cyTR, 270.0f, 360.0f);
    fillCorner(2 * seg, cxBR, cyBR,   0.0f,  90.0f);
    fillCorner(3 * seg, cxBL, cyBL,  90.0f, 180.0f);

    for (int i = 0; i < totalPts; ++i) {
        const Vec2& a = pts[i];
        const Vec2& b = pts[(i + 1) % totalPts];
        drawLine(a.x, a.y, b.x, b.y);
    }
}

void PaintContext::fillCircle(float cx, float cy, float radius)
{
    if (radius <= 0.0f) return;
    Rect bb{cx - radius, cy - radius, radius * 2, radius * 2};
    if (isClipped(bb)) return;
    const Rect& c = clipRect();
    // Fast path
    if (isContained(bb, c)) {
        fill.Circle(static_cast<int>(cx), static_cast<int>(cy), radius, true);
        return;
    }
    // Soft clip: generate triangle fan and clip each triangle
    const int segments = std::max(18, std::min(96, (int)(radius * 0.8f)));
    const float step = (2.0f * kPI) / (float)segments;
    fill.SetTexture(0u);
    fill.SetMode(0x0004); // TRIANGLES
    for (int i = 0; i < segments; ++i) {
        const float a0 = step * i, a1 = step * (i + 1);
        Vec2 tri[3] = {
            {cx, cy},
            {cx + cosf(a0) * radius, cy + sinf(a0) * radius},
            {cx + cosf(a1) * radius, cy + sinf(a1) * radius}
        };
        Vec2 clipped[12];
        int n = clipPoly(tri, 3, c.x, c.y, c.w, c.h, clipped);
        for (int j = 1; j < n - 1; ++j) {
            fill.Vertex2f(clipped[0].x, clipped[0].y);
            fill.Vertex2f(clipped[j].x, clipped[j].y);
            fill.Vertex2f(clipped[j+1].x, clipped[j+1].y);
        }
    }
}

void PaintContext::lineCircle(float cx, float cy, float radius)
{
    if (radius <= 0.0f) return;
    Rect bb{cx - radius, cy - radius, radius * 2, radius * 2};
    if (isClipped(bb)) return;
    const Rect& c = clipRect();
    if (isContained(bb, c)) {
        line.Circle(static_cast<int>(cx), static_cast<int>(cy), radius, false);
        return;
    }
    // Soft clip: line segments via Cohen-Sutherland
    const int segments = std::max(18, std::min(96, (int)(radius * 0.8f)));
    const float step = (2.0f * kPI) / (float)segments;
    for (int i = 0; i < segments; ++i) {
        const float a0 = step * i, a1 = step * (i + 1);
        drawLine(cx + cosf(a0) * radius, cy + sinf(a0) * radius,
                 cx + cosf(a1) * radius, cy + sinf(a1) * radius);
    }
}

void PaintContext::drawLine(float x1, float y1, float x2, float y2)
{
    // Cohen-Sutherland line clipping
    const Rect& c = clipRect();
    float xmin = c.x, ymin = c.y, xmax = c.x + c.w, ymax = c.y + c.h;

    auto outcode = [&](float x, float y) -> int {
        int code = 0;
        if (x < xmin) code |= 1;
        else if (x > xmax) code |= 2;
        if (y < ymin) code |= 4;
        else if (y > ymax) code |= 8;
        return code;
    };

    int c1 = outcode(x1, y1), c2 = outcode(x2, y2);
    for (int i = 0; i < 10; ++i) {
        if (!(c1 | c2)) break;         // both inside
        if (c1 & c2) return;           // both outside same side
        int co = c1 ? c1 : c2;
        float x, y;
        if      (co & 8) { x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1); y = ymax; }
        else if (co & 4) { x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1); y = ymin; }
        else if (co & 2) { y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1); x = xmax; }
        else             { y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1); x = xmin; }
        if (co == c1) { x1 = x; y1 = y; c1 = outcode(x1, y1); }
        else          { x2 = x; y2 = y; c2 = outcode(x2, y2); }
    }
    line.Line2D(static_cast<int>(x1), static_cast<int>(y1),
                static_cast<int>(x2), static_cast<int>(y2));
}

void PaintContext::fillTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
    float minX = std::min({x1, x2, x3}), maxX = std::max({x1, x2, x3});
    float minY = std::min({y1, y2, y3}), maxY = std::max({y1, y2, y3});
    Rect bb{minX, minY, maxX - minX, maxY - minY};
    if (isClipped(bb)) return;
    const Rect& c = clipRect();
    if (isContained(bb, c)) {
        fill.Triangle(x1, y1, x2, y2, x3, y3, true);
        return;
    }
    // Sutherland-Hodgman clip
    Vec2 tri[3] = { {x1, y1}, {x2, y2}, {x3, y3} };
    Vec2 clipped[12];
    int n = clipPoly(tri, 3, c.x, c.y, c.w, c.h, clipped);
    if (n < 3) return;
    fill.SetTexture(0u);
    fill.SetMode(0x0004); // TRIANGLES
    for (int j = 1; j < n - 1; ++j) {
        fill.Vertex2f(clipped[0].x, clipped[0].y);
        fill.Vertex2f(clipped[j].x, clipped[j].y);
        fill.Vertex2f(clipped[j+1].x, clipped[j+1].y);
    }
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
