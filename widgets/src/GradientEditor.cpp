#include "GradientEditor.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace {
    inline Font::ClipRect toFontClip(const Rect& r)
    { return { r.x, r.y, r.w, r.h }; }

    inline uint8_t lerpU8(uint8_t a, uint8_t b, float t)
    {
        float r = a + (float(b) - float(a)) * t + 0.5f;
        return static_cast<uint8_t>(r < 0.f ? 0 : r > 255.f ? 255 : r);
    }

    static constexpr float kHandleH = 10.0f;  // triangle height below bar
    static constexpr float kHandleW =  8.0f;  // half-width of triangle
    static constexpr int   kMinStops = 2;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────────────────────────────────────

GradientEditor::GradientEditor()
{
    // Default black→white
    stops_.push_back({0.0f, Color(0,   0,   0,   255)});
    stops_.push_back({1.0f, Color(255, 255, 255, 255)});
}

// ─────────────────────────────────────────────────────────────────────────────
//  Stop management
// ─────────────────────────────────────────────────────────────────────────────

void GradientEditor::addStop(float pos, const Color& color)
{
    pos = std::clamp(pos, 0.0f, 1.0f);
    stops_.push_back({pos, color});
    sortStops();
    markDirty();
    emit();
}

void GradientEditor::removeStop(int idx)
{
    if ((int)stops_.size() <= kMinStops) return;
    if (idx < 0 || idx >= (int)stops_.size()) return;
    stops_.erase(stops_.begin() + idx);
    if (selected_ >= (int)stops_.size()) selected_ = (int)stops_.size() - 1;
    markDirty();
    emit();
}

void GradientEditor::setStopColor(int idx, const Color& color)
{
    if (idx < 0 || idx >= (int)stops_.size()) return;
    stops_[idx].color = color;
    markDirty();
    emit();
}

void GradientEditor::setStopPos(int idx, float pos)
{
    if (idx < 0 || idx >= (int)stops_.size()) return;
    stops_[idx].pos = std::clamp(pos, 0.0f, 1.0f);
    sortStops();
    markDirty();
    emit();
}

void GradientEditor::clearStops()
{
    stops_.clear();
    selected_ = -1;
    markDirty();
}

Color GradientEditor::sample(float t) const
{
    if (stops_.empty()) return Color(0,0,0,255);
    if (stops_.size() == 1) return stops_[0].color;
    t = std::clamp(t, 0.0f, 1.0f);

    // Clamp to stop range boundaries — prevents negative lerp factor
    if (t <= stops_.front().pos) return stops_.front().color;
    if (t >= stops_.back().pos)  return stops_.back().color;

    for (int i = 0; i < (int)stops_.size() - 1; ++i) {
        const auto& a = stops_[i];
        const auto& b = stops_[i+1];
        if (t <= b.pos) {
            float span = b.pos - a.pos;
            float f = (span < 1e-6f) ? 0.0f : (t - a.pos) / span;
            f = std::clamp(f, 0.0f, 1.0f);
            return Color(
                lerpU8(a.color.r, b.color.r, f),
                lerpU8(a.color.g, b.color.g, f),
                lerpU8(a.color.b, b.color.b, f),
                lerpU8(a.color.a, b.color.a, f)
            );
        }
    }
    return stops_.back().color;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Layout helpers
// ─────────────────────────────────────────────────────────────────────────────

Rect GradientEditor::barRect() const
{
    const Rect b = absoluteRect();
    return { b.x + kHandleW, b.y + 2, b.w - kHandleW * 2, barHeight_ };
}

Rect GradientEditor::handleRect(int idx) const
{
    const Rect bar = barRect();
    float px = bar.x + stops_[idx].pos * bar.w;
    return { px - kHandleW, bar.y + bar.h + 2, kHandleW * 2, kHandleH };
}

int GradientEditor::hitStop(float mx, float my) const
{
    for (int i = 0; i < (int)stops_.size(); ++i) {
        const Rect hr = handleRect(i);
        if (mx >= hr.x - 4 && mx <= hr.x + hr.w + 4 &&
            my >= hr.y - 4 && my <= hr.y + hr.h + 4)
            return i;
    }
    return -1;
}

void GradientEditor::sortStops()
{
    std::sort(stops_.begin(), stops_.end(),
              [](const GradientStop& a, const GradientStop& b){ return a.pos < b.pos; });
}

void GradientEditor::emit()
{
    onChanged.emit(stops_);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paint
// ─────────────────────────────────────────────────────────────────────────────

void GradientEditor::paint(PaintContext& ctx)
{
    const Rect b  = absoluteRect();
    const Rect br = barRect();
    auto fc = toFontClip(b);

    ctx.pushClip(b);

    // ── Widget background ─────────────────────────────────────────────────
    ctx.fill.SetColor(32, 32, 36, 255);
    ctx.fillRect(b.x, b.y, b.w, b.h);

    // ── Checkerboard (alpha preview) behind bar ───────────────────────────
    if (showAlpha_) {
        int csize = 6;
        int bx = (int)br.x, by = (int)br.y;
        int bw = (int)br.w, bh = (int)br.h;
        int cols = bw / csize + 1;
        int rows = bh / csize + 1;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                uint8_t v = ((row + col) % 2 == 0) ? 200 : 140;
                ctx.fill.SetColor(v, v, v, 255);
                float cx = bx + col * csize;
                float cy = by + row * csize;
                float cw = std::min((float)csize, (float)bx + bw - cx);
                float ch = std::min((float)csize, (float)by + bh - cy);
                if (cw > 0 && ch > 0)
                    ctx.fillRect(cx, cy, cw, ch);
            }
        }
    }

    // ── Gradient bar — one segment per stop pair, overlap by 1px ─────────
    if (stops_.size() >= 2) {
        for (int i = 0; i < (int)stops_.size() - 1; ++i) {
            const Color& ca = stops_[i].color;
            const Color& cb = stops_[i+1].color;
            float x0 = br.x + stops_[i].pos   * br.w;
            float x1 = br.x + stops_[i+1].pos * br.w;
            float span = x1 - x0;
            if (span < 0.5f) continue;

            int segs = std::max(1, (int)(span / 2));
            for (int s = 0; s < segs; ++s) {
                float f0 = std::clamp((float)s       / segs, 0.0f, 1.0f);
                float f1 = std::clamp((float)(s + 1) / segs, 0.0f, 1.0f);
                float fc_ = (f0 + f1) * 0.5f;
                Color c(lerpU8(ca.r, cb.r, fc_),
                        lerpU8(ca.g, cb.g, fc_),
                        lerpU8(ca.b, cb.b, fc_),
                        lerpU8(ca.a, cb.a, fc_));
                ctx.fill.SetColor(c.r, c.g, c.b, c.a);
                // +1 overlap to kill sub-pixel gaps
                ctx.fillRect(x0 + f0 * span, br.y, f1 * span - f0 * span + 1.0f, br.h);
            }
        }
    } else if (!stops_.empty()) {
        const Color& c = stops_[0].color;
        ctx.fill.SetColor(c.r, c.g, c.b, c.a);
        ctx.fillRect(br.x, br.y, br.w, br.h);
    }

    // ── Bar border ────────────────────────────────────────────────────────
    ctx.fill.SetColor(80, 80, 80, 255);
    ctx.fillRect(br.x,          br.y,         br.w, 1);
    ctx.fillRect(br.x,          br.y+br.h-1,  br.w, 1);
    ctx.fillRect(br.x,          br.y,         1,    br.h);
    ctx.fillRect(br.x+br.w-1,   br.y,         1,    br.h);

    // ── Stop handles (triangles pointing up into bar) ─────────────────────
    for (int i = 0; i < (int)stops_.size(); ++i) {
        const Rect hr = handleRect(i);
        float cx   = hr.x + hr.w * 0.5f;
        float tipY = hr.y;
        float baseY = hr.y + hr.h;
        bool sel = (i == selected_);

        Color border = sel ? Color(255,255,255,255) : Color(60,60,60,255);
        ctx.fill.SetColor(border.r, border.g, border.b, 255);
        ctx.fillTriangle(cx, tipY - 1, cx - kHandleW - 1, baseY + 1, cx + kHandleW + 1, baseY + 1);

        const Color& sc = stops_[i].color;
        ctx.fill.SetColor(sc.r, sc.g, sc.b, 255);
        ctx.fillTriangle(cx, tipY, cx - kHandleW, baseY, cx + kHandleW, baseY);
    }

    // ── Selected stop info ────────────────────────────────────────────────
    if (selected_ >= 0 && selected_ < (int)stops_.size()) {
        const auto& s = stops_[selected_];
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f  #%02X%02X%02X", s.pos, s.color.r, s.color.g, s.color.b);
        ctx.fill.SetColor(180, 180, 180, 255);
        ctx.font.Print(buf, b.x + kHandleW, b.y + 2.0f + barHeight_ + 2.0f + kHandleH + 4.0f, &fc);
    }

    ctx.popClip();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Interaction
// ─────────────────────────────────────────────────────────────────────────────

void GradientEditor::onMousePress(MouseEvent& e)
{
    if (e.button != 0) {
        // Right-click → delete
        int hit = hitStop(e.x, e.y);
        if (hit >= 0) removeStop(hit);
        return;
    }

    int hit = hitStop(e.x, e.y);
    if (hit >= 0) {
        selected_ = hit;
        dragging_ = hit;
        dragOffX_ = e.x - handleRect(hit).x - kHandleW;
        markDirty();
        return;
    }

    // Click on bar → add stop
    const Rect br = barRect();
    if (e.x >= br.x && e.x <= br.x + br.w && e.y >= br.y && e.y <= br.y + br.h) {
        float t = (e.x - br.x) / br.w;
        Color c = sample(t);
        addStop(t, c);
        // Select the new stop
        for (int i = 0; i < (int)stops_.size(); ++i) {
            if (std::fabs(stops_[i].pos - t) < 0.002f) { selected_ = i; break; }
        }
        markDirty();
    }
}

void GradientEditor::onMouseRelease(MouseEvent& e)
{
    (void)e;
    dragging_ = -1;
}

void GradientEditor::onMouseMove(MouseEvent& e)
{
    if (dragging_ < 0 || dragging_ >= (int)stops_.size()) return;
    const Rect br = barRect();
    float t = (e.x - dragOffX_ - br.x) / br.w;
    t = std::clamp(t, 0.0f, 1.0f);

    // Don't let first/last stops cross each other when at extremes
    stops_[dragging_].pos = t;
    sortStops();
    // Re-find dragging_ index after sort (pos may have moved)
    for (int i = 0; i < (int)stops_.size(); ++i) {
        if (std::fabs(stops_[i].pos - t) < 1e-5f) { dragging_ = i; selected_ = i; break; }
    }
    markDirty();
    emit();
}

// onDoubleClick folded into onMousePress (double-click detected via rapid press)
