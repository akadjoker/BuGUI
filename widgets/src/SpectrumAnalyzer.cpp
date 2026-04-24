#include "SpectrumAnalyzer.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace {
    inline Font::ClipRect toFc(const Rect& r) { return {r.x, r.y, r.w, r.h}; }

    // Colour lerp between two colours
    inline Color lerpColor(const Color& a, const Color& b, float t)
    {
        return Color(
            (uint8_t)(a.r + (b.r - a.r) * t),
            (uint8_t)(a.g + (b.g - a.g) * t),
            (uint8_t)(a.b + (b.b - a.b) * t),
            255
        );
    }
}

SpectrumAnalyzer::SpectrumAnalyzer()
{
    setBinCount(32);
}

void SpectrumAnalyzer::setBinCount(int n)
{
    n = std::max(1, n);
    bins_.resize(n);
    markDirty();
}

void SpectrumAnalyzer::setMagnitudes(const float* magnitudes, int count)
{
    int n = std::min(count, (int)bins_.size());
    for (int i = 0; i < n; ++i) {
        float v = std::clamp(magnitudes[i], 0.0f, 1.0f);
        auto& bin = bins_[i];
        // Apply smoothing
        bin.value = bin.value * smoothing_ + v * (1.0f - smoothing_);
        if (bin.value >= bin.peak) {
            bin.peak      = bin.value;
            bin.peakTimer = 1.2f;
        }
    }
    markDirty();
}

void SpectrumAnalyzer::setFreqLabels(const float* freqs, int count)
{
    freqLabels_.assign(freqs, freqs + count);
    markDirty();
}

void SpectrumAnalyzer::update(float dt)
{
    bool dirty = false;
    for (auto& bin : bins_) {
        if (bin.peakTimer > 0.0f) {
            bin.peakTimer -= dt;
            dirty = true;
        }
        if (bin.peakTimer <= 0.0f && bin.peak > 0.0f) {
            bin.peak -= peakDecay_ * dt;
            if (bin.peak < 0.0f) bin.peak = 0.0f;
            dirty = true;
        }
    }
    if (dirty) markDirty();
}

void SpectrumAnalyzer::paint(PaintContext& ctx)
{
 
    const Rect b = absoluteRect();
    ctx.pushClip(b);

    // Background
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, 255);
    ctx.fillRect(b.x, b.y, b.w, b.h);

    const int   n         = (int)bins_.size();
    const float labelH    = (showLabels_ && !freqLabels_.empty()) ? 14.0f : 0.0f;
    const float areaH     = b.h - labelH - 4.0f;
    const float areaY     = b.y + 2.0f;
    const float areaX     = b.x + 2.0f;
    const float areaW     = b.w - 4.0f;

    if (n == 0 || areaH <= 0 || areaW <= 0) { ctx.popClip(); return; }

    // Subtle horizontal grid lines at 25%, 50%, 75%
    ctx.line.SetColor(40, 44, 50, 255);
    for (float frac : {0.25f, 0.5f, 0.75f}) {
        float gy = areaY + areaH * (1.0f - frac);
        ctx.drawLine(areaX, gy, areaX + areaW, gy);
    }

    // Bars
    float totalSpacing = barSpacing_ * (n - 1);
    float barW = (areaW - totalSpacing) / n;
    barW = std::max(1.0f, barW);

    for (int i = 0; i < n; ++i) {
        float xi = areaX + i * (barW + barSpacing_);
        float v  = bins_[i].value;
        float barH = v * areaH;

        if (barH > 0.5f) {
            Color c = lerpColor(colorLow_, colorHigh_, v);
            ctx.fill.SetColor(c.r, c.g, c.b, 255);
            ctx.fillRect(xi, areaY + areaH - barH, barW, barH);
        }

        // Dim background bar
        Color dim = lerpColor(colorLow_, colorHigh_, 0.5f);
        ctx.fill.SetColor(dim.r/5, dim.g/5, dim.b/5, 255);
        ctx.fillRect(xi, areaY, barW, areaH - barH);

        // Peak hold
        if (peakHold_ && bins_[i].peak > 0.0f) {
            float py = areaY + areaH - bins_[i].peak * areaH;
            Color pc = lerpColor(colorLow_, colorHigh_, bins_[i].peak);
            ctx.fill.SetColor(pc.r, pc.g, pc.b, 255);
            ctx.fillRect(xi, py, barW, 2);
        }
    }

    // Frequency labels
    if (showLabels_ && !freqLabels_.empty()) {
        auto fc = toFc(b);
        ctx.fill.SetColor(130, 130, 130, 255);
        int step = std::max(1, n / 8);
        char tmp[12];
        for (int i = 0; i < n; i += step) {
            float xi = areaX + i * (barW + barSpacing_);
            float hz = (i < (int)freqLabels_.size()) ? freqLabels_[i]
                       : (sampleRate_ * 0.5f * i / n);
            if (hz >= 1000.0f)
                snprintf(tmp, sizeof(tmp), "%.0fk", hz / 1000.0f);
            else
                snprintf(tmp, sizeof(tmp), "%.0f", hz);
            ctx.font.Print(tmp, xi, b.y + b.h - labelH, &fc);
        }
    }

    // Border
    ctx.line.SetColor(55, 58, 65, 255);
    ctx.drawLine(b.x,        b.y,        b.x+b.w,   b.y);
    ctx.drawLine(b.x,        b.y+b.h-1,  b.x+b.w,   b.y+b.h-1);
    ctx.drawLine(b.x,        b.y,        b.x,        b.y+b.h);
    ctx.drawLine(b.x+b.w-1,  b.y,        b.x+b.w-1, b.y+b.h);

    ctx.popClip();
}
