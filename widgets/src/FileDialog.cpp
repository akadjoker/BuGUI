#include "Widgets.hpp"
#include "WidgetApp.hpp"
#include "IconAtlas.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include "Texture.hpp"
#include "Pixmap.hpp"
#include "FileSystem.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

namespace {
    inline Font::ClipRect toFontClipFD(const Rect& r)
    { return {r.x, r.y, r.w, r.h}; }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Constructor / Destructor
// ═════════════════════════════════════════════════════════════════════════════

FileDialog::FileDialog(const std::string& title)
    : FloatWindow(title)
{
    setFloatSize(700, 480);
    setResizable(true);
    setMinimizable(false);

    listColumns_ = {
        {"Name", 300, SortField::Name},
        {"Size",  90, SortField::Size},
        {"Modified", 150, SortField::Date},
    };

    bookmarks_.push_back({"Home", FileSystem::homePath()});
#if !defined(_WIN32)
    bookmarks_.push_back({"/", "/"});
#endif

    currentPath_ = FileSystem::homePath();

    buildUI();
    refreshDir();
}

FileDialog::~FileDialog()
{
    clearPreview();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Configuration
// ═════════════════════════════════════════════════════════════════════════════

void FileDialog::setMode(Mode m)
{
    mode_ = m;
    if (okButton_) {
        switch (m) {
        case Mode::Open:         okButton_->setText("Open"); break;
        case Mode::Save:         okButton_->setText("Save"); break;
        case Mode::SelectFolder: okButton_->setText("Select"); break;
        case Mode::OpenImage:    okButton_->setText("Open"); break;
        case Mode::SaveImage:    okButton_->setText("Save"); break;
        }
    }
    bool hideFile = (m == Mode::SelectFolder);
    if (fileNameInput_) fileNameInput_->setVisible(!hideFile);
    if (fileLabel_)     fileLabel_->setVisible(!hideFile);
    if (hiddenToggle_)  hiddenToggle_->setVisible(!hideFile);
    if (bottomSpacer_)  bottomSpacer_->setVisible(hideFile);

    bool imageMode = (m == Mode::OpenImage || m == Mode::SaveImage);
    if (previewLayout_) previewLayout_->setVisible(imageMode);
    if (imageMode) {
        setFloatSize(850, 520);
        if (filter_.empty())
            setFilter("*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif;*.psd;*.hdr");
        if (viewMode_ == ViewMode::Detail)
            setViewMode(ViewMode::Icon);
    }

    refreshDir();
}

void FileDialog::setViewMode(ViewMode vm)
{
    viewMode_ = vm;
    scrollY_ = 0;
    updateViewButtons();
    markDirty();
}

void FileDialog::setPath(const std::string& path)
{
    navigateTo(path);
}

void FileDialog::setFilter(const std::string& f)
{
    filter_ = f;
    parseFilter();
    refreshDir();
}

void FileDialog::setShowHidden(bool show)
{
    showHidden_ = show;
    if (hiddenToggle_)
        hiddenToggle_->setText(show ? "H*" : "H");
    refreshDir();
}

void FileDialog::setMultiSelect(bool ms)
{
    multiSelect_ = ms;
    if (!ms) {
        selectedIndices_.clear();
        markDirty();
    }
}

void FileDialog::addBookmark(const std::string& name, const std::string& path)
{
    bookmarks_.push_back({name, path});
    buildSidebar();
    markDirty();
}

void FileDialog::clearBookmarks()
{
    bookmarks_.clear();
    buildSidebar();
    markDirty();
}

void FileDialog::addRecent(const std::string& path)
{
    auto it = std::find(recentFiles_.begin(), recentFiles_.end(), path);
    if (it != recentFiles_.end()) recentFiles_.erase(it);
    recentFiles_.insert(recentFiles_.begin(), path);
    if (static_cast<int>(recentFiles_.size()) > kMaxRecent)
        recentFiles_.resize(kMaxRecent);
}

void FileDialog::clearRecent()
{
    recentFiles_.clear();
}

std::vector<std::string> FileDialog::selectedPaths() const
{
    std::vector<std::string> result;
    if (multiSelect_) {
        for (int idx : selectedIndices_) {
            if (idx >= 0 && idx < static_cast<int>(entries_.size()))
                result.push_back(entries_[idx].path);
        }
    } else if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(entries_.size())) {
        result.push_back(entries_[selectedIndex_].path);
    }
    return result;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Filter
// ═════════════════════════════════════════════════════════════════════════════

void FileDialog::parseFilter()
{
    filterExts_.clear();
    if (filter_.empty()) return;
    size_t pos = 0;
    while (pos < filter_.size()) {
        size_t semi = filter_.find(';', pos);
        if (semi == std::string::npos) semi = filter_.size();
        std::string token = filter_.substr(pos, semi - pos);
        if (token.size() >= 2 && token[0] == '*' && token[1] == '.')
            token = token.substr(1);
        if (!token.empty())
            filterExts_.push_back(token);
        pos = semi + 1;
    }
}

bool FileDialog::matchesFilter(const std::string& name) const
{
    if (filterExts_.empty()) return true;
    std::string ext = FileSystem::extension(name);
    std::string extLow = ext;
    for (auto& c : extLow) c = static_cast<char>(tolower(c));
    for (auto& fe : filterExts_) {
        std::string feLow = fe;
        for (auto& c : feLow) c = static_cast<char>(tolower(c));
        if (extLow == feLow) return true;
    }
    return false;
}

bool FileDialog::isImageExtension(const std::string& ext)
{
    std::string e = ext;
    for (auto& c : e) c = static_cast<char>(tolower(c));
    return e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".bmp" ||
           e == ".tga" || e == ".gif" || e == ".psd" || e == ".hdr";
}

// ═════════════════════════════════════════════════════════════════════════════
//  Directory listing
// ═════════════════════════════════════════════════════════════════════════════

void FileDialog::refreshDir()
{
    entries_.clear();
    selectedIndex_ = -1;
    selectedIndices_.clear();
    scrollY_ = 0;

    auto rawEntries = FileSystem::listDir(currentPath_);
    for (auto& e : rawEntries) {
        if (!showHidden_ && e.hidden) continue;
        bool isDir = (e.type == FileSystem::EntryType::Directory);
        if (!isDir && mode_ == Mode::SelectFolder) continue;
        if (!isDir && !matchesFilter(e.name)) continue;

        FileEntry fe;
        fe.name = e.name;
        fe.path = e.path;
        fe.isDir = isDir;
        fe.size = e.size;
        fe.mtime = e.mtime;
        fe.hidden = e.hidden;
        fe.selected = false;

        if (isDir)
            fe.icon = IconId::Folder;
        else {
            std::string ext = FileSystem::extension(e.name);
            for (auto& c : ext) c = static_cast<char>(tolower(c));
            if (ext == ".cpp" || ext == ".c" || ext == ".hpp" || ext == ".h" ||
                ext == ".py" || ext == ".js" || ext == ".ts" || ext == ".java")
                fe.icon = IconId::FileCode;
            else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
                     ext == ".tga" || ext == ".gif" || ext == ".psd" || ext == ".hdr" ||
                     ext == ".svg" || ext == ".ico" || ext == ".webp")
                fe.icon = IconId::FileImage;
            else if (ext == ".zip" || ext == ".tar" || ext == ".gz" || ext == ".bz2" ||
                     ext == ".xz" || ext == ".7z" || ext == ".rar" || ext == ".deb" ||
                     ext == ".rpm")
                fe.icon = IconId::FileArchive;
            else
                fe.icon = IconId::File;
        }
        entries_.push_back(std::move(fe));
    }
    sortEntries();
    buildBreadcrumbs();
    clearPreview();
    markDirty();
}

void FileDialog::sortEntries()
{
    auto cmp = [this](const FileEntry& a, const FileEntry& b) {
        if (a.isDir != b.isDir) return a.isDir > b.isDir;
        switch (sortField_) {
        case SortField::Size:
            return sortAscending_ ? a.size < b.size : a.size > b.size;
        case SortField::Date:
            return sortAscending_ ? a.mtime < b.mtime : a.mtime > b.mtime;
        case SortField::Name:
        default: {
            std::string na = a.name, nb = b.name;
            for (auto& c : na) c = static_cast<char>(tolower(c));
            for (auto& c : nb) c = static_cast<char>(tolower(c));
            return sortAscending_ ? na < nb : na > nb;
        }
        }
    };
    std::stable_sort(entries_.begin(), entries_.end(), cmp);
}

void FileDialog::navigateTo(const std::string& path)
{
    if (!FileSystem::isDir(path)) return;
    pendingNav_ = FileSystem::normalizePath(path);
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Multi-select helpers
// ═════════════════════════════════════════════════════════════════════════════

bool FileDialog::isSelected(int idx) const
{
    if (!multiSelect_) return idx == selectedIndex_;
    return selectedIndices_.count(idx) > 0;
}

void FileDialog::toggleSelect(int idx)
{
    if (selectedIndices_.count(idx))
        selectedIndices_.erase(idx);
    else
        selectedIndices_.insert(idx);
}

void FileDialog::selectRange(int from, int to)
{
    if (from > to) std::swap(from, to);
    for (int i = from; i <= to; ++i) {
        if (i >= 0 && i < static_cast<int>(entries_.size()))
            selectedIndices_.insert(i);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Preview (image modes)
// ═════════════════════════════════════════════════════════════════════════════

void FileDialog::loadPreview(const std::string& path)
{
    if (path == previewPath_) return;
    clearPreview();
    previewPath_ = path;
    std::string ext = FileSystem::extension(path);
    if (!isImageExtension(ext)) return;
    previewTex_ = LoadTextureFromFile("fd_preview", path.c_str(), false);
    markDirty();
}

void FileDialog::clearPreview()
{
    if (previewTex_) {
        delete previewTex_;
        previewTex_ = nullptr;
    }
    previewPath_.clear();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Build UI
// ═════════════════════════════════════════════════════════════════════════════

void FileDialog::buildUI()
{
    mainLayout_ = setContent<BoxLayout>(LayoutDir::Vertical);
    mainLayout_->setSpacing(4);
    mainLayout_->setPadding(4);

    // ── Toolbar: breadcrumbs + view mode buttons ─────────────────────────
    toolBar_ = mainLayout_->createChild<BoxLayout>(LayoutDir::Horizontal);
    toolBar_->setSize(0, 26);
    toolBar_->setSpacing(2);
    toolBar_->setPadding(0, 0, 2, 2);

    breadcrumbBar_ = toolBar_->createChild<BoxLayout>(LayoutDir::Horizontal);
    breadcrumbBar_->setStretch(1);
    breadcrumbBar_->setSpacing(2);

    auto& atlas = WidgetApp::instance().iconAtlas();
    Texture* atlasTex = atlas.texture();

    viewDetailBtn_ = toolBar_->createChild<ImageButton>();
    viewDetailBtn_->setSize(24, 22);
    viewDetailBtn_->setTexture(atlasTex);
    viewDetailBtn_->setSrcRect(atlas.srcRect(IconId::ViewDetail));
    viewDetailBtn_->setTooltip("Detail view");
    viewDetailBtn_->clicked.connect([this] { setViewMode(ViewMode::Detail); });

    viewListBtn_ = toolBar_->createChild<ImageButton>();
    viewListBtn_->setSize(24, 22);
    viewListBtn_->setTexture(atlasTex);
    viewListBtn_->setSrcRect(atlas.srcRect(IconId::ViewList));
    viewListBtn_->setTooltip("List view");
    viewListBtn_->clicked.connect([this] { setViewMode(ViewMode::List); });

    viewIconBtn_ = toolBar_->createChild<ImageButton>();
    viewIconBtn_->setSize(24, 22);
    viewIconBtn_->setTexture(atlasTex);
    viewIconBtn_->setSrcRect(atlas.srcRect(IconId::ViewGrid));
    viewIconBtn_->setTooltip("Icon view");
    viewIconBtn_->clicked.connect([this] { setViewMode(ViewMode::Icon); });

    updateViewButtons();

    // ── Body: sidebar + file list + preview ──────────────────────────────
    bodyLayout_ = mainLayout_->createChild<BoxLayout>(LayoutDir::Horizontal);
    bodyLayout_->setStretch(1);
    bodyLayout_->setSpacing(4);

    sidebarLayout_ = bodyLayout_->createChild<BoxLayout>(LayoutDir::Vertical);
    sidebarLayout_->setSize(130, 0);
    sidebarLayout_->setSpacing(2);
    sidebarLayout_->setPadding(2);
    buildSidebar();

    fileListLayout_ = bodyLayout_->createChild<BoxLayout>(LayoutDir::Vertical);
    fileListLayout_->setStretch(1);

    previewLayout_ = bodyLayout_->createChild<Widget>();
    previewLayout_->setSize(180, 0);
    previewLayout_->setVisible(false);

    // ── Bottom bar ───────────────────────────────────────────────────────
    bottomBar_ = mainLayout_->createChild<BoxLayout>(LayoutDir::Horizontal);
    bottomBar_->setSize(0, 30);
    bottomBar_->setSpacing(6);
    bottomBar_->setPadding(0, 0, 2, 2);

    fileLabel_ = bottomBar_->createChild<Label>("File:");
    fileLabel_->setSize(30, 0);

    fileNameInput_ = bottomBar_->createChild<TextInput>();
    fileNameInput_->setStretch(1);
    fileNameInput_->setPlaceholder("filename");

    bottomSpacer_ = bottomBar_->createChild<Widget>();
    bottomSpacer_->setStretch(1);
    bottomSpacer_->setVisible(false);

    hiddenToggle_ = bottomBar_->createChild<Button>("H");
    hiddenToggle_->setSize(28, 0);
    hiddenToggle_->setTooltip("Toggle hidden files");
    hiddenToggle_->clicked.connect([this] { setShowHidden(!showHidden_); });

    cancelButton_ = bottomBar_->createChild<Button>("Cancel");
    cancelButton_->setSize(70, 0);
    cancelButton_->clicked.connect([this] { onCancel(); });

    okButton_ = bottomBar_->createChild<Button>("Open");
    okButton_->setSize(70, 0);
    okButton_->clicked.connect([this] { onOk(); });

    statusLabel_ = mainLayout_->createChild<Label>("");
    statusLabel_->setSize(0, 16);
    statusLabel_->setColor(Color(120, 120, 130, 255));
}

void FileDialog::updateViewButtons()
{
    Color active(40, 70, 120, 255);
    Color normal(0, 0, 0, 0); // transparent = theme default
    if (viewDetailBtn_) viewDetailBtn_->setBgColor(viewMode_ == ViewMode::Detail ? active : normal);
    if (viewListBtn_)   viewListBtn_->setBgColor(viewMode_ == ViewMode::List ? active : normal);
    if (viewIconBtn_)   viewIconBtn_->setBgColor(viewMode_ == ViewMode::Icon ? active : normal);
}

void FileDialog::buildBreadcrumbs()
{
    while (!breadcrumbBar_->children().empty())
        breadcrumbBar_->removeChild(breadcrumbBar_->children().back());

    std::string p = currentPath_;
    std::vector<std::pair<std::string, std::string>> parts;
#if defined(_WIN32)
    parts.push_back({"C:", "C:\\"});
    std::string accum = "C:";
#else
    parts.push_back({"/", "/"});
    std::string accum;
#endif
    size_t start = 1;
    for (size_t i = start; i <= p.size(); ++i) {
        if (i == p.size() || p[i] == '/' || p[i] == '\\') {
            std::string seg = p.substr(start, i - start);
            if (!seg.empty()) {
                accum += "/" + seg;
                parts.push_back({seg, accum});
            }
            start = i + 1;
        }
    }

    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            auto* sep = breadcrumbBar_->createChild<Label>(">");
            sep->setSize(12, 0);
            sep->setColor(Color(120, 120, 130, 255));
        }
        std::string target = parts[i].second;
        auto* btn = breadcrumbBar_->createChild<Button>(parts[i].first);
        btn->setSize(0, 22);
        btn->clicked.connect([this, target] { navigateTo(target); });
    }
    markDirty();
}

void FileDialog::buildSidebar()
{
    while (!sidebarLayout_->children().empty())
        sidebarLayout_->removeChild(sidebarLayout_->children().back());

    auto* bookLabel = sidebarLayout_->createChild<Label>("Bookmarks");
    bookLabel->setColor(Color(100, 160, 255, 255));

    for (size_t i = 0; i < bookmarks_.size(); ++i) {
        int idx = static_cast<int>(i);
        auto* btn = sidebarLayout_->createChild<Button>(bookmarks_[i].name);
        btn->setSize(0, 22);
        btn->clicked.connect([this, idx] { onBookmarkClick(idx); });
    }

    if (!recentFiles_.empty()) {
        sidebarLayout_->createChild<Label>("")->setSize(0, 4);
        auto* recLabel = sidebarLayout_->createChild<Label>("Recent");
        recLabel->setColor(Color(100, 160, 255, 255));
        for (size_t i = 0; i < recentFiles_.size() && i < 8; ++i) {
            int idx = static_cast<int>(i);
            std::string name = FileSystem::fileName(recentFiles_[i]);
            auto* btn = sidebarLayout_->createChild<Button>(name);
            btn->setSize(0, 20);
            btn->setTooltip(recentFiles_[i]);
            btn->clicked.connect([this, idx] { onRecentClick(idx); });
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Layout
// ═════════════════════════════════════════════════════════════════════════════

void FileDialog::layout()
{
    if (!pendingNav_.empty()) {
        std::string nav = std::move(pendingNav_);
        pendingNav_.clear();
        currentPath_ = nav;
        refreshDir();
    }

    FloatWindow::layout();

    if (statusLabel_) {
        char buf[128];
        int dirs = 0, files = 0, sel = 0;
        for (auto& e : entries_) { if (e.isDir) dirs++; else files++; }
        if (multiSelect_) sel = static_cast<int>(selectedIndices_.size());
        if (multiSelect_ && sel > 0)
            snprintf(buf, sizeof(buf), "%d folders, %d files  |  %d selected", dirs, files, sel);
        else
            snprintf(buf, sizeof(buf), "%d folders, %d files", dirs, files);
        statusLabel_->setText(buf);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Paint
// ═════════════════════════════════════════════════════════════════════════════

void FileDialog::paint(PaintContext& ctx)
{
    FloatWindow::paint(ctx);

    if (!fileListLayout_) return;
    Rect area = fileListLayout_->absoluteRect();
    if (area.w < 10 || area.h < 10) return;

    const auto& t = Theme::instance();

    ctx.fill.SetColor(t.panelColor.r - 5, t.panelColor.g - 5, t.panelColor.b - 5, 255);
    ctx.fillRect(area.x, area.y, area.w, area.h);

    switch (viewMode_) {
    case ViewMode::Detail: {
        float headerH = 22.0f;
        paintFileListHeader(ctx, {area.x, area.y, area.w, headerH});
        Rect bodyArea = {area.x, area.y + headerH, area.w, area.h - headerH};
        ctx.pushClip(bodyArea);
        paintFileListDetail(ctx, bodyArea);
        ctx.popClip();
        break;
    }
    case ViewMode::List:
        ctx.pushClip(area);
        paintFileListCompact(ctx, area);
        ctx.popClip();
        break;
    case ViewMode::Icon:
        ctx.pushClip(area);
        paintFileListIcons(ctx, area);
        ctx.popClip();
        break;
    }

    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
    ctx.lineRect(area.x, area.y, area.w, area.h);

    if (sidebarLayout_) {
        Rect sr = sidebarLayout_->absoluteRect();
        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, 60);
        ctx.drawLine(sr.x + sr.w, sr.y, sr.x + sr.w, sr.y + sr.h);
    }

    bool imageMode = (mode_ == Mode::OpenImage || mode_ == Mode::SaveImage);
    if (imageMode && previewLayout_ && previewLayout_->isVisible()) {
        Rect pr = previewLayout_->absoluteRect();
        // Debug: draw visible background so we can see the panel area
        ctx.fill.SetColor(t.panelColor.r + 15, t.panelColor.g + 15, t.panelColor.b + 15, 255);
        ctx.fillRect(pr.x, pr.y, pr.w, pr.h);
        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, 60);
        ctx.drawLine(pr.x, pr.y, pr.x, pr.y + pr.h);
        paintPreview(ctx, pr);
    }
}

void FileDialog::paintFileListHeader(PaintContext& ctx, const Rect& area)
{
    const auto& t = Theme::instance();
    ctx.fill.SetColor(t.panelColor.r + 10, t.panelColor.g + 10, t.panelColor.b + 10, 255);
    ctx.fillRect(area.x, area.y, area.w, area.h);

    ctx.font.SetFontSize(t.fontSize * 0.9f);
    ctx.font.SetBatch(&ctx.text);
    auto fc = toFontClipFD(area);

    float x = area.x;
    for (auto& col : listColumns_) {
        ctx.font.SetColor(t.textColor);
        ctx.font.Print(col.name.c_str(), x + 4, area.y + 3, &fc);
        if (sortField_ == col.field) {
            float arrowX = x + col.width - 14;
            float arrowY = area.y + area.h * 0.5f;
            ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, t.textColor.a);
            if (sortAscending_)
                ctx.fillTriangle(arrowX, arrowY + 3, arrowX + 6, arrowY + 3, arrowX + 3, arrowY - 3);
            else
                ctx.fillTriangle(arrowX, arrowY - 3, arrowX + 6, arrowY - 3, arrowX + 3, arrowY + 3);
        }
        x += col.width;
        ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, 60);
        ctx.drawLine(x, area.y + 2, x, area.y + area.h - 2);
    }
    ctx.line.SetColor(t.borderColor.r, t.borderColor.g, t.borderColor.b, t.borderColor.a);
    ctx.drawLine(area.x, area.y + area.h, area.x + area.w, area.y + area.h);
}

// ── Detail view ─────────────────────────────────────────────────────────

void FileDialog::paintFileListDetail(PaintContext& ctx, const Rect& area)
{
    const auto& t = Theme::instance();
    ctx.font.SetFontSize(t.fontSize * 0.9f);
    ctx.font.SetBatch(&ctx.text);
    auto fc = toFontClipFD(area);

    int total = static_cast<int>(entries_.size());
    int first = static_cast<int>(scrollY_ / rowHeight_);
    int last = first + static_cast<int>(area.h / rowHeight_) + 2;
    if (first < 0) first = 0;
    if (last > total) last = total;

    for (int i = first; i < last; ++i) {
        auto& fe = entries_[i];
        float ry = area.y + i * rowHeight_ - scrollY_;
        if (ry + rowHeight_ < area.y || ry > area.y + area.h) continue;

        if (i % 2 == 1) {
            ctx.fill.SetColor(t.panelColor.r + 3, t.panelColor.g + 3, t.panelColor.b + 3, 255);
            ctx.fillRect(area.x, ry, area.w, rowHeight_);
        }
        if (isSelected(i)) {
            ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g, t.selectionColor.b, t.selectionColor.a);
            ctx.fillRect(area.x, ry, area.w, rowHeight_);
        } else if (i == hoveredIndex_) {
            ctx.fill.SetColor(t.buttonHover.r, t.buttonHover.g, t.buttonHover.b, 50);
            ctx.fillRect(area.x, ry, area.w, rowHeight_);
        }

        float textY = ry + (rowHeight_ - t.fontSize * 0.9f) * 0.5f;
        float cx = area.x;

        // Name
        {
            float nameW = listColumns_[0].width;
            if (ctx.icons && fe.icon != IconId::None) {
                float isz = rowHeight_ - 6;
                ctx.drawIcon(fe.icon, cx + 2, ry + 3, isz);
                cx += isz + 4;
            } else cx += 4;

            ctx.font.SetColor(fe.isDir ? Color(120, 180, 255, 255)
                                       : (isSelected(i) ? Color(255, 255, 255, 255) : t.textColor));
            ctx.font.Print(fe.name.c_str(), cx, textY, &fc);
            cx = area.x + nameW;
        }
        // Size
        {
            float w = listColumns_[1].width;
            if (!fe.isDir) {
                ctx.font.SetColor(Color(160, 160, 165, 255));
                ctx.font.Print(FileSystem::humanSize(fe.size).c_str(), cx + 4, textY, &fc);
            }
            cx += w;
        }
        // Date
        {
            ctx.font.SetColor(Color(140, 140, 145, 255));
            ctx.font.Print(FileSystem::humanDate(fe.mtime).c_str(), cx + 4, textY, &fc);
        }
    }

    // Scrollbar
    float contentH = total * rowHeight_;
    if (contentH > area.h) {
        float sbW = 6, sbX = area.x + area.w - sbW;
        float thumbH = std::max(20.0f, (area.h / contentH) * area.h);
        float maxSY = contentH - area.h;
        float thumbY = area.y + (maxSY > 0 ? (scrollY_ / maxSY) * (area.h - thumbH) : 0);
        ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, 60);
        ctx.fillRect(sbX + 1, thumbY, sbW - 2, thumbH);
    }
}

// ── List view (compact multi-column) ────────────────────────────────────

void FileDialog::paintFileListCompact(PaintContext& ctx, const Rect& area)
{
    const auto& t = Theme::instance();
    float rh = rowHeight_;
    float colW = 200.0f;
    int rowsPerCol = std::max(1, static_cast<int>(area.h / rh));
    int total = static_cast<int>(entries_.size());

    ctx.font.SetFontSize(t.fontSize * 0.9f);
    ctx.font.SetBatch(&ctx.text);
    auto fc = toFontClipFD(area);

    for (int i = 0; i < total; ++i) {
        int col = i / rowsPerCol;
        int row = i % rowsPerCol;
        float rx = area.x + col * colW - scrollY_;
        float ry = area.y + row * rh;
        if (rx + colW < area.x || rx > area.x + area.w) continue;

        auto& fe = entries_[i];
        if (isSelected(i)) {
            ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g, t.selectionColor.b, t.selectionColor.a);
            ctx.fillRect(rx, ry, colW, rh);
        } else if (i == hoveredIndex_) {
            ctx.fill.SetColor(t.buttonHover.r, t.buttonHover.g, t.buttonHover.b, 50);
            ctx.fillRect(rx, ry, colW, rh);
        }

        float cx = rx;
        if (ctx.icons && fe.icon != IconId::None) {
            float isz = rh - 6;
            ctx.drawIcon(fe.icon, cx + 2, ry + 3, isz);
            cx += isz + 4;
        } else cx += 4;

        float textY = ry + (rh - t.fontSize * 0.9f) * 0.5f;
        ctx.font.SetColor(fe.isDir ? Color(120, 180, 255, 255) : t.textColor);
        ctx.font.Print(fe.name.c_str(), cx, textY, &fc);
    }

    // Horizontal scrollbar
    int cols = std::max(1, (total + rowsPerCol - 1) / rowsPerCol);
    float totalW = cols * colW;
    if (totalW > area.w) {
        float sbH = 6, sbY = area.y + area.h - sbH;
        float thumbW = std::max(20.0f, (area.w / totalW) * area.w);
        float maxSX = totalW - area.w;
        float thumbX = area.x + (maxSX > 0 ? (scrollY_ / maxSX) * (area.w - thumbW) : 0);
        ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, 60);
        ctx.fillRect(thumbX, sbY + 1, thumbW, sbH - 2);
    }
}

// ── Icon view (grid) ────────────────────────────────────────────────────

void FileDialog::paintFileListIcons(PaintContext& ctx, const Rect& area)
{
    const auto& t = Theme::instance();
    float cell = iconCellSize_;
    int colsPerRow = std::max(1, static_cast<int>(area.w / cell));
    int total = static_cast<int>(entries_.size());

    ctx.font.SetFontSize(t.fontSize * 0.8f);
    ctx.font.SetBatch(&ctx.text);
    auto fc = toFontClipFD(area);

    for (int i = 0; i < total; ++i) {
        int c = i % colsPerRow;
        int r = i / colsPerRow;
        float cx = area.x + c * cell;
        float cy = area.y + r * cell - scrollY_;
        if (cy + cell < area.y || cy > area.y + area.h) continue;

        auto& fe = entries_[i];
        if (isSelected(i)) {
            ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g, t.selectionColor.b, t.selectionColor.a);
            ctx.fillRect(cx + 2, cy + 2, cell - 4, cell - 4);
        } else if (i == hoveredIndex_) {
            ctx.fill.SetColor(t.buttonHover.r, t.buttonHover.g, t.buttonHover.b, 40);
            ctx.fillRect(cx + 2, cy + 2, cell - 4, cell - 4);
        }

        float isz = cell * 0.45f;
        float ix = cx + (cell - isz) * 0.5f;
        float iy = cy + 6;
        if (ctx.icons && fe.icon != IconId::None)
            ctx.drawIcon(fe.icon, ix, iy, isz);

        std::string disp = fe.name;
        if (disp.size() > 12) disp = disp.substr(0, 10) + "..";
        float tw = ctx.font.GetTextWidth(disp.c_str());
        float tx = cx + (cell - tw) * 0.5f;
        float ty = cy + cell - 20;
        ctx.font.SetColor(fe.isDir ? Color(120, 180, 255, 255) : t.textColor);
        ctx.font.Print(disp.c_str(), tx, ty, &fc);
    }

    // Vertical scrollbar
    int totalRows = (total + colsPerRow - 1) / colsPerRow;
    float contentH = totalRows * cell;
    if (contentH > area.h) {
        float sbW = 6, sbX = area.x + area.w - sbW;
        float thumbH = std::max(20.0f, (area.h / contentH) * area.h);
        float maxSY = contentH - area.h;
        float thumbY = area.y + (maxSY > 0 ? (scrollY_ / maxSY) * (area.h - thumbH) : 0);
        ctx.fill.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, 60);
        ctx.fillRect(sbX + 1, thumbY, sbW - 2, thumbH);
    }
}

// ── Preview panel ───────────────────────────────────────────────────────

void FileDialog::paintPreview(PaintContext& ctx, const Rect& area)
{
    const auto& t = Theme::instance();
    if (!previewTex_ || !previewTex_->id) {
        ctx.font.SetFontSize(t.fontSize * 0.8f);
        ctx.font.SetBatch(&ctx.text);
        ctx.font.SetColor(Color(100, 100, 110, 255));
        auto fc = toFontClipFD(area);
        ctx.font.Print("No preview", area.x + 10, area.y + area.h * 0.5f, &fc);
        return;
    }

    float imgW = static_cast<float>(previewTex_->width);
    float imgH = static_cast<float>(previewTex_->height);
    float maxW = area.w - 8, maxH = area.h - 30;
    float scale = std::min(maxW / imgW, maxH / imgH);
    if (scale > 1.0f) scale = 1.0f;
    float dw = imgW * scale, dh = imgH * scale;
    float dx = area.x + (area.w - dw) * 0.5f;
    float dy = area.y + 4;

    ctx.fill.SetColor(255, 255, 255, 255);
    ctx.fill.DrawImage(previewTex_, dx, dy, dw, dh);

    char buf[64];
    snprintf(buf, sizeof(buf), "%dx%d", previewTex_->width, previewTex_->height);
    ctx.font.SetFontSize(t.fontSize * 0.75f);
    ctx.font.SetBatch(&ctx.text);
    ctx.font.SetColor(Color(130, 130, 140, 255));
    auto fc = toFontClipFD(area);
    ctx.font.Print(buf, area.x + 6, dy + dh + 4, &fc);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Mouse - helper to compute index from position per view mode
// ═════════════════════════════════════════════════════════════════════════════

static int hitIndexForView(FileDialog::ViewMode vm, const Rect& area,
                           float mx, float my, float scrollY,
                           float rowH, float iconCell, int total)
{
    switch (vm) {
    case FileDialog::ViewMode::Detail: {
        float headerH = 22.0f;
        float localY = my - (area.y + headerH) + scrollY;
        return static_cast<int>(localY / rowH);
    }
    case FileDialog::ViewMode::List: {
        int rowsPerCol = std::max(1, static_cast<int>(area.h / rowH));
        float colW = 200.0f;
        float localX = mx - area.x + scrollY;
        float localY = my - area.y;
        int col = static_cast<int>(localX / colW);
        int row = static_cast<int>(localY / rowH);
        return col * rowsPerCol + row;
    }
    case FileDialog::ViewMode::Icon: {
        int colsPerRow = std::max(1, static_cast<int>(area.w / iconCell));
        float localX = mx - area.x;
        float localY = my - area.y + scrollY;
        int col = static_cast<int>(localX / iconCell);
        int row = static_cast<int>(localY / iconCell);
        return row * colsPerRow + col;
    }
    }
    return -1;
}

void FileDialog::onMousePress(MouseEvent& e)
{
    if (!visible_ || !enabled_) { FloatWindow::onMousePress(e); return; }

    if (fileListLayout_) {
        Rect area = fileListLayout_->absoluteRect();

        // Header click (detail mode only) → sort
        if (viewMode_ == ViewMode::Detail) {
            float headerH = 22.0f;
            Rect headerArea = {area.x, area.y, area.w, headerH};
            if (headerArea.contains(e.x, e.y)) {
                e.consumed = true;
                float cx = area.x;
                for (auto& col : listColumns_) {
                    if (e.x >= cx && e.x < cx + col.width) {
                        if (sortField_ == col.field) sortAscending_ = !sortAscending_;
                        else { sortField_ = col.field; sortAscending_ = true; }
                        sortEntries();
                        markDirty();
                        return;
                    }
                    cx += col.width;
                }
                return;
            }
        }

        // Body click in any view
        bool inBody = false;
        if (viewMode_ == ViewMode::Detail) {
            float headerH = 22.0f;
            Rect bodyArea = {area.x, area.y + headerH, area.w, area.h - headerH};
            inBody = bodyArea.contains(e.x, e.y);
        } else {
            inBody = area.contains(e.x, e.y);
        }

        if (inBody) {
            e.consumed = true;
            int total = static_cast<int>(entries_.size());
            int idx = hitIndexForView(viewMode_, area, e.x, e.y, scrollY_, rowHeight_, iconCellSize_, total);
            if (idx < 0 || idx >= total) return;

            // Double-click detection
            static Uint32 lastClick = 0;
            static int lastIdx = -1;
            Uint32 now = SDL_GetTicks();
            bool dblClick = (idx == lastIdx && (now - lastClick) < 400);
            lastClick = now;
            lastIdx = idx;

            // Modifier keys from SDL
            auto mod = SDL_GetModState();
            bool ctrl  = (mod & KMOD_CTRL) != 0;
            bool shift = (mod & KMOD_SHIFT) != 0;

            // Selection
            if (multiSelect_ && ctrl) {
                toggleSelect(idx);
                lastClickedIndex_ = idx;
                selectedIndex_ = idx;
            } else if (multiSelect_ && shift && lastClickedIndex_ >= 0) {
                selectedIndices_.clear();
                selectRange(lastClickedIndex_, idx);
                selectedIndex_ = idx;
            } else {
                selectedIndices_.clear();
                selectedIndex_ = idx;
                selectedIndices_.insert(idx);
                lastClickedIndex_ = idx;
            }

            auto& fe = entries_[idx];
            if (fileNameInput_ && !fe.isDir)
                fileNameInput_->setText(fe.name);

            // Preview for image modes
            bool imageMode = (mode_ == Mode::OpenImage || mode_ == Mode::SaveImage);
            if (imageMode && !fe.isDir)
                loadPreview(fe.path);

            // Double-click action
            if (dblClick) {
                if (fe.isDir) {
                    navigateTo(fe.path);
                } else {
                    selectedPath_ = fe.path;
                    addRecent(fe.path);
                    accepted.emit(selectedPath_);
                    WidgetApp::instance().removeFloat(this);
                }
            }
            markDirty();
            return;
        }
    }

    FloatWindow::onMousePress(e);
}

void FileDialog::onMouseRelease(MouseEvent& e)
{
    FloatWindow::onMouseRelease(e);
}

void FileDialog::onMouseMove(MouseEvent& e)
{
    if (fileListLayout_) {
        Rect area = fileListLayout_->absoluteRect();
        bool inBody = false;
        if (viewMode_ == ViewMode::Detail) {
            float headerH = 22.0f;
            Rect bodyArea = {area.x, area.y + headerH, area.w, area.h - headerH};
            inBody = bodyArea.contains(e.x, e.y);
        } else {
            inBody = area.contains(e.x, e.y);
        }

        if (inBody) {
            int total = static_cast<int>(entries_.size());
            int idx = hitIndexForView(viewMode_, area, e.x, e.y, scrollY_, rowHeight_, iconCellSize_, total);
            if (idx >= 0 && idx < total && hoveredIndex_ != idx) {
                hoveredIndex_ = idx;
                markDirty();
            }
        } else if (hoveredIndex_ >= 0) {
            hoveredIndex_ = -1;
            markDirty();
        }
    }
    FloatWindow::onMouseMove(e);
}

void FileDialog::onMouseScroll(MouseEvent& e)
{
    if (fileListLayout_) {
        Rect area = fileListLayout_->absoluteRect();
        if (area.contains(e.x, e.y)) {
            e.consumed = true;
            if (viewMode_ == ViewMode::Icon)
                fileListScroll(-e.scrollY * iconCellSize_);
            else
                fileListScroll(-e.scrollY * rowHeight_ * 3.0f);
            return;
        }
    }
    FloatWindow::onMouseScroll(e);
}

void FileDialog::onKeyPress(KeyEvent& e)
{
    if (e.key == SDLK_ESCAPE) { e.consumed = true; onCancel(); return; }
    if (e.key == SDLK_RETURN || e.key == SDLK_KP_ENTER) { e.consumed = true; onOk(); return; }
    if (e.key == SDLK_BACKSPACE && e.alt) { e.consumed = true; navigateTo(FileSystem::parentPath(currentPath_)); return; }

    if (e.key == SDLK_a && e.ctrl && multiSelect_) {
        e.consumed = true;
        selectedIndices_.clear();
        for (int i = 0; i < static_cast<int>(entries_.size()); ++i)
            selectedIndices_.insert(i);
        markDirty();
        return;
    }

    if (e.key == SDLK_UP && selectedIndex_ > 0) {
        e.consumed = true;
        selectedIndex_--;
        selectedIndices_.clear();
        selectedIndices_.insert(selectedIndex_);
        lastClickedIndex_ = selectedIndex_;
        if (fileNameInput_ && selectedIndex_ < static_cast<int>(entries_.size()) && !entries_[selectedIndex_].isDir)
            fileNameInput_->setText(entries_[selectedIndex_].name);
        markDirty();
    } else if (e.key == SDLK_DOWN && selectedIndex_ < static_cast<int>(entries_.size()) - 1) {
        e.consumed = true;
        selectedIndex_++;
        selectedIndices_.clear();
        selectedIndices_.insert(selectedIndex_);
        lastClickedIndex_ = selectedIndex_;
        if (fileNameInput_ && selectedIndex_ < static_cast<int>(entries_.size()) && !entries_[selectedIndex_].isDir)
            fileNameInput_->setText(entries_[selectedIndex_].name);
        markDirty();
    }

    FloatWindow::onKeyPress(e);
}

// ═════════════════════════════════════════════════════════════════════════════
//  File list helpers (kept for internal use)
// ═════════════════════════════════════════════════════════════════════════════

void FileDialog::fileListMousePress(float, float localY, bool dblClick, bool ctrl, bool shift)
{
    int idx = static_cast<int>(localY / rowHeight_);
    if (idx < 0 || idx >= static_cast<int>(entries_.size())) return;

    if (multiSelect_ && ctrl) { toggleSelect(idx); lastClickedIndex_ = idx; selectedIndex_ = idx; }
    else if (multiSelect_ && shift && lastClickedIndex_ >= 0) { selectedIndices_.clear(); selectRange(lastClickedIndex_, idx); selectedIndex_ = idx; }
    else { selectedIndices_.clear(); selectedIndex_ = idx; selectedIndices_.insert(idx); lastClickedIndex_ = idx; }

    auto& fe = entries_[idx];
    if (dblClick) {
        if (fe.isDir) navigateTo(fe.path);
        else { selectedPath_ = fe.path; addRecent(fe.path); accepted.emit(selectedPath_); WidgetApp::instance().removeFloat(this); }
    } else {
        if (fileNameInput_ && !fe.isDir) fileNameInput_->setText(fe.name);
        bool imageMode = (mode_ == Mode::OpenImage || mode_ == Mode::SaveImage);
        if (imageMode && !fe.isDir) loadPreview(fe.path);
    }
    markDirty();
}

void FileDialog::fileListMouseMove(float, float localY)
{
    int idx = static_cast<int>(localY / rowHeight_);
    if (idx != hoveredIndex_) { hoveredIndex_ = idx; markDirty(); }
}

void FileDialog::fileListScroll(float dy)
{
    Rect area = fileListLayout_ ? fileListLayout_->absoluteRect() : Rect{0, 0, 100, 100};
    if (viewMode_ == ViewMode::Icon) {
        int colsPerRow = std::max(1, static_cast<int>(area.w / iconCellSize_));
        int totalRows = (static_cast<int>(entries_.size()) + colsPerRow - 1) / colsPerRow;
        float contentH = totalRows * iconCellSize_;
        scrollY_ = Clamp(scrollY_ + dy, 0.0f, std::max(0.0f, contentH - area.h));
    } else if (viewMode_ == ViewMode::List) {
        int rowsPerCol = std::max(1, static_cast<int>(area.h / rowHeight_));
        int cols = (static_cast<int>(entries_.size()) + rowsPerCol - 1) / rowsPerCol;
        float totalW = cols * 200.0f;
        scrollY_ = Clamp(scrollY_ + dy, 0.0f, std::max(0.0f, totalW - area.w));
    } else {
        float contentH = static_cast<float>(entries_.size()) * rowHeight_;
        float bodyH = area.h - 22.0f;
        scrollY_ = Clamp(scrollY_ + dy, 0.0f, std::max(0.0f, contentH - bodyH));
    }
    markDirty();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Actions
// ═════════════════════════════════════════════════════════════════════════════

void FileDialog::onOk()
{
    if (mode_ == Mode::SelectFolder) {
        selectedPath_ = currentPath_;
        accepted.emit(selectedPath_);
        WidgetApp::instance().removeFloat(this);
        return;
    }

    // Multi-select emit
    if (multiSelect_ && !selectedIndices_.empty()) {
        std::vector<std::string> paths;
        for (int idx : selectedIndices_) {
            if (idx >= 0 && idx < static_cast<int>(entries_.size()) && !entries_[idx].isDir)
                paths.push_back(entries_[idx].path);
        }
        if (!paths.empty()) {
            for (auto& p : paths) addRecent(p);
            selectedPath_ = paths.front();
            acceptedMulti.emit(paths);
            accepted.emit(selectedPath_);
            WidgetApp::instance().removeFloat(this);
            return;
        }
    }

    std::string fname;
    if (fileNameInput_) fname = fileNameInput_->text();

    if (fname.empty() && selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(entries_.size())) {
        auto& fe = entries_[selectedIndex_];
        if (fe.isDir) { navigateTo(fe.path); return; }
        fname = fe.name;
    }
    if (fname.empty()) return;

    selectedPath_ = FileSystem::joinPath(currentPath_, fname);
    bool needsExist = (mode_ == Mode::Open || mode_ == Mode::OpenImage);
    if (needsExist && !FileSystem::exists(selectedPath_)) return;

    addRecent(selectedPath_);
    accepted.emit(selectedPath_);
    WidgetApp::instance().removeFloat(this);
}

void FileDialog::onCancel()
{
    cancelled.emit();
    WidgetApp::instance().removeFloat(this);
}

void FileDialog::onBookmarkClick(int idx)
{
    if (idx >= 0 && idx < static_cast<int>(bookmarks_.size()))
        navigateTo(bookmarks_[idx].path);
}

void FileDialog::onRecentClick(int idx)
{
    if (idx >= 0 && idx < static_cast<int>(recentFiles_.size())) {
        std::string dir = FileSystem::parentPath(recentFiles_[idx]);
        navigateTo(dir);
        std::string name = FileSystem::fileName(recentFiles_[idx]);
        if (fileNameInput_) fileNameInput_->setText(name);
        for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
            if (entries_[i].name == name) { selectedIndex_ = i; break; }
        }
        markDirty();
    }
}

// ── Compat shim ─────────────────────────────────────────────────────────
void FileDialog::paintFileList(PaintContext& ctx, const Rect& area)
{
    paintFileListDetail(ctx, area);
}
