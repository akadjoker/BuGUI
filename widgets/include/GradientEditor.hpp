#pragma once

#include "Widget.hpp"
#include "Signal.hpp"
#include "Color.hpp"
#include <vector>
#include <algorithm>

// ═════════════════════════════════════════════════════════════════════════════
//  GradientEditor - horizontal color gradient with draggable stops
//
//  Usage:
//    auto* g = parent->createChild<GradientEditor>();
//    g->addStop(0.0f, Color(0,0,0,255));
//    g->addStop(1.0f, Color(255,255,255,255));
//    g->onChanged.connect([&](const std::vector<GradientStop>& stops){ ... });
//
//  Interaction:
//    - Click empty strip area  → add stop (samples color at that position)
//    - Drag stop triangle      → move stop
//    - Double-click stop       → open color picker (onEditStop signal)
//    - Right-click stop        → delete stop (min 2 stops kept)
//    - Selected stop shown with white border
// ═════════════════════════════════════════════════════════════════════════════

struct GradientStop {
    float pos;   // 0..1
    Color color;
};

class GradientEditor : public Widget
{
public:
    GradientEditor();

    // ── Stops ─────────────────────────────────────────────────────────────
    void addStop(float pos, const Color& color);
    void removeStop(int idx);
    void setStopColor(int idx, const Color& color);
    void setStopPos(int idx, float pos);
    void clearStops();

    const std::vector<GradientStop>& stops() const { return stops_; }
    int  stopCount() const { return static_cast<int>(stops_.size()); }
    int  selectedStop() const { return selected_; }

    // Sample the gradient at position t ∈ [0,1]
    Color sample(float t) const;

    // ── Appearance ────────────────────────────────────────────────────────
    void setBarHeight(float h) { barHeight_ = h; markDirty(); }
    float barHeight() const    { return barHeight_; }

    void setShowAlpha(bool s) { showAlpha_ = s; markDirty(); }
    bool showAlpha() const    { return showAlpha_; }

    // ── Signals ───────────────────────────────────────────────────────────
    Signal<const std::vector<GradientStop>&> onChanged;   // any change
    Signal<int, Color>                       onEditStop;  // double-click → edit color (idx, current color)

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override { return {200, 58}; }
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;

private:
    std::vector<GradientStop> stops_;
    int   selected_   = -1;
    int   dragging_   = -1;
    float dragOffX_   = 0;
    float barHeight_  = 24.0f;
    bool  showAlpha_  = true;

    // Layout helpers
    Rect barRect() const;        // the color bar area
    Rect handleRect(int idx) const;  // triangle handle below bar
    int  hitStop(float mx, float my) const;

    void sortStops();
    void emit();
};
