#include "Widgets.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
    inline Font::ClipRect toFontClipTG(const Rect& r)
    { return {r.x, r.y, r.w, r.h}; }
}

// ═════════════════════════════════════════════════════════════════════════════
//  TreeGrid::Node
// ═════════════════════════════════════════════════════════════════════════════

TreeGrid::Node* TreeGrid::Node::addChild(const std::vector<std::string>& cellData)
{
    auto* child = new Node();
    child->cells = cellData;
    child->parent = this;
    child->depth = depth + 1;
    children.push_back(child);
    return child;
}

// ═════════════════════════════════════════════════════════════════════════════
//  TreeGrid - constructor / destructor helpers
// ═════════════════════════════════════════════════════════════════════════════

TreeGrid::TreeGrid()
{
    acceptsFocus_ = true;
}

TreeGrid::~TreeGrid()
{
    for (auto* r : roots_) deleteNodes(r);
}

void TreeGrid::deleteNodes(Node* n)
{
    for (auto* c : n->children)
        deleteNodes(c);
    delete n;
}

void TreeGrid::clearAll()
{
    for (auto* r : roots_) deleteNodes(r);
    roots_.clear();
    flatList_.clear();
    selectedNode_ = nullptr;
    selectedNodes_.clear();
    scrollY_ = 0;
    markDirty();
}

int TreeGrid::nodeCount() const
{
    // Count all nodes recursively
    int count = 0;
    std::vector<const Node*> stack;
    for (auto* r : roots_) stack.push_back(r);
    while (!stack.empty()) {
        auto* n = stack.back(); stack.pop_back();
        count++;
        for (auto* c : n->children) stack.push_back(c);
    }
    return count;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Columns
// ═════════════════════════════════════════════════════════════════════════════

int TreeGrid::addColumn(const std::string& name, float width, bool sortable)
{
    Column c;
    c.name = name;
    c.width = width;
    c.sortable = sortable;
    columns_.push_back(std::move(c));
    markDirty();
    return static_cast<int>(columns_.size()) - 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Roots
// ═════════════════════════════════════════════════════════════════════════════

TreeGrid::Node* TreeGrid::addRoot(const std::vector<std::string>& cells)
{
    auto* n = new Node();
    n->cells = cells;
    n->depth = 0;
    roots_.push_back(n);
    rebuildFlatList();
    markDirty();
    return n;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Flat list (visible nodes)
// ═════════════════════════════════════════════════════════════════════════════

void TreeGrid::rebuildFlatList()
{
    flatList_.clear();
    for (auto* r : roots_)
        flattenNode(r, 0);
}

void TreeGrid::flattenNode(Node* n, int depth)
{
    flatList_.push_back({n, depth});
    if (n->expanded) {
        for (auto* c : n->children)
            flattenNode(c, depth + 1);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Sort
// ═════════════════════════════════════════════════════════════════════════════

void TreeGrid::sortChildren(Node* parent)
{
    if (sortCol_ < 0 || sortOrder_ == DataGrid::SortOrder::None) return;
    int col = sortCol_;
    bool asc = (sortOrder_ == DataGrid::SortOrder::Ascending);

    auto cmp = [col, asc](Node* a, Node* b) {
        const std::string& sa = (col < static_cast<int>(a->cells.size())) ? a->cells[col] : "";
        const std::string& sb = (col < static_cast<int>(b->cells.size())) ? b->cells[col] : "";
        char* endA; char* endB;
        double da = strtod(sa.c_str(), &endA);
        double db = strtod(sb.c_str(), &endB);
        if (endA != sa.c_str() && endB != sb.c_str())
            return asc ? da < db : da > db;
        return asc ? sa < sb : sa > sb;
    };

    if (parent) {
        std::stable_sort(parent->children.begin(), parent->children.end(), cmp);
        for (auto* c : parent->children)
            sortChildren(c);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Selection
// ═════════════════════════════════════════════════════════════════════════════

void TreeGrid::clearSelection()
{
    selectedNode_ = nullptr;
    selectedNodes_.clear();
    markDirty();
}

bool TreeGrid::isSelected(Node* n) const
{
    if (!multiSelect_) return n == selectedNode_;
    for (auto* s : selectedNodes_)
        if (s == n) return true;
    return false;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Geometry helpers
// ═════════════════════════════════════════════════════════════════════════════

float TreeGrid::totalColumnsWidth() const
{
    float w = 0;
    for (auto& c : columns_) w += c.width;
    return w;
}

float TreeGrid::columnX(int col) const
{
    float x = 0;
    for (int i = 0; i < col && i < static_cast<int>(columns_.size()); ++i)
        x += columns_[i].width;
    return x;
}

int TreeGrid::hitColumn(float localX) const
{
    float x = -scrollX_;
    for (int i = 0; i < static_cast<int>(columns_.size()); ++i) {
        if (localX >= x && localX < x + columns_[i].width)
            return i;
        x += columns_[i].width;
    }
    return -1;
}

int TreeGrid::hitRow(float localY) const
{
    if (localY < headerHeight_) return -1;
    float y = localY - headerHeight_ + scrollY_;
    int idx = static_cast<int>(y / rowHeight_);
    if (idx < 0 || idx >= static_cast<int>(flatList_.size()))
        return -1;
    return idx;
}

int TreeGrid::hitResizeEdge(float localX) const
{
    float x = -scrollX_;
    for (int i = 0; i < static_cast<int>(columns_.size()); ++i) {
        x += columns_[i].width;
        if (std::abs(localX - x) <= 4.0f)
            return i;
    }
    return -1;
}

Widget::Vec2f TreeGrid::sizeHint() const
{
    return {totalColumnsWidth(), headerHeight_ + flatList_.size() * rowHeight_};
}

// ═════════════════════════════════════════════════════════════════════════════
//  Read-only
// ═════════════════════════════════════════════════════════════════════════════

void TreeGrid::setColumnReadOnly(int col, bool ro)
{
    if (col >= 0 && col < static_cast<int>(columns_.size()))
        columns_[col].readOnly = ro;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Inline editing
// ═════════════════════════════════════════════════════════════════════════════

void TreeGrid::startEdit(Node* node, int col)
{
    if (!node || col < 0 || col >= static_cast<int>(columns_.size())) return;
    // Check read-only: global, per-column, or per-node
    if (readOnly_ || columns_[col].readOnly || node->readOnly) return;
    editing_ = true;
    editNode_ = node;
    editCol_ = col;
    editBuf_ = (col < static_cast<int>(node->cells.size())) ? node->cells[col] : "";
    editCursor_ = static_cast<int>(editBuf_.size());
    markDirty();
}

void TreeGrid::commitEdit()
{
    if (!editing_ || !editNode_) return;
    editing_ = false;
    if (editCol_ >= 0 && editCol_ < static_cast<int>(editNode_->cells.size())) {
        editNode_->cells[editCol_] = editBuf_;
        cellEdited.emit(editNode_, editCol_);
    }
    editNode_ = nullptr;
    editCol_ = -1;
    markDirty();
}

void TreeGrid::cancelEdit()
{
    editing_ = false;
    editNode_ = nullptr;
    editCol_ = -1;
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Paint
// ═════════════════════════════════════════════════════════════════════════════

void TreeGrid::paint(PaintContext& ctx)
{
    if (!visible_) return;
    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();

    // Background
    ctx.fill.SetColor(t.panelColor.r, t.panelColor.g, t.panelColor.b, t.panelColor.a);
    ctx.fillRect(abs.x, abs.y, abs.w, abs.h);

    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetBatch(&ctx.text);

    // ── Column headers ───────────────────────────────────────────────────
    Rect headerClip = {abs.x, abs.y, abs.w, headerHeight_};
    ctx.pushClip(headerClip);
    {
        Color hdrBg(t.panelColor.r + 15, t.panelColor.g + 15, t.panelColor.b + 15, 255);
        ctx.fill.SetColor(hdrBg.r, hdrBg.g, hdrBg.b, hdrBg.a);
        ctx.fillRect(abs.x, abs.y, abs.w, headerHeight_);

        float hx = abs.x - scrollX_;
        auto fc = toFontClipTG(ctx.clipRect());
        for (int i = 0; i < static_cast<int>(columns_.size()); ++i) {
            auto& col = columns_[i];
            float colR = hx + col.width;

            ctx.font.SetColor(t.textColor);
            ctx.font.Print(col.name.c_str(), hx + 4, abs.y + (headerHeight_ - t.fontSize) * 0.5f, &fc);

            // Sort indicator
            if (sortCol_ == i && sortOrder_ != DataGrid::SortOrder::None) {
                float arrowX = colR - 14.0f;
                float arrowY = abs.y + headerHeight_ * 0.5f;
                ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, t.textColor.a);
                if (sortOrder_ == DataGrid::SortOrder::Ascending)
                    ctx.fillTriangle(arrowX, arrowY + 3, arrowX + 6, arrowY + 3, arrowX + 3, arrowY - 3);
                else
                    ctx.fillTriangle(arrowX, arrowY - 3, arrowX + 6, arrowY - 3, arrowX + 3, arrowY + 3);
            }

            ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, 80);
            ctx.drawLine(colR, abs.y + 2, colR, abs.y + headerHeight_ - 2);
            hx = colR;
        }

        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
        ctx.drawLine(abs.x, abs.y + headerHeight_, abs.x + abs.w, abs.y + headerHeight_);
    }
    ctx.popClip();

    // ── Rows ─────────────────────────────────────────────────────────────
    float bodyY = abs.y + headerHeight_;
    float bodyH = abs.h - headerHeight_;
    Rect bodyClip = {abs.x, bodyY, abs.w, bodyH};
    ctx.pushClip(bodyClip);
    {
        int totalVis = static_cast<int>(flatList_.size());
        int firstVis = static_cast<int>(scrollY_ / rowHeight_);
        int lastVis = firstVis + static_cast<int>(bodyH / rowHeight_) + 2;
        if (firstVis < 0) firstVis = 0;
        if (lastVis > totalVis) lastVis = totalVis;

        auto fc = toFontClipTG(ctx.clipRect());

        for (int vi = firstVis; vi < lastVis; ++vi) {
            auto& entry = flatList_[vi];
            Node* node = entry.node;
            int depth = entry.depth;

            float ry = bodyY + vi * rowHeight_ - scrollY_;
            Rect rowR = {abs.x, ry, abs.w, rowHeight_};
            if (ctx.isClipped(rowR)) continue;

            // Zebra stripes
            if (zebraStripes_ && (vi % 2 == 1)) {
                ctx.fill.SetColor(t.panelColor.r + 5, t.panelColor.g + 5, t.panelColor.b + 5, 255);
                ctx.fillRect(abs.x, ry, abs.w, rowHeight_);
            }

            // Selection
            if (isSelected(node)) {
                ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g, t.selectionColor.b, t.selectionColor.a);
                ctx.fillRect(abs.x, ry, abs.w, rowHeight_);
            }

            // Hover
            if (hoveredRow_ == vi && !isSelected(node)) {
                ctx.fill.SetColor(t.buttonHover.r, t.buttonHover.g, t.buttonHover.b, 60);
                ctx.fillRect(abs.x, ry, abs.w, rowHeight_);
            }

            float textY = ry + (rowHeight_ - t.fontSize) * 0.5f;

            // Draw each column
            float cx = abs.x - scrollX_;
            for (int ci = 0; ci < static_cast<int>(columns_.size()); ++ci) {
                float colW = columns_[ci].width;

                if (ci == 0) {
                    // First column: tree indent + expand arrow + icon + text
                    float indent = depth * indentWidth_;
                    float arrowX = cx + indent + 2;
                    float arrowCY = ry + rowHeight_ * 0.5f;

                    // Expand/collapse arrow
                    if (!node->children.empty()) {
                        ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, 180);
                        if (node->expanded) {
                            ctx.fillTriangle(arrowX, arrowCY - 4, arrowX + 8, arrowCY - 4, arrowX + 4, arrowCY + 4);
                        } else {
                            ctx.fillTriangle(arrowX, arrowCY - 4, arrowX + 8, arrowCY, arrowX, arrowCY + 4);
                        }
                    }

                    float textX = cx + indent + 14;

                    // Icon
                    if (node->iconId != IconId::None && ctx.icons) {
                        float iconS = rowHeight_ - 6;
                        ctx.drawIcon(node->iconId, textX, ry + 3, iconS);
                        textX += iconS + 2;
                    }

                    // Inline edit or text
                    if (editing_ && editNode_ == node && editCol_ == 0) {
                        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, 255);
                        ctx.fillRect(textX - 1, ry + 1, colW - indent - 16, rowHeight_ - 2);
                        ctx.line.SetColor(t.inputBorderHover.r, t.inputBorderHover.g, t.inputBorderHover.b, 255);
                        ctx.lineRect(textX - 1, ry + 1, colW - indent - 16, rowHeight_ - 2);
                        ctx.font.SetColor(Color(255, 255, 255, 255));
                        ctx.font.Print(editBuf_.c_str(), textX + 2, textY, &fc);
                        std::string bc = editBuf_.substr(0, editCursor_);
                        float curX = textX + 2 + ctx.font.GetTextWidth(bc.c_str());
                        ctx.line.SetColor(255, 255, 255, 200);
                        ctx.drawLine(curX, ry + 3, curX, ry + rowHeight_ - 3);
                    } else {
                        if (!node->cells.empty()) {
                            ctx.font.SetColor(isSelected(node) ? Color(255, 255, 255, 255) : t.textColor);
                            ctx.font.Print(node->cells[0].c_str(), textX + 2, textY, &fc);
                        }
                    }
                } else {
                    // Other columns: plain text or edit
                    if (editing_ && editNode_ == node && editCol_ == ci) {
                        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, 255);
                        ctx.fillRect(cx + 1, ry + 1, colW - 2, rowHeight_ - 2);
                        ctx.line.SetColor(t.inputBorderHover.r, t.inputBorderHover.g, t.inputBorderHover.b, 255);
                        ctx.lineRect(cx + 1, ry + 1, colW - 2, rowHeight_ - 2);
                        ctx.font.SetColor(Color(255, 255, 255, 255));
                        ctx.font.Print(editBuf_.c_str(), cx + 4, textY, &fc);
                        std::string bc = editBuf_.substr(0, editCursor_);
                        float curX = cx + 4 + ctx.font.GetTextWidth(bc.c_str());
                        ctx.line.SetColor(255, 255, 255, 200);
                        ctx.drawLine(curX, ry + 3, curX, ry + rowHeight_ - 3);
                    } else {
                        if (ci < static_cast<int>(node->cells.size())) {
                            ctx.font.SetColor(isSelected(node) ? Color(255, 255, 255, 255) : Color(170, 170, 175, 255));
                            ctx.font.Print(node->cells[ci].c_str(), cx + 4, textY, &fc);
                        }
                    }
                }

                // Column separator
                ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, 30);
                ctx.drawLine(cx + colW, ry, cx + colW, ry + rowHeight_);
                cx += colW;
            }

            // Row separator
            ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, 30);
            ctx.drawLine(abs.x, ry + rowHeight_, abs.x + abs.w, ry + rowHeight_);
        }
    }
    ctx.popClip();

    // ── Scrollbars ───────────────────────────────────────────────────────
    float contentH = static_cast<float>(flatList_.size()) * rowHeight_;
    float contentW = totalColumnsWidth();

    if (contentH > bodyH) {
        float sbW = 8.0f;
        float sbX = abs.x + abs.w - sbW;
        float trackH = bodyH;
        float thumbH = std::max(20.0f, (bodyH / contentH) * trackH);
        float maxSY = contentH - bodyH;
        float thumbY = bodyY + (maxSY > 0 ? (scrollY_ / maxSY) * (trackH - thumbH) : 0);
        ctx.fill.SetColor(t.panelColor.r + 20, t.panelColor.g + 20, t.panelColor.b + 20, 150);
        ctx.fillRect(sbX, bodyY, sbW, trackH);
        ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, 80);
        ctx.fillRect(sbX + 1, thumbY, sbW - 2, thumbH);
    }

    if (contentW > abs.w) {
        float sbH = 8.0f;
        float sbY = abs.y + abs.h - sbH;
        float trackW = abs.w;
        float thumbW = std::max(20.0f, (abs.w / contentW) * trackW);
        float maxSX = contentW - abs.w;
        float thumbX = abs.x + (maxSX > 0 ? (scrollX_ / maxSX) * (trackW - thumbW) : 0);
        ctx.fill.SetColor(t.panelColor.r + 20, t.panelColor.g + 20, t.panelColor.b + 20, 150);
        ctx.fillRect(abs.x, sbY, trackW, sbH);
        ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, 80);
        ctx.fillRect(thumbX, sbY + 1, thumbW, sbH - 2);
    }

    // Outer border
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
    ctx.lineRect(abs.x, abs.y, abs.w, abs.h);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Mouse events
// ═════════════════════════════════════════════════════════════════════════════

void TreeGrid::onMousePress(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y)) return;

    e.consumed = true;
    float localX = e.x - abs.x;
    float localY = e.y - abs.y;

    // ── Header ───────────────────────────────────────────────────────────
    if (localY < headerHeight_) {
        int resEdge = hitResizeEdge(localX);
        if (resEdge >= 0) {
            resizeCol_ = resEdge;
            resizeStartX_ = e.x;
            resizeStartW_ = columns_[resEdge].width;
            return;
        }

        int col = hitColumn(localX);
        if (col >= 0 && columns_[col].sortable) {
            if (sortCol_ == col) {
                if (sortOrder_ == DataGrid::SortOrder::None)
                    sortOrder_ = DataGrid::SortOrder::Ascending;
                else if (sortOrder_ == DataGrid::SortOrder::Ascending)
                    sortOrder_ = DataGrid::SortOrder::Descending;
                else
                    sortOrder_ = DataGrid::SortOrder::None;
            } else {
                sortCol_ = col;
                sortOrder_ = DataGrid::SortOrder::Ascending;
            }
            // Sort all root children recursively
            for (auto* r : roots_)
                sortChildren(r);
            rebuildFlatList();
            columnSorted.emit(col);
            markDirty();
        }
        return;
    }

    // ── Body ─────────────────────────────────────────────────────────────
    int visIdx = hitRow(localY);
    if (visIdx < 0 || visIdx >= static_cast<int>(flatList_.size())) return;
    Node* node = flatList_[visIdx].node;
    int depth = flatList_[visIdx].depth;

    // Check expand/collapse arrow click (first column, in indent area)
    float arrowAreaEnd = depth * indentWidth_ + 14;
    int col = hitColumn(localX);
    if (col == 0 && (localX + scrollX_) < arrowAreaEnd && !node->children.empty()) {
        node->expanded = !node->expanded;
        if (node->expanded)
            nodeExpanded.emit(node);
        else
            nodeCollapsed.emit(node);
        rebuildFlatList();
        markDirty();
        return;
    }

    // Double-click detection for inline edit
    static Uint32 lastClickTime = 0;
    static Node* lastClickNode = nullptr;
    Uint32 now = SDL_GetTicks();

    if (node == lastClickNode && (now - lastClickTime) < 400 && col >= 0) {
        startEdit(node, col);
        lastClickTime = 0;
        lastClickNode = nullptr;
        return;
    }
    lastClickTime = now;
    lastClickNode = node;

    if (editing_) commitEdit();

    // Selection
    if (multiSelect_) {
        bool ctrl = (SDL_GetModState() & KMOD_CTRL) != 0;
        if (ctrl) {
            auto it = std::find(selectedNodes_.begin(), selectedNodes_.end(), node);
            if (it != selectedNodes_.end())
                selectedNodes_.erase(it);
            else
                selectedNodes_.push_back(node);
            selectedNode_ = node;
        } else {
            selectedNode_ = node;
            selectedNodes_ = {node};
        }
    } else {
        selectedNode_ = node;
        selectedNodes_ = {node};
    }
    selectionChanged.emit(node);
    markDirty();
}

void TreeGrid::onMouseRelease(MouseEvent& e)
{
    if (resizeCol_ >= 0) resizeCol_ = -1;
    (void)e;
}

void TreeGrid::onMouseMove(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();

    if (resizeCol_ >= 0) {
        float dx = e.x - resizeStartX_;
        columns_[resizeCol_].width = std::max(columns_[resizeCol_].minWidth, resizeStartW_ + dx);
        markDirty();
        return;
    }

    float localY = e.y - abs.y;
    int visIdx = hitRow(localY);
    if (visIdx != hoveredRow_) {
        hoveredRow_ = visIdx;
        markDirty();
    }
}

void TreeGrid::onMouseScroll(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y)) return;

    e.consumed = true;
    bool shift = (SDL_GetModState() & KMOD_SHIFT) != 0;
    float bodyH = abs.h - headerHeight_;
    float contentH = static_cast<float>(flatList_.size()) * rowHeight_;
    float contentW = totalColumnsWidth();

    if (shift) {
        scrollX_ -= e.scrollY * 40.0f;
        scrollX_ = Clamp(scrollX_, 0.0f, std::max(0.0f, contentW - abs.w));
    } else {
        scrollY_ -= e.scrollY * rowHeight_ * 2.0f;
        scrollY_ = Clamp(scrollY_, 0.0f, std::max(0.0f, contentH - bodyH));
    }
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Keyboard
// ═════════════════════════════════════════════════════════════════════════════

void TreeGrid::onKeyPress(KeyEvent& e)
{
    if (!focused_ && !editing_) return;

    if (editing_) {
        e.consumed = true;
        switch (e.key) {
        case SDLK_RETURN: case SDLK_KP_ENTER:
            commitEdit(); return;
        case SDLK_ESCAPE:
            cancelEdit(); return;
        case SDLK_BACKSPACE:
            if (editCursor_ > 0 && !editBuf_.empty()) {
                editBuf_.erase(editCursor_ - 1, 1);
                editCursor_--; markDirty();
            } return;
        case SDLK_DELETE:
            if (editCursor_ < static_cast<int>(editBuf_.size())) {
                editBuf_.erase(editCursor_, 1); markDirty();
            } return;
        case SDLK_LEFT:
            if (editCursor_ > 0) { editCursor_--; markDirty(); } return;
        case SDLK_RIGHT:
            if (editCursor_ < static_cast<int>(editBuf_.size())) { editCursor_++; markDirty(); } return;
        case SDLK_HOME: editCursor_ = 0; markDirty(); return;
        case SDLK_END: editCursor_ = static_cast<int>(editBuf_.size()); markDirty(); return;
        case SDLK_c:
            if (e.ctrl) { SDL_SetClipboardText(editBuf_.c_str()); return; }
            break;
        case SDLK_v:
            if (e.ctrl) {
                char* clip = SDL_GetClipboardText();
                if (clip) { editBuf_.insert(editCursor_, clip); editCursor_ += static_cast<int>(strlen(clip)); SDL_free(clip); markDirty(); }
                return;
            } break;
        default: break;
        }
    }

    if (!editing_) {
        // Find current index in flat list
        int curIdx = -1;
        for (int i = 0; i < static_cast<int>(flatList_.size()); ++i)
            if (flatList_[i].node == selectedNode_) { curIdx = i; break; }

        if (e.key == SDLK_UP && curIdx > 0) {
            e.consumed = true;
            selectedNode_ = flatList_[curIdx - 1].node;
            selectedNodes_ = {selectedNode_};
            selectionChanged.emit(selectedNode_);
            markDirty();
        } else if (e.key == SDLK_DOWN && curIdx < static_cast<int>(flatList_.size()) - 1) {
            e.consumed = true;
            selectedNode_ = flatList_[curIdx + 1].node;
            selectedNodes_ = {selectedNode_};
            selectionChanged.emit(selectedNode_);
            markDirty();
        } else if (e.key == SDLK_LEFT && selectedNode_) {
            e.consumed = true;
            if (selectedNode_->expanded && !selectedNode_->children.empty()) {
                selectedNode_->expanded = false;
                nodeCollapsed.emit(selectedNode_);
                rebuildFlatList();
            } else if (selectedNode_->parent) {
                selectedNode_ = selectedNode_->parent;
                selectedNodes_ = {selectedNode_};
                selectionChanged.emit(selectedNode_);
            }
            markDirty();
        } else if (e.key == SDLK_RIGHT && selectedNode_) {
            e.consumed = true;
            if (!selectedNode_->expanded && !selectedNode_->children.empty()) {
                selectedNode_->expanded = true;
                nodeExpanded.emit(selectedNode_);
                rebuildFlatList();
            } else if (!selectedNode_->children.empty()) {
                selectedNode_ = selectedNode_->children[0];
                selectedNodes_ = {selectedNode_};
                selectionChanged.emit(selectedNode_);
            }
            markDirty();
        } else if (e.key == SDLK_RETURN && selectedNode_) {
            e.consumed = true;
            startEdit(selectedNode_, 0);
        } else if (e.key == SDLK_c && e.ctrl) {
            // Copy selected node cells
            e.consumed = true;
            if (selectedNode_) {
                std::string text;
                for (int c = 0; c < static_cast<int>(selectedNode_->cells.size()); ++c) {
                    if (c > 0) text += '\t';
                    text += selectedNode_->cells[c];
                }
                SDL_SetClipboardText(text.c_str());
            }
        }
    }
}

void TreeGrid::onTextInput(KeyEvent& e)
{
    if (!editing_) return;
    e.consumed = true;
    editBuf_.insert(editCursor_, e.text);
    editCursor_ += static_cast<int>(strlen(e.text));
    markDirty();
}
