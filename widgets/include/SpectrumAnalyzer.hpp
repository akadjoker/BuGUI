#pragma once

#include "Widget.hpp"
#include <vector>

// ═════════════════════════════════════════════════════════════════════════════
//  SpectrumAnalyzer — frequency spectrum bar display
//
//  Usage:
//    auto* sa = parent->createChild<SpectrumAnalyzer>();
//    sa->setBinCount(32);
//    sa->setMagnitudes(mags.data(), mags.size());  // 0..1 linear
//
//  Features:
//    - Bar display with optional gradient colour (low=green, high=red)
//    - Peak hold per bar with decay
//    - Optional frequency labels (Hz) on X axis
//    - Optional dB labels on Y axis
//    - Log-frequency axis option (musical)
//    - Smooth animation via update(dt)
// ═════════════════════════════════════════════════════════════════════════════

class SpectrumAnalyzer : public Widget
{
public:
    SpectrumAnalyzer();

    // ── Data ──────────────────────────────────────────────────────────────
    void setBinCount(int n);
    int  binCount() const { return (int)bins_.size(); }

    // magnitudes[i] in 0..1 (linear), count must match binCount()
    void setMagnitudes(const float* magnitudes, int count);

    // Smooth decay — call each frame
    void update(float dt);

    // ── Optional axis labels ──────────────────────────────────────────────
    // freqLabels[i] = Hz for bin i
    void setFreqLabels(const float* freqs, int count);
    void setSampleRate(float sr) { sampleRate_ = sr; markDirty(); }

    // ── Appearance ────────────────────────────────────────────────────────
    void setLogFrequency(bool l) { logFreq_     = l;  markDirty(); }
    void setShowLabels(bool s)   { showLabels_  = s;  markDirty(); }
    void setPeakHold(bool p)     { peakHold_    = p;  markDirty(); }
    void setPeakDecay(float r)   { peakDecay_   = r; }
    void setSmoothing(float s)   { smoothing_   = s; }  // 0=instant, 0.9=slow

    void setBarColor(const Color& lo, const Color& hi)
        { colorLow_ = lo; colorHigh_ = hi; markDirty(); }
    void setBgColor(const Color& c)   { bgColor_ = c; markDirty(); }
    void setBarSpacing(int s)         { barSpacing_ = s; markDirty(); }

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override { return {300, 140}; }
    void  paint(PaintContext& ctx) override;

private:
    struct Bin {
        float value   = 0.0f;  // smoothed magnitude
        float peak    = 0.0f;
        float peakTimer = 0.0f;
    };

    std::vector<Bin>   bins_;
    std::vector<float> freqLabels_;
    float sampleRate_  = 44100.0f;

    bool  logFreq_     = false;
    bool  showLabels_  = true;
    bool  peakHold_    = true;
    float peakDecay_   = 1.2f;
    float smoothing_   = 0.7f;
    int   barSpacing_  = 2;

    Color colorLow_    = Color( 80, 200, 100, 255);
    Color colorHigh_   = Color(220,  60,  50, 255);
    Color bgColor_     = Color( 18,  20,  26, 255);
};
