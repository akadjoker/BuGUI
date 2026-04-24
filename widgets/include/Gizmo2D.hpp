#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include "Signal.hpp"
#include <functional>
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
//  Gizmo2D - interactive 2D transform handles (Move / Rotate / Scale)
//
//  Drawn as an overlay inside a ViewportWidget or Canvas.
//  All coordinates are in the parent's local space (pixels).
//
//  Usage:
//    auto* gizmo = viewport->createChild<Gizmo2D>();
//    gizmo->setPosition(200, 150);
//    gizmo->setMode(GizmoMode::Translate);
//    gizmo->onTranslate.connect([](float dx, float dy) { ... });
//    gizmo->onRotate.connect([](float angleDeg) { ... });
//    gizmo->onScale.connect([](float sx, float sy) { ... });
// ═════════════════════════════════════════════════════════════════════════════

enum class GizmoMode  { Translate, Rotate, Scale };
enum class GizmoAxis  { None, X, Y, XY };   // XY = center/free move

class Gizmo2D : public Widget
{
public:
    Gizmo2D();

    // ── Transform target ─────────────────────────────────────────────────

    /// Set the gizmo center position (in parent space).
    void setPosition(float x, float y) { posX_ = x; posY_ = y; }
    float positionX() const { return posX_; }
    float positionY() const { return posY_; }

    /// Current rotation (degrees) - used to orient scale/translate axes.
    void  setRotation(float deg) { rotation_ = deg; }
    float rotation() const       { return rotation_; }

    /// Current scale - used for visual feedback only.
    void  setScale(float sx, float sy) { scaleX_ = sx; scaleY_ = sy; }
    float scaleX() const { return scaleX_; }
    float scaleY() const { return scaleY_; }

    // ── Mode ─────────────────────────────────────────────────────────────

    void setMode(GizmoMode m)    { mode_ = m; }
    GizmoMode mode() const       { return mode_; }

    // ── Snapping ─────────────────────────────────────────────────────────

    void  setSnapTranslate(float grid) { snapTranslate_ = grid; }
    float snapTranslate() const        { return snapTranslate_; }

    void  setSnapRotate(float deg) { snapRotate_ = deg; }
    float snapRotate() const       { return snapRotate_; }

    void  setSnapScale(float step) { snapScale_ = step; }
    float snapScale() const        { return snapScale_; }

    // ── Visual config ────────────────────────────────────────────────────

    void  setAxisLength(float len) { axisLen_ = len; }
    float axisLength() const       { return axisLen_; }

    void  setHandleSize(float s) { handleSize_ = s; }
    float handleSize() const     { return handleSize_; }

    // ── Signals ──────────────────────────────────────────────────────────

    /// Emitted during drag: delta in parent space.
    Signal<float, float> onTranslate;

    /// Emitted during drag: cumulative angle in degrees.
    Signal<float> onRotate;

    /// Emitted during drag: scale factors (1.0 = no change).
    Signal<float, float> onScale;

    /// Emitted when drag starts.
    Signal<> onDragStart;

    /// Emitted when drag ends.
    Signal<> onDragEnd;

    // ── Active axis (read-only) ──────────────────────────────────────────

    GizmoAxis activeAxis() const { return activeAxis_; }
    bool      isDragging()  const { return dragging_; }

    // ── Overrides ────────────────────────────────────────────────────────

    void layout() override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;

private:
    // State
    GizmoMode mode_       = GizmoMode::Translate;
    float     posX_       = 100.0f;
    float     posY_       = 100.0f;
    float     rotation_   = 0.0f;
    float     scaleX_     = 1.0f;
    float     scaleY_     = 1.0f;

    // Interaction
    GizmoAxis activeAxis_ = GizmoAxis::None;
    GizmoAxis hoverAxis_  = GizmoAxis::None;
    bool      dragging_   = false;
    float     dragStartX_ = 0, dragStartY_ = 0;
    float     dragStartAngle_ = 0;
    float     dragStartScaleX_ = 1, dragStartScaleY_ = 1;

    // Snapping (0 = disabled)
    float snapTranslate_ = 0;
    float snapRotate_    = 0;
    float snapScale_     = 0;

    // Visual
    float axisLen_    = 70.0f;
    float handleSize_ = 10.0f;

    // Colors
    static inline const Color kAxisX   {200,  50,  50, 255};
    static inline const Color kAxisY   { 50, 180,  50, 255};
    static inline const Color kCenter  {255, 255, 100, 255};
    static inline const Color kHover   {255, 128,  16, 220};

    // ── Helpers ──────────────────────────────────────────────────────────

    // Convert parent-local mouse coords to gizmo-relative
    void toGizmoSpace(float mx, float my, float& gx, float& gy) const;

    // Hit-test: which axis is under the mouse?
    GizmoAxis hitTest(float mx, float my) const;

    // Snap helper
    float snap(float val, float grid) const;

    // Paint sub-methods
    void paintTranslate(PaintContext& ctx, float cx, float cy);
    void paintRotate(PaintContext& ctx, float cx, float cy);
    void paintScale(PaintContext& ctx, float cx, float cy);
};
