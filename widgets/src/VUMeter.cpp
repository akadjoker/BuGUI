#include "VUMeter.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace {
    inline Font::ClipRect toFc(const Rect& r) { return {r.x, r.y, r.w, r.h}; }

    // dB from linear (0..1), returns -inf..0
    inline float todB(float v) { return (v < 1e-7f) ? -96.0f : 20.0f * std::log10(v); }

    // Linear 0..1 from dB (-96..0)
    inline float fromDB(float db) { return std::pow(10.0f, db / 20.0f); }

    // Colour for a level fraction 0..1
    inline Color levelColor(float t)
    {
        if (t > 0.90f) return Color(230, 50,  50, 255);
        if (t > 0.70f) return Color(240,190,  60, 255);
        return              Color( 70, 200, 100, 255);
    }
}

VUMeter::VUMeter()
{
    setChannelCount(2);
}

void VUMeter::setChannelCount(int n)
{
    n = std::max(1, n);
    levels_.resize(n);
    markDirty();
}

void VUMeter::setLevel(int ch, float level)
{
    if (ch < 0 || ch >= (int)levels_.size()) return;
    auto& c = levels_[ch];
    c.level = std::clamp(level, 0.0f, 1.0f);
    if (c.level >= c.peak) {
        c.peak      = c.level;
        c.peakTimer = 1.5f;  // hold for 1.5 s
    }
    markDirty();
}

float VUMeter::level(int ch) const
{
    if (ch < 0 || ch >= (int)levels_.size()) return 0.0f;
    return levels_[ch].level;
}

void VUMeter::update(float dt)
{
    bool dirty = false;
    for (auto& c : levels_) {
        if (c.peakTimer > 0.0f) {
            c.peakTimer -= dt;
            if (c.peakTimer <= 0.0f) {
                c.peakTimer = 0.0f;
                // Start decay
            } else {
                dirty = true;
            }
        }
        if (c.peakTimer <= 0.0f && c.peak > 0.0f) {
            c.peak -= peakDecay_ * dt;
            if (c.peak < 0.0f) c.peak = 0.0f;
            dirty = true;
        }
    }
    if (dirty) markDirty();
}

Widget::Vec2f VUMeter::sizeHint() const
{
    int n = std::max(1, (int)levels_.size());
    int scaleW = (showScale_ ? 28 : 0);
    if (orient_ == Orientation::Vertical) {
        int barW = 18;
        int totalW = scaleW + n * barW + (n - 1) * barSpacing_ + 8;
        return {(float)totalW, 120.0f};
    } else {
        int barH = 18;
        int totalH = n * barH + (n - 1) * barSpacing_ + (showScale_ ? 16 : 0) + 8;
        return {180.0f, (float)totalH};
    }
}

void VUMeter::paintChannel(PaintContext& ctx, const Rect& bar, const Channel& ch) const
{
    const bool vert = (orient_ == Orientation::Vertical);
    const float level = ch.level;
    const float peak  = ch.peak;

    // Background of bar
    ctx.fill.SetColor(30, 32, 36, 255);
    ctx.fillRect(bar.x, bar.y, bar.w, bar.h);

    // Segment count (one per 2px is fine)
    const int segs = vert ? (int)bar.h / 3 : (int)bar.w / 3;
    if (segs < 1) return;

    for (int i = 0; i < segs; ++i) {
        float t = (float)(segs - 1 - i) / (segs - 1);  // 0=bottom, 1=top (or 0=left, 1=right)
        if (vert) t = (float)(segs - 1 - i) / (segs - 1);
        else      t = (float)i / (segs - 1);

        if (t > level) {
            // Dim unlit segment
            Color dim = levelColor(t);
            ctx.fill.SetColor(dim.r/4, dim.g/4, dim.b/4, 255);
        } else {
            Color lit = levelColor(t);
            ctx.fill.SetColor(lit.r, lit.g, lit.b, 255);
        }

        float x, y, w, h;
        if (vert) {
            float segH = bar.h / segs;
            x = bar.x;
            y = bar.y + i * segH + 0.5f;
            w = bar.w;
            h = segH - 1.0f;
        } else {
            float segW = bar.w / segs;
            x = bar.x + i * segW + 0.5f;
            y = bar.y;
            w = segW - 1.0f;
            h = bar.h;
        }
        if (w > 0 && h > 0) ctx.fillRect(x, y, w, h);
    }

    // Peak hold line
    if (peakHold_ && peak > 0.0f) {
        Color c = levelColor(peak);
        ctx.fill.SetColor(c.r, c.g, c.b, 255);
        if (vert) {
            float py = bar.y + bar.h * (1.0f - peak);
            ctx.fillRect(bar.x, py, bar.w, 2);
        } else {
            float px = bar.x + bar.w * peak;
            ctx.fillRect(px, bar.y, 2, bar.h);
        }
    }

    // Border
    ctx.line.SetColor(60, 60, 65, 255);
    ctx.drawLine(bar.x,          bar.y,          bar.x+bar.w,   bar.y);
    ctx.drawLine(bar.x,          bar.y+bar.h-1,  bar.x+bar.w,   bar.y+bar.h-1);
    ctx.drawLine(bar.x,          bar.y,          bar.x,          bar.y+bar.h);
    ctx.drawLine(bar.x+bar.w-1,  bar.y,          bar.x+bar.w-1, bar.y+bar.h);
}

void VUMeter::paint(PaintContext& ctx)
{
    const Rect b = absoluteRect();
    ctx.pushClip(b);

    // Background
    ctx.fill.SetColor(24, 26, 30, 255);
    ctx.fillRect(b.x, b.y, b.w, b.h);

    const bool vert = (orient_ == Orientation::Vertical);
    const int  n    = (int)levels_.size();
    if (n == 0) { ctx.popClip(); return; }

    // dB scale
    const int   scaleW  = (showScale_ && vert)  ? 28 : 0;
    const int   scaleH  = (showScale_ && !vert) ? 16 : 0;
    const float areaX   = b.x + scaleW + 4;
    const float areaY   = b.y + 4;
    const float areaW   = b.w - scaleW - 8;
    const float areaH   = b.h - scaleH - 8;

    // Draw dB scale labels - only show marks that fit (min 12px apart)
    if (showScale_) {
        auto fc = toFc(b);
        ctx.fill.SetColor(140, 140, 140, 255);
        float marks[] = {0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f, -60.0f};
        const float minGap = 12.0f;
        float lastPos = -9999.0f;
        char tmp[8];
        for (float db : marks) {
            // linear position: 0dB = top (t=0), -60dB = bottom (t=1)
            float t = 1.0f - fromDB(db);  // fromDB(-60)~0 → t~1 (bottom), fromDB(0)=1 → t=0 (top)
            if (vert) {
                float y = areaY + (1.0f - fromDB(db)) * areaH;
                // skip if too close to previous label
                if (y - lastPos < minGap) continue;
                lastPos = y;
                snprintf(tmp, sizeof(tmp), "%d", (int)db);
                ctx.font.Print(tmp, b.x + 2, y - 6, &fc);
            } else {
                float x = areaX + fromDB(db) * areaW;
                if (x - lastPos < minGap * 2) continue;
                lastPos = x;
                snprintf(tmp, sizeof(tmp), "%d", (int)db);
                ctx.font.Print(tmp, x - 4, b.y + areaH + 2, &fc);
            }
        }
    }

    // Bar layout
    float totalBarSpace = vert ? areaW : areaH;
    float barSize = (totalBarSpace - (n - 1) * barSpacing_) / n;
    barSize = std::max(4.0f, barSize);

    for (int i = 0; i < n; ++i) {
        Rect bar;
        if (vert) {
            bar.x = areaX + i * (barSize + barSpacing_);
            bar.y = areaY;
            bar.w = barSize;
            bar.h = areaH;
        } else {
            bar.x = areaX;
            bar.y = areaY + i * (barSize + barSpacing_);
            bar.w = areaW;
            bar.h = barSize;
        }
        paintChannel(ctx, bar, levels_[i]);
    }

    ctx.popClip();
}
