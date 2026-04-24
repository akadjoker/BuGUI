#pragma once

#include "Widget.hpp"
#include "Color.hpp"
#include <vector>
#include <string>

// ═════════════════════════════════════════════════════════════════════════════
//  HistogramWidget — Value distribution display
//
//  Usage:
//    auto* h = parent->createChild<HistogramWidget>();
//    h->setData(myFloatVec);          // compute bins automatically
//    h->setBinCount(32);
//    h->setBarColor(Color(100,180,255,200));
//    h->setTitle("Pixel Brightness");
// ═════════════════════════════════════════════════════════════════════════════

class HistogramWidget : public Widget
{
public:
    HistogramWidget();

    // ── Data ──────────────────────────────────────────────────────────────
    void setData(const std::vector<float>& values);
    void setBins(const std::vector<float>& bins, float rangeMin, float rangeMax);
    void clearData();

    // Recompute bins from raw data
    void setBinCount(int n);
    int  binCount() const { return binCount_; }

    // ── Range ─────────────────────────────────────────────────────────────
    void setRange(float lo, float hi) { rangeMin_ = lo; rangeMax_ = hi; recomputeBins(); markDirty(); }
    float rangeMin() const { return rangeMin_; }
    float rangeMax() const { return rangeMax_; }
    void  setAutoRange(bool a) { autoRange_ = a; }

    // ── Appearance ────────────────────────────────────────────────────────
    void  setBarColor(const Color& c) { barColor_ = c; markDirty(); }
    Color barColor() const            { return barColor_; }

    void setTitle(const std::string& t) { title_ = t; markDirty(); }
    void setShowValues(bool s) { showValues_ = s; markDirty(); }
    void setShowMean(bool s)   { showMean_ = s; markDirty(); }
    void setLogScale(bool s)   { logScale_ = s; markDirty(); }

    // Stats (computed from raw data)
    float mean()   const { return mean_; }
    float stddev() const { return stddev_; }
    float minVal() const { return rangeMin_; }
    float maxVal() const { return rangeMax_; }

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override { return {200, 120}; }
    void  paint(PaintContext& ctx) override;

private:
    std::vector<float> rawData_;
    std::vector<float> bins_;   // count per bin

    int   binCount_  = 32;
    float rangeMin_  = 0, rangeMax_ = 1;
    bool  autoRange_ = true;
    bool  logScale_  = false;
    bool  showValues_ = false;
    bool  showMean_   = true;
    float mean_       = 0;
    float stddev_     = 0;

    Color barColor_ = Color(100, 160, 230, 200);
    std::string title_;

    static constexpr float kPadL = 8.0f;
    static constexpr float kPadR = 8.0f;
    static constexpr float kPadT = 20.0f;  // title
    static constexpr float kPadB = 20.0f;  // x labels

    void recomputeBins();
    void computeStats();
};
