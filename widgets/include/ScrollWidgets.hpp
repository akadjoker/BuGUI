#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include <functional>

struct Texture;

class Canvas : public Widget
{
public:
    Canvas() = default;

    void setBgColor(const Color& c) { bgColor_ = c; }
    const Color& bgColor() const { return bgColor_; }

    // User-supplied paint callback — called inside the canvas clip region
    using PaintCallback = std::function<void(PaintContext& ctx, const Rect& bounds)>;
    void setOnPaint(PaintCallback cb) { onPaint_ = std::move(cb); }

    void layout() override;
    void paint(PaintContext& ctx) override;

private:
    Color bgColor_ = Theme::instance().panelColor;
    PaintCallback onPaint_;
};

// ═════════════════════════════════════════════════════════════════════════════
//  ImageView — draws a texture with offset and rotation (pure display)
// ═════════════════════════════════════════════════════════════════════════════

class ImageView : public Widget
{
public:
    ImageView() = default;

    void setTexture(Texture* tex);
    Texture* texture() const { return texture_; }

    void setOffset(float ox, float oy) { offsetX_ = ox; offsetY_ = oy; }
    float offsetX() const { return offsetX_; }
    float offsetY() const { return offsetY_; }

    void setAngle(float deg) { angle_ = deg; }
    float angle() const { return angle_; }

    void paint(PaintContext& ctx) override;

private:
    Texture* texture_ = nullptr;
    float offsetX_ = 0, offsetY_ = 0;
    float angle_ = 0;
};

// ═════════════════════════════════════════════════════════════════════════════
//  GridLayout — uniform grid with N columns, rows auto-calculated
//    Children flow left-to-right, top-to-bottom. Each child = 1 cell.
//    Cell sizes are equal: cellW = (width - padding - gaps) / cols
//    Children can use setStretch() for row-span (like BoxLayout vertical).
// ═════════════════════════════════════════════════════════════════════════════
