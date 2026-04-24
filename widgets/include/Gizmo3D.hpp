#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include "Signal.hpp"
#include "Math.hpp"
#include <functional>

// ═════════════════════════════════════════════════════════════════════════════
//  Gizmo3D — interactive 3D transform handles (Move / Rotate / Scale)
//
//  Drawn as a 2D overlay but uses view/projection matrices to project
//  3D axis handles to screen space. Like ImGuizmo — everything is rendered
//  with 2D primitives (lines, triangles, circles).
//
//  Usage:
//    auto* gizmo = viewport->createChild<Gizmo3D>();
//    gizmo->setViewProjection(viewMat, projMat, vpWidth, vpHeight);
//    gizmo->setTarget(worldPos, worldRot, worldScale);
//    gizmo->setMode(GizmoMode3D::Translate);
//    gizmo->onTranslate3D.connect([](const glm::vec3& delta) { ... });
// ═════════════════════════════════════════════════════════════════════════════

enum class GizmoMode3D { Translate, Rotate, Scale };
enum class GizmoAxis3D { None, X, Y, Z, XY, XZ, YZ, XYZ };

class Gizmo3D : public Widget
{
public:
    Gizmo3D();

    // ── Camera (must be set each frame before paint) ─────────────────────

    /// Set view and projection matrices + viewport size.
    void setViewProjection(const glm::mat4& view, const glm::mat4& proj,
                           int vpWidth, int vpHeight);

    // ── Transform target ─────────────────────────────────────────────────

    void setTarget(const glm::vec3& pos, const glm::vec3& rotDeg, const glm::vec3& scale);

    const glm::vec3& targetPosition() const { return targetPos_; }
    const glm::vec3& targetRotation() const { return targetRot_; }
    const glm::vec3& targetScale()    const { return targetScale_; }

    // ── Mode ─────────────────────────────────────────────────────────────

    void setMode(GizmoMode3D m)   { mode_ = m; }
    GizmoMode3D mode() const      { return mode_; }

    // ── Snapping ─────────────────────────────────────────────────────────

    void  setSnapTranslate(float grid) { snapT_ = grid; }
    void  setSnapRotate(float deg)     { snapR_ = deg; }
    void  setSnapScale(float step)     { snapS_ = step; }

    // ── Visual ───────────────────────────────────────────────────────────

    void  setScreenScale(float s) { screenScale_ = s; }
    float screenScale() const     { return screenScale_; }

    // ── Signals ──────────────────────────────────────────────────────────

    Signal<glm::vec3> onTranslate3D;   // delta in world space
    Signal<glm::vec3> onRotate3D;      // cumulative euler (deg)
    Signal<glm::vec3> onScale3D;       // absolute scale
    Signal<>          onDragStart;
    Signal<>          onDragEnd;

    // ── State ────────────────────────────────────────────────────────────

    GizmoAxis3D activeAxis() const { return activeAxis_; }
    bool        isDragging()  const { return dragging_; }

    // ── Overrides ────────────────────────────────────────────────────────

    void layout() override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;

private:
    // Camera
    glm::mat4 view_ = glm::mat4(1.0f);
    glm::mat4 proj_ = glm::mat4(1.0f);
    int       vpW_  = 800;
    int       vpH_  = 600;

    // Target transform
    glm::vec3 targetPos_   = glm::vec3(0.0f);
    glm::vec3 targetRot_   = glm::vec3(0.0f);  // euler degrees
    glm::vec3 targetScale_ = glm::vec3(1.0f);

    // Mode
    GizmoMode3D mode_ = GizmoMode3D::Translate;

    // Interaction
    GizmoAxis3D activeAxis_ = GizmoAxis3D::None;
    GizmoAxis3D hoverAxis_  = GizmoAxis3D::None;
    bool        dragging_   = false;
    float       dragStartX_ = 0, dragStartY_ = 0;
    glm::vec3   dragStartPos_;
    glm::vec3   dragStartRot_;
    glm::vec3   dragStartScale_;

    // Snapping
    float snapT_ = 0, snapR_ = 0, snapS_ = 0;

    // Visual
    float screenScale_ = 80.0f; // pixel size of gizmo on screen

    // Colors (with orange hover)
    static inline const Color kRed   {170,  50,  50, 255};
    static inline const Color kGreen { 50, 170,  50, 255};
    static inline const Color kBlue  { 50,  80, 180, 255};
    static inline const Color kYellow{255, 255, 100, 255};
    static inline const Color kHover {255, 128,  16, 220};

    // ── Helpers ──────────────────────────────────────────────────────────

    /// Project a 3D world point to 2D screen coords (relative to parent widget).
    glm::vec2 project(const glm::vec3& world) const;

    /// Unproject a 2D screen point to a ray (origin, direction).
    void unproject(float sx, float sy, glm::vec3& rayOrig, glm::vec3& rayDir) const;

    /// Compute the screen-space scale factor so gizmo is consistent size.
    float computeScale() const;

    /// Hit-test axes at screen position.
    GizmoAxis3D hitTest(float mx, float my) const;

    float snap(float val, float grid) const;

    /// Paint translate/rotate/scale handles.
    void paintTranslate3D(PaintContext& ctx);
    void paintRotate3D(PaintContext& ctx);
    void paintScale3D(PaintContext& ctx);
};
