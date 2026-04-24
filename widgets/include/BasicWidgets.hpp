#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include "Font.hpp"
#include <string>
#include <functional>

struct Texture;
class RadioButton;

// ═════════════════════════════════════════════════════════════════════════════

class Label : public Widget
{
public:
    explicit Label(const std::string& text = "");

    void setText(const std::string& t);
    const std::string& text() const { return text_; }

    void setColor(const Color& c);
    void setAlign(TextAlign a);
    TextAlign align() const { return align_; }

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    std::string text_;
    Color color_ = Theme::instance().textColor;
    TextAlign align_ = TextAlign::LEFT;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Spacer — invisible widget that takes up space in layouts
//    Fixed size: Spacer(20)        → 20px gap
//    Stretch:    Spacer() + setStretch(1) → fills remaining space
// ═════════════════════════════════════════════════════════════════════════════

class Spacer : public Widget
{
public:
    explicit Spacer(float size = 0.0f);

    Vec2f sizeHint() const override;
    void paint(PaintContext&) override {}  // invisible

private:
    float size_;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Line — visual separator (horizontal or vertical, adapts to parent layout)
// ═════════════════════════════════════════════════════════════════════════════

class Line : public Widget
{
public:
    explicit Line(float thickness = 1.0f);

    void setThickness(float t) { thickness_ = t; markDirty(); }
    float thickness() const    { return thickness_; }

    void setColor(const Color& c) { color_ = c; markDirty(); }
    const Color& color() const    { return color_; }

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    float thickness_;
    Color color_ = Theme::instance().borderColor;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Button
// ═════════════════════════════════════════════════════════════════════════════

class Button : public Widget
{
public:
    explicit Button(const std::string& text = "");

    void setText(const std::string& t);
    const std::string& text() const { return text_; }

    void setAlign(TextAlign a);
    TextAlign align() const { return align_; }

    // Custom background color (overrides theme). Set alpha=0 to use theme default.
    void setBgColor(const Color& c) { bgColor_ = c; }
    const Color& bgColor() const { return bgColor_; }

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    std::string text_;
    TextAlign align_ = TextAlign::CENTER;
    Color bgColor_ = Color(0, 0, 0, 0);  // transparent = use theme
};

// ═════════════════════════════════════════════════════════════════════════════
//  ImageButton — button that draws a textured region from any atlas/texture
//    auto* btn = parent->createChild<ImageButton>();
//    btn->setTexture(atlas->texture());
//    btn->setSrcRect({0, 0, 24, 24});   // region within the texture
//    btn->setTint(Color(255,255,255,255)); // optional tint
// ═════════════════════════════════════════════════════════════════════════════

class ImageButton : public Widget
{
public:
    ImageButton() = default;

    void setTexture(Texture* tex) { tex_ = tex; markDirty(); }
    Texture* texture() const { return tex_; }

    // Source rect within the texture (in pixels)
    void setSrcRect(const FloatRect& r) { srcRect_ = r; markDirty(); }
    const FloatRect& srcRect() const { return srcRect_; }

    // Icon padding inside the button (px from each edge)
    void setIconPadding(float p) { iconPad_ = p; markDirty(); }
    float iconPadding() const { return iconPad_; }

    // Tint color (multiplied with texture). White = no tint.
    void setTint(const Color& c) { tint_ = c; markDirty(); }
    const Color& tint() const { return tint_; }

    // Custom background color (overrides theme). alpha=0 → theme default.
    void setBgColor(const Color& c) { bgColor_ = c; }
    const Color& bgColor() const { return bgColor_; }

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    Texture*  tex_     = nullptr;
    FloatRect srcRect_ = {0, 0, 0, 0};
    float     iconPad_ = 3.0f;
    Color     tint_    = Color(255, 255, 255, 255);
    Color     bgColor_ = Color(0, 0, 0, 0);
};

// ═════════════════════════════════════════════════════════════════════════════
//  IconButton — button that draws a built-in icon using lines/shapes
//    Supports animated transitions (e.g. hamburger ↔ X)
//    auto* btn = parent->createChild<IconButton>(IconButton::Hamburger);
//    btn->setIcon(IconButton::Close);  // switch icon
//    btn->setAnimated(true);           // animate transitions
// ═════════════════════════════════════════════════════════════════════════════

class IconButton : public Widget
{
public:
    enum Icon {
        Hamburger,   // ≡  three horizontal lines
        Close,       // ×  X mark
        ArrowLeft,   // ←
        ArrowRight,  // →
        ArrowUp,     // ↑
        ArrowDown,   // ↓
        Plus,        // +
        Minus,       // −
        Check,       // ✓
        Chevron,     // >  (rotates with direction)
    };

    explicit IconButton(Icon icon = Hamburger);

    void setIcon(Icon icon);
    Icon icon() const { return icon_; }

    // When animated=true, transitions between icons with interpolation
    void setAnimated(bool a) { animated_ = a; }
    bool isAnimated() const  { return animated_; }

    // Icon color (defaults to theme text color)
    void setIconColor(const Color& c) { iconColor_ = c; markDirty(); }
    const Color& iconColor() const    { return iconColor_; }

    // Line thickness for icon drawing
    void setLineWidth(float w) { lineWidth_ = w; markDirty(); }
    float lineWidth() const    { return lineWidth_; }

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    Icon  icon_       = Hamburger;
    Icon  targetIcon_ = Hamburger;
    bool  animated_   = false;
    float animProgress_ = 1.0f;  // 1.0 = fully at target
    Color iconColor_  = Theme::instance().textColor;
    float lineWidth_  = 2.0f;

    void drawIcon(PaintContext& ctx, const Rect& abs, Icon icon, float t);
};

// ═════════════════════════════════════════════════════════════════════════════
//  Panel — container with background
// ═════════════════════════════════════════════════════════════════════════════

class Panel : public Widget
{
public:
    Panel() = default;

    void setBgColor(const Color& c);
    const Color& bgColor() const { return bgColor_; }

    void layout() override;
    void paint(PaintContext& ctx) override;

private:
    Color bgColor_ = Theme::instance().panelColor;
};

// ═════════════════════════════════════════════════════════════════════════════
//  CheckBox
// ═════════════════════════════════════════════════════════════════════════════

class CheckBox : public Widget
{
public:
    explicit CheckBox(const std::string& text = "");

    void setText(const std::string& t);
    const std::string& text() const { return text_; }
    bool isChecked() const { return checked_; }
    void setChecked(bool c);

    Signal<bool> toggled;

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    std::string text_;
    bool checked_ = false;
};

// ═════════════════════════════════════════════════════════════════════════════
//  RadioGroup — manages mutual exclusion for a set of RadioButtons
// ═════════════════════════════════════════════════════════════════════════════

class RadioButton; // forward

class RadioGroup
{
public:
    RadioGroup() = default;
    ~RadioGroup();

    void add(RadioButton* rb);
    void remove(RadioButton* rb);

    // Currently selected button (nullptr if none)
    RadioButton* selected() const { return selected_; }
    int selectedIndex() const;

    Signal<int> selectionChanged;  // emits index of newly selected

private:
    friend class RadioButton;
    std::vector<RadioButton*> buttons_;
    RadioButton* selected_ = nullptr;
    void select(RadioButton* rb);
};

// ═════════════════════════════════════════════════════════════════════════════
//  RadioButton — exclusive toggle within a RadioGroup
//    auto* group = new RadioGroup();
//    auto* rb1 = parent->createChild<RadioButton>("Option A", group);
//    auto* rb2 = parent->createChild<RadioButton>("Option B", group);
// ═════════════════════════════════════════════════════════════════════════════

class RadioButton : public Widget
{
public:
    explicit RadioButton(const std::string& text = "", RadioGroup* group = nullptr);
    ~RadioButton() override;

    void setText(const std::string& t) { text_ = t; markDirty(); }
    const std::string& text() const    { return text_; }

    bool isSelected() const { return selected_; }
    void setSelected(bool s);

    void setGroup(RadioGroup* g);
    RadioGroup* group() const { return group_; }

    Signal<bool> toggled;

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    friend class RadioGroup;
    std::string text_;
    bool selected_    = false;
    RadioGroup* group_ = nullptr;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Switch — on/off toggle (like iOS/Android toggle switch)
//    auto* sw = parent->createChild<Switch>("Dark Mode");
//    sw->toggled.connect([](bool on) { ... });
// ═════════════════════════════════════════════════════════════════════════════

class Switch : public Widget
{
public:
    explicit Switch(const std::string& text = "");

    void setText(const std::string& t) { text_ = t; markDirty(); }
    const std::string& text() const    { return text_; }

    bool isOn() const { return on_; }
    void setOn(bool v);

    // Track colors
    void setOnColor(const Color& c)  { onColor_ = c; markDirty(); }
    void setOffColor(const Color& c) { offColor_ = c; markDirty(); }

    Signal<bool> toggled;

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    std::string text_;
    bool on_ = false;
    Color onColor_  = Color(80, 160, 80, 255);
    Color offColor_ = Color(70, 70, 75, 255);
};

// ═════════════════════════════════════════════════════════════════════════════
//  Slider
// ═════════════════════════════════════════════════════════════════════════════

class Slider : public Widget
{
public:
    Slider(float minVal = 0.0f, float maxVal = 1.0f, float value = 0.5f);

    float value() const    { return value_; }
    float minValue() const  { return min_; }
    float maxValue() const  { return max_; }
    void setValue(float v);

    void setRange(float minVal, float maxVal);

    void setOrientation(LayoutDir dir) { orientation_ = dir; markDirty(); }
    LayoutDir orientation() const      { return orientation_; }

    Signal<float> onValueChanged;

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;

private:
    float min_, max_, value_;
    LayoutDir orientation_ = LayoutDir::Horizontal;
    bool dragging_ = false;

    void updateFromMouse(float localX, float localY);
};

// ═════════════════════════════════════════════════════════════════════════════
//  ProgressBar — non-interactive progress display with optional text
// ═════════════════════════════════════════════════════════════════════════════

class ProgressBar : public Widget
{
public:
    ProgressBar(float minVal = 0.0f, float maxVal = 1.0f, float value = 0.0f);

    float value() const     { return value_; }
    float minValue() const  { return min_; }
    float maxValue() const  { return max_; }
    void setValue(float v);
    void setRange(float minVal, float maxVal);

    // Orientation
    void setOrientation(LayoutDir dir) { orientation_ = dir; markDirty(); }
    LayoutDir orientation() const      { return orientation_; }

    // Status text (shown on top of the bar)
    void setText(const std::string& t) { text_ = t; markDirty(); }
    const std::string& text() const    { return text_; }

    // Text alignment within the bar
    void setTextAlign(TextAlign a)  { textAlign_ = a; markDirty(); }
    TextAlign textAlign() const     { return textAlign_; }

    // Colors
    void setBarColor(const Color& c)  { barColor_ = c; markDirty(); }
    const Color& barColor() const     { return barColor_; }
    void setTextColor(const Color& c) { textColor_ = c; markDirty(); }
    const Color& textColor() const    { return textColor_; }

    Signal<float> onValueChanged;

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;

private:
    float min_, max_, value_;
    LayoutDir orientation_ = LayoutDir::Horizontal;
    std::string text_;
    TextAlign textAlign_ = TextAlign::CENTER;
    Color barColor_  = Color(140, 140, 145, 255);
    Color textColor_ = Color(220, 220, 220, 255);
};



