#include "Gizmo3D.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

// ═════════════════════════════════════════════════════════════════════════════
//  Gizmo3D - ImGuizmo-quality 3D transform handles
// ═════════════════════════════════════════════════════════════════════════════

static constexpr float kPi = 3.14159265f;

// ─────────────────────────────────────────────────────────────────────────────
//  2D drawing helpers
// ─────────────────────────────────────────────────────────────────────────────

static void thickLine(PaintContext& ctx, float x0, float y0, float x1, float y1,
                      float thickness, const Color& c)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.5f) return;
    float nx = -dy / len * thickness * 0.5f;
    float ny =  dx / len * thickness * 0.5f;

    ctx.fill.SetColor(c.r, c.g, c.b, c.a);
    ctx.fill.Triangle(x0 + nx, y0 + ny, x0 - nx, y0 - ny, x1 - nx, y1 - ny, true);
    ctx.fill.Triangle(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x1 + nx, y1 + ny, true);
}

static void filledArrow(PaintContext& ctx, float tipX, float tipY,
                        float dx, float dy, float length, float width, const Color& c)
{
    float px = -dy, py = dx;
    ctx.fill.SetColor(c.r, c.g, c.b, c.a);
    ctx.fill.Triangle(
        tipX + dx * length, tipY + dy * length,
        tipX + px * width,  tipY + py * width,
        tipX - px * width,  tipY - py * width, true);
}

// ─────────────────────────────────────────────────────────────────────────────

Gizmo3D::Gizmo3D()
{
    setSize(0, 0);
}

// Fill parent area so hit-testing reaches us
void Gizmo3D::layout()
{
    if (parent_) {
        rect_.x = 0;
        rect_.y = 0;
        rect_.w = parent_->rect().w;
        rect_.h = parent_->rect().h;
    }
}

void Gizmo3D::setViewProjection(const glm::mat4& view, const glm::mat4& proj,
                                 int vpWidth, int vpHeight)
{
    view_ = view;  proj_ = proj;
    vpW_ = vpWidth;  vpH_ = vpHeight;
}

void Gizmo3D::setTarget(const glm::vec3& pos, const glm::vec3& rotDeg, const glm::vec3& scale)
{
    targetPos_ = pos;  targetRot_ = rotDeg;  targetScale_ = scale;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Projection / Unprojection
// ─────────────────────────────────────────────────────────────────────────────

glm::vec2 Gizmo3D::project(const glm::vec3& world) const
{
    Rect pAbs = parent_ ? parent_->absoluteRect() : Rect{0, 0, (float)vpW_, (float)vpH_};
    glm::vec4 clip = proj_ * view_ * glm::vec4(world, 1.0f);
    if (std::fabs(clip.w) < 1e-6f) return {-9999, -9999};
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    float sx = pAbs.x + (ndc.x * 0.5f + 0.5f) * pAbs.w;
    float sy = pAbs.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * pAbs.h;
    return {sx, sy};
}

void Gizmo3D::unproject(float sx, float sy, glm::vec3& rayOrig, glm::vec3& rayDir) const
{
    Rect pAbs = parent_ ? parent_->absoluteRect() : Rect{0, 0, (float)vpW_, (float)vpH_};
    float ndcX = ((sx - pAbs.x) / pAbs.w) * 2.0f - 1.0f;
    float ndcY = 1.0f - ((sy - pAbs.y) / pAbs.h) * 2.0f;
    glm::mat4 invVP = glm::inverse(proj_ * view_);
    glm::vec4 near4 = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 far4  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
    near4 /= near4.w;  far4 /= far4.w;
    rayOrig = glm::vec3(near4);
    rayDir  = glm::normalize(glm::vec3(far4 - near4));
}

float Gizmo3D::computeScale() const
{
    glm::vec4 viewPos = view_ * glm::vec4(targetPos_, 1.0f);
    float distToCam = std::fabs(viewPos.z);
    float s = distToCam * screenScale_ / static_cast<float>(vpH_);
    return std::max(s, 0.01f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hit testing - center FIRST (small exclusive zone), then axes
// ─────────────────────────────────────────────────────────────────────────────

GizmoAxis3D Gizmo3D::hitTest(float mx, float my) const
{
    glm::vec2 center = project(targetPos_);
    float scale = computeScale();
    float hitR = 12.0f;

    glm::vec3 axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
    GizmoAxis3D axisIds[3] = { GizmoAxis3D::X, GizmoAxis3D::Y, GizmoAxis3D::Z };

    // 0) Center always wins within 12px - tested FIRST so axes don't steal it
    float centerDist = glm::length(glm::vec2(mx, my) - center);
    if (centerDist < 12.0f) return GizmoAxis3D::XYZ;

    // 1) Individual axes (point-to-segment distance)
    float bestDist = 9999.0f;
    GizmoAxis3D bestAxis = GizmoAxis3D::None;

    for (int i = 0; i < 3; ++i) {
        glm::vec3 startPt = targetPos_ + axes[i] * (scale * 0.1f);
        glm::vec3 endPt   = targetPos_ + axes[i] * scale;
        glm::vec2 s2 = project(startPt);
        glm::vec2 e2 = project(endPt);

        glm::vec2 seg = e2 - s2;
        float segLen = glm::length(seg);
        if (segLen < 1.0f) continue;

        glm::vec2 d = glm::vec2(mx, my) - s2;
        float t = glm::dot(d, seg) / (segLen * segLen);
        t = std::clamp(t, 0.0f, 1.0f);
        glm::vec2 closest = s2 + seg * t;
        float lineDist = glm::length(glm::vec2(mx, my) - closest);

        if (lineDist < hitR && lineDist < bestDist) {
            bestDist = lineDist;
            bestAxis = axisIds[i];
        }
    }

    if (bestAxis != GizmoAxis3D::None) return bestAxis;

    // 2) Plane quadrants (for translate/scale) - XY, XZ, YZ
    if (mode_ != GizmoMode3D::Rotate) {
        GizmoAxis3D planeIds[3] = { GizmoAxis3D::XY, GizmoAxis3D::XZ, GizmoAxis3D::YZ };
        int planeAxes[3][2] = { {0,1}, {0,2}, {1,2} };

        for (int p = 0; p < 3; ++p) {
            int a = planeAxes[p][0], b = planeAxes[p][1];
            float lo = 0.3f, hi = 0.6f;
            glm::vec3 c00 = targetPos_ + axes[a] * (scale * lo) + axes[b] * (scale * lo);
            glm::vec3 c10 = targetPos_ + axes[a] * (scale * hi) + axes[b] * (scale * lo);
            glm::vec3 c11 = targetPos_ + axes[a] * (scale * hi) + axes[b] * (scale * hi);
            glm::vec3 c01 = targetPos_ + axes[a] * (scale * lo) + axes[b] * (scale * hi);

            glm::vec2 s00 = project(c00), s10 = project(c10);
            glm::vec2 s11 = project(c11), s01 = project(c01);

            auto sign = [](glm::vec2 p1, glm::vec2 p2, glm::vec2 p3) {
                return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
            };
            auto inTri = [&](glm::vec2 pt, glm::vec2 v1, glm::vec2 v2, glm::vec2 v3) {
                float d1 = sign(pt, v1, v2), d2 = sign(pt, v2, v3), d3 = sign(pt, v3, v1);
                bool neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
                bool pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
                return !(neg && pos);
            };

            glm::vec2 mp(mx, my);
            if (inTri(mp, s00, s10, s11) || inTri(mp, s00, s11, s01))
                return planeIds[p];
        }
    }

    // 3) Rotation rings - per-axis
    if (mode_ == GizmoMode3D::Rotate) {
        for (int i = 0; i < 3; ++i) {
            glm::vec3 normal = axes[i];
            glm::vec3 u, v;
            if (std::fabs(normal.x) < 0.9f)
                u = glm::normalize(glm::cross(normal, glm::vec3(1, 0, 0)));
            else
                u = glm::normalize(glm::cross(normal, glm::vec3(0, 1, 0)));
            v = glm::cross(normal, u);

            int segs = 36;
            float bestRingDist = 9999.0f;
            for (int s = 0; s < segs; ++s) {
                float a0 = (float(s) / segs) * 2.0f * kPi;
                glm::vec3 rp = targetPos_ + (u * std::cos(a0) + v * std::sin(a0)) * scale;
                glm::vec2 sp = project(rp);
                float d = glm::length(glm::vec2(mx, my) - sp);
                if (d < bestRingDist) bestRingDist = d;
            }
            if (bestRingDist < hitR) return axisIds[i];
        }
    }

    // 4) Nothing hit
    return GizmoAxis3D::None;
}

float Gizmo3D::snap(float val, float grid) const
{
    if (grid <= 0) return val;
    return std::round(val / grid) * grid;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse interaction
// ─────────────────────────────────────────────────────────────────────────────

void Gizmo3D::onMousePress(MouseEvent& e)
{
    if (e.button != 0) return;

    GizmoAxis3D hit = hitTest(e.x, e.y);
    if (hit == GizmoAxis3D::None) return;

    dragging_   = true;
    activeAxis_ = hit;
    dragStartX_ = e.x;
    dragStartY_ = e.y;
    dragStartPos_   = targetPos_;
    dragStartRot_   = targetRot_;
    dragStartScale_ = targetScale_;
    e.consumed = true;
    onDragStart.emit();
}

void Gizmo3D::onMouseRelease(MouseEvent& e)
{
    if (!dragging_) return;
    dragging_   = false;
    activeAxis_ = GizmoAxis3D::None;
    e.consumed  = true;
    onDragEnd.emit();
}

void Gizmo3D::onMouseMove(MouseEvent& e)
{
    if (!dragging_) {
        hoverAxis_ = hitTest(e.x, e.y);
        return;
    }

    e.consumed = true;

    float dx = e.x - dragStartX_;
    float dy = e.y - dragStartY_;

    glm::vec4 viewPos = view_ * glm::vec4(targetPos_, 1.0f);
    float distToCam = std::fabs(viewPos.z);
    float pixelToWorld = distToCam / static_cast<float>(vpH_);

    if (mode_ == GizmoMode3D::Translate) {
        glm::vec3 delta(0.0f);
        glm::vec2 center = project(targetPos_);

        auto axisMove = [&](const glm::vec3& axis) {
            glm::vec2 axEnd = project(targetPos_ + axis);
            glm::vec2 axDir = axEnd - center;
            float axLen = glm::length(axDir);
            if (axLen < 1.0f) return 0.0f;
            axDir /= axLen;
            return glm::dot(glm::vec2(dx, dy), axDir) * pixelToWorld;
        };

        bool hasX = (activeAxis_ == GizmoAxis3D::X || activeAxis_ == GizmoAxis3D::XY ||
                     activeAxis_ == GizmoAxis3D::XZ || activeAxis_ == GizmoAxis3D::XYZ);
        bool hasY = (activeAxis_ == GizmoAxis3D::Y || activeAxis_ == GizmoAxis3D::XY ||
                     activeAxis_ == GizmoAxis3D::YZ || activeAxis_ == GizmoAxis3D::XYZ);
        bool hasZ = (activeAxis_ == GizmoAxis3D::Z || activeAxis_ == GizmoAxis3D::XZ ||
                     activeAxis_ == GizmoAxis3D::YZ || activeAxis_ == GizmoAxis3D::XYZ);

        if (hasX) delta.x = axisMove(glm::vec3(1, 0, 0));
        if (hasY) delta.y = axisMove(glm::vec3(0, 1, 0));
        if (hasZ) delta.z = axisMove(glm::vec3(0, 0, 1));

        if (snapT_ > 0) {
            delta.x = snap(delta.x, snapT_);
            delta.y = snap(delta.y, snapT_);
            delta.z = snap(delta.z, snapT_);
        }

        targetPos_ = dragStartPos_ + delta;
        onTranslate3D.emit(delta);
    }
    else if (mode_ == GizmoMode3D::Rotate) {
        glm::vec2 center = project(targetPos_);
        float a1 = std::atan2(dragStartY_ - center.y, dragStartX_ - center.x);
        float a2 = std::atan2(e.y - center.y, e.x - center.x);
        float da = (a2 - a1) * 180.0f / kPi;

        if (snapR_ > 0) da = snap(da, snapR_);

        glm::vec3 rot = dragStartRot_;
        if (activeAxis_ == GizmoAxis3D::X) rot.x += da;
        else if (activeAxis_ == GizmoAxis3D::Y) rot.y += da;
        else if (activeAxis_ == GizmoAxis3D::Z) rot.z += da;
        else rot.y += da;

        targetRot_ = rot;
        onRotate3D.emit(rot);
    }
    else if (mode_ == GizmoMode3D::Scale) {
        glm::vec3 s = dragStartScale_;

        if (activeAxis_ == GizmoAxis3D::XYZ) {
            // Center handle → uniform scale
            float uniform = 1.0f + (dx - dy) * 0.005f;
            s *= uniform;
        } else {
            float factor = 1.0f + dx * 0.005f;
            bool hasX = (activeAxis_ == GizmoAxis3D::X || activeAxis_ == GizmoAxis3D::XY ||
                         activeAxis_ == GizmoAxis3D::XZ);
            bool hasY = (activeAxis_ == GizmoAxis3D::Y || activeAxis_ == GizmoAxis3D::XY ||
                         activeAxis_ == GizmoAxis3D::YZ);
            bool hasZ = (activeAxis_ == GizmoAxis3D::Z || activeAxis_ == GizmoAxis3D::XZ ||
                         activeAxis_ == GizmoAxis3D::YZ);

            if (hasX) s.x *= factor;
            if (hasY) s.y *= (1.0f + (-dy) * 0.005f);
            if (hasZ) s.z *= factor;
        }

        if (snapS_ > 0) {
            s.x = snap(s.x, snapS_);
            s.y = snap(s.y, snapS_);
            s.z = snap(s.z, snapS_);
        }

        targetScale_ = s;
        onScale3D.emit(s);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Painting
// ─────────────────────────────────────────────────────────────────────────────

void Gizmo3D::paint(PaintContext& ctx)
{
    if (!visible_) return;

    switch (mode_) {
        case GizmoMode3D::Translate: paintTranslate3D(ctx); break;
        case GizmoMode3D::Rotate:    paintRotate3D(ctx);    break;
        case GizmoMode3D::Scale:     paintScale3D(ctx);     break;
    }

    // Info text with shadow
    glm::vec2 c = project(targetPos_);
    char buf[80];
    if (mode_ == GizmoMode3D::Translate)
        snprintf(buf, sizeof(buf), "%.1f, %.1f, %.1f", targetPos_.x, targetPos_.y, targetPos_.z);
    else if (mode_ == GizmoMode3D::Rotate)
        snprintf(buf, sizeof(buf), "%.1f\xC2\xB0, %.1f\xC2\xB0, %.1f\xC2\xB0",
                 targetRot_.x, targetRot_.y, targetRot_.z);
    else
        snprintf(buf, sizeof(buf), "%.2f, %.2f, %.2f",
                 targetScale_.x, targetScale_.y, targetScale_.z);

    ctx.font.SetFontSize(11.0f);
    ctx.font.SetBatch(&ctx.text);
    ctx.font.SetColor(Color(0, 0, 0, 180));
    ctx.font.Print(buf, c.x + 16 + 1, c.y + 16 + 1);
    ctx.font.SetColor(Color(220, 220, 225, 230));
    ctx.font.Print(buf, c.x + 16, c.y + 16);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Translate: thick axis lines with gap, filled arrow heads, plane quadrants
// ─────────────────────────────────────────────────────────────────────────────

void Gizmo3D::paintTranslate3D(PaintContext& ctx)
{
    float scale = computeScale();
    glm::vec2 center = project(targetPos_);

    glm::vec3 axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
    Color colors[3]   = { kRed, kGreen, kBlue };
    GizmoAxis3D axisIds[3] = { GizmoAxis3D::X, GizmoAxis3D::Y, GizmoAxis3D::Z };

    // Plane quadrants (draw behind axes)
    GizmoAxis3D planeIds[3] = { GizmoAxis3D::XY, GizmoAxis3D::XZ, GizmoAxis3D::YZ };
    int planeAxes[3][2] = { {0,1}, {0,2}, {1,2} };
    Color planeColors[3] = { kBlue, kGreen, kRed };

    for (int p = 0; p < 3; ++p) {
        int a = planeAxes[p][0], b = planeAxes[p][1];
        bool hPlane = (hoverAxis_ == planeIds[p] || activeAxis_ == planeIds[p]);

        glm::vec3 c00 = targetPos_ + axes[a] * (scale * 0.3f) + axes[b] * (scale * 0.3f);
        glm::vec3 c10 = targetPos_ + axes[a] * (scale * 0.6f) + axes[b] * (scale * 0.3f);
        glm::vec3 c11 = targetPos_ + axes[a] * (scale * 0.6f) + axes[b] * (scale * 0.6f);
        glm::vec3 c01 = targetPos_ + axes[a] * (scale * 0.3f) + axes[b] * (scale * 0.6f);

        glm::vec2 s00 = project(c00), s10 = project(c10);
        glm::vec2 s11 = project(c11), s01 = project(c01);

        Color fc = hPlane ? kHover : planeColors[p];
        uint8_t alpha = hPlane ? 100 : 50;

        ctx.fill.SetColor(fc.r, fc.g, fc.b, alpha);
        ctx.fill.Triangle(s00.x, s00.y, s10.x, s10.y, s11.x, s11.y, true);
        ctx.fill.Triangle(s00.x, s00.y, s11.x, s11.y, s01.x, s01.y, true);

        Color oc = hPlane ? kHover : planeColors[p];
        ctx.line.SetColor(oc.r, oc.g, oc.b, 180);
        ctx.line.Line2D(s00.x, s00.y, s10.x, s10.y);
        ctx.line.Line2D(s10.x, s10.y, s11.x, s11.y);
        ctx.line.Line2D(s11.x, s11.y, s01.x, s01.y);
        ctx.line.Line2D(s01.x, s01.y, s00.x, s00.y);
    }

    // Axis lines + arrow heads
    for (int i = 0; i < 3; ++i) {
        bool h = (hoverAxis_ == axisIds[i] || activeAxis_ == axisIds[i]);
        Color c = h ? kHover : colors[i];

        glm::vec3 startPt = targetPos_ + axes[i] * (scale * 0.1f);
        glm::vec3 endPt   = targetPos_ + axes[i] * scale;
        glm::vec2 s0 = project(startPt);
        glm::vec2 s1 = project(endPt);

        thickLine(ctx, s0.x, s0.y, s1.x, s1.y, 3.0f, c);

        glm::vec2 d = s1 - s0;
        float dLen = glm::length(d);
        if (dLen < 2.0f) continue;
        d /= dLen;

        filledArrow(ctx, s1.x, s1.y, d.x, d.y, 14.0f, 6.0f, c);
    }

    // Center circle
    bool hXYZ = (hoverAxis_ == GizmoAxis3D::XYZ || activeAxis_ == GizmoAxis3D::XYZ);
    Color cc = hXYZ ? kHover : kYellow;
    ctx.fill.SetColor(cc.r, cc.g, cc.b, 200);
    ctx.fill.Circle(static_cast<int>(center.x), static_cast<int>(center.y), 6, true);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Rotate: per-axis rings (visible half only), pie wedge during drag
// ─────────────────────────────────────────────────────────────────────────────

void Gizmo3D::paintRotate3D(PaintContext& ctx)
{
    float scale = computeScale();
    glm::vec2 center = project(targetPos_);

    glm::mat4 invView = glm::inverse(view_);
    glm::vec3 camPos(invView[3]);
    glm::vec3 viewDir = glm::normalize(targetPos_ - camPos);

    struct RingDef { glm::vec3 normal; Color color; GizmoAxis3D axis; };
    RingDef rings[] = {
        {{1,0,0}, kRed,   GizmoAxis3D::X},
        {{0,1,0}, kGreen, GizmoAxis3D::Y},
        {{0,0,1}, kBlue,  GizmoAxis3D::Z},
    };

    int segments = 64;

    for (auto& ring : rings) {
        bool h = (hoverAxis_ == ring.axis || activeAxis_ == ring.axis);
        Color c = h ? kHover : ring.color;

        glm::vec3 u, v;
        if (std::fabs(ring.normal.x) < 0.9f)
            u = glm::normalize(glm::cross(ring.normal, glm::vec3(1, 0, 0)));
        else
            u = glm::normalize(glm::cross(ring.normal, glm::vec3(0, 1, 0)));
        v = glm::cross(ring.normal, u);

        for (int i = 0; i < segments; ++i) {
            float a0 = (float(i) / segments) * 2.0f * kPi;
            float a1 = (float(i + 1) / segments) * 2.0f * kPi;

            glm::vec3 p0 = targetPos_ + (u * std::cos(a0) + v * std::sin(a0)) * scale;
            glm::vec3 p1 = targetPos_ + (u * std::cos(a1) + v * std::sin(a1)) * scale;

            glm::vec3 midDir = glm::normalize((p0 + p1) * 0.5f - targetPos_);
            float faceDot = glm::dot(midDir, viewDir);

            if (faceDot < 0) {
                thickLine(ctx, project(p0).x, project(p0).y,
                         project(p1).x, project(p1).y, 2.5f, c);
            } else {
                Color faded(c.r, c.g, c.b, 60);
                glm::vec2 sp0 = project(p0), sp1 = project(p1);
                ctx.line.SetColor(faded.r, faded.g, faded.b, faded.a);
                ctx.line.Line2D(sp0.x, sp0.y, sp1.x, sp1.y);
            }
        }
    }

    // Filled pie wedge during drag
    if (dragging_ && activeAxis_ != GizmoAxis3D::None) {
        float startAngle = std::atan2(dragStartY_ - center.y, dragStartX_ - center.x);

        float ringR = 0.0f;
        for (int i = 0; i < 3; ++i) {
            glm::vec3 rp = targetPos_ + glm::vec3(i==0?1.0f:0, i==1?1.0f:0, i==2?1.0f:0) * scale;
            float d = glm::length(project(rp) - center);
            if (d > ringR) ringR = d;
        }

        float deltaAngle = 0.0f;
        if (activeAxis_ == GizmoAxis3D::X) deltaAngle = (targetRot_.x - dragStartRot_.x) * kPi / 180.0f;
        else if (activeAxis_ == GizmoAxis3D::Y) deltaAngle = (targetRot_.y - dragStartRot_.y) * kPi / 180.0f;
        else if (activeAxis_ == GizmoAxis3D::Z) deltaAngle = (targetRot_.z - dragStartRot_.z) * kPi / 180.0f;

        if (std::fabs(deltaAngle) > 0.001f) {
            int wedgeSegs = 32;
            Color wedge(255, 128, 16, 60);
            for (int i = 0; i < wedgeSegs; ++i) {
                float a0 = startAngle + deltaAngle * (float(i) / wedgeSegs);
                float a1 = startAngle + deltaAngle * (float(i + 1) / wedgeSegs);
                ctx.fill.SetColor(wedge.r, wedge.g, wedge.b, wedge.a);
                ctx.fill.Triangle(
                    center.x, center.y,
                    center.x + std::cos(a0) * ringR, center.y + std::sin(a0) * ringR,
                    center.x + std::cos(a1) * ringR, center.y + std::sin(a1) * ringR, true);
            }
            Color wol(255, 128, 16, 180);
            thickLine(ctx, center.x, center.y,
                     center.x + std::cos(startAngle) * ringR,
                     center.y + std::sin(startAngle) * ringR, 1.5f, wol);
            thickLine(ctx, center.x, center.y,
                     center.x + std::cos(startAngle + deltaAngle) * ringR,
                     center.y + std::sin(startAngle + deltaAngle) * ringR, 1.5f, wol);
        }
    }

    // Outer screen-space ring
    float outerR = 0;
    for (int i = 0; i < 3; ++i) {
        glm::vec3 rp = targetPos_ + glm::vec3(i==0?1.0f:0, i==1?1.0f:0, i==2?1.0f:0) * scale;
        float d = glm::length(project(rp) - center);
        if (d > outerR) outerR = d;
    }
    outerR *= 1.15f;
    bool hXYZ = (hoverAxis_ == GizmoAxis3D::XYZ || activeAxis_ == GizmoAxis3D::XYZ);
    Color outerC = hXYZ ? kHover : Color(180, 180, 180, 120);
    for (int i = 0; i < segments; ++i) {
        float a0 = (float(i) / segments) * 2.0f * kPi;
        float a1 = (float(i + 1) / segments) * 2.0f * kPi;
        thickLine(ctx,
                 center.x + std::cos(a0) * outerR, center.y + std::sin(a0) * outerR,
                 center.x + std::cos(a1) * outerR, center.y + std::sin(a1) * outerR,
                 1.5f, outerC);
    }

    // Center dot
    Color cc = hXYZ ? kHover : kYellow;
    ctx.fill.SetColor(cc.r, cc.g, cc.b, 200);
    ctx.fill.Circle(static_cast<int>(center.x), static_cast<int>(center.y), 5, true);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scale: thick lines with gap, circle tips, plane quadrants
// ─────────────────────────────────────────────────────────────────────────────

void Gizmo3D::paintScale3D(PaintContext& ctx)
{
    float scale = computeScale();
    glm::vec2 center = project(targetPos_);

    glm::vec3 axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
    Color colors[3]   = { kRed, kGreen, kBlue };
    GizmoAxis3D axisIds[3] = { GizmoAxis3D::X, GizmoAxis3D::Y, GizmoAxis3D::Z };

    // Plane quadrants
    GizmoAxis3D planeIds[3] = { GizmoAxis3D::XY, GizmoAxis3D::XZ, GizmoAxis3D::YZ };
    int planeAxes[3][2] = { {0,1}, {0,2}, {1,2} };
    Color planeColors[3] = { kBlue, kGreen, kRed };

    for (int p = 0; p < 3; ++p) {
        int a = planeAxes[p][0], b = planeAxes[p][1];
        bool hPlane = (hoverAxis_ == planeIds[p] || activeAxis_ == planeIds[p]);

        glm::vec3 c00 = targetPos_ + axes[a] * (scale * 0.3f) + axes[b] * (scale * 0.3f);
        glm::vec3 c10 = targetPos_ + axes[a] * (scale * 0.6f) + axes[b] * (scale * 0.3f);
        glm::vec3 c11 = targetPos_ + axes[a] * (scale * 0.6f) + axes[b] * (scale * 0.6f);
        glm::vec3 c01 = targetPos_ + axes[a] * (scale * 0.3f) + axes[b] * (scale * 0.6f);

        glm::vec2 s00 = project(c00), s10 = project(c10);
        glm::vec2 s11 = project(c11), s01 = project(c01);

        Color fc = hPlane ? kHover : planeColors[p];
        uint8_t alpha = hPlane ? 100 : 50;

        ctx.fill.SetColor(fc.r, fc.g, fc.b, alpha);
        ctx.fill.Triangle(s00.x, s00.y, s10.x, s10.y, s11.x, s11.y, true);
        ctx.fill.Triangle(s00.x, s00.y, s11.x, s11.y, s01.x, s01.y, true);
    }

    // Axis lines + circle tips
    for (int i = 0; i < 3; ++i) {
        bool h = (hoverAxis_ == axisIds[i] || activeAxis_ == axisIds[i]);
        Color c = h ? kHover : colors[i];

        glm::vec3 startPt = targetPos_ + axes[i] * (scale * 0.1f);
        glm::vec3 endPt   = targetPos_ + axes[i] * scale;
        glm::vec2 s0 = project(startPt);
        glm::vec2 s1 = project(endPt);

        thickLine(ctx, s0.x, s0.y, s1.x, s1.y, 3.0f, c);

        ctx.fill.SetColor(c.r, c.g, c.b, c.a);
        ctx.fill.Circle(static_cast<int>(s1.x), static_cast<int>(s1.y), 5, true);
    }

    // Center circle
    bool hXYZ = (hoverAxis_ == GizmoAxis3D::XYZ || activeAxis_ == GizmoAxis3D::XYZ);
    Color cc = hXYZ ? kHover : kYellow;
    ctx.fill.SetColor(cc.r, cc.g, cc.b, 200);
    ctx.fill.Circle(static_cast<int>(center.x), static_cast<int>(center.y), 6, true);
}
