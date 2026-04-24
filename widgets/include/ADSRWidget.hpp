#pragma once

#include "Widget.hpp"
#include <vector>

// ═════════════════════════════════════════════════════════════════════════════
//  ADSRWidget — Attack / Decay / Sustain / Release envelope editor
//
//  Usage:
//    auto* adsr = parent->createChild<ADSRWidget>();
//    adsr->setADSR(0.01f, 0.1f, 0.7f, 0.3f);  // seconds, seconds, 0..1, seconds
//    adsr->onChanged.connect([&](float a, float d, float s, float r){ ... });
//
//  Interaction:
//    - Drag the 3 curve handles (A peak, D knee, S/R start) to edit
//    - Each handle snaps horizontally; sustain snaps vertically
// ═════════════════════════════════════════════════════════════════════════════

class ADSRWidget : public Widget
{
public:
    ADSRWidget();

    // ── Parameters ────────────────────────────────────────────────────────
    void  setAttack(float s)  { a_ = std::max(0.001f, s); markDirty(); }
    void  setDecay(float s)   { d_ = std::max(0.001f, s); markDirty(); }
    void  setSustain(float v) { s_ = std::clamp(v, 0.0f, 1.0f); markDirty(); }
    void  setRelease(float s) { r_ = std::max(0.001f, s); markDirty(); }

    void  setADSR(float a, float d, float s, float r)
          { a_ = std::max(0.001f, a); d_ = std::max(0.001f, d);
            s_ = std::clamp(s, 0.0f, 1.0f); r_ = std::max(0.001f, r);
            markDirty(); }

    float attack()  const { return a_; }
    float decay()   const { return d_; }
    float sustain() const { return s_; }
    float release() const { return r_; }

    // ── Appearance ────────────────────────────────────────────────────────
    void setLineColor(const Color& c)     { lineColor_  = c; markDirty(); }
    void setFillColor(const Color& c)     { fillColor_  = c; markDirty(); }
    void setBgColor(const Color& c)       { bgColor_    = c; markDirty(); }
    void setHandleColor(const Color& c)   { handleColor_ = c; markDirty(); }
    void setShowLabels(bool s)            { showLabels_ = s; markDirty(); }

    // ── Signals ───────────────────────────────────────────────────────────
    Signal<float,float,float,float> onChanged;  // a, d, s, r

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override { return {260, 120}; }
    void  paint(PaintContext& ctx) override;
    void  onMousePress(MouseEvent& e) override;
    void  onMouseMove(MouseEvent& e) override;
    void  onMouseRelease(MouseEvent& e) override;

private:
    // Map ADSR parameters → screen X positions
    // Returns 4 X coords: peakX, decayX, sustainX, releaseX
    struct Points { float x0,y0, x1,y1, x2,y2, x3,y3, x4,y4; };
    Points computePoints(const Rect& area) const;

    float a_ = 0.05f;   // attack  (seconds, treated as ratio for display)
    float d_ = 0.10f;   // decay
    float s_ = 0.70f;   // sustain level 0..1
    float r_ = 0.20f;   // release

    Color lineColor_   = Color(100, 200, 240, 255);
    Color fillColor_   = Color(100, 200, 240,  50);
    Color bgColor_     = Color( 20,  24,  30, 255);
    Color handleColor_ = Color(255, 255, 255, 220);
    bool  showLabels_  = true;

    int   dragging_    = -1;  // 0=attackPeak, 1=decayKnee, 2=sustainEnd
    float dragOffX_    = 0.0f;
    float dragOffY_    = 0.0f;

    static constexpr float kHandleR = 5.0f;
    int hitHandle(float mx, float my) const;
};
