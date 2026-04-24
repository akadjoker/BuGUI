#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include <functional>

// ═════════════════════════════════════════════════════════════════════════════
//  DataGrid - spreadsheet/database-style table widget
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

// NOTE: TreeGrid uses DataGrid::SortOrder  both in same header
    std::vector<Row>    rows_;

    // Sort state
    int       sortCol_   = -1;
    SortOrder sortOrder_ = SortOrder::None;
    std::vector<int> sortedIndices_;  // row display order

    // Selection
    int selectedRow_ = -1;
    std::vector<int> selectedRows_;
    bool multiSelect_ = false;

    // Appearance
    float rowHeight_    = 24.0f;
    float headerHeight_ = 26.0f;
    bool  zebraStripes_ = true;
    bool  showCheckboxes_ = false;
    float checkboxColW_   = 28.0f;
    bool  readOnly_       = false;

    // Scroll
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;

    // Column resize
    int   resizeCol_    = -1;   // column being resized (-1 = none)
    float resizeStartX_ = 0;
    float resizeStartW_ = 0;

    // Hover
    int hoveredRow_ = -1;
    int hoveredCol_ = -1;

    // Inline editing
    bool editing_     = false;
    int  editRow_     = -1;
    int  editCol_     = -1;
    std::string editBuf_;
    int  editCursor_  = 0;

    // Helpers
    float totalColumnsWidth() const;
    float contentX() const { return showCheckboxes_ ? checkboxColW_ : 0.0f; }
    float columnX(int col) const;  // X offset of column (before scroll)
    int   hitColumn(float localX) const;
    int   hitRow(float localY) const;
    int   hitResizeEdge(float localX) const;  // returns col index or -1
    int   displayRow(int idx) const;  // maps sorted index to data index
    void  rebuildSortOrder();
    void  sortByColumn(int col);
    void  startEdit(int row, int col);
    void  commitEdit();
    void  cancelEdit();
    bool  isSelected(int row) const;
};

// ═════════════════════════════════════════════════════════════════════════════
//  TreeGrid - hierarchical table (tree + columns)
//    auto* tg = parent->createChild<TreeGrid>();
//    tg->addColumn("Name",  250);
//    tg->addColumn("Type",   80);
//    tg->addColumn("Size",   80);
//    auto* root = tg->addRoot({"src", "Folder", "12 KB"});
//    root->addChild({"main.cpp", "C++", "3 KB"});
//
//  Features:
//    - Expandable/collapsible tree nodes
//    - Sortable columns (click header)
//    - Drag-resizable columns
//    - Single/multi select
//    - Inline cell editing (double-click)
//    - Zebra stripes
//    - Icon support (via IconAtlas)
//    - Vertical + horizontal scroll
// ═════════════════════════════════════════════════════════════════════════════

class TreeGrid : public Widget
{
public:
    TreeGrid();
    ~TreeGrid() override;

    // ── Column definition ────────────────────────────────────────────────
    struct Column {
        std::string name;
        float width = 100.0f;
        float minWidth = 40.0f;
        bool  sortable = true;
        bool  readOnly = false;
    };

    int  addColumn(const std::string& name, float width = 100.0f, bool sortable = true);
    int  columnCount() const { return static_cast<int>(columns_.size()); }

    // ── Tree nodes ───────────────────────────────────────────────────────
    struct Node {
        std::vector<std::string> cells;  // one per column
        std::vector<Node*> children;
        Node* parent = nullptr;
        bool expanded = true;
        int depth = 0;
        IconId iconId = IconId::None;
        bool readOnly = false;

        Node* addChild(const std::vector<std::string>& cellData);
        void setIcon(IconId id) { iconId = id; }
        void setReadOnly(bool ro) { readOnly = ro; }
    };

    Node* addRoot(const std::vector<std::string>& cells);
    void clearAll();
    int nodeCount() const;  // total visible + hidden

    // ── Selection ────────────────────────────────────────────────────────
    void setMultiSelect(bool m) { multiSelect_ = m; }
    bool multiSelect() const { return multiSelect_; }
    Node* selectedNode() const { return selectedNode_; }
    void clearSelection();

    // ── Appearance ───────────────────────────────────────────────────────
    void  setRowHeight(float h) { rowHeight_ = h; markDirty(); }
    float rowHeight() const { return rowHeight_; }
    void  setHeaderHeight(float h) { headerHeight_ = h; markDirty(); }
    float headerHeight() const { return headerHeight_; }
    void  setZebraStripes(bool z) { zebraStripes_ = z; markDirty(); }
    bool  zebraStripes() const { return zebraStripes_; }
    void  setIndentWidth(float w) { indentWidth_ = w; markDirty(); }
    float indentWidth() const { return indentWidth_; }
    void  setReadOnly(bool ro) { readOnly_ = ro; }
    bool  readOnly() const { return readOnly_; }
    void  setColumnReadOnly(int col, bool ro);

    // ── Signals ──────────────────────────────────────────────────────────
    Signal<Node*>       selectionChanged;
    Signal<Node*, int>  cellEdited;       // node, col
    Signal<int>         columnSorted;     // col
    Signal<Node*>       nodeExpanded;
    Signal<Node*>       nodeCollapsed;

    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;
    void onKeyPress(KeyEvent& e) override;
    void onTextInput(KeyEvent& e) override;

private:
    std::vector<Column> columns_;
    std::vector<Node*> roots_;

    // Flat list of visible nodes (rebuilt on expand/collapse)
    struct FlatEntry { Node* node; int depth; };
    std::vector<FlatEntry> flatList_;
    void rebuildFlatList();
    void flattenNode(Node* n, int depth);

    // Sort
    int sortCol_ = -1;
    DataGrid::SortOrder sortOrder_ = DataGrid::SortOrder::None;
    void sortChildren(Node* parent);

    // Selection
    Node* selectedNode_ = nullptr;
    std::vector<Node*> selectedNodes_;
    bool multiSelect_ = false;

    // Appearance
    float rowHeight_    = 24.0f;
    float headerHeight_ = 26.0f;
    bool  zebraStripes_ = true;
    float indentWidth_  = 20.0f;
    bool  readOnly_     = false;

    // Scroll
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;

    // Column resize
    int   resizeCol_    = -1;
    float resizeStartX_ = 0;
    float resizeStartW_ = 0;

    // Hover
    int hoveredRow_ = -1;

    // Inline editing
    bool editing_     = false;
    Node* editNode_   = nullptr;
    int  editCol_     = -1;
    std::string editBuf_;
    int  editCursor_  = 0;

    // Helpers
    float totalColumnsWidth() const;
    float columnX(int col) const;
    int   hitColumn(float localX) const;
    int   hitRow(float localY) const;
    int   hitResizeEdge(float localX) const;
    void  startEdit(Node* node, int col);
    void  commitEdit();
    void  cancelEdit();
    bool  isSelected(Node* n) const;

    // Cleanup
    void deleteNodes(Node* n);
};

