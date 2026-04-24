#include "ADSRWidget.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace {
    inline Font::ClipRect toFc(const Rect& r) { return {r.x, r.y, r.w, r.h}; }
}

ADSRWidget::ADSRWidget() {}

// ── Geometry ──────────────────────────────────────────────────────────────────
// The envelope is displayed over the area with padding.
// Time axis: total = a+d+sustainW+r, normalised to fit the area.
// Points on screen:
//   P0 = (left, bottom)           - start (0, 0)
//   P1 = (left + a_norm, top)     - attack peak  (a, 1)
//   P2 = (left + a+d_norm, s*h)   - decay knee   (a+d, s)
//   P3 = (left + a+d+sw_norm, s*h)- sustain end  (a+d+sustainW, s)
//   P4 = (right, bottom)          - release end  (a+d+sW+r, 0)

ADSRWidget::Points ADSRWidget::computePoints(const Rect& area) const
{
    const float pad = 8.0f;
    float ax = area.x + pad;
    float aw = area.w - pad * 2;
    float ay = area.y + pad;
    float ah = area.h - pad * 2;
    float bottom = ay + ah;
    float top    = ay;

    // Sustain portion is fixed at 40% of display width
    float sustainW = aw * 0.30f;
    float total = a_ + d_ + sustainW / aw * (a_ + d_ + r_) + r_;
    // Simpler: treat sustain display as fixed fraction, scale a,d,r proportionally
    float adsr_sum = a_ + d_ + r_;
    float timeW = aw * 0.70f;  // 70% for a+d+r, 30% for sustain display
    float scale = (adsr_sum > 0) ? timeW / adsr_sum : 1.0f;

    float x0 = ax;
    float x1 = ax + a_ * scale;
    float x2 = x1 + d_ * scale;
    float x3 = x2 + sustainW;
    float x4 = x3 + r_ * scale;

    float y0 = bottom;
    float y1 = top;
    float y2 = top + (1.0f - s_) * ah;
    float y3 = y2;
    float y4 = bottom;

    return {x0,y0, x1,y1, x2,y2, x3,y3, x4,y4};
}

int ADSRWidget::hitHandle(float mx, float my) const
{
    const Rect b = absoluteRect();
    const Rect area = b;
    auto p = computePoints(area);
    struct H { float x, y; } handles[] = {
        {p.x1, p.y1},   // 0: attack peak
        {p.x2, p.y2},   // 1: decay knee
        {p.x3, p.y3},   // 2: sustain end
    };
    for (int i = 0; i < 3; ++i) {
        float dx = mx - handles[i].x;
        float dy = my - handles[i].y;
        if (dx*dx + dy*dy <= (kHandleR + 2) * (kHandleR + 2))
            return i;
    }
    return -1;
}

void ADSRWidget::paint(PaintContext& ctx)
{
    const Rect b = absoluteRect();
    ctx.pushClip(b);

    // Background
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, 255);
    ctx.fillRect(b.x, b.y, b.w, b.h);

    auto p = computePoints(b);

    // Fill under envelope - trapezoid per segment (2 triangles each)
    float bot = b.y + b.h - 8.0f;
    ctx.fill.SetColor(fillColor_.r, fillColor_.g, fillColor_.b, fillColor_.a);
    auto fillSeg = [&](float ax, float ay, float bx, float by) {
        // Trapezoid: (ax,ay)-(bx,by)-(bx,bot)-(ax,bot)
        ctx.fillTriangle(ax, ay,   bx, by,   ax, bot);
        ctx.fillTriangle(bx, by,   bx, bot,  ax, bot);
    };
    fillSeg(p.x0, p.y0, p.x1, p.y1);  // attack
    fillSeg(p.x1, p.y1, p.x2, p.y2);  // decay
    fillSeg(p.x2, p.y2, p.x3, p.y3);  // sustain
    fillSeg(p.x3, p.y3, p.x4, p.y4);  // release

    // Envelope line - drawLine (line batch, no artefacts)
    ctx.line.SetColor(lineColor_.r, lineColor_.g, lineColor_.b, lineColor_.a);
    ctx.drawLine(p.x0, p.y0, p.x1, p.y1);  // attack
    ctx.drawLine(p.x1, p.y1, p.x2, p.y2);  // decay
    ctx.drawLine(p.x2, p.y2, p.x3, p.y3);  // sustain
    ctx.drawLine(p.x3, p.y3, p.x4, p.y4);  // release

    // Sustain section divider (dashed vertical at x2)
    ctx.line.SetColor(lineColor_.r, lineColor_.g, lineColor_.b, 60);
    {
        float dashH = 4.f, gapH = 4.f;
        float yy = p.y2;
        while (yy < bot) {
            float y2 = std::min(yy + dashH, bot);
            ctx.drawLine(p.x2, yy, p.x2, y2);
            yy += dashH + gapH;
        }
    }

    // Handles
    struct HPos { float x, y; const char* label; };
    HPos handles[] = {
        {p.x1, p.y1, "A"},
        {p.x2, p.y2, "D"},
        {p.x3, p.y3, "R"},
    };
    for (int i = 0; i < 3; ++i) {
        bool active = (dragging_ == i);
        ctx.fill.SetColor(active ? 255 : 200, active ? 220 : 200, active ? 80 : 200, 255);
        ctx.fillCircle(handles[i].x, handles[i].y, kHandleR);
        ctx.fill.SetColor(30, 34, 40, 255);
        ctx.fillCircle(handles[i].x, handles[i].y, kHandleR - 2.0f);
    }

    // Labels
    if (showLabels_) {
        auto fc = toFc(b);
        char buf[32];
        ctx.fill.SetColor(160, 160, 160, 255);

        snprintf(buf, sizeof(buf), "A:%.0fms", a_ * 1000.0f);
        ctx.font.Print(buf, p.x0 + 2, b.y + 4, &fc);

        snprintf(buf, sizeof(buf), "D:%.0fms", d_ * 1000.0f);
        ctx.font.Print(buf, p.x2 + 4, b.y + 4, &fc);

        snprintf(buf, sizeof(buf), "S:%.0f%%", s_ * 100.0f);
        ctx.font.Print(buf, p.x2 + 4, p.y2 - 14, &fc);

        snprintf(buf, sizeof(buf), "R:%.0fms", r_ * 1000.0f);
        ctx.font.Print(buf, p.x3 + 4, b.y + 4, &fc);
    }

    // Border
    ctx.line.SetColor(55, 60, 66, 255);
    ctx.drawLine(b.x,        b.y,        b.x+b.w-1, b.y);
    ctx.drawLine(b.x,        b.y+b.h-1,  b.x+b.w-1, b.y+b.h-1);
    ctx.drawLine(b.x,        b.y,        b.x,        b.y+b.h-1);
    ctx.drawLine(b.x+b.w-1,  b.y,        b.x+b.w-1, b.y+b.h-1);

    ctx.popClip();
}

void ADSRWidget::onMousePress(MouseEvent& e)
{
    if (e.button != 0) return;
    int h = hitHandle(e.x, e.y);
    if (h >= 0) {
        dragging_ = h;
        markDirty();
        e.consumed = true;
    }
}

void ADSRWidget::onMouseMove(MouseEvent& e)
{
    if (dragging_ < 0) return;
    const Rect b = absoluteRect();
    const float pad = 8.0f;
    float aw = b.w - pad * 2;
    float ah = b.h - pad * 2;
    float ay = b.y + pad;

    // Horizontal normalisation
    float sustainW = aw * 0.30f;
    float timeW    = aw * 0.70f;
    float adsr_sum = a_ + d_ + r_;
    float scale    = (adsr_sum > 0) ? timeW / adsr_sum : 1.0f;

    float ax0 = b.x + pad;
    // x1 = ax0 + a_*scale
    // x2 = x1  + d_*scale
    // x3 = x2  + sustainW
    float x1 = ax0 + a_ * scale;
    float x2 = x1  + d_ * scale;
    float x3 = x2  + sustainW;

    if (dragging_ == 0) {
        // Attack - move x1 horizontally, clamped between ax0 and x2
        float nx = std::clamp(e.x, ax0 + 2, x2 - 2);
        float newA = (nx - ax0) / scale;
        a_ = std::max(0.001f, newA);
    } else if (dragging_ == 1) {
        // Decay - move x2 horizontally (between x1 and x3)
        float nx = std::clamp(e.x, x1 + 2, x3 - 2);
        float newD = (nx - x1) / scale;
        d_ = std::max(0.001f, newD);
        // Sustain level via Y
        float ny = std::clamp(e.y, ay, ay + ah);
        s_ = 1.0f - (ny - ay) / ah;
        s_ = std::clamp(s_, 0.0f, 1.0f);
    } else if (dragging_ == 2) {
        // Release end - x4 = x3 + r_*scale; adjust r_ via x
        float nx = std::clamp(e.x, x3 + 2, ax0 + aw - 2);
        float newR = (nx - x3) / scale;
        r_ = std::max(0.001f, newR);
    }

    onChanged.emit(a_, d_, s_, r_);
    markDirty();
    e.consumed = true;
}

void ADSRWidget::onMouseRelease(MouseEvent& e)
{
    dragging_ = -1;
}
