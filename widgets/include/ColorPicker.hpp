#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>

{
public:
    DataGrid();

    // ── Column definition ────────────────────────────────────────────────
    struct Column {
        std::string name;
        float width = 100.0f;
        float minWidth = 40.0f;
        bool  sortable = true;
        bool  readOnly = false;
    };

    enum class SortOrder { None, Ascending, Descending };

    int  addColumn(const std::string& name, float width = 100.0f, bool sortable = true);
    int  columnCount() const { return static_cast<int>(columns_.size()); }
    void setColumnWidth(int col, float w);

    // ── Row data ─────────────────────────────────────────────────────────
    int  addRow(const std::vector<std::string>& cells);
    void setCell(int row, int col, const std::string& value);
    std::string cell(int row, int col) const;
    void removeRow(int row);
    void clearRows();
    int  rowCount() const { return static_cast<int>(rows_.size()); }

    // ── Checkbox column ──────────────────────────────────────────────────
    void setShowCheckboxes(bool s) { showCheckboxes_ = s; markDirty(); }
    bool showCheckboxes() const { return showCheckboxes_; }
    bool isRowChecked(int row) const;
    void setRowChecked(int row, bool checked);
    std::vector<int> checkedRows() const;

    // ── Selection ────────────────────────────────────────────────────────
    void setMultiSelect(bool m) { multiSelect_ = m; }
    bool multiSelect() const { return multiSelect_; }
    int  selectedRow() const { return selectedRow_; }
    const std::vector<int>& selectedRows() const { return selectedRows_; }
    void setSelectedRow(int row);
    void clearSelection();

    // ── Appearance ───────────────────────────────────────────────────────
    void  setRowHeight(float h) { rowHeight_ = h; markDirty(); }
    float rowHeight() const { return rowHeight_; }
    void  setHeaderHeight(float h) { headerHeight_ = h; markDirty(); }
    float headerHeight() const { return headerHeight_; }
    void  setZebraStripes(bool z) { zebraStripes_ = z; markDirty(); }
    bool  zebraStripes() const { return zebraStripes_; }
    void  setReadOnly(bool ro) { readOnly_ = ro; }
    bool  readOnly() const { return readOnly_; }
    void  setColumnReadOnly(int col, bool ro);

    // ── Signals ──────────────────────────────────────────────────────────
    Signal<int>       selectionChanged;   // row index
    Signal<int, int>  cellEdited;         // row, col
    Signal<int>       columnSorted;       // col index
    Signal<int, bool> rowCheckChanged;    // row, checked

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;
    void onKeyPress(KeyEvent& e) override;
    void onTextInput(KeyEvent& e) override;

private:
    struct Row {
        std::vector<std::string> cells;
        bool checked = false;
    };

    std::vector<Column> columns_;
