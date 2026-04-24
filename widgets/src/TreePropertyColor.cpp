#include "Widgets.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
    inline Font::ClipRect toFontClipTPC(const Rect& r)
    { return {r.x, r.y, r.w, r.h}; }
}

// ─────────────────────────────────────────────────────────────────────────────
//  ColorPickerPopup_ - popup wrapper for PropertyGrid color editing
// ─────────────────────────────────────────────────────────────────────────────
class ColorPickerPopup_ : public Widget
{
public:
    ColorPickerPopup_(PropertyGrid* grid, int row)
        : grid_(grid), row_(row)
    {
        acceptsFocus_ = true;
        picker_ = new ColorPicker();
        picker_->setShowAlpha(true);
        picker_->setColor(grid->rows_[row].colorVal);

        // When user changes color in the wheel, update the property grid row live
        picker_->colorChanged.connect([this](const Color& c) {
            if (row_ >= 0 && row_ < static_cast<int>(grid_->rows_.size())) {
                grid_->rows_[row_].colorVal = c;
                if (grid_->rows_[row_].onChangeColor)
                    grid_->rows_[row_].onChangeColor(c);
                grid_->propertyChanged.emit(row_);
                grid_->markDirty();
            }
        });
    }

    ~ColorPickerPopup_() override { delete picker_; }

    void syncPickerRect()
    {
        Rect r = rect_;  // popup has no parent, rect_ == absolute
        float pad = 6.0f;
        Rect pr{r.x + pad, r.y + pad, r.w - pad * 2, r.h - pad * 2};
        if (pr.x != picker_->rect().x || pr.y != picker_->rect().y ||
            pr.w != picker_->rect().w || pr.h != picker_->rect().h)
            picker_->setRect(pr);
    }

    void paint(PaintContext& ctx) override
    {
        Rect abs = absoluteRect();
        const auto& t = Theme::instance();

        // Shadow
        ctx.fill.SetColor(0, 0, 0, 60);
        ctx.fillRect(abs.x + 3, abs.y + 3, abs.w, abs.h);

        // Background panel
        ctx.fill.SetColor(t.panelColor.r, t.panelColor.g, t.panelColor.b, 250);
        ctx.fill.RoundedRectangle(static_cast<int>(abs.x), static_cast<int>(abs.y),
                                   static_cast<int>(abs.w), static_cast<int>(abs.h),
                                   t.borderRadius, 6, true);
        ctx.line.SetColor(t.inputBorderHover.r, t.inputBorderHover.g, t.inputBorderHover.b, 200);
        ctx.line.Rectangle(static_cast<int>(abs.x), static_cast<int>(abs.y),
                           static_cast<int>(abs.w), static_cast<int>(abs.h), false);

        // Paint the color picker inside
        syncPickerRect();
        picker_->paint(ctx);
    }

    void onMousePress(MouseEvent& e) override
    {
        syncPickerRect();
        Rect abs = absoluteRect();
        if (abs.contains(e.x, e.y)) {
            picker_->onMousePress(e);
            e.consumed = true;
        }
    }

    void onMouseRelease(MouseEvent& e) override
    {
        syncPickerRect();
        picker_->onMouseRelease(e);
    }

    void onMouseMove(MouseEvent& e) override
    {
        syncPickerRect();
        picker_->onMouseMove(e);
    }

    bool popupContains(float x, float y) const override
    {
        return absoluteRect().contains(x, y);
    }

private:
    PropertyGrid* grid_;
    int row_;
    ColorPicker* picker_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  PropComboPopup_ - dropdown list for PropertyGrid combo properties
// ─────────────────────────────────────────────────────────────────────────────
class PropComboPopup_ : public Widget
{
public:
    PropComboPopup_(PropertyGrid* grid, int row)
        : grid_(grid), row_(row)
    {
        acceptsFocus_ = true;
        auto& r = grid->rows_[row];
        items_ = r.comboOptions;
        selected_ = r.comboIdx;
    }

    void paint(PaintContext& ctx) override
    {
        Rect abs = absoluteRect();
        const auto& t = Theme::instance();

        // Background
        ctx.fill.SetColor(t.panelColor.r, t.panelColor.g, t.panelColor.b, 250);
        ctx.fill.RoundedRectangle(static_cast<int>(abs.x), static_cast<int>(abs.y),
                                   static_cast<int>(abs.w), static_cast<int>(abs.h),
                                   t.borderRadius, 6, true);
        ctx.line.SetColor(t.inputBorderHover.r, t.inputBorderHover.g, t.inputBorderHover.b, 200);
        ctx.line.Rectangle(static_cast<int>(abs.x), static_cast<int>(abs.y),
                           static_cast<int>(abs.w), static_cast<int>(abs.h), false);

        ctx.pushClip(abs);
        float rowH = 22.0f;
        int n = static_cast<int>(items_.size());
        ctx.font.SetFontSize(t.fontSize);
        ctx.font.SetBatch(&ctx.text);
        auto fc = toFontClipTPC(ctx.clipRect());

        for (int i = 0; i < n; ++i) {
            float ry = abs.y + i * rowH;
            bool isHov = (i == hovered_);
            bool isSel = (i == selected_);

            if (isHov || isSel) {
                Color c = isSel ? t.selectionColor : Color(t.buttonHover.r, t.buttonHover.g, t.buttonHover.b, 120);
                ctx.fill.SetColor(c.r, c.g, c.b, c.a);
                ctx.fillRect(abs.x + 1, ry, abs.w - 2, rowH);
            }

            ctx.font.SetColor(isHov ? Color(255, 255, 255, 255) : t.textColor);
            ctx.font.Print(items_[i].c_str(), abs.x + 6, ry + (rowH - t.fontSize) * 0.5f, &fc);
        }
        ctx.popClip();
    }

    void onMousePress(MouseEvent& e) override
    {
        Rect abs = absoluteRect();
        if (!abs.contains(e.x, e.y)) return;
        e.consumed = true;

        float rowH = 22.0f;
        int idx = static_cast<int>((e.y - abs.y) / rowH);
        if (idx >= 0 && idx < static_cast<int>(items_.size())) {
            if (row_ >= 0 && row_ < static_cast<int>(grid_->rows_.size())) {
                auto& row = grid_->rows_[row_];
                row.comboIdx = idx;
                if (row.onChangeCombo) row.onChangeCombo(idx);
                grid_->propertyChanged.emit(row_);
                grid_->markDirty();
            }
        }
        WidgetApp::instance().closePopup();
    }

    void onMouseMove(MouseEvent& e) override
    {
        Rect abs = absoluteRect();
        float rowH = 22.0f;
        int idx = static_cast<int>((e.y - abs.y) / rowH);
        if (idx < 0 || idx >= static_cast<int>(items_.size())) idx = -1;
        if (idx != hovered_) {
            hovered_ = idx;
            markDirty();
        }
    }

    void onMouseLeave() override
    {
        if (hovered_ != -1) { hovered_ = -1; markDirty(); }
    }

    bool popupContains(float x, float y) const override
    {
        return absoluteRect().contains(x, y);
    }

private:
    PropertyGrid* grid_;
    int row_;
    std::vector<std::string> items_;
    int selected_ = -1;
    int hovered_ = -1;
};
// ═════════════════════════════════════════════════════════════════════════════

TreeNode::TreeNode(const std::string& text) : text_(text) {}

TreeNode::~TreeNode()
{
    for (auto* c : children_)
        delete c;
}

TreeNode* TreeNode::addChild(const std::string& text)
{
    auto* n = new TreeNode(text);
    n->parent_ = this;
    children_.push_back(n);
    return n;
}

void TreeNode::removeChild(int index)
{
    if (index < 0 || index >= static_cast<int>(children_.size())) return;
    delete children_[index];
    children_.erase(children_.begin() + index);
}

void TreeNode::clear()
{
    for (auto* c : children_)
        delete c;
    children_.clear();
}

void TreeNode::setIcon(const std::string& name)
{
    iconStr_ = name;
    // Map well-known names to IconId
    if      (name == "folder")      iconId_ = IconId::Folder;
    else if (name == "folder_open") iconId_ = IconId::FolderOpen;
    else if (name == "file")        iconId_ = IconId::File;
    else if (name == "file_code")   iconId_ = IconId::FileCode;
    else if (name == "book")        iconId_ = IconId::Book;
    else if (name == "gear")        iconId_ = IconId::Gear;
    else if (name == "star")        iconId_ = IconId::Star;
    else if (name == "heart")       iconId_ = IconId::Heart;
    else if (name == "search")      iconId_ = IconId::Search;
    else if (name == "plus")        iconId_ = IconId::Plus;
    else if (name == "minus")       iconId_ = IconId::Minus;
    else if (name == "check")       iconId_ = IconId::Check;
    else if (name == "cross")       iconId_ = IconId::Cross;
    else if (name == "arrow_right") iconId_ = IconId::ArrowRight;
    else if (name == "arrow_down")  iconId_ = IconId::ArrowDown;
    else if (name == "arrow_up")    iconId_ = IconId::ArrowUp;
    else if (name == "arrow_left")  iconId_ = IconId::ArrowLeft;
    else if (name == "eye")         iconId_ = IconId::Eye;
    else if (name == "eye_off")     iconId_ = IconId::EyeOff;
    else if (name == "lock")        iconId_ = IconId::Lock;
    else if (name == "unlock")      iconId_ = IconId::Unlock;
    else if (name == "refresh")     iconId_ = IconId::Refresh;
    else if (name == "trash")       iconId_ = IconId::Trash;
    else if (name == "edit")        iconId_ = IconId::Edit;
    else if (name == "home")        iconId_ = IconId::Home;
    else if (name == "user")        iconId_ = IconId::User;
    else if (name == "warning")     iconId_ = IconId::Warning;
    else if (name == "info")        iconId_ = IconId::Info;
    else if (name == "error")       iconId_ = IconId::Error;
    else                            iconId_ = IconId::None;
}

// ═════════════════════════════════════════════════════════════════════════════
//  TreeView
// ═════════════════════════════════════════════════════════════════════════════

TreeView::TreeView()
{
    acceptsFocus_ = true;
}

TreeView::~TreeView()
{
    clear();
}

TreeNode* TreeView::addRoot(const std::string& text)
{
    auto* n = new TreeNode(text);
    roots_.push_back(n);
    markDirty();
    return n;
}

void TreeView::clear()
{
    for (auto* r : roots_)
        delete r;
    roots_.clear();
    selected_ = nullptr;
    scrollOffset_ = 0;
    markDirty();
}

void TreeView::setSelectedNode(TreeNode* n)
{
    if (selected_ != n) {
        selected_ = n;
        selectionChanged.emit(n);
        markDirty();
    }
}

Widget::Vec2f TreeView::sizeHint() const
{
    return {200.0f, 200.0f};
}

void TreeView::rebuildFlat()
{
    flatRows_.clear();
    for (auto* r : roots_)
        flattenNode(r, 0);
}

void TreeView::flattenNode(TreeNode* n, int depth)
{
    flatRows_.push_back({n, depth});
    if (n->isExpanded()) {
        for (auto* c : n->children())
            flattenNode(c, depth + 1);
    }
}

float TreeView::maxScroll() const
{
    float total = totalHeight();
    float visible = rect_.h;
    return (total > visible) ? total - visible : 0.0f;
}

void TreeView::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    // Background
    ctx.fill.SetColor(t.panelColor.r, t.panelColor.g, t.panelColor.b, t.panelColor.a);
    ctx.fillRect(abs.x, abs.y, abs.w, abs.h);

    ctx.pushClip(abs);

    rebuildFlat();

    int firstVisible = static_cast<int>(scrollOffset_ / rowHeight_);
    int lastVisible = firstVisible + static_cast<int>(abs.h / rowHeight_) + 2;
    lastVisible = std::min(lastVisible, static_cast<int>(flatRows_.size()));

    for (int i = firstVisible; i < lastVisible; ++i) {
        const auto& row = flatRows_[i];
        float rowY = abs.y + i * rowHeight_ - scrollOffset_;
        float indentX = abs.x + 4.0f + row.depth * indent_;

        // Selection highlight
        if (row.node == selected_) {
            ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g,
                              t.selectionColor.b, t.selectionColor.a);
            ctx.fillRect(abs.x, rowY, abs.w, rowHeight_);
        }

        // Expand/collapse triangle
        if (row.node->hasChildren()) {
            float triSize = 7.0f;
            float triX = indentX;
            float triCY = rowY + rowHeight_ * 0.5f;

            ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, t.textColor.a);

            if (row.node->isExpanded()) {
                // ▼
                ctx.fillTriangle(
                    triX, triCY - triSize * 0.35f,
                    triX + triSize, triCY - triSize * 0.35f,
                    triX + triSize * 0.5f, triCY + triSize * 0.5f);
            } else {
                // ▶
                ctx.fillTriangle(
                    triX + 1, triCY - triSize * 0.5f,
                    triX + 1, triCY + triSize * 0.5f,
                    triX + triSize, triCY);
            }
            indentX += triSize + 4.0f;
        } else {
            indentX += 11.0f; // align with nodes that have triangles
        }

        // Icon (if set) - draw from atlas texture
        if (row.node->iconId() != IconId::None) {
            float iconSz = t.fontSize;
            float iconX = indentX;
            float iconY = rowY + (rowHeight_ - iconSz) * 0.5f;
            ctx.drawIcon(row.node->iconId(), iconX, iconY, iconSz);
            indentX += iconSz + 3.0f;
        }

        // Label
        ctx.font.SetFontSize(t.fontSize);
        ctx.font.SetColor(row.node == selected_ ? Color(255, 255, 255, 255) : t.textColor);
        ctx.font.SetBatch(&ctx.text);
        float textY = rowY + (rowHeight_ - t.fontSize) * 0.5f;
        auto fc = toFontClipTPC(ctx.clipRect());
        ctx.font.Print(row.node->text().c_str(), indentX, textY, &fc);
    }

    // Border
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
    ctx.lineRect(abs.x, abs.y, abs.w, abs.h);

    ctx.popClip();
}

void TreeView::onMousePress(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y)) return;

    e.consumed = true;

    rebuildFlat();
    float localY = e.y - abs.y + scrollOffset_;
    int idx = static_cast<int>(localY / rowHeight_);

    if (idx < 0 || idx >= static_cast<int>(flatRows_.size())) return;

    auto& row = flatRows_[idx];
    float indentX = 4.0f + row.depth * indent_;

    // Click on triangle area → toggle expand
    if (row.node->hasChildren()) {
        float triEnd = indentX + 11.0f;
        float clickX = e.x - abs.x;
        if (clickX < triEnd) {
            row.node->setExpanded(!row.node->isExpanded());
            if (row.node->isExpanded())
                nodeExpanded.emit(row.node);
            else
                nodeCollapsed.emit(row.node);
            markDirty();
            return;
        }
    }

    // Click on label → select
    setSelectedNode(row.node);
}

void TreeView::onMouseScroll(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y)) return;

    e.consumed = true;
    scrollOffset_ -= e.scrollY * rowHeight_ * 2.0f;
    scrollOffset_ = Clamp(scrollOffset_, 0.0f, maxScroll());
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  PropertyGrid
// ═════════════════════════════════════════════════════════════════════════════

PropertyGrid::PropertyGrid()
{
    acceptsFocus_ = true;
}

int PropertyGrid::addSection(const std::string& title)
{
    PropRow r;
    r.type = PropType::Section;
    r.name = title;
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addString(const std::string& name, const std::string& value,
                             std::function<void(const std::string&)> onChange,
                             const std::string& desc)
{
    PropRow r;
    r.type = PropType::String;
    r.name = name;
    r.strVal = value;
    r.description = desc;
    r.onChangeStr = std::move(onChange);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addFloat(const std::string& name, float value, float minVal, float maxVal,
                            std::function<void(float)> onChange,
                            const std::string& desc)
{
    PropRow r;
    r.type = PropType::Float;
    r.name = name;
    r.floatVal = value;
    r.floatMin = minVal;
    r.floatMax = maxVal;
    r.description = desc;
    r.onChangeFloat = std::move(onChange);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addInt(const std::string& name, int value, int minVal, int maxVal,
                          std::function<void(int)> onChange,
                          const std::string& desc)
{
    PropRow r;
    r.type = PropType::Int;
    r.name = name;
    r.intVal = value;
    r.intMin = minVal;
    r.intMax = maxVal;
    r.description = desc;
    r.onChangeInt = std::move(onChange);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addBool(const std::string& name, bool value,
                           std::function<void(bool)> onChange,
                           const std::string& desc)
{
    PropRow r;
    r.type = PropType::Bool;
    r.name = name;
    r.boolVal = value;
    r.description = desc;
    r.onChangeBool = std::move(onChange);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addColor(const std::string& name, const Color& value,
                            std::function<void(const Color&)> onChange,
                            const std::string& desc)
{
    PropRow r;
    r.type = PropType::Color;
    r.name = name;
    r.colorVal = value;
    r.description = desc;
    r.onChangeColor = std::move(onChange);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addCombo(const std::string& name, const std::vector<std::string>& options,
                            int selected, std::function<void(int)> onChange,
                            const std::string& desc)
{
    PropRow r;
    r.type = PropType::Combo;
    r.name = name;
    r.comboOptions = options;
    r.comboIdx = selected;
    r.description = desc;
    r.onChangeCombo = std::move(onChange);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

void PropertyGrid::setString(int row, const std::string& v)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) && rows_[row].type == PropType::String)
        rows_[row].strVal = v;
    markDirty();
}

void PropertyGrid::setFloat(int row, float v)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) && rows_[row].type == PropType::Float) {
        rows_[row].floatVal = Clamp(v, rows_[row].floatMin, rows_[row].floatMax);
        markDirty();
    }
}

void PropertyGrid::setInt(int row, int v)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) && rows_[row].type == PropType::Int) {
        rows_[row].intVal = Clamp(v, rows_[row].intMin, rows_[row].intMax);
        markDirty();
    }
}

void PropertyGrid::setBool(int row, bool v)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) && rows_[row].type == PropType::Bool)
        rows_[row].boolVal = v;
    markDirty();
}

void PropertyGrid::setColor(int row, const Color& c)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) && rows_[row].type == PropType::Color)
        rows_[row].colorVal = c;
    markDirty();
}

void PropertyGrid::setCombo(int row, int idx)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) && rows_[row].type == PropType::Combo) {
        if (idx >= 0 && idx < static_cast<int>(rows_[row].comboOptions.size()))
            rows_[row].comboIdx = idx;
        markDirty();
    }
}

void PropertyGrid::setVec2(int row, float x, float y)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) && rows_[row].type == PropType::Vec2) {
        rows_[row].vecVal[0] = Clamp(x, rows_[row].floatMin, rows_[row].floatMax);
        rows_[row].vecVal[1] = Clamp(y, rows_[row].floatMin, rows_[row].floatMax);
        markDirty();
    }
}

void PropertyGrid::setVec3(int row, float x, float y, float z)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) && rows_[row].type == PropType::Vec3) {
        rows_[row].vecVal[0] = Clamp(x, rows_[row].floatMin, rows_[row].floatMax);
        rows_[row].vecVal[1] = Clamp(y, rows_[row].floatMin, rows_[row].floatMax);
        rows_[row].vecVal[2] = Clamp(z, rows_[row].floatMin, rows_[row].floatMax);
        markDirty();
    }
}

void PropertyGrid::setVec4(int row, float x, float y, float z, float w)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) && rows_[row].type == PropType::Vec4) {
        rows_[row].vecVal[0] = Clamp(x, rows_[row].floatMin, rows_[row].floatMax);
        rows_[row].vecVal[1] = Clamp(y, rows_[row].floatMin, rows_[row].floatMax);
        rows_[row].vecVal[2] = Clamp(z, rows_[row].floatMin, rows_[row].floatMax);
        rows_[row].vecVal[3] = Clamp(w, rows_[row].floatMin, rows_[row].floatMax);
        markDirty();
    }
}

void PropertyGrid::setRange(int row, float lo, float hi)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) && rows_[row].type == PropType::Range) {
        rows_[row].rangeLo = Clamp(lo, rows_[row].floatMin, rows_[row].floatMax);
        rows_[row].rangeHi = Clamp(hi, rows_[row].floatMin, rows_[row].floatMax);
        if (rows_[row].rangeLo > rows_[row].rangeHi)
            std::swap(rows_[row].rangeLo, rows_[row].rangeHi);
        markDirty();
    }
}

int PropertyGrid::addVec2(const std::string& name, float x, float y,
                           float minVal, float maxVal,
                           std::function<void(float,float)> onChange,
                           const std::string& desc)
{
    PropRow r;
    r.type = PropType::Vec2;
    r.name = name;
    r.vecVal[0] = x; r.vecVal[1] = y;
    r.vecComponents = 2;
    r.floatMin = minVal; r.floatMax = maxVal;
    r.description = desc;
    r.onChangeVec2 = std::move(onChange);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addVec3(const std::string& name, float x, float y, float z,
                           float minVal, float maxVal,
                           std::function<void(float,float,float)> onChange,
                           const std::string& desc)
{
    PropRow r;
    r.type = PropType::Vec3;
    r.name = name;
    r.vecVal[0] = x; r.vecVal[1] = y; r.vecVal[2] = z;
    r.vecComponents = 3;
    r.floatMin = minVal; r.floatMax = maxVal;
    r.description = desc;
    r.onChangeVec3 = std::move(onChange);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addVec4(const std::string& name, float x, float y, float z, float w,
                           float minVal, float maxVal,
                           std::function<void(float,float,float,float)> onChange,
                           const std::string& desc)
{
    PropRow r;
    r.type = PropType::Vec4;
    r.name = name;
    r.vecVal[0] = x; r.vecVal[1] = y; r.vecVal[2] = z; r.vecVal[3] = w;
    r.vecComponents = 4;
    r.floatMin = minVal; r.floatMax = maxVal;
    r.description = desc;
    r.onChangeVec4 = std::move(onChange);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addButton(const std::string& name, std::function<void()> onClick,
                             const std::string& desc)
{
    PropRow r;
    r.type = PropType::Button;
    r.name = name;
    r.description = desc;
    r.onClick = std::move(onClick);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addSeparator()
{
    PropRow r;
    r.type = PropType::Separator;
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

int PropertyGrid::addRange(const std::string& name, float lo, float hi,
                            float minVal, float maxVal,
                            std::function<void(float,float)> onChange,
                            const std::string& desc)
{
    PropRow r;
    r.type = PropType::Range;
    r.name = name;
    r.description = desc;
    r.rangeLo = lo;
    r.rangeHi = hi;
    r.floatMin = minVal;
    r.floatMax = maxVal;
    r.onChangeRange = std::move(onChange);
    rows_.push_back(std::move(r));
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

std::vector<int> PropertyGrid::visibleRows() const
{
    std::vector<int> vis;
    bool sectionCollapsed = false;
    for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
        if (rows_[i].type == PropType::Section) {
            sectionCollapsed = !rows_[i].expanded;
            vis.push_back(i);
        } else if (!sectionCollapsed) {
            vis.push_back(i);
        }
    }
    return vis;
}

float PropertyGrid::visibleTotalHeight() const
{
    return static_cast<float>(visibleRows().size()) * rowHeight_;
}

float PropertyGrid::totalHeight() const
{
    return visibleTotalHeight();
}

float PropertyGrid::gridHeight() const
{
    return rect_.h - (descHeight_ > 0 ? descHeight_ : 0);
}

float PropertyGrid::maxScroll() const
{
    float total = totalHeight();
    float visible = gridHeight();
    return (total > visible) ? total - visible : 0.0f;
}

int PropertyGrid::hitRow(float localY) const
{
    auto vis = visibleRows();
    float adjustedY = localY + scrollOffset_;
    int visIdx = static_cast<int>(adjustedY / rowHeight_);
    if (visIdx < 0 || visIdx >= static_cast<int>(vis.size())) return -1;
    return vis[visIdx];
}

Widget::Vec2f PropertyGrid::sizeHint() const
{
    return {280.0f, std::max(100.0f, visibleTotalHeight() + descHeight_)};
}

// ── Spinbox step ─────────────────────────────────────────────────────────

void PropertyGrid::spinStep(int row, int dir)
{
    if (row < 0 || row >= static_cast<int>(rows_.size())) return;
    auto& r = rows_[row];
    if (r.type == PropType::Int) {
        int step = std::max(1, (r.intMax - r.intMin) / 100);
        r.intVal = Clamp(r.intVal + dir * step, r.intMin, r.intMax);
        if (r.onChangeInt) r.onChangeInt(r.intVal);
    } else if (r.type == PropType::Float) {
        float step = (r.floatMax - r.floatMin) / 100.0f;
        r.floatVal = Clamp(r.floatVal + dir * step, r.floatMin, r.floatMax);
        if (r.onChangeFloat) r.onChangeFloat(r.floatVal);
    }
    propertyChanged.emit(row);
    markDirty();
}

// ── Inline editing ───────────────────────────────────────────────────────

void PropertyGrid::startEdit(int row)
{
    if (row < 0 || row >= static_cast<int>(rows_.size())) return;
    auto& r = rows_[row];
    editing_ = true;
    activeRow_ = row;

    switch (r.type) {
    case PropType::String:
        editBuf_ = r.strVal;
        break;
    case PropType::Float: {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", r.floatVal);
        editBuf_ = buf;
        break;
    }
    case PropType::Int: {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", r.intVal);
        editBuf_ = buf;
        break;
    }
    default:
        editing_ = false;
        return;
    }
    editCursor_ = static_cast<int>(editBuf_.size());
    markDirty();
}

void PropertyGrid::commitEdit()
{
    if (!editing_ || activeRow_ < 0) return;
    auto& r = rows_[activeRow_];

    switch (r.type) {
    case PropType::String:
        r.strVal = editBuf_;
        if (r.onChangeStr) r.onChangeStr(r.strVal);
        break;
    case PropType::Float: {
        float v = 0;
        try { v = std::stof(editBuf_); } catch (...) {}
        r.floatVal = Clamp(v, r.floatMin, r.floatMax);
        if (r.onChangeFloat) r.onChangeFloat(r.floatVal);
        break;
    }
    case PropType::Int: {
        int v = 0;
        try { v = std::stoi(editBuf_); } catch (...) {}
        r.intVal = Clamp(v, r.intMin, r.intMax);
        if (r.onChangeInt) r.onChangeInt(r.intVal);
        break;
    }
    default: break;
    }
    propertyChanged.emit(activeRow_);
    editing_ = false;
    activeRow_ = -1;
    markDirty();
}

void PropertyGrid::cancelEdit()
{
    editing_ = false;
    activeRow_ = -1;
    markDirty();
}

// ── Paint ────────────────────────────────────────────────────────────────

void PropertyGrid::paintRow(PaintContext& ctx, const Rect& abs, int idx, float y)
{
    const auto& t = Theme::instance();
    const auto& row = rows_[idx];
    float labelW = abs.w * labelRatio_;
    float valueX = abs.x + labelW;
    float valueW = abs.w - labelW;

    if (row.type == PropType::Section) {
        // Section header - full width bg
        Color bg = t.collapsibleHeaderBg;
        if (hoveredRow_ == idx) {
            bg.r = std::min(255, bg.r + 12);
            bg.g = std::min(255, bg.g + 12);
            bg.b = std::min(255, bg.b + 12);
        }
        ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
        ctx.fillRect(abs.x, y, abs.w, rowHeight_);

        // Triangle indicator
        float triSize = 7.0f;
        float triX = abs.x + 6.0f;
        float triCY = y + rowHeight_ * 0.5f;
        ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, t.textColor.a);
        if (row.expanded) {
            ctx.fillTriangle(triX, triCY - triSize * 0.35f,
                             triX + triSize, triCY - triSize * 0.35f,
                             triX + triSize * 0.5f, triCY + triSize * 0.5f);
        } else {
            ctx.fillTriangle(triX + 1, triCY - triSize * 0.5f,
                             triX + 1, triCY + triSize * 0.5f,
                             triX + triSize, triCY);
        }

        ctx.font.SetFontSize(t.fontSize);
        ctx.font.SetColor(t.textColor);
        ctx.font.SetBatch(&ctx.text);
        auto fc = toFontClipTPC(ctx.clipRect());
        ctx.font.Print(row.name.c_str(), triX + triSize + 4.0f,
                       y + (rowHeight_ - t.fontSize) * 0.5f, &fc);
        return;
    }

    // Selected row highlight
    if (idx == selectedRow_) {
        ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g,
                          t.selectionColor.b, t.selectionColor.a);
        ctx.fillRect(abs.x, y, abs.w, rowHeight_);
    }
    // Hover highlight (if not selected)
    else if (idx == hoveredRow_) {
        ctx.fill.SetColor(t.buttonHover.r, t.buttonHover.g, t.buttonHover.b, 30);
        ctx.fillRect(abs.x, y, abs.w, rowHeight_);
    }

    // Alternating row bg
    if (idx % 2 == 0 && idx != selectedRow_) {
        ctx.fill.SetColor(0, 0, 0, 10);
        ctx.fillRect(abs.x, y, abs.w, rowHeight_);
    }

    // Label column
    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetColor(idx == selectedRow_ ? Color(255, 255, 255, 255) : t.textColor);
    ctx.font.SetBatch(&ctx.text);
    float textY = y + (rowHeight_ - t.fontSize) * 0.5f;
    auto fc = toFontClipTPC(ctx.clipRect());
    ctx.font.Print(row.name.c_str(), abs.x + 8.0f, textY, &fc);

    // Separator line between columns
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, 50);
    ctx.drawLine(valueX, y, valueX, y + rowHeight_);

    // Value column
    float pad = 4.0f;
    float valTextX = valueX + pad;
    float valW = valueW - pad * 2;
    float spinW = 16.0f;  // width of spinbox up/down button area

    // ── Active editing mode (text input) ─────────────────────────────────
    if (editing_ && activeRow_ == idx) {
        // Draw edit field bg
        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, t.inputBg.a);
        ctx.fillRect(valTextX, y + 1, valW, rowHeight_ - 2);
        ctx.line.SetColor(t.focusColor.r, t.focusColor.g, t.focusColor.b, t.focusColor.a);
        ctx.lineRect(valTextX, y + 1, valW, rowHeight_ - 2);

        // Edit text
        ctx.font.SetColor(Color(255, 255, 255, 255));
        ctx.font.Print(editBuf_.c_str(), valTextX + 3, textY, &fc);

        // Cursor
        float curX = valTextX + 3 + ctx.font.GetTextWidth(editBuf_.substr(0, editCursor_).c_str());
        ctx.line.SetColor(255, 255, 255, 255);
        ctx.drawLine(curX, y + 3, curX, y + rowHeight_ - 3);
        return;
    }

    // ── Normal display mode ──────────────────────────────────────────────

    // Helper lambda: draw a Blender-style slider bar for a float value
    auto drawSliderBar = [&](float sx, float sy, float sw, float sh,
                             float val, float mn, float mx, const char* label,
                             Color barCol) {
        // Background
        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, t.inputBg.a);
        ctx.fillRect(sx, sy, sw, sh);
        // Fill bar
        float range = mx - mn;
        float ratio = (range > 0.0001f) ? (val - mn) / range : 0.0f;
        ratio = Clamp(ratio, 0.0f, 1.0f);
        float fillW = sw * ratio;
        ctx.fill.SetColor(barCol.r, barCol.g, barCol.b, barCol.a);
        ctx.fillRect(sx, sy, fillW, sh);
        // Border
        ctx.line.SetColor(t.inputBorder.r, t.inputBorder.g, t.inputBorder.b, t.inputBorder.a);
        ctx.lineRect(sx, sy, sw, sh);
        // Text overlay (value + name)
        ctx.font.SetColor(Color(230, 230, 230, 255));
        float ty = sy + (sh - t.fontSize) * 0.5f;
        ctx.font.Print(label, sx + 3, ty, &fc);
    };

    switch (row.type) {
    case PropType::String: {
        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, t.inputBg.a);
        ctx.fillRect(valTextX, y + 2, valW, rowHeight_ - 4);
        ctx.line.SetColor(t.inputBorder.r, t.inputBorder.g, t.inputBorder.b, t.inputBorder.a);
        ctx.lineRect(valTextX, y + 2, valW, rowHeight_ - 4);
        ctx.font.SetColor(Color(200, 220, 255, 255));
        ctx.font.Print(row.strVal.c_str(), valTextX + 3, textY, &fc);
        break;
    }
    case PropType::Float: {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", row.floatVal);
        Color barCol(60, 100, 160, 200);
        if (draggingSlider_ && activeRow_ == idx) barCol = Color(80, 130, 200, 220);
        else if (hoveredRow_ == idx) barCol = Color(70, 110, 175, 210);
        drawSliderBar(valTextX, y + 2, valW, rowHeight_ - 4,
                      row.floatVal, row.floatMin, row.floatMax, buf, barCol);
        break;
    }
    case PropType::Int: {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", row.intVal);
        Color barCol(60, 140, 90, 200);
        if (draggingSlider_ && activeRow_ == idx) barCol = Color(80, 170, 110, 220);
        else if (hoveredRow_ == idx) barCol = Color(70, 155, 100, 210);
        drawSliderBar(valTextX, y + 2, valW, rowHeight_ - 4,
                      static_cast<float>(row.intVal),
                      static_cast<float>(row.intMin), static_cast<float>(row.intMax),
                      buf, barCol);
        break;
    }
    case PropType::Bool: {
        float boxS = 14.0f;
        float boxX = valTextX;
        float boxY = y + (rowHeight_ - boxS) * 0.5f;
        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, t.inputBg.a);
        ctx.fillRect(boxX, boxY, boxS, boxS);
        ctx.line.SetColor(t.inputBorder.r, t.inputBorder.g, t.inputBorder.b, t.inputBorder.a);
        ctx.lineRect(boxX, boxY, boxS, boxS);
        if (row.boolVal) {
            ctx.line.SetColor(t.checkMark.r, t.checkMark.g, t.checkMark.b, t.checkMark.a);
            ctx.drawLine(boxX + 3, boxY + boxS * 0.5f, boxX + boxS * 0.4f, boxY + boxS - 3);
            ctx.drawLine(boxX + boxS * 0.4f, boxY + boxS - 3, boxX + boxS - 3, boxY + 3);
        }
        break;
    }
    case PropType::Color: {
        float swatchH = rowHeight_ - 6.0f;
        float swatchW = std::min(valW, swatchH * 3.0f);
        float swatchY = y + 3.0f;
        ctx.fill.SetColor(row.colorVal.r, row.colorVal.g, row.colorVal.b, row.colorVal.a);
        ctx.fillRect(valTextX, swatchY, swatchW, swatchH);
        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
        ctx.lineRect(valTextX, swatchY, swatchW, swatchH);
        char buf[20];
        snprintf(buf, sizeof(buf), "#%02X%02X%02X", row.colorVal.r, row.colorVal.g, row.colorVal.b);
        ctx.font.SetColor(Color(180, 180, 180, 255));
        ctx.font.Print(buf, valTextX + swatchW + 4.0f, textY, &fc);
        break;
    }
    case PropType::Combo: {
        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, t.inputBg.a);
        ctx.fillRect(valTextX, y + 2, valW, rowHeight_ - 4);
        ctx.line.SetColor(t.inputBorder.r, t.inputBorder.g, t.inputBorder.b, t.inputBorder.a);
        ctx.lineRect(valTextX, y + 2, valW, rowHeight_ - 4);
        const std::string& txt = (row.comboIdx >= 0 &&
            row.comboIdx < static_cast<int>(row.comboOptions.size()))
            ? row.comboOptions[row.comboIdx] : "";
        ctx.font.SetColor(Color(180, 220, 180, 255));
        ctx.font.Print(txt.c_str(), valTextX + 3, textY, &fc);
        float arrowX = valueX + valueW - 14.0f;
        float arrowY = y + rowHeight_ * 0.5f;
        ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, t.textColor.a);
        ctx.fillTriangle(arrowX, arrowY - 3, arrowX + 8, arrowY - 3, arrowX + 4, arrowY + 3);
        break;
    }
    case PropType::Vec2:
    case PropType::Vec3:
    case PropType::Vec4: {
        // Multi-component slider: split value area into N equal bars
        int nc = row.vecComponents;
        static const Color compColors[] = {
            {180, 60, 60, 200},   // X = red
            {60, 140, 60, 200},   // Y = green
            {60, 80, 180, 200},   // Z = blue
            {140, 120, 60, 200},  // W = yellow
        };
        static const char* compLabels[] = {"X", "Y", "Z", "W"};
        float gap = 2.0f;
        float compW = (valW - gap * (nc - 1)) / nc;
        for (int c = 0; c < nc; ++c) {
            float cx = valTextX + c * (compW + gap);
            char buf[32];
            snprintf(buf, sizeof(buf), "%s %.1f", compLabels[c], row.vecVal[c]);
            Color barCol = compColors[c];
            if (draggingSlider_ && activeRow_ == idx && dragComponent_ == c)
                barCol = Color(std::min(255, barCol.r + 30),
                               std::min(255, barCol.g + 30),
                               std::min(255, barCol.b + 30), 220);
            drawSliderBar(cx, y + 2, compW, rowHeight_ - 4,
                          row.vecVal[c], row.floatMin, row.floatMax, buf, barCol);
        }
        break;
    }
    case PropType::Button: {
        // Draw as a clickable button spanning the value column
        Color btnCol = (hoveredRow_ == idx) ? t.buttonHover : t.buttonNormal;
        ctx.fill.SetColor(btnCol.r, btnCol.g, btnCol.b, btnCol.a);
        ctx.fillRoundedRect(valTextX, y + 2, valW, rowHeight_ - 4, 3.0f);
        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
        ctx.lineRoundedRect(valTextX, y + 2, valW, rowHeight_ - 4, 3.0f);
        // Center the label text
        float tw = ctx.font.GetTextWidth(row.name.c_str());
        ctx.font.SetColor(t.textColor);
        ctx.font.Print(row.name.c_str(), valTextX + (valW - tw) * 0.5f, textY, &fc);
        break;
    }
    case PropType::Separator: {
        // Thin horizontal line
        float lineY = y + rowHeight_ * 0.5f;
        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
        ctx.drawLine(abs.x + 4.0f, lineY, abs.x + abs.w - 4.0f, lineY);
        break;
    }
    case PropType::Range: {
        float range = row.floatMax - row.floatMin;
        float ratioLo = (range > 0.0001f) ? (row.rangeLo - row.floatMin) / range : 0.0f;
        float ratioHi = (range > 0.0001f) ? (row.rangeHi - row.floatMin) / range : 1.0f;
        ratioLo = Clamp(ratioLo, 0.0f, 1.0f);
        ratioHi = Clamp(ratioHi, 0.0f, 1.0f);
        float barY = y + 2;
        float barH = rowHeight_ - 4;
        // Track background
        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, t.inputBg.a);
        ctx.fillRect(valTextX, barY, valW, barH);
        // Filled region between lo and hi
        float loX = valTextX + ratioLo * valW;
        float hiX = valTextX + ratioHi * valW;
        Color barCol(80, 140, 180, 200);
        if (draggingSlider_ && activeRow_ == idx) barCol = Color(100, 170, 210, 230);
        else if (hoveredRow_ == idx) barCol = Color(90, 155, 195, 215);
        ctx.fill.SetColor(barCol.r, barCol.g, barCol.b, barCol.a);
        ctx.fillRect(loX, barY, hiX - loX, barH);
        // Handle indicators (two small vertical bars)
        ctx.fill.SetColor(220, 220, 220, 240);
        ctx.fillRect(loX - 1.5f, barY, 3.0f, barH);
        ctx.fillRect(hiX - 1.5f, barY, 3.0f, barH);
        // Border
        ctx.line.SetColor(t.inputBorder.r, t.inputBorder.g, t.inputBorder.b, t.inputBorder.a);
        ctx.lineRect(valTextX, barY, valW, barH);
        // Text: "lo - hi"
        char buf[48];
        snprintf(buf, sizeof(buf), "%.1f - %.1f", row.rangeLo, row.rangeHi);
        ctx.font.SetColor(Color(230, 230, 230, 255));
        ctx.font.Print(buf, valTextX + 3, textY, &fc);
        break;
    }
    default: break;
    }
}

void PropertyGrid::paintDescPanel(PaintContext& ctx, const Rect& abs)
{
    if (descHeight_ <= 0) return;

    const auto& t = Theme::instance();
    float gh = gridHeight();
    float descY = abs.y + gh;

    // Separator line
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
    ctx.drawLine(abs.x, descY, abs.x + abs.w, descY);

    // Background
    ctx.fill.SetColor(t.panelColor.r, t.panelColor.g, t.panelColor.b, t.panelColor.a);
    ctx.fillRect(abs.x, descY + 1, abs.w, descHeight_ - 1);

    if (selectedRow_ < 0 || selectedRow_ >= static_cast<int>(rows_.size())) return;

    const auto& row = rows_[selectedRow_];

    // Property name (bold-like - just brighter)
    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetColor(Color(220, 220, 225, 255));
    ctx.font.SetBatch(&ctx.text);
    auto fc = toFontClipTPC(ctx.clipRect());
    ctx.font.Print(row.name.c_str(), abs.x + 6, descY + 4, &fc);

    // Description text (dimmer, second line)
    if (!row.description.empty()) {
        ctx.font.SetColor(Color(150, 150, 155, 255));
        ctx.font.Print(row.description.c_str(), abs.x + 6, descY + 4 + t.fontSize + 2, &fc);
    }
}

void PropertyGrid::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    float gh = gridHeight();

    // Background
    ctx.fill.SetColor(t.panelColor.r, t.panelColor.g, t.panelColor.b, t.panelColor.a);
    ctx.fillRect(abs.x, abs.y, abs.w, abs.h);

    // Grid area clip
    Rect gridClip = {abs.x, abs.y, abs.w, gh};
    ctx.pushClip(gridClip);

    auto vis = visibleRows();
    int count = static_cast<int>(vis.size());
    int firstVisible = static_cast<int>(scrollOffset_ / rowHeight_);
    int lastVisible = firstVisible + static_cast<int>(gh / rowHeight_) + 2;
    lastVisible = std::min(lastVisible, count);

    for (int vi = firstVisible; vi < lastVisible; ++vi) {
        float rowY = abs.y + vi * rowHeight_ - scrollOffset_;
        paintRow(ctx, abs, vis[vi], rowY);
    }

    // Row separator lines
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, 40);
    for (int vi = firstVisible; vi < lastVisible; ++vi) {
        float ry = abs.y + (vi + 1) * rowHeight_ - scrollOffset_;
        ctx.drawLine(abs.x, ry, abs.x + abs.w, ry);
    }

    ctx.popClip();

    // Description panel
    paintDescPanel(ctx, abs);

    // Border
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
    ctx.lineRect(abs.x, abs.y, abs.w, abs.h);
}

void PropertyGrid::onMousePress(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y)) return;

    e.consumed = true;

    // Ignore clicks in desc panel
    float gh = gridHeight();
    if (e.y > abs.y + gh) return;

    int idx = hitRow(e.y - abs.y);
    if (idx < 0) return;

    auto& row = rows_[idx];

    // Section toggle
    if (row.type == PropType::Section) {
        row.expanded = !row.expanded;
        markDirty();
        return;
    }

    // Separator - ignore
    if (row.type == PropType::Separator) return;

    // Select row
    if (selectedRow_ != idx) {
        if (editing_) commitEdit();
        selectedRow_ = idx;
        markDirty();
    }

    float labelW = abs.w * labelRatio_;
    float valueX = abs.x + labelW;
    bool inValue = (e.x >= valueX);
    if (!inValue) return;

    float valueW = abs.w - labelW;
    float pad = 4.0f;
    float valTextX = valueX + pad;
    float valW = valueW - pad * 2;

    switch (row.type) {
    case PropType::Bool:
        row.boolVal = !row.boolVal;
        if (row.onChangeBool) row.onChangeBool(row.boolVal);
        propertyChanged.emit(idx);
        markDirty();
        break;

    case PropType::Float: {
        // If already editing, stay in edit mode
        if (editing_ && activeRow_ == idx) break;
        // Start slider drag - set value to click position immediately
        draggingSlider_ = true;
        activeRow_ = idx;
        dragStartX_ = e.x;
        dragComponent_ = 0;
        {
            float ratio = Clamp((e.x - valTextX) / valW, 0.0f, 1.0f);
            row.floatVal = row.floatMin + ratio * (row.floatMax - row.floatMin);
            dragStartVal_ = row.floatVal;
            if (row.onChangeFloat) row.onChangeFloat(row.floatVal);
            markDirty();
        }
        break;
    }
    case PropType::Int: {
        if (editing_ && activeRow_ == idx) break;
        draggingSlider_ = true;
        activeRow_ = idx;
        dragStartX_ = e.x;
        dragComponent_ = 0;
        {
            float ratio = Clamp((e.x - valTextX) / valW, 0.0f, 1.0f);
            row.intVal = static_cast<int>(row.intMin + ratio * (row.intMax - row.intMin) + 0.5f);
            dragStartVal_ = static_cast<float>(row.intVal);
            if (row.onChangeInt) row.onChangeInt(row.intVal);
            markDirty();
        }
        break;
    }
    case PropType::Vec2:
    case PropType::Vec3:
    case PropType::Vec4: {
        // Determine which component was clicked
        int nc = row.vecComponents;
        float gap = 2.0f;
        float compW = (valW - gap * (nc - 1)) / nc;
        int comp = static_cast<int>((e.x - valTextX) / (compW + gap));
        comp = Clamp(comp, 0, nc - 1);
        draggingSlider_ = true;
        activeRow_ = idx;
        dragStartX_ = e.x;
        dragComponent_ = comp;
        {
            float compX = valTextX + comp * (compW + gap);
            float ratio = Clamp((e.x - compX) / compW, 0.0f, 1.0f);
            row.vecVal[comp] = row.floatMin + ratio * (row.floatMax - row.floatMin);
            dragStartVal_ = row.vecVal[comp];
            if (row.type == PropType::Vec2 && row.onChangeVec2)
                row.onChangeVec2(row.vecVal[0], row.vecVal[1]);
            else if (row.type == PropType::Vec3 && row.onChangeVec3)
                row.onChangeVec3(row.vecVal[0], row.vecVal[1], row.vecVal[2]);
            else if (row.type == PropType::Vec4 && row.onChangeVec4)
                row.onChangeVec4(row.vecVal[0], row.vecVal[1], row.vecVal[2], row.vecVal[3]);
            markDirty();
        }
        break;
    }
    case PropType::String:
        if (!editing_ || activeRow_ != idx)
            startEdit(idx);
        break;

    case PropType::Combo: {
        // Open dropdown popup below the combo row
        auto vis = visibleRows();
        float rowY = abs.y;
        for (int vi = 0; vi < static_cast<int>(vis.size()); ++vi) {
            if (vis[vi] == idx) {
                rowY = abs.y + vi * rowHeight_ - scrollOffset_;
                break;
            }
        }
        int numItems = static_cast<int>(row.comboOptions.size());
        float itemH = 22.0f;
        float popH = numItems * itemH + 2;
        float popW = abs.w - abs.w * labelRatio_;
        float popX = abs.x + abs.w * labelRatio_;
        float popY = rowY + rowHeight_;
        auto* popup = new PropComboPopup_(this, idx);
        popup->setRect({popX, popY, popW, popH});
        WidgetApp::instance().showPopup(popup, nullptr);
        e.consumed = true;
        markDirty();
        break;
    }
    case PropType::Button:
        if (row.onClick) row.onClick();
        markDirty();
        break;

    case PropType::Range: {
        // Determine which handle is closer: lo or hi
        float range = row.floatMax - row.floatMin;
        float ratioLo = (range > 0.0001f) ? (row.rangeLo - row.floatMin) / range : 0.0f;
        float ratioHi = (range > 0.0001f) ? (row.rangeHi - row.floatMin) / range : 1.0f;
        float loX = valTextX + ratioLo * valW;
        float hiX = valTextX + ratioHi * valW;
        float distLo = std::abs(e.x - loX);
        float distHi = std::abs(e.x - hiX);
        int handle = (distLo <= distHi) ? 0 : 1;  // 0 = lo, 1 = hi
        draggingSlider_ = true;
        activeRow_ = idx;
        dragStartX_ = e.x;
        dragComponent_ = handle;
        {
            float ratio = Clamp((e.x - valTextX) / valW, 0.0f, 1.0f);
            float newVal = row.floatMin + ratio * range;
            if (handle == 0) {
                row.rangeLo = Clamp(newVal, row.floatMin, row.rangeHi);
            } else {
                row.rangeHi = Clamp(newVal, row.rangeLo, row.floatMax);
            }
            dragStartVal_ = (handle == 0) ? row.rangeLo : row.rangeHi;
            if (row.onChangeRange) row.onChangeRange(row.rangeLo, row.rangeHi);
            markDirty();
        }
        break;
    }

    case PropType::Color: {
        // Open a floating ColorPicker popup below the swatch
        auto vis = visibleRows();
        float rowY = abs.y;
        for (int vi = 0; vi < static_cast<int>(vis.size()); ++vi) {
            if (vis[vi] == idx) {
                rowY = abs.y + vi * rowHeight_ - scrollOffset_;
                break;
            }
        }
        float popW = 230.0f;
        float popH = 270.0f;
        float popX = abs.x + abs.w * labelRatio_;
        float popY = rowY + rowHeight_;
        auto* popup = new ColorPickerPopup_(this, idx);
        popup->setRect({popX, popY, popW, popH});
        WidgetApp::instance().showPopup(popup, nullptr);  // no owner → click outside always closes
        e.consumed = true;
        break;
    }

    default: break;
    }
}

void PropertyGrid::onMouseRelease(MouseEvent& e)
{
    if (draggingSlider_) {
        draggingSlider_ = false;
        if (activeRow_ >= 0) {
            // If drag was tiny (< 3px), treat as click → start edit
            float dx = std::abs(e.x - dragStartX_);
            if (dx < 3.0f) {
                auto& row = rows_[activeRow_];
                if (row.type == PropType::Float || row.type == PropType::Int)
                    startEdit(activeRow_);
                // Vec: no inline edit for individual components
            } else {
                propertyChanged.emit(activeRow_);
            }
            if (!editing_) activeRow_ = -1;
        }
    }
    (void)e;
}

void PropertyGrid::onMouseMove(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();

    // ── Slider drag in progress ──────────────────────────────────────────
    if (draggingSlider_ && activeRow_ >= 0 && activeRow_ < static_cast<int>(rows_.size())) {
        e.consumed = true;
        auto& row = rows_[activeRow_];
        float labelW = abs.w * labelRatio_;
        float valueX = abs.x + labelW;
        float valueW = abs.w - labelW;
        float pad = 4.0f;
        float valTextX = valueX + pad;
        float valW = valueW - pad * 2;

        float minVal = 0, maxVal = 1;
        float sliderX = valTextX;
        float sliderW = valW;

        if (row.type == PropType::Float) {
            minVal = row.floatMin;
            maxVal = row.floatMax;
        } else if (row.type == PropType::Int) {
            minVal = static_cast<float>(row.intMin);
            maxVal = static_cast<float>(row.intMax);
        } else if (row.type == PropType::Vec2 || row.type == PropType::Vec3 || row.type == PropType::Vec4) {
            minVal = row.floatMin;
            maxVal = row.floatMax;
            int nc = row.vecComponents;
            float gap = 2.0f;
            float compW = (valW - gap * (nc - 1)) / nc;
            sliderX = valTextX + dragComponent_ * (compW + gap);
            sliderW = compW;
        } else if (row.type == PropType::Range) {
            minVal = row.floatMin;
            maxVal = row.floatMax;
        }

        float range = maxVal - minVal;
        if (range > 0.0001f && sliderW > 1.0f) {
            // Map mouse X directly to value (absolute, not relative)
            float ratio = Clamp((e.x - sliderX) / sliderW, 0.0f, 1.0f);
            float newVal = minVal + ratio * range;

            if (row.type == PropType::Float) {
                row.floatVal = newVal;
                if (row.onChangeFloat) row.onChangeFloat(row.floatVal);
            } else if (row.type == PropType::Int) {
                row.intVal = static_cast<int>(newVal + 0.5f);
                if (row.onChangeInt) row.onChangeInt(row.intVal);
            } else if (row.type == PropType::Range) {
                // dragComponent_ 0=lo, 1=hi
                if (dragComponent_ == 0) {
                    row.rangeLo = Clamp(newVal, row.floatMin, row.rangeHi);
                } else {
                    row.rangeHi = Clamp(newVal, row.rangeLo, row.floatMax);
                }
                if (row.onChangeRange) row.onChangeRange(row.rangeLo, row.rangeHi);
            } else {
                // Vec component
                row.vecVal[dragComponent_] = newVal;
                if (row.type == PropType::Vec2 && row.onChangeVec2)
                    row.onChangeVec2(row.vecVal[0], row.vecVal[1]);
                else if (row.type == PropType::Vec3 && row.onChangeVec3)
                    row.onChangeVec3(row.vecVal[0], row.vecVal[1], row.vecVal[2]);
                else if (row.type == PropType::Vec4 && row.onChangeVec4)
                    row.onChangeVec4(row.vecVal[0], row.vecVal[1], row.vecVal[2], row.vecVal[3]);
            }
            markDirty();
        }
        return;
    }

    // ── Hover tracking ───────────────────────────────────────────────────
    if (abs.contains(e.x, e.y) && e.y < abs.y + gridHeight()) {
        int idx = hitRow(e.y - abs.y);
        if (idx != hoveredRow_) {
            hoveredRow_ = idx;
            markDirty();
        }
    } else {
        if (hoveredRow_ != -1) {
            hoveredRow_ = -1;
            markDirty();
        }
    }
}

void PropertyGrid::onMouseScroll(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y)) return;

    e.consumed = true;
    scrollOffset_ -= e.scrollY * rowHeight_ * 2.0f;
    scrollOffset_ = Clamp(scrollOffset_, 0.0f, maxScroll());
    markDirty();
}

void PropertyGrid::onKeyPress(KeyEvent& e)
{
    if (!focused_ && !editing_) return;

    if (editing_) {
        e.consumed = true;
        switch (e.key) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            commitEdit();
            return;
        case SDLK_ESCAPE:
            cancelEdit();
            return;
        case SDLK_BACKSPACE:
            if (editCursor_ > 0 && !editBuf_.empty()) {
                editBuf_.erase(editCursor_ - 1, 1);
                editCursor_--;
                markDirty();
            }
            return;
        case SDLK_DELETE:
            if (editCursor_ < static_cast<int>(editBuf_.size())) {
                editBuf_.erase(editCursor_, 1);
                markDirty();
            }
            return;
        case SDLK_RIGHT:
            if (editCursor_ < static_cast<int>(editBuf_.size())) {
                editCursor_++;
                markDirty();
            }
            return;
        case SDLK_LEFT:
            if (editCursor_ > 0) {
                editCursor_--;
                markDirty();
            }
            return;
        case SDLK_HOME:
            editCursor_ = 0;
            markDirty();
            return;
        case SDLK_END:
            editCursor_ = static_cast<int>(editBuf_.size());
            markDirty();
            return;
        }
    }

    // Arrow keys for row navigation (when not editing)
    if (!editing_) {
        auto vis = visibleRows();
        int visIdx = -1;
        for (int i = 0; i < static_cast<int>(vis.size()); ++i)
            if (vis[i] == selectedRow_) { visIdx = i; break; }

        if (e.key == SDLK_UP) {
            e.consumed = true;
            if (visIdx > 0) {
                selectedRow_ = vis[visIdx - 1];
                if (rows_[selectedRow_].type == PropType::Section && visIdx > 1)
                    selectedRow_ = vis[visIdx - 1]; // stay on it (section is selectable)
                markDirty();
            }
        } else if (e.key == SDLK_DOWN) {
            e.consumed = true;
            if (visIdx < static_cast<int>(vis.size()) - 1) {
                selectedRow_ = vis[visIdx + 1];
                markDirty();
            }
        } else if (e.key == SDLK_RETURN || e.key == SDLK_KP_ENTER) {
            e.consumed = true;
            if (selectedRow_ >= 0 && selectedRow_ < static_cast<int>(rows_.size())) {
                auto& row = rows_[selectedRow_];
                if (row.type == PropType::Section)
                    row.expanded = !row.expanded;
                else if (row.type == PropType::String || row.type == PropType::Float || row.type == PropType::Int)
                    startEdit(selectedRow_);
                else if (row.type == PropType::Bool) {
                    row.boolVal = !row.boolVal;
                    if (row.onChangeBool) row.onChangeBool(row.boolVal);
                    propertyChanged.emit(selectedRow_);
                }
                markDirty();
            }
        }
    }
}

void PropertyGrid::onTextInput(KeyEvent& e)
{
    if (!editing_) return;
    e.consumed = true;

    // Filter for numeric types
    if (activeRow_ >= 0 && activeRow_ < static_cast<int>(rows_.size())) {
        auto& row = rows_[activeRow_];
        if (row.type == PropType::Int) {
            for (int i = 0; e.text[i]; ++i) {
                char c = e.text[i];
                if ((c >= '0' && c <= '9') || c == '-') {
                    editBuf_.insert(editCursor_, 1, c);
                    editCursor_++;
                }
            }
            markDirty();
            return;
        }
        if (row.type == PropType::Float) {
            for (int i = 0; e.text[i]; ++i) {
                char c = e.text[i];
                if ((c >= '0' && c <= '9') || c == '-' || c == '.') {
                    editBuf_.insert(editCursor_, 1, c);
                    editCursor_++;
                }
            }
            markDirty();
            return;
        }
    }

    // String - accept anything
    for (int i = 0; e.text[i]; ++i) {
        editBuf_.insert(editCursor_, 1, e.text[i]);
        editCursor_++;
    }
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  ColorPicker
// ═════════════════════════════════════════════════════════════════════════════

ColorPicker::ColorPicker()
{
    acceptsFocus_ = true;
}

void ColorPicker::setColor(const Color& c)
{
    rgbToHsv(c, hue_, sat_, val_);
    alpha_ = c.a / 255.0f;
    markDirty();
}

Color ColorPicker::color() const
{
    return hsvToRgb(hue_, sat_, val_, alpha_);
}

Widget::Vec2f ColorPicker::sizeHint() const
{
    return {220.0f, 260.0f};
}

float ColorPicker::wheelRadius(const Rect& abs) const
{
    float barSpace = 30.0f + (showAlpha_ ? 30.0f : 0.0f);
    float available = std::min(abs.w - barSpace, abs.h - 44.0f);
    return std::max(20.0f, available * 0.5f);
}

float ColorPicker::wheelCenterX(const Rect& abs) const
{
    return abs.x + wheelRadius(abs) + 4.0f;
}

float ColorPicker::wheelCenterY(const Rect& abs) const
{
    return abs.y + wheelRadius(abs) + 4.0f;
}

Rect ColorPicker::valueBarRect(const Rect& abs) const
{
    float r = wheelRadius(abs);
    float cx = wheelCenterX(abs);
    float barX = cx + r + 10.0f;
    float barW = 18.0f;
    float barY = abs.y + 4.0f;
    float barH = r * 2.0f;
    return {barX, barY, barW, barH};
}

Rect ColorPicker::alphaBarRect(const Rect& abs) const
{
    Rect vb = valueBarRect(abs);
    return {vb.x + vb.w + 8.0f, vb.y, vb.w, vb.h};
}

Rect ColorPicker::previewRect(const Rect& abs) const
{
    float r = wheelRadius(abs);
    float bottomY = abs.y + r * 2.0f + 12.0f;
    return {abs.x + 4.0f, bottomY, 40.0f, 28.0f};
}

Rect ColorPicker::hexRect(const Rect& abs) const
{
    Rect pr = previewRect(abs);
    return {pr.x + pr.w + 8.0f, pr.y, 100.0f, pr.h};
}

void ColorPicker::updateFromWheel(float mx, float my, const Rect& abs)
{
    float cx = wheelCenterX(abs);
    float cy = wheelCenterY(abs);
    float r = wheelRadius(abs);
    float innerR = r * 0.75f;

    float dx = mx - cx;
    float dy = my - cy;

    float dist = sqrtf(dx * dx + dy * dy);

    hue_ = atan2f(dy, dx) / (2.0f * 3.14159265f);
    if (hue_ < 0) hue_ += 1.0f;

    // Saturation is mapped to innerR (the saturation disc), not outer radius
    sat_ = dist / innerR;
    if (sat_ > 1.0f) sat_ = 1.0f;
}

void ColorPicker::updateFromValueBar(float my, const Rect& abs)
{
    Rect bar = valueBarRect(abs);
    float ratio = (my - bar.y) / bar.h;
    val_ = 1.0f - Clamp(ratio, 0.0f, 1.0f);
}

void ColorPicker::updateFromAlphaBar(float my, const Rect& abs)
{
    Rect bar = alphaBarRect(abs);
    float ratio = (my - bar.y) / bar.h;
    alpha_ = 1.0f - Clamp(ratio, 0.0f, 1.0f);
}

Color ColorPicker::hsvToRgb(float h, float s, float v, float a)
{
    float r = v, g = v, b = v;
    if (s > 0.0f) {
        h = fmodf(h, 1.0f) * 6.0f;
        int i = static_cast<int>(h);
        float f = h - i;
        float p = v * (1 - s);
        float q = v * (1 - s * f);
        float t = v * (1 - s * (1 - f));
        switch (i) {
            case 0: r=v; g=t; b=p; break;
            case 1: r=q; g=v; b=p; break;
            case 2: r=p; g=v; b=t; break;
            case 3: r=p; g=q; b=v; break;
            case 4: r=t; g=p; b=v; break;
            default: r=v; g=p; b=q; break;
        }
    }
    return Color(static_cast<u8>(r * 255), static_cast<u8>(g * 255),
                 static_cast<u8>(b * 255), static_cast<u8>(a * 255));
}

void ColorPicker::rgbToHsv(const Color& c, float& h, float& s, float& v)
{
    float r = c.r / 255.0f, g = c.g / 255.0f, b = c.b / 255.0f;
    float mx = std::max({r, g, b});
    float mn = std::min({r, g, b});
    float d = mx - mn;

    v = mx;
    s = (mx > 0) ? d / mx : 0;

    if (d < 0.00001f) {
        h = 0;
    } else if (mx == r) {
        h = fmodf((g - b) / d, 6.0f) / 6.0f;
    } else if (mx == g) {
        h = ((b - r) / d + 2.0f) / 6.0f;
    } else {
        h = ((r - g) / d + 4.0f) / 6.0f;
    }
    if (h < 0) h += 1.0f;
}

void ColorPicker::paint(PaintContext& ctx)
{
    if (!visible_) return;

    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    // Background
    ctx.fill.SetColor(t.panelColor.r, t.panelColor.g, t.panelColor.b, t.panelColor.a);
    ctx.fillRect(abs.x, abs.y, abs.w, abs.h);

    ctx.pushClip(abs);

    float cx = wheelCenterX(abs);
    float cy = wheelCenterY(abs);
    float r = wheelRadius(abs);

    // ── Color wheel (hue ring) ───────────────────────────────────────────
    // Draw hue ring as colored segments
    const int segments = 90;
    const float step = 2.0f * 3.14159265f / segments;
    float outerR = r;
    float innerR = r * 0.75f;

    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * (i + 1) + 0.005f; // slight overlap to avoid gaps
        float hue = static_cast<float>(i) / segments;
        Color c = hsvToRgb(hue, 1.0f, 1.0f, 1.0f);
        ctx.fill.SetColor(c.r, c.g, c.b, c.a);

        // Outer triangle
        ctx.fillTriangle(
            cx + cosf(a0) * outerR, cy + sinf(a0) * outerR,
            cx + cosf(a1) * outerR, cy + sinf(a1) * outerR,
            cx + cosf(a0) * innerR, cy + sinf(a0) * innerR);
        // Inner triangle
        ctx.fillTriangle(
            cx + cosf(a1) * outerR, cy + sinf(a1) * outerR,
            cx + cosf(a1) * innerR, cy + sinf(a1) * innerR,
            cx + cosf(a0) * innerR, cy + sinf(a0) * innerR);
    }

    // ── Saturation fill (inner disc: white center → full color at edge) ──
    // Approximate with concentric rings of triangles
    const int satRings = 16;
    const int satSegs = 48;
    for (int ring = 0; ring < satRings; ++ring) {
        float r0 = innerR * ring / satRings;
        float r1 = innerR * (ring + 1) / satRings + 0.5f; // slight overlap
        float s0 = static_cast<float>(ring) / satRings;
        float s1 = static_cast<float>(ring + 1) / satRings;

        for (int seg = 0; seg < satSegs; ++seg) {
            float a0 = 2.0f * 3.14159265f * seg / satSegs;
            float a1 = 2.0f * 3.14159265f * (seg + 1) / satSegs + 0.005f;
            float h = static_cast<float>(seg) / satSegs;

            // Blend between white (s=0) and hue color (s=1)
            Color c0 = hsvToRgb(h, s0, val_, 1.0f);
            Color c1 = hsvToRgb(h, s1, val_, 1.0f);
            // Use average color for the quad
            Color avg;
            avg.r = (c0.r + c1.r) / 2;
            avg.g = (c0.g + c1.g) / 2;
            avg.b = (c0.b + c1.b) / 2;
            avg.a = 255;
            ctx.fill.SetColor(avg.r, avg.g, avg.b, avg.a);

            ctx.fillTriangle(
                cx + cosf(a0) * r0, cy + sinf(a0) * r0,
                cx + cosf(a0) * r1, cy + sinf(a0) * r1,
                cx + cosf(a1) * r1, cy + sinf(a1) * r1);
            ctx.fillTriangle(
                cx + cosf(a0) * r0, cy + sinf(a0) * r0,
                cx + cosf(a1) * r1, cy + sinf(a1) * r1,
                cx + cosf(a1) * r0, cy + sinf(a1) * r0);
        }
    }

    // ── Hue/sat cursor on wheel ──────────────────────────────────────────
    float cursorAngle = hue_ * 2.0f * 3.14159265f;
    float cursorDist = sat_ * innerR;
    float cursorX = cx + cosf(cursorAngle) * cursorDist;
    float cursorY = cy + sinf(cursorAngle) * cursorDist;
    ctx.line.SetColor(255, 255, 255, 255);
    ctx.lineCircle(cursorX, cursorY, 5.0f);
    ctx.line.SetColor(0, 0, 0, 255);
    ctx.lineCircle(cursorX, cursorY, 4.0f);

    // ── Value (brightness) bar ───────────────────────────────────────────
    Rect vbar = valueBarRect(abs);
    const int barSteps = 20;
    float stepH = vbar.h / barSteps;
    for (int i = 0; i < barSteps; ++i) {
        float v = 1.0f - static_cast<float>(i) / barSteps;
        Color c = hsvToRgb(hue_, sat_, v, 1.0f);
        ctx.fill.SetColor(c.r, c.g, c.b, 255);
        ctx.fillRect(vbar.x, vbar.y + i * stepH, vbar.w, stepH + 1);
    }
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
    ctx.lineRect(vbar.x, vbar.y, vbar.w, vbar.h);

    // Value cursor
    float valY = vbar.y + (1.0f - val_) * vbar.h;
    ctx.fill.SetColor(255, 255, 255, 255);
    ctx.fillTriangle(vbar.x - 4, valY - 4, vbar.x - 4, valY + 4, vbar.x, valY);
    ctx.fillTriangle(vbar.x + vbar.w + 4, valY - 4, vbar.x + vbar.w + 4, valY + 4, vbar.x + vbar.w, valY);

    // ── Alpha bar (optional) ─────────────────────────────────────────────
    if (showAlpha_) {
        Rect abar = alphaBarRect(abs);
        Color full = hsvToRgb(hue_, sat_, val_, 1.0f);
        for (int i = 0; i < barSteps; ++i) {
            float a = 1.0f - static_cast<float>(i) / barSteps;
            ctx.fill.SetColor(full.r, full.g, full.b, static_cast<u8>(a * 255));
            ctx.fillRect(abar.x, abar.y + i * stepH, abar.w, stepH + 1);
        }
        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
        ctx.lineRect(abar.x, abar.y, abar.w, abar.h);

        float alphaY = abar.y + (1.0f - alpha_) * abar.h;
        ctx.fill.SetColor(255, 255, 255, 255);
        ctx.fillTriangle(abar.x - 4, alphaY - 4, abar.x - 4, alphaY + 4, abar.x, alphaY);
        ctx.fillTriangle(abar.x + abar.w + 4, alphaY - 4, abar.x + abar.w + 4, alphaY + 4, abar.x + abar.w, alphaY);
    }

    // ── Preview swatch + hex ─────────────────────────────────────────────
    Rect pr = previewRect(abs);
    Color cur = color();
    ctx.fill.SetColor(cur.r, cur.g, cur.b, cur.a);
    ctx.fillRect(pr.x, pr.y, pr.w, pr.h);
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
    ctx.lineRect(pr.x, pr.y, pr.w, pr.h);

    Rect hr = hexRect(abs);
    char hexBuf[20];
    if (showAlpha_)
        snprintf(hexBuf, sizeof(hexBuf), "#%02X%02X%02X%02X", cur.r, cur.g, cur.b, cur.a);
    else
        snprintf(hexBuf, sizeof(hexBuf), "#%02X%02X%02X", cur.r, cur.g, cur.b);

    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetColor(t.textColor);
    ctx.font.SetBatch(&ctx.text);
    auto fc = toFontClipTPC(ctx.clipRect());
    ctx.font.Print(hexBuf, hr.x, hr.y + (hr.h - t.fontSize) * 0.5f, &fc);

    // Border
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
    ctx.lineRect(abs.x, abs.y, abs.w, abs.h);

    ctx.popClip();
}

void ColorPicker::onMousePress(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y)) return;

    e.consumed = true;

    float cx = wheelCenterX(abs);
    float cy = wheelCenterY(abs);
    float r = wheelRadius(abs);

    float dx = e.x - cx;
    float dy = e.y - cy;
    float dist = sqrtf(dx * dx + dy * dy);

    // Check value bar
    Rect vbar = valueBarRect(abs);
    if (vbar.contains(e.x, e.y)) {
        dragTarget_ = ValueBar;
        updateFromValueBar(e.y, abs);
        colorChanged.emit(color());
        markDirty();
        return;
    }

    // Check alpha bar
    if (showAlpha_) {
        Rect abar = alphaBarRect(abs);
        if (abar.contains(e.x, e.y)) {
            dragTarget_ = AlphaBar;
            updateFromAlphaBar(e.y, abs);
            colorChanged.emit(color());
            markDirty();
            return;
        }
    }

    // Check wheel
    if (dist <= r) {
        dragTarget_ = Wheel;
        updateFromWheel(e.x, e.y, abs);
        colorChanged.emit(color());
        markDirty();
    }
}

void ColorPicker::onMouseRelease(MouseEvent& e)
{
    dragTarget_ = None;
    (void)e;
}

void ColorPicker::onMouseMove(MouseEvent& e)
{
    if (dragTarget_ == None) return;

    Rect abs = absoluteRect();
    e.consumed = true;

    switch (dragTarget_) {
    case Wheel:
        updateFromWheel(e.x, e.y, abs);
        break;
    case ValueBar:
        updateFromValueBar(e.y, abs);
        break;
    case AlphaBar:
        updateFromAlphaBar(e.y, abs);
        break;
    default: break;
    }

    colorChanged.emit(color());
    markDirty();
}
