#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include <functional>

class ColorPickerPopup_;
class PropComboPopup_;

public:
    TreeView();
    ~TreeView() override;

    // Add a root-level node. Returns it.
    TreeNode* addRoot(const std::string& text);

    // Remove all nodes
    void clear();

    // Roots
    const std::vector<TreeNode*>& roots() const { return roots_; }

    // Selection
    TreeNode* selectedNode() const { return selected_; }
    void setSelectedNode(TreeNode* n);

    // Row height
    void setRowHeight(float h) { rowHeight_ = h; markDirty(); }
    float rowHeight() const { return rowHeight_; }

    // Indent per level
    void setIndent(float px) { indent_ = px; markDirty(); }
    float indent() const { return indent_; }

    Signal<TreeNode*> selectionChanged;
    Signal<TreeNode*> nodeExpanded;
    Signal<TreeNode*> nodeCollapsed;
    Signal<TreeNode*> nodeDoubleClicked;

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;

private:
    std::vector<TreeNode*> roots_;
    TreeNode* selected_ = nullptr;
    float rowHeight_ = 22.0f;
    float indent_ = 18.0f;
    float scrollOffset_ = 0.0f;

    // Flatten visible tree into rows for painting/hit-test
    struct FlatRow { TreeNode* node; int depth; };
    std::vector<FlatRow> flatRows_;
    void rebuildFlat();
    void flattenNode(TreeNode* n, int depth);
    int totalVisibleRows() const { return static_cast<int>(flatRows_.size()); }
    float totalHeight() const { return totalVisibleRows() * rowHeight_; }
    float maxScroll() const;
};

// ═════════════════════════════════════════════════════════════════════════════
//  PropertyGrid — two-column name/value editor (like Qt Property Browser)
//    auto* grid = parent->createChild<PropertyGrid>();
//    grid->addString("Name", "Player1", [](const std::string& v){ ... });
//    grid->addFloat("Speed", 5.0f, 0, 100, [](float v){ ... });
//    grid->addBool("Visible", true, [](bool v){ ... });
//    grid->addColor("Tint", Color(255,0,0,255), [](const Color& c){ ... });
//    grid->addCombo("Mode", {"Walk","Run","Fly"}, 0, [](int i){ ... });
//    grid->addSection("Transform");  // visual separator/group header
// ═════════════════════════════════════════════════════════════════════════════

class PropertyGrid : public Widget
{
public:
    PropertyGrid();

    // Property types
    enum class PropType { Section, String, Float, Int, Bool, Color, Combo,
                          Vec2, Vec3, Vec4, Button, Separator, Range };

    // Add properties — returns row index
    int addSection(const std::string& title);
    int addString(const std::string& name, const std::string& value,
                  std::function<void(const std::string&)> onChange = nullptr,
                  const std::string& desc = "");
    int addFloat(const std::string& name, float value, float minVal, float maxVal,
                 std::function<void(float)> onChange = nullptr,
                 const std::string& desc = "");
    int addInt(const std::string& name, int value, int minVal, int maxVal,
               std::function<void(int)> onChange = nullptr,
               const std::string& desc = "");
    int addBool(const std::string& name, bool value,
                std::function<void(bool)> onChange = nullptr,
                const std::string& desc = "");
    int addColor(const std::string& name, const Color& value,
                 std::function<void(const Color&)> onChange = nullptr,
                 const std::string& desc = "");
    int addCombo(const std::string& name, const std::vector<std::string>& options,
                 int selected, std::function<void(int)> onChange = nullptr,
                 const std::string& desc = "");
    int addVec2(const std::string& name, float x, float y,
                float minVal, float maxVal,
                std::function<void(float,float)> onChange = nullptr,
                const std::string& desc = "");
    int addVec3(const std::string& name, float x, float y, float z,
                float minVal, float maxVal,
                std::function<void(float,float,float)> onChange = nullptr,
                const std::string& desc = "");
    int addVec4(const std::string& name, float x, float y, float z, float w,
                float minVal, float maxVal,
                std::function<void(float,float,float,float)> onChange = nullptr,
                const std::string& desc = "");
    int addButton(const std::string& name, std::function<void()> onClick = nullptr,
                  const std::string& desc = "");
    int addRange(const std::string& name, float lo, float hi,
                 float minVal, float maxVal,
                 std::function<void(float,float)> onChange = nullptr,
                 const std::string& desc = "");
    int addSeparator();

    // Access/modify values
    void setString(int row, const std::string& v);
    void setFloat(int row, float v);
    void setInt(int row, int v);
    void setBool(int row, bool v);
    void setColor(int row, const Color& c);
    void setCombo(int row, int idx);
    void setVec2(int row, float x, float y);
    void setVec3(int row, float x, float y, float z);
    void setVec4(int row, float x, float y, float z, float w);
    void setRange(int row, float lo, float hi);

    // Row height
    void setRowHeight(float h) { rowHeight_ = h; markDirty(); }
    float rowHeight() const { return rowHeight_; }

    // Label column width ratio (0..1)
    void setLabelRatio(float r) { labelRatio_ = Clamp(r, 0.15f, 0.85f); markDirty(); }
    float labelRatio() const { return labelRatio_; }

    // Description panel height (0 = hidden)
    void setDescHeight(float h) { descHeight_ = h; markDirty(); }
    float descHeight() const { return descHeight_; }

    Signal<int> propertyChanged;  // emits row index

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;
    void onKeyPress(KeyEvent& e) override;
    void onTextInput(KeyEvent& e) override;

    friend class ColorPickerPopup_;
    friend class PropComboPopup_;

private:
    struct PropRow {
        PropType type;
        std::string name;
        std::string description;  // shown in desc panel when selected
        // Values (union-like)
        std::string strVal;
        float floatVal = 0; float floatMin = 0; float floatMax = 1;
        int intVal = 0; int intMin = 0; int intMax = 100;
        bool boolVal = false;
        Color colorVal;
        std::vector<std::string> comboOptions;
        int comboIdx = 0;
        // Vec2/3/4 (reuses floatMin/floatMax for range)
        float vecVal[4] = {0, 0, 0, 0};
        int vecComponents = 0;  // 2, 3, or 4
        // Callbacks
        std::function<void(const std::string&)> onChangeStr;
        std::function<void(float)> onChangeFloat;
        std::function<void(int)> onChangeInt;
        std::function<void(bool)> onChangeBool;
        std::function<void(const Color&)> onChangeColor;
        std::function<void(int)> onChangeCombo;
        std::function<void(float,float)> onChangeVec2;
        std::function<void(float,float,float)> onChangeVec3;
        std::function<void(float,float,float,float)> onChangeVec4;
        std::function<void()> onClick;  // for Button type
        std::function<void(float,float)> onChangeRange;
        float rangeLo = 0, rangeHi = 1;  // for Range type
        bool expanded = true;  // for sections
    };

    std::vector<PropRow> rows_;
    float rowHeight_ = 24.0f;
    float labelRatio_ = 0.4f;
    float scrollOffset_ = 0.0f;
    float descHeight_ = 52.0f;   // description panel at bottom
    int hoveredRow_ = -1;
    int selectedRow_ = -1;       // selected row (shows desc, keyboard nav)
    int activeRow_ = -1;
    bool draggingSlider_ = false;
    float dragStartX_ = 0;
    float dragStartVal_ = 0;
    int dragComponent_ = 0;  // which vec component (0..3) is being dragged

    // Inline text editing
    bool editing_ = false;
    std::string editBuf_;
    int editCursor_ = 0;

    // Spinbox button hovered: 0=none, 1=up, 2=down
    int spinHover_ = 0;

    float totalHeight() const;
    float maxScroll() const;
    float gridHeight() const;   // area for rows (excluding desc panel)
    int hitRow(float localY) const;
    void paintRow(PaintContext& ctx, const Rect& abs, int idx, float y);
    void paintDescPanel(PaintContext& ctx, const Rect& abs);
    void commitEdit();
    void cancelEdit();
    void startEdit(int row);
    void spinStep(int row, int dir);  // dir = +1 or -1

    // Returns visible row indices (skipping hidden children of collapsed sections)
    std::vector<int> visibleRows() const;
    float visibleTotalHeight() const;
};

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

// ═════════════════════════════════════════════════════════════════════════════
//  DataGrid — spreadsheet/database-style table widget
//    auto* grid = parent->createChild<DataGrid>();
//    grid->addColumn("Name",  200);
//    grid->addColumn("Age",    60);
//    grid->addColumn("Email", 250);
//    grid->addRow({"Alice", "30", "alice@example.com"});
//    grid->addRow({"Bob",   "25", "bob@example.com"});
//
//  Features:
//    - Clickable headers with sort (asc/desc/none)
//    - Drag-resizable columns
//    - Single or multi-select rows (Ctrl+click, Shift+click)
//    - Inline cell editing (double-click)
//    - Optional checkbox column
//    - Alternating row colors (zebra stripes)
//    - Vertical + horizontal scroll
// ═════════════════════════════════════════════════════════════════════════════

class DataGrid : public Widget
