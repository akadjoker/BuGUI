#include "Knob.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

static constexpr float kPi = 3.14159265f;

namespace {
    inline Font::ClipRect toFc(const Rect& r) { return {r.x, r.y, r.w, r.h}; }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Knob
// ─────────────────────────────────────────────────────────────────────────────

Knob::Knob() {}

void Knob::setValue(float v)
{
    value_ = std::clamp(v, min_, max_);
    onChanged.emit(value_);
    markDirty();
}

void Knob::paint(PaintContext& ctx)
{
    const Rect b = absoluteRect();
    ctx.pushClip(b);

    // Layout: circle in top portion, label below
    const float labelH  = label_.empty() ? 0.f : 14.f;
    const float valH    = showValue_     ? 12.f :  0.f;
    const float knobH   = b.h - labelH - valH;
    const float radius  = std::min(b.w, knobH) * 0.5f - 4.f;
    const float cx      = b.x + b.w * 0.5f;
    const float cy      = b.y + knobH * 0.5f;

    // ── Body ────────────────────────────────────────────────────────────
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, 255);
    ctx.fillCircle(cx, cy, radius);

    // ── Track arc (full 270° background) ─────────────────────────────
    const int   kSegs       = 40;
    const float norm        = normalised();
    ctx.line.SetColor(trackColor_.r, trackColor_.g, trackColor_.b, 255);
    for (int i = 0; i < kSegs; ++i) {
        float t0 = (float)i     / kSegs;
        float t1 = (float)(i+1) / kSegs;
        float a0 = kStartAngle + t0 * kSweep;
        float a1 = kStartAngle + t1 * kSweep;
        float r  = radius - 3.f;
        ctx.drawLine(cx + r * std::cos(a0), cy + r * std::sin(a0),
                     cx + r * std::cos(a1), cy + r * std::sin(a1));
    }

    // ── Filled arc (value) ─────────────────────────────────────────────
    const int fillSegs = (int)(norm * kSegs);  // 0 at norm=0, no arc drawn
    if (fillSegs > 0) {
        ctx.line.SetColor(arcColor_.r, arcColor_.g, arcColor_.b, 255);
        for (int i = 0; i < fillSegs; ++i) {
            float t0 = (float)i     / kSegs;
            float t1 = (float)(i+1) / kSegs;
            float a0 = kStartAngle + t0 * kSweep;
            float a1 = kStartAngle + t1 * kSweep;
            float r  = radius - 3.f;
            ctx.drawLine(cx + r * std::cos(a0), cy + r * std::sin(a0),
                         cx + r * std::cos(a1), cy + r * std::sin(a1));
        }
    }

    // ── Pointer line (from centre toward rim) ──────────────────────────
    float angle = kStartAngle + norm * kSweep;
    float r0    = radius * 0.35f;
    float r1    = radius - 5.f;
    ctx.line.SetColor(arcColor_.r, arcColor_.g, arcColor_.b, 255);
    ctx.drawLine(cx + r0 * std::cos(angle), cy + r0 * std::sin(angle),
                 cx + r1 * std::cos(angle), cy + r1 * std::sin(angle));

    // ── Centre dot ──────────────────────────────────────────────────────
    ctx.fill.SetColor(arcColor_.r, arcColor_.g, arcColor_.b, 255);
    ctx.fillCircle(cx, cy, 3.f);
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, 255);
    ctx.fillCircle(cx, cy, 1.5f);

    // ── Value text (centered) ────────────────────────────────────────────
    if (showValue_) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", value_);
        float tw  = ctx.font.GetTextWidth(buf);
        float tx2 = cx - tw * 0.5f;
        auto fc = toFc(b);
        ctx.fill.SetColor(160, 165, 175, 255);
        ctx.font.Print(buf, tx2, b.y + knobH + 1.f, &fc);
    }

    // ── Label (centered) ─────────────────────────────────────────────────
    if (!label_.empty()) {
        float tw  = ctx.font.GetTextWidth(label_.c_str());
        float tx2 = cx - tw * 0.5f;
        auto fc = toFc(b);
        ctx.fill.SetColor(200, 205, 215, 255);
        ctx.font.Print(label_.c_str(), tx2, b.y + knobH + valH, &fc);
    }

    ctx.popClip();
}

void Knob::onMousePress(MouseEvent& e)
{
    if (e.button != 0) return;
    dragging_   = true;
    dragStartY_ = e.y;
    dragStartV_ = value_;
    e.consumed  = true;
}

void Knob::onMouseMove(MouseEvent& e)
{
    if (!dragging_) return;
    float delta = (dragStartY_ - e.y) / sensitivity_; // up = increase
    float newV  = dragStartV_ + delta * (max_ - min_);
    setValue(std::clamp(newV, min_, max_));
    e.consumed = true;
}

void Knob::onMouseRelease(MouseEvent& e)
{
    if (e.button != 0) return;
    dragging_ = false;
}

void Knob::onMouseScroll(MouseEvent& e)
{
    float step = (max_ - min_) * 0.02f;
    setValue(std::clamp(value_ + e.scrollY * step, min_, max_));
    e.consumed = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  LinearKnob
// ─────────────────────────────────────────────────────────────────────────────

LinearKnob::LinearKnob() {}

void LinearKnob::setValue(float v)
{
    value_ = std::clamp(v, min_, max_);
    onChanged.emit(value_);
    markDirty();
}

Widget::Vec2f LinearKnob::sizeHint() const
{
    return vertical_ ? Vec2f{28, 100} : Vec2f{120, 28};
}

Rect LinearKnob::trackRect() const
{
    const Rect b = absoluteRect();
    const float labelH = label_.empty() ? 0.f : 14.f;
    const float valH   = showValue_     ? 12.f :  0.f;
    const float footH  = labelH + valH;
    if (vertical_) {
        float tw = 8.f;
        return {b.x + (b.w - tw) * 0.5f, b.y + 8.f, tw, b.h - footH - 16.f};
    } else {
        float th = 8.f;
        return {b.x + 8.f, b.y + (b.h - footH - th) * 0.5f, b.w - 16.f, th};
    }
}

void LinearKnob::paint(PaintContext& ctx)
{
    const Rect b = absoluteRect();
    ctx.pushClip(b);

    const float labelH = label_.empty() ? 0.f : 14.f;
    const float valH   = showValue_     ? 12.f :  0.f;
    const float norm   = normalised();
    Rect tr = trackRect();

    // ── Track background ─────────────────────────────────────────────────
    ctx.fill.SetColor(trackColor_.r, trackColor_.g, trackColor_.b, 255);
    ctx.fillRect(tr.x, tr.y, tr.w, tr.h);

    // ── Filled portion ───────────────────────────────────────────────────
    ctx.fill.SetColor(fillColor_.r, fillColor_.g, fillColor_.b, 200);
    if (vertical_) {
        float fillH = tr.h * norm;
        ctx.fillRect(tr.x, tr.y + tr.h - fillH, tr.w, fillH);
    } else {
        ctx.fillRect(tr.x, tr.y, tr.w * norm, tr.h);
    }

    // ── Thumb ─────────────────────────────────────────────────────────────
    const float thumbW = vertical_ ? tr.w + 8.f : 8.f;
    const float thumbH = vertical_ ? 8.f : tr.h + 8.f;
    float tx, ty;
    if (vertical_) {
        tx = tr.x - 4.f;
        ty = tr.y + tr.h * (1.f - norm) - thumbH * 0.5f;
    } else {
        tx = tr.x + tr.w * norm - thumbW * 0.5f;
        ty = tr.y - 4.f;
    }
    ctx.fill.SetColor(thumbColor_.r, thumbColor_.g, thumbColor_.b, 255);
    ctx.fillRect(tx, ty, thumbW, thumbH);

    // Inner shadow line on thumb
    ctx.line.SetColor(30, 34, 40, 180);
    if (vertical_) {
        ctx.drawLine(tx + 3.f, ty + 1.f, tx + thumbW - 3.f, ty + 1.f);
    } else {
        ctx.drawLine(tx + 1.f, ty + 3.f, tx + 1.f, ty + thumbH - 3.f);
    }

    // Track border
    ctx.line.SetColor(30, 34, 40, 180);
    ctx.drawLine(tr.x,         tr.y,         tr.x + tr.w - 1, tr.y);
    ctx.drawLine(tr.x,         tr.y + tr.h,  tr.x + tr.w - 1, tr.y + tr.h);
    ctx.drawLine(tr.x,         tr.y,         tr.x,             tr.y + tr.h);
    ctx.drawLine(tr.x + tr.w,  tr.y,         tr.x + tr.w,      tr.y + tr.h);

    // ── Value text ──────────────────────────────────────────────────────
    if (showValue_) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", value_);
        auto fc = toFc(b);
        ctx.fill.SetColor(160, 165, 175, 255);
        float ty2 = b.y + b.h - labelH - valH + 1.f;
        ctx.font.Print(buf, b.x + 2.f, ty2, &fc);
    }

    // ── Label ────────────────────────────────────────────────────────────
    if (!label_.empty()) {
        auto fc = toFc(b);
        ctx.fill.SetColor(200, 205, 215, 255);
        float ty2 = b.y + b.h - labelH + 1.f;
        ctx.font.Print(label_.c_str(), b.x + 2.f, ty2, &fc);
    }

    ctx.popClip();
}

void LinearKnob::onMousePress(MouseEvent& e)
{
    if (e.button != 0) return;
    dragging_   = true;
    Rect tr = trackRect();
    // Snap thumb to clicked position immediately
    float norm;
    if (vertical_) {
        norm = 1.f - std::clamp((e.y - tr.y) / tr.h, 0.f, 1.f);
    } else {
        norm = std::clamp((e.x - tr.x) / tr.w, 0.f, 1.f);
    }
    float newV = min_ + norm * (max_ - min_);
    setValue(std::clamp(newV, min_, max_));
    // Set drag anchor to current mouse position at snapped value
    dragStart_  = vertical_ ? e.y : e.x;
    dragStartV_ = value_;
    e.consumed  = true;
}

void LinearKnob::onMouseMove(MouseEvent& e)
{
    if (!dragging_) return;
    Rect tr = trackRect();
    float range  = vertical_ ? tr.h : tr.w;
    float delta  = (vertical_ ? (dragStart_ - e.y) : (e.x - dragStart_)) / range;
    float newV   = dragStartV_ + delta * (max_ - min_);
    setValue(std::clamp(newV, min_, max_));
    e.consumed = true;
}

void LinearKnob::onMouseRelease(MouseEvent& e)
{
    if (e.button != 0) return;
    dragging_ = false;
}

void LinearKnob::onMouseScroll(MouseEvent& e)
{
    float step = (max_ - min_) * 0.02f;
    setValue(std::clamp(value_ + e.scrollY * step, min_, max_));
    e.consumed = true;
}
