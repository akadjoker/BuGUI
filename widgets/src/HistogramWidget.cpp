#include "HistogramWidget.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdio>

namespace {
    inline Font::ClipRect toFontClip(const Rect& r)
    { return { r.x, r.y, r.w, r.h }; }
}

HistogramWidget::HistogramWidget()
{
    bins_.resize(binCount_, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Data
// ─────────────────────────────────────────────────────────────────────────────

void HistogramWidget::setData(const std::vector<float>& values)
{
    rawData_ = values;
    if (autoRange_ && !values.empty()) {
        rangeMin_ = *std::min_element(values.begin(), values.end());
        rangeMax_ = *std::max_element(values.begin(), values.end());
        if (rangeMax_ <= rangeMin_) rangeMax_ = rangeMin_ + 1.0f;
    }
    computeStats();
    recomputeBins();
    markDirty();
}

void HistogramWidget::setBins(const std::vector<float>& bins, float rangeMin, float rangeMax)
{
    bins_     = bins;
    rangeMin_ = rangeMin;
    rangeMax_ = rangeMax;
    rawData_.clear();
    markDirty();
}

void HistogramWidget::clearData()
{
    rawData_.clear();
    bins_.assign(binCount_, 0.0f);
    markDirty();
}

void HistogramWidget::setBinCount(int n)
{
    binCount_ = std::max(1, n);
    recomputeBins();
    markDirty();
}

void HistogramWidget::computeStats()
{
    if (rawData_.empty()) { mean_ = 0; stddev_ = 0; return; }
    double sum = std::accumulate(rawData_.begin(), rawData_.end(), 0.0);
    mean_ = static_cast<float>(sum / rawData_.size());
    double sq = 0;
    for (float v : rawData_) sq += (v - mean_) * (v - mean_);
    stddev_ = static_cast<float>(std::sqrt(sq / rawData_.size()));
}

void HistogramWidget::recomputeBins()
{
    bins_.assign(binCount_, 0.0f);
    if (rawData_.empty() || binCount_ <= 0) return;
    float span = rangeMax_ - rangeMin_;
    if (span < 1e-9f) return;
    for (float v : rawData_) {
        int idx = static_cast<int>((v - rangeMin_) / span * binCount_);
        idx = std::clamp(idx, 0, binCount_ - 1);
        bins_[idx] += 1.0f;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paint
// ─────────────────────────────────────────────────────────────────────────────

void HistogramWidget::paint(PaintContext& ctx)
{
    
    const Rect b = absoluteRect();
    auto fc = toFontClip(b);

    // Background
    ctx.fill.SetColor(28, 28, 32, 255);
    ctx.fillRect(b.x, b.y, b.w, b.h);

    // Plot area
    Rect pa = { b.x + kPadL, b.y + kPadT, b.w - kPadL - kPadR, b.h - kPadT - kPadB };
    ctx.fill.SetColor(20, 20, 24, 255);
    ctx.fillRect(pa.x, pa.y, pa.w, pa.h);

    // Title
    if (!title_.empty()) {
        ctx.fill.SetColor(200, 200, 200, 255);
        ctx.font.Print(title_.c_str(), b.x + b.w * 0.5f - title_.size() * 3.5f, b.y + 4, &fc);
    }

    if (bins_.empty()) return;

    // Find max bin value
    float maxBin = *std::max_element(bins_.begin(), bins_.end());
    if (maxBin < 1.0f) maxBin = 1.0f;

    ctx.pushClip(pa);

    float barW = pa.w / (float)bins_.size();
    for (int i = 0; i < (int)bins_.size(); ++i) {
        float val = bins_[i];
        float norm;
        if (logScale_) {
            norm = (val > 0) ? std::log1p(val) / std::log1p(maxBin) : 0.0f;
        } else {
            norm = val / maxBin;
        }
        float bh  = norm * pa.h;
        float bx  = pa.x + i * barW;
        float by  = pa.y + pa.h - bh;

        ctx.fill.SetColor(barColor_.r, barColor_.g, barColor_.b, barColor_.a);
        ctx.fillRect(bx + 0.5f, by, barW - 1.0f, bh);

        if (showValues_ && val > 0) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%g", val);
            ctx.fill.SetColor(220, 220, 220, 200);
            ctx.font.Print(buf, bx + 1, by - 12, &fc);
        }
    }

    // Mean line
    if (showMean_ && !rawData_.empty()) {
        float span = rangeMax_ - rangeMin_;
        if (span > 1e-9f) {
            float mx = pa.x + (mean_ - rangeMin_) / span * pa.w;
            ctx.fill.SetColor(255, 200, 80, 220);
            ctx.fillRect(mx, pa.y, 1, pa.h);
        }
    }

    ctx.popClip();

    // Border
    ctx.fill.SetColor(70, 70, 75, 255);
    ctx.fillRect(pa.x,          pa.y,          pa.w, 1);
    ctx.fillRect(pa.x,          pa.y+pa.h-1,   pa.w, 1);
    ctx.fillRect(pa.x,          pa.y,          1,    pa.h);
    ctx.fillRect(pa.x+pa.w-1,   pa.y,          1,    pa.h);

    // X axis labels (min/max/mean)
    ctx.fill.SetColor(130, 130, 130, 255);
    char bufMin[16], bufMax[16];
    snprintf(bufMin, sizeof(bufMin), "%.4g", rangeMin_);
    snprintf(bufMax, sizeof(bufMax), "%.4g", rangeMax_);
    ctx.font.Print(bufMin, pa.x,              pa.y + pa.h + 4, &fc);
    ctx.font.Print(bufMax, pa.x + pa.w - 28,  pa.y + pa.h + 4, &fc);

    if (!rawData_.empty()) {
        // Stats line: mean ± stddev
        char statBuf[48];
        snprintf(statBuf, sizeof(statBuf), "u=%.3g  s=%.3g", mean_, stddev_);
        ctx.fill.SetColor(180, 180, 120, 255);
        ctx.font.Print(statBuf, pa.x + pa.w * 0.3f, pa.y + pa.h + 4, &fc);
    }
}
