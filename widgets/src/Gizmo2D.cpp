#include "Gizmo2D.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <algorithm>

// ═════════════════════════════════════════════════════════════════════════════
//  Gizmo2D implementation
// ═════════════════════════════════════════════════════════════════════════════

static constexpr float kPi = 3.14159265f;

Gizmo2D::Gizmo2D()
{
    // Gizmo doesn't draw a background - it's a transparent overlay
    setSize(0, 0);
}

// Fill parent area so hit-testing reaches us
void Gizmo2D::layout()
{
    if (parent_) {
        rect_.x = 0;
        rect_.y = 0;
        rect_.w = parent_->rect().w;
        rect_.h = parent_->rect().h;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Coordinate helpers
// ─────────────────────────────────────────────────────────────────────────────

void Gizmo2D::toGizmoSpace(float mx, float my, float& gx, float& gy) const
{
    // Parent-absolute to gizmo-relative
    Rect pAbs = parent_ ? parent_->absoluteRect() : Rect{0,0,0,0};
    float lx = mx - pAbs.x - posX_;
    float ly = my - pAbs.y - posY_;

    // Undo rotation
    if (rotation_ != 0.0f) {
        float rad = -rotation_ * kPi / 180.0f;
        float cs = std::cos(rad), sn = std::sin(rad);
        gx = lx * cs - ly * sn;
        gy = lx * sn + ly * cs;
    } else {
        gx = lx;
        gy = ly;
    }
}

GizmoAxis Gizmo2D::hitTest(float mx, float my) const
{
    float gx, gy;
    toGizmoSpace(mx, my, gx, gy);

    float hs = handleSize_;
    float al = axisLen_;

    if (mode_ == GizmoMode::Translate || mode_ == GizmoMode::Scale) {
        // Center square (free move / uniform scale)
        float cs = hs * 1.5f;
        if (gx >= -cs && gx <= cs && gy >= -cs && gy <= cs)
            return GizmoAxis::XY;

        // X axis: arrow region
        if (gx > cs && gx <= al + hs && std::fabs(gy) <= hs)
            return GizmoAxis::X;

        // Y axis: arrow region (Y points UP in screen = negative)
        if (gy < -cs && gy >= -(al + hs) && std::fabs(gx) <= hs)
            return GizmoAxis::Y;
    }
    else if (mode_ == GizmoMode::Rotate) {
        // Ring hit test
        float dist = std::sqrt(gx * gx + gy * gy);
        float ringR = axisLen_ * 0.8f;
        if (std::fabs(dist - ringR) <= hs)
            return GizmoAxis::XY; // rotation is always around center
    }

    return GizmoAxis::None;
}

float Gizmo2D::snap(float val, float grid) const
{
    if (grid <= 0) return val;
    return std::round(val / grid) * grid;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse interaction
// ─────────────────────────────────────────────────────────────────────────────

void Gizmo2D::onMousePress(MouseEvent& e)
{
    if (e.button != 0) return;

    GizmoAxis hit = hitTest(e.x, e.y);
    if (hit == GizmoAxis::None) return;

    dragging_   = true;
    activeAxis_ = hit;
    dragStartX_ = e.x;
    dragStartY_ = e.y;
    dragStartAngle_  = rotation_;
    dragStartScaleX_ = scaleX_;
    dragStartScaleY_ = scaleY_;
    e.consumed = true;
    onDragStart.emit();
}

void Gizmo2D::onMouseRelease(MouseEvent& e)
{
    if (!dragging_) return;
    dragging_   = false;
    activeAxis_ = GizmoAxis::None;
    e.consumed  = true;
    onDragEnd.emit();
}

void Gizmo2D::onMouseMove(MouseEvent& e)
{
    if (!dragging_) {
        // Just update hover
        hoverAxis_ = hitTest(e.x, e.y);
        return;
    }

    e.consumed = true;

    float dx = e.x - dragStartX_;
    float dy = e.y - dragStartY_;

    if (mode_ == GizmoMode::Translate) {
        float tx = 0, ty = 0;
        if (activeAxis_ == GizmoAxis::X || activeAxis_ == GizmoAxis::XY)
            tx = dx;
        if (activeAxis_ == GizmoAxis::Y || activeAxis_ == GizmoAxis::XY)
            ty = dy;

        if (snapTranslate_ > 0) {
            tx = snap(tx, snapTranslate_);
            ty = snap(ty, snapTranslate_);
        }

        // Apply
        posX_ += tx;
        posY_ += ty;
        dragStartX_ = e.x;
        dragStartY_ = e.y;
        onTranslate.emit(tx, ty);
    }
    else if (mode_ == GizmoMode::Rotate) {
        Rect pAbs = parent_ ? parent_->absoluteRect() : Rect{0,0,0,0};
        float cx = pAbs.x + posX_;
        float cy = pAbs.y + posY_;

        float a1 = std::atan2(dragStartY_ - cy, dragStartX_ - cx);
        float a2 = std::atan2(e.y - cy, e.x - cx);
        float da = (a2 - a1) * 180.0f / kPi;

        if (snapRotate_ > 0)
            da = snap(da, snapRotate_);

        rotation_ = dragStartAngle_ + da;
        onRotate.emit(rotation_);
    }
    else if (mode_ == GizmoMode::Scale) {
        float fx = 1.0f, fy = 1.0f;

        if (activeAxis_ == GizmoAxis::XY) {
            // Center handle → uniform scale driven by diagonal drag
            float uniform = 1.0f + (dx - dy) * 0.01f;
            fx = fy = uniform;
        } else {
            if (activeAxis_ == GizmoAxis::X)
                fx = 1.0f + dx * 0.01f;
            if (activeAxis_ == GizmoAxis::Y)
                fy = 1.0f + (-dy) * 0.01f; // negative dy = scale up
        }

        if (snapScale_ > 0) {
            fx = snap(fx, snapScale_);
            fy = snap(fy, snapScale_);
        }

        scaleX_ = dragStartScaleX_ * fx;
        scaleY_ = dragStartScaleY_ * fy;
        onScale.emit(scaleX_, scaleY_);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Painting helpers
// ─────────────────────────────────────────────────────────────────────────────

// Draw a "thick" line (2 triangles forming a ribbon)
static void thickLine(PaintContext& ctx, float x0, float y0, float x1, float y1,
                      float thickness, const Color& c)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.01f) return;
    float nx = -dy / len * thickness * 0.5f;
    float ny =  dx / len * thickness * 0.5f;

    ctx.fill.SetColor(c.r, c.g, c.b, c.a);
    ctx.fill.Triangle(x0 + nx, y0 + ny, x0 - nx, y0 - ny, x1 - nx, y1 - ny, true);
    ctx.fill.Triangle(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x1 + nx, y1 + ny, true);
}

void Gizmo2D::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect pAbs = parent_ ? parent_->absoluteRect() : Rect{0,0,0,0};
    float cx = pAbs.x + posX_;
    float cy = pAbs.y + posY_;

    switch (mode_) {
        case GizmoMode::Translate: paintTranslate(ctx, cx, cy); break;
        case GizmoMode::Rotate:    paintRotate(ctx, cx, cy);    break;
        case GizmoMode::Scale:     paintScale(ctx, cx, cy);     break;
    }

    // Info text with shadow
    char buf[64];
    if (mode_ == GizmoMode::Translate)
        snprintf(buf, sizeof(buf), "%.0f, %.0f", posX_, posY_);
    else if (mode_ == GizmoMode::Rotate)
        snprintf(buf, sizeof(buf), "%.1f\xC2\xB0", rotation_);
    else
        snprintf(buf, sizeof(buf), "%.2f, %.2f", scaleX_, scaleY_);

    ctx.font.SetFontSize(11.0f);
    ctx.font.SetBatch(&ctx.text);
    ctx.font.SetColor(Color(0, 0, 0, 180));
    ctx.font.Print(buf, cx + 16 + 1, cy + 16 + 1);
    ctx.font.SetColor(Color(220, 220, 225, 230));
    ctx.font.Print(buf, cx + 16, cy + 16);
}

void Gizmo2D::paintTranslate(PaintContext& ctx, float cx, float cy)
{
    float al = axisLen_;
    float rad = rotation_ * kPi / 180.0f;
    float cs = std::cos(rad), sn = std::sin(rad);

    float xxD = cs, xyD = sn;
    float yxD = -sn, yyD = cs;

    // Gap at origin (10% of axis length)
    float gap = al * 0.1f;
    float xsX = cx + xxD * gap, xsY = cy + xyD * gap;
    float xeX = cx + xxD * al,  xeY = cy + xyD * al;
    float ysX = cx - yxD * gap, ysY = cy - yyD * gap;
    float yeX = cx - yxD * al,  yeY = cy - yyD * al;

    bool hX  = (hoverAxis_ == GizmoAxis::X  || activeAxis_ == GizmoAxis::X);
    bool hY  = (hoverAxis_ == GizmoAxis::Y  || activeAxis_ == GizmoAxis::Y);
    bool hXY = (hoverAxis_ == GizmoAxis::XY || activeAxis_ == GizmoAxis::XY);

    // X axis - thick line + filled arrow
    Color cX = hX ? kHover : kAxisX;
    thickLine(ctx, xsX, xsY, xeX, xeY, 3.0f, cX);
    float ahLen = 14.0f, ahW = 6.0f;
    ctx.fill.SetColor(cX.r, cX.g, cX.b, cX.a);
    ctx.fill.Triangle(
        xeX + xxD * ahLen,            xeY + xyD * ahLen,
        xeX + xyD * ahW,  xeY - xxD * ahW,
        xeX - xyD * ahW,  xeY + xxD * ahW,
        true);

    // Y axis - thick line + filled arrow
    Color cY = hY ? kHover : kAxisY;
    thickLine(ctx, ysX, ysY, yeX, yeY, 3.0f, cY);
    ctx.fill.SetColor(cY.r, cY.g, cY.b, cY.a);
    ctx.fill.Triangle(
        yeX - yxD * ahLen,            yeY - yyD * ahLen,
        yeX + yyD * ahW,  yeY - yxD * ahW,
        yeX - yyD * ahW,  yeY + yxD * ahW,
        true);

    // Center circle
    Color cC = hXY ? kHover : kCenter;
    ctx.fill.SetColor(cC.r, cC.g, cC.b, 180);
    ctx.fill.Circle(static_cast<int>(cx), static_cast<int>(cy), 6, true);
    ctx.line.SetColor(cC.r, cC.g, cC.b, cC.a);
    ctx.line.Circle(static_cast<int>(cx), static_cast<int>(cy), 6, false);
}

void Gizmo2D::paintRotate(PaintContext& ctx, float cx, float cy)
{
    float ringR = axisLen_ * 0.8f;
    int segments = 64;

    bool hover = (hoverAxis_ == GizmoAxis::XY || activeAxis_ == GizmoAxis::XY);
    Color cR = hover ? kHover : Color(100, 160, 240, 255);

    // Ring - draw as thick line segments
    for (int i = 0; i < segments; ++i) {
        float a0 = (static_cast<float>(i) / segments) * 2.0f * kPi;
        float a1 = (static_cast<float>(i + 1) / segments) * 2.0f * kPi;
        float x0 = cx + std::cos(a0) * ringR, y0 = cy + std::sin(a0) * ringR;
        float x1 = cx + std::cos(a1) * ringR, y1 = cy + std::sin(a1) * ringR;
        thickLine(ctx, x0, y0, x1, y1, 2.5f, cR);
    }

    // Filled pie wedge during drag - rotation feedback
    if (dragging_) {
        float startRad = dragStartAngle_ * kPi / 180.0f;
        float curRad   = rotation_ * kPi / 180.0f;
        Color wedge(255, 128, 16, 80);
        int wedgeSegs = 32;
        float dAngle = curRad - startRad;
        for (int i = 0; i < wedgeSegs; ++i) {
            float a0 = startRad + dAngle * (static_cast<float>(i) / wedgeSegs);
            float a1 = startRad + dAngle * (static_cast<float>(i + 1) / wedgeSegs);
            ctx.fill.SetColor(wedge.r, wedge.g, wedge.b, wedge.a);
            ctx.fill.Triangle(
                cx, cy,
                cx + std::cos(a0) * ringR, cy + std::sin(a0) * ringR,
                cx + std::cos(a1) * ringR, cy + std::sin(a1) * ringR,
                true);
        }
        // Wedge outline
        Color wedgeOutline(255, 128, 16, 200);
        thickLine(ctx, cx, cy, cx + std::cos(startRad) * ringR, cy + std::sin(startRad) * ringR, 1.5f, wedgeOutline);
        thickLine(ctx, cx, cy, cx + std::cos(curRad) * ringR, cy + std::sin(curRad) * ringR, 1.5f, wedgeOutline);
    }

    // Current angle indicator
    float rad = rotation_ * kPi / 180.0f;
    Color indic(255, 200, 60, 255);
    thickLine(ctx, cx, cy, cx + std::cos(rad) * ringR, cy + std::sin(rad) * ringR, 2.0f, indic);

    // Center dot
    ctx.fill.SetColor(255, 200, 60, 220);
    ctx.fill.Circle(static_cast<int>(cx), static_cast<int>(cy), 5, true);
}

void Gizmo2D::paintScale(PaintContext& ctx, float cx, float cy)
{
    float al = axisLen_;
    float rad = rotation_ * kPi / 180.0f;
    float cs = std::cos(rad), sn = std::sin(rad);

    float xxD = cs, xyD = sn;
    float yxD = -sn, yyD = cs;

    float gap = al * 0.1f;
    float xsX = cx + xxD * gap, xsY = cy + xyD * gap;
    float xeX = cx + xxD * al,  xeY = cy + xyD * al;
    float ysX = cx - yxD * gap, ysY = cy - yyD * gap;
    float yeX = cx - yxD * al,  yeY = cy - yyD * al;

    bool hX  = (hoverAxis_ == GizmoAxis::X  || activeAxis_ == GizmoAxis::X);
    bool hY  = (hoverAxis_ == GizmoAxis::Y  || activeAxis_ == GizmoAxis::Y);
    bool hXY = (hoverAxis_ == GizmoAxis::XY || activeAxis_ == GizmoAxis::XY);

    // X axis - thick line + circle tip (like ImGuizmo)
    Color cX = hX ? kHover : kAxisX;
    thickLine(ctx, xsX, xsY, xeX, xeY, 3.0f, cX);
    ctx.fill.SetColor(cX.r, cX.g, cX.b, cX.a);
    ctx.fill.Circle(static_cast<int>(xeX), static_cast<int>(xeY), 5, true);

    // Y axis - thick line + circle tip
    Color cY = hY ? kHover : kAxisY;
    thickLine(ctx, ysX, ysY, yeX, yeY, 3.0f, cY);
    ctx.fill.SetColor(cY.r, cY.g, cY.b, cY.a);
    ctx.fill.Circle(static_cast<int>(yeX), static_cast<int>(yeY), 5, true);

    // Center circle
    Color cC = hXY ? kHover : kCenter;
    ctx.fill.SetColor(cC.r, cC.g, cC.b, 180);
    ctx.fill.Circle(static_cast<int>(cx), static_cast<int>(cy), 6, true);
    ctx.line.SetColor(cC.r, cC.g, cC.b, cC.a);
    ctx.line.Circle(static_cast<int>(cx), static_cast<int>(cy), 6, false);
}
