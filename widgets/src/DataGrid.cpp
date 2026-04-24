#include "Widgets.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

namespace {
    inline Font::ClipRect toFontClipDG(const Rect& r)
    { return {r.x, r.y, r.w, r.h}; }
}

// ═════════════════════════════════════════════════════════════════════════════
//  DataGrid - constructor
// ═════════════════════════════════════════════════════════════════════════════

DataGrid::DataGrid()
{
    acceptsFocus_ = true;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Columns
// ═════════════════════════════════════════════════════════════════════════════

int DataGrid::addColumn(const std::string& name, float width, bool sortable)
{
    Column c;
    c.name = name;
    c.width = width;
    c.sortable = sortable;
    columns_.push_back(std::move(c));
    rebuildSortOrder();
    markDirty();
    return static_cast<int>(columns_.size()) - 1;
}

void DataGrid::setColumnWidth(int col, float w)
{
    if (col >= 0 && col < static_cast<int>(columns_.size())) {
        columns_[col].width = std::max(w, columns_[col].minWidth);
        markDirty();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Rows
// ═════════════════════════════════════════════════════════════════════════════

int DataGrid::addRow(const std::vector<std::string>& cells)
{
    Row r;
    r.cells = cells;
    // Pad with empty strings if fewer cells than columns
    while (r.cells.size() < columns_.size())
        r.cells.push_back("");
    rows_.push_back(std::move(r));
    rebuildSortOrder();
    markDirty();
    return static_cast<int>(rows_.size()) - 1;
}

void DataGrid::setCell(int row, int col, const std::string& value)
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) &&
        col >= 0 && col < static_cast<int>(columns_.size())) {
        rows_[row].cells[col] = value;
        markDirty();
    }
}

std::string DataGrid::cell(int row, int col) const
{
    if (row >= 0 && row < static_cast<int>(rows_.size()) &&
        col >= 0 && col < static_cast<int>(columns_.size()))
        return rows_[row].cells[col];
    return "";
}

void DataGrid::removeRow(int row)
{
    if (row >= 0 && row < static_cast<int>(rows_.size())) {
        rows_.erase(rows_.begin() + row);
        rebuildSortOrder();
        if (selectedRow_ == row) selectedRow_ = -1;
        else if (selectedRow_ > row) selectedRow_--;
        markDirty();
    }
}

void DataGrid::clearRows()
{
    rows_.clear();
    sortedIndices_.clear();
    selectedRow_ = -1;
    selectedRows_.clear();
    scrollY_ = 0;
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Checkboxes
// ═════════════════════════════════════════════════════════════════════════════

bool DataGrid::isRowChecked(int row) const
{
    if (row >= 0 && row < static_cast<int>(rows_.size()))
        return rows_[row].checked;
    return false;
}

void DataGrid::setRowChecked(int row, bool checked)
{
    if (row >= 0 && row < static_cast<int>(rows_.size())) {
        rows_[row].checked = checked;
        markDirty();
    }
}

std::vector<int> DataGrid::checkedRows() const
{
    std::vector<int> result;
    for (int i = 0; i < static_cast<int>(rows_.size()); ++i)
        if (rows_[i].checked) result.push_back(i);
    return result;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Selection
// ═════════════════════════════════════════════════════════════════════════════

void DataGrid::setSelectedRow(int row)
{
    selectedRow_ = row;
    selectedRows_.clear();
    if (row >= 0) selectedRows_.push_back(row);
    markDirty();
}

void DataGrid::clearSelection()
{
    selectedRow_ = -1;
    selectedRows_.clear();
    markDirty();
}

bool DataGrid::isSelected(int row) const
{
    if (!multiSelect_) return row == selectedRow_;
    for (int r : selectedRows_)
        if (r == row) return true;
    return false;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Sort
// ═════════════════════════════════════════════════════════════════════════════

void DataGrid::rebuildSortOrder()
{
    sortedIndices_.resize(rows_.size());
    std::iota(sortedIndices_.begin(), sortedIndices_.end(), 0);
    if (sortCol_ >= 0 && sortOrder_ != SortOrder::None)
        sortByColumn(sortCol_);
}

void DataGrid::sortByColumn(int col)
{
    if (col < 0 || col >= static_cast<int>(columns_.size())) return;
    sortCol_ = col;

    bool asc = (sortOrder_ == SortOrder::Ascending);
    std::stable_sort(sortedIndices_.begin(), sortedIndices_.end(),
        [&](int a, int b) {
            const std::string& sa = rows_[a].cells[col];
            const std::string& sb = rows_[b].cells[col];
            // Try numeric comparison first
            char* endA; char* endB;
            double da = strtod(sa.c_str(), &endA);
            double db = strtod(sb.c_str(), &endB);
            if (endA != sa.c_str() && endB != sb.c_str()) {
                return asc ? da < db : da > db;
            }
            return asc ? sa < sb : sa > sb;
        });
}

int DataGrid::displayRow(int idx) const
{
    if (idx >= 0 && idx < static_cast<int>(sortedIndices_.size()))
        return sortedIndices_[idx];
    return idx;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Geometry helpers
// ═════════════════════════════════════════════════════════════════════════════

float DataGrid::totalColumnsWidth() const
{
    float w = contentX();
    for (auto& c : columns_) w += c.width;
    return w;
}

float DataGrid::columnX(int col) const
{
    float x = contentX();
    for (int i = 0; i < col && i < static_cast<int>(columns_.size()); ++i)
        x += columns_[i].width;
    return x;
}

int DataGrid::hitColumn(float localX) const
{
    float x = contentX() - scrollX_;
    for (int i = 0; i < static_cast<int>(columns_.size()); ++i) {
        if (localX >= x && localX < x + columns_[i].width)
            return i;
        x += columns_[i].width;
    }
    return -1;
}

int DataGrid::hitRow(float localY) const
{
    if (localY < headerHeight_) return -1;
    float y = localY - headerHeight_ + scrollY_;
    int idx = static_cast<int>(y / rowHeight_);
    if (idx < 0 || idx >= static_cast<int>(sortedIndices_.size()))
        return -1;
    return idx;
}

int DataGrid::hitResizeEdge(float localX) const
{
    float x = contentX() - scrollX_;
    for (int i = 0; i < static_cast<int>(columns_.size()); ++i) {
        x += columns_[i].width;
        if (std::abs(localX - x) <= 4.0f)
            return i;
    }
    return -1;
}

Widget::Vec2f DataGrid::sizeHint() const
{
    float w = totalColumnsWidth();
    float h = headerHeight_ + rows_.size() * rowHeight_;
    return {w, h};
}

// ═════════════════════════════════════════════════════════════════════════════
//  Inline editing
// ═════════════════════════════════════════════════════════════════════════════

void DataGrid::setColumnReadOnly(int col, bool ro)
{
    if (col >= 0 && col < static_cast<int>(columns_.size()))
        columns_[col].readOnly = ro;
}

void DataGrid::startEdit(int row, int col)
{
    if (row < 0 || row >= static_cast<int>(rows_.size())) return;
    if (col < 0 || col >= static_cast<int>(columns_.size())) return;
    if (readOnly_ || columns_[col].readOnly) return;
    editing_ = true;
    editRow_ = row;
    editCol_ = col;
    editBuf_ = rows_[row].cells[col];
    editCursor_ = static_cast<int>(editBuf_.size());
    markDirty();
}

void DataGrid::commitEdit()
{
    if (!editing_) return;
    editing_ = false;
    if (editRow_ >= 0 && editRow_ < static_cast<int>(rows_.size()) &&
        editCol_ >= 0 && editCol_ < static_cast<int>(columns_.size())) {
        rows_[editRow_].cells[editCol_] = editBuf_;
        cellEdited.emit(editRow_, editCol_);
    }
    editRow_ = -1;
    editCol_ = -1;
    markDirty();
}

void DataGrid::cancelEdit()
{
    editing_ = false;
    editRow_ = -1;
    editCol_ = -1;
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Paint
// ═════════════════════════════════════════════════════════════════════════════

void DataGrid::paint(PaintContext& ctx)
{
    if (!visible_) return;
    Rect abs = absoluteRect();
    if (ctx.isClipped(abs)) return;

    const auto& t = Theme::instance();
    float totalW = totalColumnsWidth();

    // Background
    ctx.fill.SetColor(t.panelColor.r, t.panelColor.g, t.panelColor.b, t.panelColor.a);
    ctx.fillRect(abs.x, abs.y, abs.w, abs.h);

    ctx.font.SetFontSize(t.fontSize);
    ctx.font.SetBatch(&ctx.text);

    // ── Column headers ───────────────────────────────────────────────────
    Rect headerClip = {abs.x, abs.y, abs.w, headerHeight_};
    ctx.pushClip(headerClip);
    {
        // Header background
        Color hdrBg(t.panelColor.r + 15, t.panelColor.g + 15, t.panelColor.b + 15, 255);
        ctx.fill.SetColor(hdrBg.r, hdrBg.g, hdrBg.b, hdrBg.a);
        ctx.fillRect(abs.x, abs.y, abs.w, headerHeight_);

        float hx = abs.x + contentX() - scrollX_;
        auto fc = toFontClipDG(ctx.clipRect());
        for (int i = 0; i < static_cast<int>(columns_.size()); ++i) {
            auto& col = columns_[i];
            float colR = hx + col.width;

            // Header text
            ctx.font.SetColor(t.textColor);
            ctx.font.Print(col.name.c_str(), hx + 4, abs.y + (headerHeight_ - t.fontSize) * 0.5f, &fc);

            // Sort indicator
            if (sortCol_ == i && sortOrder_ != SortOrder::None) {
                float arrowX = colR - 14.0f;
                float arrowY = abs.y + headerHeight_ * 0.5f;
                ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, t.textColor.a);
                if (sortOrder_ == SortOrder::Ascending) {
                    ctx.fillTriangle(arrowX, arrowY + 3, arrowX + 6, arrowY + 3, arrowX + 3, arrowY - 3);
                } else {
                    ctx.fillTriangle(arrowX, arrowY - 3, arrowX + 6, arrowY - 3, arrowX + 3, arrowY + 3);
                }
            }

            // Column separator
            ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, 80);
            ctx.drawLine(colR, abs.y + 2, colR, abs.y + headerHeight_ - 2);

            hx = colR;
        }

        // Checkbox header
        if (showCheckboxes_) {
            ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, 80);
            ctx.drawLine(abs.x + checkboxColW_, abs.y + 2, abs.x + checkboxColW_, abs.y + headerHeight_ - 2);
        }

        // Header bottom line
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
        int totalRows = static_cast<int>(sortedIndices_.size());
        int firstVis = static_cast<int>(scrollY_ / rowHeight_);
        int lastVis = firstVis + static_cast<int>(bodyH / rowHeight_) + 2;
        if (firstVis < 0) firstVis = 0;
        if (lastVis > totalRows) lastVis = totalRows;

        auto fc = toFontClipDG(ctx.clipRect());

        for (int vi = firstVis; vi < lastVis; ++vi) {
            int dataIdx = displayRow(vi);
            if (dataIdx < 0 || dataIdx >= static_cast<int>(rows_.size())) continue;
            auto& row = rows_[dataIdx];

            float ry = bodyY + vi * rowHeight_ - scrollY_;
            Rect rowR = {abs.x, ry, abs.w, rowHeight_};
            if (ctx.isClipped(rowR)) continue;

            // Zebra stripes
            if (zebraStripes_ && (vi % 2 == 1)) {
                ctx.fill.SetColor(t.panelColor.r + 5, t.panelColor.g + 5, t.panelColor.b + 5, 255);
                ctx.fillRect(abs.x, ry, abs.w, rowHeight_);
            }

            // Selection highlight
            if (isSelected(dataIdx)) {
                ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g, t.selectionColor.b, t.selectionColor.a);
                ctx.fillRect(abs.x, ry, abs.w, rowHeight_);
            }

            // Hover highlight
            if (hoveredRow_ == vi && !isSelected(dataIdx)) {
                ctx.fill.SetColor(t.buttonHover.r, t.buttonHover.g, t.buttonHover.b, 60);
                ctx.fillRect(abs.x, ry, abs.w, rowHeight_);
            }

            float textY = ry + (rowHeight_ - t.fontSize) * 0.5f;

            // Checkbox
            if (showCheckboxes_) {
                float cbX = abs.x + 6.0f;
                float cbY = ry + (rowHeight_ - 14.0f) * 0.5f;
                float cbS = 14.0f;
                ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, t.inputBg.a);
                ctx.fillRect(cbX, cbY, cbS, cbS);
                ctx.line.SetColor(t.inputBorder.r, t.inputBorder.g, t.inputBorder.b, t.inputBorder.a);
                ctx.lineRect(cbX, cbY, cbS, cbS);
                if (row.checked) {
                    ctx.line.SetColor(t.checkMark.r, t.checkMark.g, t.checkMark.b, t.checkMark.a);
                    ctx.drawLine(cbX + 3, cbY + cbS * 0.5f, cbX + cbS * 0.4f, cbY + cbS - 3);
                    ctx.drawLine(cbX + cbS * 0.4f, cbY + cbS - 3, cbX + cbS - 3, cbY + 3);
                }
            }

            // Cell text
            float cx = abs.x + contentX() - scrollX_;
            for (int ci = 0; ci < static_cast<int>(columns_.size()); ++ci) {
                float colW = columns_[ci].width;

                // Inline edit mode for this cell?
                if (editing_ && editRow_ == dataIdx && editCol_ == ci) {
                    // Edit background
                    ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, 255);
                    ctx.fillRect(cx + 1, ry + 1, colW - 2, rowHeight_ - 2);
                    ctx.line.SetColor(t.inputBorderHover.r, t.inputBorderHover.g, t.inputBorderHover.b, 255);
                    ctx.lineRect(cx + 1, ry + 1, colW - 2, rowHeight_ - 2);

                    // Edit text
                    ctx.font.SetColor(Color(255, 255, 255, 255));
                    ctx.font.Print(editBuf_.c_str(), cx + 4, textY, &fc);

                    // Cursor
                    std::string beforeCursor = editBuf_.substr(0, editCursor_);
                    float cursorX = cx + 4 + ctx.font.GetTextWidth(beforeCursor.c_str());
                    ctx.line.SetColor(255, 255, 255, 200);
                    ctx.drawLine(cursorX, ry + 3, cursorX, ry + rowHeight_ - 3);
                } else {
                    // Normal cell text
                    if (ci < static_cast<int>(row.cells.size())) {
                        ctx.font.SetColor(isSelected(dataIdx) ? Color(255, 255, 255, 255) : t.textColor);
                        ctx.font.Print(row.cells[ci].c_str(), cx + 4, textY, &fc);
                    }
                }

                // Column separator in body
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
    float contentH = static_cast<float>(rows_.size()) * rowHeight_;
    float contentW = totalColumnsWidth();

    // Vertical scrollbar
    if (contentH > bodyH) {
        float sbW = 8.0f;
        float sbX = abs.x + abs.w - sbW;
        float trackH = bodyH;
        float thumbH = std::max(20.0f, (bodyH / contentH) * trackH);
        float maxSY = contentH - bodyH;
        float thumbY = bodyY + (scrollY_ / maxSY) * (trackH - thumbH);

        ctx.fill.SetColor(t.panelColor.r + 20, t.panelColor.g + 20, t.panelColor.b + 20, 150);
        ctx.fillRect(sbX, bodyY, sbW, trackH);
        ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, 80);
        ctx.fillRect(sbX + 1, thumbY, sbW - 2, thumbH);
    }

    // Horizontal scrollbar
    if (contentW > abs.w) {
        float sbH = 8.0f;
        float sbY = abs.y + abs.h - sbH;
        float trackW = abs.w;
        float thumbW = std::max(20.0f, (abs.w / contentW) * trackW);
        float maxSX = contentW - abs.w;
        float thumbX = abs.x + (scrollX_ / maxSX) * (trackW - thumbW);

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

void DataGrid::onMousePress(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y)) return;

    e.consumed = true;
    float localX = e.x - abs.x;
    float localY = e.y - abs.y;

    // ── Header area ──────────────────────────────────────────────────────
    if (localY < headerHeight_) {
        // Check for resize edge
        int resEdge = hitResizeEdge(localX);
        if (resEdge >= 0) {
            resizeCol_ = resEdge;
            resizeStartX_ = e.x;
            resizeStartW_ = columns_[resEdge].width;
            return;
        }

        // Header click → sort
        int col = hitColumn(localX);
        if (col >= 0 && columns_[col].sortable) {
            if (sortCol_ == col) {
                // Cycle: None → Asc → Desc → None
                if (sortOrder_ == SortOrder::None)
                    sortOrder_ = SortOrder::Ascending;
                else if (sortOrder_ == SortOrder::Ascending)
                    sortOrder_ = SortOrder::Descending;
                else
                    sortOrder_ = SortOrder::None;
            } else {
                sortCol_ = col;
                sortOrder_ = SortOrder::Ascending;
            }
            rebuildSortOrder();
            columnSorted.emit(col);
            markDirty();
        }
        return;
    }

    // ── Body area ────────────────────────────────────────────────────────
    int visIdx = hitRow(localY);
    if (visIdx < 0) return;
    int dataIdx = displayRow(visIdx);
    if (dataIdx < 0 || dataIdx >= static_cast<int>(rows_.size())) return;

    // Checkbox click?
    if (showCheckboxes_ && localX < checkboxColW_) {
        rows_[dataIdx].checked = !rows_[dataIdx].checked;
        rowCheckChanged.emit(dataIdx, rows_[dataIdx].checked);
        markDirty();
        return;
    }

    // Double-click → inline edit
    // (SDL sends single clicks; we detect double-click by timestamp)
    static Uint32 lastClickTime = 0;
    static int lastClickRow = -1;
    Uint32 now = SDL_GetTicks();
    int col = hitColumn(localX);

    if (dataIdx == lastClickRow && (now - lastClickTime) < 400 && col >= 0) {
        startEdit(dataIdx, col);
        lastClickTime = 0;
        lastClickRow = -1;
        return;
    }
    lastClickTime = now;
    lastClickRow = dataIdx;

    // Commit any pending edit
    if (editing_) commitEdit();

    // Selection
    if (multiSelect_) {
        bool ctrl = (SDL_GetModState() & KMOD_CTRL) != 0;
        bool shift = (SDL_GetModState() & KMOD_SHIFT) != 0;

        if (ctrl) {
            // Toggle this row in selection
            auto it = std::find(selectedRows_.begin(), selectedRows_.end(), dataIdx);
            if (it != selectedRows_.end())
                selectedRows_.erase(it);
            else
                selectedRows_.push_back(dataIdx);
            selectedRow_ = dataIdx;
        } else if (shift && selectedRow_ >= 0) {
            // Range select from selectedRow_ to dataIdx
            selectedRows_.clear();
            int lo = std::min(selectedRow_, dataIdx);
            int hi = std::max(selectedRow_, dataIdx);
            for (int i = lo; i <= hi; ++i)
                selectedRows_.push_back(i);
        } else {
            // Single select
            selectedRow_ = dataIdx;
            selectedRows_ = {dataIdx};
        }
    } else {
        selectedRow_ = dataIdx;
        selectedRows_ = {dataIdx};
    }
    selectionChanged.emit(dataIdx);
    markDirty();
}

void DataGrid::onMouseRelease(MouseEvent& e)
{
    if (resizeCol_ >= 0) {
        resizeCol_ = -1;
    }
    (void)e;
}

void DataGrid::onMouseMove(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();

    // Column resize drag
    if (resizeCol_ >= 0) {
        float dx = e.x - resizeStartX_;
        float newW = std::max(columns_[resizeCol_].minWidth, resizeStartW_ + dx);
        columns_[resizeCol_].width = newW;
        markDirty();
        return;
    }

    float localX = e.x - abs.x;
    float localY = e.y - abs.y;

    // Resize cursor hint
    if (localY < headerHeight_) {
        int edge = hitResizeEdge(localX);
        (void)edge; // cursor change would go here
    }

    // Row hover
    int visIdx = hitRow(localY);
    int col = hitColumn(localX);
    if (visIdx != hoveredRow_ || col != hoveredCol_) {
        hoveredRow_ = visIdx;
        hoveredCol_ = col;
        markDirty();
    }
}

void DataGrid::onMouseScroll(MouseEvent& e)
{
    if (!visible_ || !enabled_) return;
    Rect abs = absoluteRect();
    if (!abs.contains(e.x, e.y)) return;

    e.consumed = true;
    bool shift = (SDL_GetModState() & KMOD_SHIFT) != 0;
    float bodyH = abs.h - headerHeight_;
    float contentH = static_cast<float>(rows_.size()) * rowHeight_;
    float contentW = totalColumnsWidth();

    if (shift) {
        // Horizontal scroll
        scrollX_ -= e.scrollY * 40.0f;
        scrollX_ = Clamp(scrollX_, 0.0f, std::max(0.0f, contentW - abs.w));
    } else {
        // Vertical scroll
        scrollY_ -= e.scrollY * rowHeight_ * 2.0f;
        scrollY_ = Clamp(scrollY_, 0.0f, std::max(0.0f, contentH - bodyH));
    }
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Keyboard events
// ═════════════════════════════════════════════════════════════════════════════

void DataGrid::onKeyPress(KeyEvent& e)
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
        case SDLK_LEFT:
            if (editCursor_ > 0) { editCursor_--; markDirty(); }
            return;
        case SDLK_RIGHT:
            if (editCursor_ < static_cast<int>(editBuf_.size())) { editCursor_++; markDirty(); }
            return;
        case SDLK_HOME:
            editCursor_ = 0; markDirty();
            return;
        case SDLK_END:
            editCursor_ = static_cast<int>(editBuf_.size()); markDirty();
            return;
        case SDLK_TAB: {
            // Tab → move to next cell
            commitEdit();
            int nextCol = editCol_ + 1;
            int nextRow = editRow_;
            if (nextCol >= static_cast<int>(columns_.size())) {
                nextCol = 0;
                nextRow++;
            }
            if (nextRow < static_cast<int>(rows_.size()))
                startEdit(nextRow, nextCol);
            return;
        }
        case SDLK_c:
            if (e.ctrl) {
                // Copy edit buffer
                SDL_SetClipboardText(editBuf_.c_str());
                return;
            }
            break;
        case SDLK_v:
            if (e.ctrl) {
                char* clip = SDL_GetClipboardText();
                if (clip) {
                    editBuf_.insert(editCursor_, clip);
                    editCursor_ += static_cast<int>(strlen(clip));
                    SDL_free(clip);
                    markDirty();
                }
                return;
            }
            break;
        case SDLK_a:
            if (e.ctrl) {
                // Select all text in edit buffer
                editCursor_ = static_cast<int>(editBuf_.size());
                markDirty();
                return;
            }
            break;
        default: break;
        }
    }

    // ── Ctrl shortcuts (not editing) ─────────────────────────────────────
    if (!editing_ && e.ctrl) {
        if (e.key == SDLK_c) {
            // Copy selected rows as tab-separated text
            e.consumed = true;
            std::string text;
            // Header
            for (int c = 0; c < static_cast<int>(columns_.size()); ++c) {
                if (c > 0) text += '\t';
                text += columns_[c].name;
            }
            text += '\n';
            // Rows
            const auto& sel = selectedRows_;
            for (int ri : sel) {
                if (ri < 0 || ri >= static_cast<int>(rows_.size())) continue;
                for (int c = 0; c < static_cast<int>(columns_.size()); ++c) {
                    if (c > 0) text += '\t';
                    if (c < static_cast<int>(rows_[ri].cells.size()))
                        text += rows_[ri].cells[c];
                }
                text += '\n';
            }
            if (!text.empty())
                SDL_SetClipboardText(text.c_str());
            return;
        }
        if (e.key == SDLK_a) {
            // Select all rows
            e.consumed = true;
            selectedRows_.clear();
            for (int i = 0; i < static_cast<int>(rows_.size()); ++i)
                selectedRows_.push_back(i);
            if (!rows_.empty()) selectedRow_ = 0;
            markDirty();
            return;
        }
    }

    // Navigation (not editing)
    if (!editing_) {
        if (e.key == SDLK_UP) {
            e.consumed = true;
            if (selectedRow_ > 0) {
                setSelectedRow(selectedRow_ - 1);
                selectionChanged.emit(selectedRow_);
            }
        } else if (e.key == SDLK_DOWN) {
            e.consumed = true;
            if (selectedRow_ < static_cast<int>(rows_.size()) - 1) {
                setSelectedRow(selectedRow_ + 1);
                selectionChanged.emit(selectedRow_);
            }
        } else if (e.key == SDLK_RETURN || e.key == SDLK_KP_ENTER) {
            e.consumed = true;
            if (selectedRow_ >= 0)
                startEdit(selectedRow_, 0);
        } else if (e.key == SDLK_SPACE && showCheckboxes_) {
            e.consumed = true;
            if (selectedRow_ >= 0 && selectedRow_ < static_cast<int>(rows_.size())) {
                rows_[selectedRow_].checked = !rows_[selectedRow_].checked;
                rowCheckChanged.emit(selectedRow_, rows_[selectedRow_].checked);
                markDirty();
            }
        }
    }
}

void DataGrid::onTextInput(KeyEvent& e)
{
    if (!editing_) return;
    e.consumed = true;
    editBuf_.insert(editCursor_, e.text);
    editCursor_ += static_cast<int>(strlen(e.text));
    markDirty();
}
