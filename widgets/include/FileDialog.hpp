#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include "ViewWidgets.hpp"
#include <functional>
#include <set>

struct Texture;
class BoxLayout;
class Label;
class TextInput;
class Button;
class ImageButton;

// ═════════════════════════════════════════════════════════════════════════════
//  FileDialog - cross-platform file/folder picker as a FloatWindow popup
//
//    auto* fd = app.addFloat<FileDialog>("Open File");
//    fd->setMode(FileDialog::Mode::Open);
//    fd->setFilter("*.cpp;*.hpp");
//    fd->accepted.connect([](const std::string& path) { ... });
//
//  Features:
//    - Open / Save / SelectFolder modes
//    - Breadcrumb path navigation
//    - Sort by name / size / date (click headers)
//    - Hidden files toggle
//    - File extension filter
//    - Bookmarks sidebar (Home, Desktop, /, custom)
//    - Recent files list
//    - Icons from IconAtlas
// ═════════════════════════════════════════════════════════════════════════════


class FileDialog : public FloatWindow
{
public:
    enum class Mode { Open, Save, SelectFolder, OpenImage, SaveImage };
    enum class ViewMode { Detail, List, Icon };

    explicit FileDialog(const std::string& title = "Open File");
    ~FileDialog() override;

    // ── Configuration ────────────────────────────────────────────────────
    void setMode(Mode m);
    Mode mode() const { return mode_; }

    void setViewMode(ViewMode vm);
    ViewMode viewMode() const { return viewMode_; }

    void setPath(const std::string& path);
    std::string path() const { return currentPath_; }

    // Filter: semicolon-separated extensions, e.g. "*.cpp;*.hpp;*.h"
    void setFilter(const std::string& filter);
    std::string filter() const { return filter_; }

    void setShowHidden(bool show);
    bool showHidden() const { return showHidden_; }

    void setMultiSelect(bool ms);
    bool multiSelect() const { return multiSelect_; }

    // ── Bookmarks ────────────────────────────────────────────────────────
    void addBookmark(const std::string& name, const std::string& path);
    void clearBookmarks();

    // ── Recent files ─────────────────────────────────────────────────────
    void addRecent(const std::string& path);
    void clearRecent();

    // ── Result ───────────────────────────────────────────────────────────
    std::string selectedPath() const { return selectedPath_; }
    std::vector<std::string> selectedPaths() const;

    // ── Signals ──────────────────────────────────────────────────────────
    Signal<std::string>              accepted;       // single path chosen
    Signal<std::vector<std::string>> acceptedMulti;  // multiple paths chosen
    Signal<>                         cancelled;

private:
    Mode mode_ = Mode::Open;
    ViewMode viewMode_ = ViewMode::Detail;
    std::string currentPath_;
    std::string selectedPath_;
    std::string filter_;
    bool showHidden_ = false;
    bool multiSelect_ = false;

    // Bookmarks
    struct Bookmark { std::string name; std::string path; };
    std::vector<Bookmark> bookmarks_;

    // Recent files
    std::vector<std::string> recentFiles_;
    static constexpr int kMaxRecent = 20;

    // Sort
    enum class SortField { Name, Size, Date };
    SortField sortField_ = SortField::Name;
    bool sortAscending_ = true;

    // Parsed filter extensions
    std::vector<std::string> filterExts_;
    void parseFilter();
    bool matchesFilter(const std::string& name) const;
    static bool isImageExtension(const std::string& ext);

    // Directory contents (cached)
    struct FileEntry {
        std::string name;
        std::string path;
        bool isDir = false;
        uint64_t size = 0;
        time_t mtime = 0;
        bool hidden = false;
        IconId icon = IconId::None;
        bool selected = false;   // for multi-select
    };
    std::vector<FileEntry> entries_;
    void refreshDir();
    void sortEntries();
    void navigateTo(const std::string& path);

    // Multi-select helpers
    std::set<int> selectedIndices_;
    bool isSelected(int idx) const;
    void toggleSelect(int idx);
    void selectRange(int from, int to);
    int  lastClickedIndex_ = -1;  // for shift-click range

    // Preview (for image modes)
    Texture* previewTex_ = nullptr;
    std::string previewPath_;
    void loadPreview(const std::string& path);
    void clearPreview();

    // ── Child widget pointers (not owned, children_ owns them) ───────
    BoxLayout*  mainLayout_     = nullptr;
    BoxLayout*  toolBar_        = nullptr;   // breadcrumbs + view mode buttons
    BoxLayout*  breadcrumbBar_  = nullptr;
    BoxLayout*  bodyLayout_     = nullptr;
    BoxLayout*  sidebarLayout_  = nullptr;
    BoxLayout*  fileListLayout_ = nullptr;
    Widget*     previewLayout_  = nullptr;   // image preview panel (plain Widget for sizeHint)
    BoxLayout*  bottomBar_      = nullptr;
    TextInput*  fileNameInput_  = nullptr;
    Label*      fileLabel_      = nullptr;
    Button*      okButton_       = nullptr;
    Button*      cancelButton_   = nullptr;
    Button*      hiddenToggle_   = nullptr;
    ImageButton* viewDetailBtn_  = nullptr;
    ImageButton* viewListBtn_    = nullptr;
    ImageButton* viewIconBtn_    = nullptr;
    Label*      statusLabel_    = nullptr;
    Widget*     bottomSpacer_   = nullptr;  // stretch spacer for SelectFolder mode

    // File list state
    int  selectedIndex_ = -1;
    int  hoveredIndex_  = -1;
    float scrollY_      = 0.0f;
    float rowHeight_    = 22.0f;
    float iconCellSize_ = 80.0f;  // for icon view

    // Deferred navigation (avoid destroying widgets mid-signal)
    std::string pendingNav_;

    // File list header sort
    struct ListColumn { std::string name; float width; SortField field; };
    std::vector<ListColumn> listColumns_;

    // Build UI tree
    void buildUI();
    void buildBreadcrumbs();
    void buildSidebar();
    void updateViewButtons();

    // Paint helpers
    void paintFileList(PaintContext& ctx, const Rect& area);
    void paintFileListHeader(PaintContext& ctx, const Rect& area);
    void paintFileListDetail(PaintContext& ctx, const Rect& area);
    void paintFileListCompact(PaintContext& ctx, const Rect& area);
    void paintFileListIcons(PaintContext& ctx, const Rect& area);
    void paintPreview(PaintContext& ctx, const Rect& area);

    // Events for file list area
    void fileListMousePress(float localX, float localY, bool dblClick, bool ctrl, bool shift);
    void fileListMouseMove(float localX, float localY);
    void fileListScroll(float dy);

    // Actions
    void onOk();
    void onCancel();
    void onBookmarkClick(int idx);
    void onRecentClick(int idx);

    // Overrides
    void layout() override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;
    void onKeyPress(KeyEvent& e) override;
};

