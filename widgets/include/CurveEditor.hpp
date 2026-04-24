#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include "Signal.hpp"
#include <vector>
#include <string>
#include <functional>

// ═════════════════════════════════════════════════════════════════════════════
//  CurveEditor — Bezier/Hermite animation curve editing
//
//  Features:
//  - Multiple named curves with independent colors
//  - Keyframe editing: add, move, delete
//  - Bezier tangent handles (free/aligned/auto)
//  - Zoom and pan (mouse wheel + middle-drag)
//  - Grid with value/time labels
//  - Selection (single + multi-select with Shift)
//  - Snapping (grid, time, value)
//
//  Usage:
//    auto* curve = parent->createChild<CurveEditor>();
//    int id = curve->addCurve("Position X", Color(220,60,60,255));
//    curve->addKey(id, 0.0f, 0.0f);
//    curve->addKey(id, 1.0f, 100.0f);
//    curve->onKeyChanged.connect([](int curveId, int keyIdx, float t, float v){ ... });
// ═════════════════════════════════════════════════════════════════════════════

enum class TangentMode { Free, Aligned, Auto };

struct CurveKey {
    float time  = 0;
    float value = 0;
    // Bezier tangent handles (relative offsets)
    float tanInX  = -0.1f, tanInY  = 0;
    float tanOutX =  0.1f, tanOutY = 0;
    TangentMode tangentMode = TangentMode::Auto;
    bool selected = false;
};

struct CurveData {
    std::string name;
    Color       color;
    std::vector<CurveKey> keys;
    bool visible = true;
};

class CurveEditor : public Widget
{
public:
    CurveEditor();

    // ── Curve management ─────────────────────────────────────────────────

    int  addCurve(const std::string& name, const Color& color);
    void removeCurve(int curveId);
    void clearCurves();

    int  curveCount() const { return static_cast<int>(curves_.size()); }
    CurveData&       curve(int id)       { return curves_[id]; }
    const CurveData& curve(int id) const { return curves_[id]; }

    void setCurveVisible(int curveId, bool v) { curves_[curveId].visible = v; markDirty(); }

    // ── Keyframe management ──────────────────────────────────────────────

    int  addKey(int curveId, float time, float value);
    void removeKey(int curveId, int keyIdx);
    void setKey(int curveId, int keyIdx, float time, float value);

    // ── View control ─────────────────────────────────────────────────────

    void setViewRange(float minT, float maxT, float minV, float maxV);
    void fitView();  // auto-fit to all keyframes

    float viewMinT() const { return viewMinT_; }
    float viewMaxT() const { return viewMaxT_; }
    float viewMinV() const { return viewMinV_; }
    float viewMaxV() const { return viewMaxV_; }

    // ── Snapping ─────────────────────────────────────────────────────────

    void setSnapTime(float s)  { snapTime_ = s; }
    void setSnapValue(float s) { snapValue_ = s; }

    // ── Playhead ─────────────────────────────────────────────────────────

    void  setPlayhead(float t) { playhead_ = t; markDirty(); }
    float playhead() const     { return playhead_; }
    void  setShowPlayhead(bool b) { showPlayhead_ = b; }

    // ── Evaluate ─────────────────────────────────────────────────────────

    float evaluate(int curveId, float time) const;

    // ── Signals ──────────────────────────────────────────────────────────

    Signal<int, int, float, float> onKeyChanged;   // curveId, keyIdx, time, value
    Signal<int, int>               onKeyAdded;      // curveId, keyIdx
    Signal<int, int>               onKeyRemoved;    // curveId, keyIdx

    // ── Overrides ────────────────────────────────────────────────────────

    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;

    void setBgColor(const Color& c) { bgColor_ = c; }

private:
    std::vector<CurveData> curves_;
    Color bgColor_ = Color(28, 30, 34, 255);

    // View transform (time/value → pixel)
    float viewMinT_ = 0, viewMaxT_ = 2;
    float viewMinV_ = -1, viewMaxV_ = 2;

    // Interaction
    bool dragging_     = false;
    bool panning_      = false;
    int  dragCurve_    = -1;
    int  dragKey_      = -1;
    bool dragTangent_  = false;  // dragging a tangent handle?
    bool dragTanIn_    = false;  // in or out tangent?
    float dragStartX_  = 0, dragStartY_ = 0;
    float panStartMinT_ = 0, panStartMaxT_ = 0;
    float panStartMinV_ = 0, panStartMaxV_ = 0;

    // Snapping
    float snapTime_  = 0;
    float snapValue_ = 0;

    // Playhead
    float playhead_     = 0;
    bool  showPlayhead_ = true;

    // ── Helpers ──────────────────────────────────────────────────────────

    // Convert time/value ↔ screen pixel
    float timeToX(float t) const;
    float valueToY(float v) const;
    float xToTime(float x) const;
    float yToValue(float y) const;

    float snap(float val, float grid) const;

    void  autoTangents(CurveData& c, int keyIdx);

    // Bezier evaluation between two keys
    float evalSegment(const CurveKey& k0, const CurveKey& k1, float t) const;

    // Hit testing
    struct HitResult { int curve = -1; int key = -1; bool tangent = false; bool tanIn = false; };
    HitResult hitTest(float mx, float my) const;

    // Painting helpers
    void paintGrid(PaintContext& ctx, const Rect& b);
    void paintCurves(PaintContext& ctx, const Rect& b);
    void paintKeys(PaintContext& ctx, const Rect& b);
    void paintPlayhead(PaintContext& ctx, const Rect& b);
};
