#pragma once

#include "Widget.hpp"
#include "Signal.hpp"
#include <cmath>
#include <string>

// ═════════════════════════════════════════════════════════════════════════════
//  Knob - rotary knob control (circular arc style)
//
//  Usage:
//    auto* k = parent->createChild<Knob>();
//    k->setRange(0.0f, 1.0f);
//    k->setValue(0.5f);
//    k->setLabel("Volume");
//    k->onChanged.connect([](float v){ ... });
//
//  Interaction:
//    - Drag up/down to change value
//    - Double-click to reset to default
//    - Scroll wheel to nudge
// ═════════════════════════════════════════════════════════════════════════════

class Knob : public Widget
{
public:
    Knob();

    // ── Value ─────────────────────────────────────────────────────────────
    void  setValue(float v);
    float value()        const { return value_; }
    float normalised()   const { return (max_ - min_ > 0) ? (value_ - min_) / (max_ - min_) : 0.f; }

    void  setRange(float mn, float mx)   { min_ = mn; max_ = mx; setValue(value_); }
    void  setDefault(float v)            { default_ = v; }
    float minimum() const { return min_; }
    float maximum() const { return max_; }

    // ── Appearance ────────────────────────────────────────────────────────
    void setLabel(const std::string& l) { label_ = l; markDirty(); }
    const std::string& label() const    { return label_; }

    void setArcColor(const Color& c)    { arcColor_   = c; markDirty(); }
    void setTrackColor(const Color& c)  { trackColor_ = c; markDirty(); }
    void setBgColor(const Color& c)     { bgColor_    = c; markDirty(); }
    void setShowValue(bool s)           { showValue_  = s; markDirty(); }

    // Drag sensitivity: pixels per full range
    void setSensitivity(float s)        { sensitivity_ = s; }

    // ── Signal ────────────────────────────────────────────────────────────
    Signal<float> onChanged;

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override { return {56, 68}; }
    void  paint(PaintContext& ctx) override;
    void  onMousePress(MouseEvent& e) override;
    void  onMouseMove(MouseEvent& e) override;
    void  onMouseRelease(MouseEvent& e) override;
    void  onMouseScroll(MouseEvent& e) override;

private:
    float value_       = 0.5f;
    float min_         = 0.0f;
    float max_         = 1.0f;
    float default_     = 0.5f;
    float sensitivity_ = 120.0f;  // pixels for full range

    std::string label_;
    bool  showValue_   = true;

    Color arcColor_    = Color(100, 180, 240, 255);
    Color trackColor_  = Color( 50,  55,  65, 255);
    Color bgColor_     = Color( 28,  30,  36, 255);

    bool  dragging_    = false;
    float dragStartY_  = 0.f;
    float dragStartV_  = 0.f;

    // Arc geometry: starts at 225° (bottom-left), sweeps 270° clockwise
    static constexpr float kStartAngle = 225.0f * (3.14159265f / 180.0f);
    static constexpr float kSweep      = 270.0f * (3.14159265f / 180.0f);
};

// ═════════════════════════════════════════════════════════════════════════════
//  LinearKnob - horizontal slider styled as a "linear pot"
//
//  Looks like a guitar volume slider - a narrow track with a
//  prominent thumb that shows the current position.
// ═════════════════════════════════════════════════════════════════════════════

class LinearKnob : public Widget
{
public:
    LinearKnob();

    void  setValue(float v);
    float value()      const { return value_; }
    float normalised() const { return (max_ - min_ > 0) ? (value_ - min_) / (max_ - min_) : 0.f; }

    void  setRange(float mn, float mx) { min_ = mn; max_ = mx; setValue(value_); }
    void  setDefault(float v)          { default_ = v; }

    void  setLabel(const std::string& l) { label_ = l; markDirty(); }
    void  setOrientation(bool vertical)  { vertical_ = vertical; markDirty(); }
    void  setFillColor(const Color& c)   { fillColor_  = c; markDirty(); }
    void  setTrackColor(const Color& c)  { trackColor_ = c; markDirty(); }
    void  setThumbColor(const Color& c)  { thumbColor_ = c; markDirty(); }
    void  setShowValue(bool s)           { showValue_  = s; markDirty(); }

    Signal<float> onChanged;

    Vec2f sizeHint() const override;
    void  paint(PaintContext& ctx) override;
    void  onMousePress(MouseEvent& e) override;
    void  onMouseMove(MouseEvent& e) override;
    void  onMouseRelease(MouseEvent& e) override;
    void  onMouseScroll(MouseEvent& e) override;

private:
    float value_    = 0.5f;
    float min_      = 0.0f;
    float max_      = 1.0f;
    float default_  = 0.5f;
    bool  vertical_ = false;
    std::string label_;
    bool  showValue_ = true;

    Color fillColor_  = Color(100, 180, 240, 255);
    Color trackColor_ = Color( 40,  44,  52, 255);
    Color thumbColor_ = Color(220, 222, 228, 255);

    bool  dragging_   = false;
    float dragStart_  = 0.f;
    float dragStartV_ = 0.f;

    Rect trackRect() const;
};
