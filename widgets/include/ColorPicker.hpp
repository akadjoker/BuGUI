#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>

// ═════════════════════════════════════════════════════════════════════════════
//  ColorPicker — HSV color wheel + brightness slider + hex input
//    auto* picker = parent->createChild<ColorPicker>();
//    picker->setColor(Color(255, 128, 0, 255));
//    picker->colorChanged.connect([](const Color& c){ ... });
// ═════════════════════════════════════════════════════════════════════════════

class ColorPicker : public Widget
{
public:
    ColorPicker();

    void setColor(const Color& c);
    Color color() const;

    // Show alpha slider
    void setShowAlpha(bool s) { showAlpha_ = s; markDirty(); }
    bool showAlpha() const { return showAlpha_; }

    Signal<const Color&> colorChanged;

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;

private:
    // HSV representation (0..1 each)
    float hue_ = 0, sat_ = 0, val_ = 1;
    float alpha_ = 1.0f;
    bool showAlpha_ = true;

    enum DragTarget { None, Wheel, ValueBar, AlphaBar };
    DragTarget dragTarget_ = None;

    // Geometry helpers
    float wheelRadius(const Rect& abs) const;
    float wheelCenterX(const Rect& abs) const;
    float wheelCenterY(const Rect& abs) const;
    Rect valueBarRect(const Rect& abs) const;
    Rect alphaBarRect(const Rect& abs) const;
    Rect previewRect(const Rect& abs) const;
    Rect hexRect(const Rect& abs) const;

    void updateFromWheel(float mx, float my, const Rect& abs);
    void updateFromValueBar(float my, const Rect& abs);
    void updateFromAlphaBar(float my, const Rect& abs);

    // HSV ↔ RGB conversion
    static Color hsvToRgb(float h, float s, float v, float a);
    static void rgbToHsv(const Color& c, float& h, float& s, float& v);
};

