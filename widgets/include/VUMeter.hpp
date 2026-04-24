#pragma once

#include "Widget.hpp"
#include <vector>
#include <algorithm>

// ═════════════════════════════════════════════════════════════════════════════
//  VUMeter - Volume Unit meter, one or more channels
//
//  Usage:
//    auto* vu = parent->createChild<VUMeter>();
//    vu->setChannelCount(2);  // stereo
//    vu->setLevel(0, 0.8f);   // channel 0, 0..1
//    vu->setLevel(1, 0.5f);
//
//  Features:
//    - Vertical or horizontal orientation
//    - Peak hold indicator (decays over time via update())
//    - Colour zones: green (0–70%), yellow (70–90%), red (90–100%)
//    - Optional dB scale labels (–inf to 0 dBFS)
//    - call update(dt) each frame to animate peak decay
// ═════════════════════════════════════════════════════════════════════════════

class VUMeter : public Widget
{
public:
    enum class Orientation { Vertical, Horizontal };

    VUMeter();

    // ── Data ──────────────────────────────────────────────────────────────
    void setChannelCount(int n);
    int  channelCount() const { return (int)levels_.size(); }

    // level: 0.0 (silence) … 1.0 (0 dBFS / clip)
    void setLevel(int ch, float level);
    float level(int ch) const;

    // Advance peak-hold decay. Call every frame with elapsed seconds.
    void update(float dt);

    // ── Appearance ────────────────────────────────────────────────────────
    void setOrientation(Orientation o) { orient_ = o; markDirty(); }
    Orientation orientation() const    { return orient_; }

    void setShowScale(bool s) { showScale_ = s; markDirty(); }
    bool showScale() const    { return showScale_; }

    void setPeakHold(bool p)  { peakHold_ = p; markDirty(); }
    bool peakHold() const     { return peakHold_; }

    void setPeakDecayRate(float r) { peakDecay_ = r; }

    void setBarSpacing(int s) { barSpacing_ = s; markDirty(); }

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override;
    void  paint(PaintContext& ctx) override;

private:
    struct Channel {
        float level = 0.0f;
        float peak  = 0.0f;   // peak hold value
        float peakTimer = 0.0f; // time since peak was set
    };

    std::vector<Channel> levels_;
    Orientation orient_    = Orientation::Vertical;
    bool        showScale_ = true;
    bool        peakHold_  = true;
    float       peakDecay_ = 0.5f;  // units/sec
    int         barSpacing_ = 4;

    void paintChannel(PaintContext& ctx, const Rect& bar, const Channel& ch) const;
};
