#pragma once

#include "Widget.hpp"
#include "IconAtlas.hpp"

namespace BuGUI
{
// ═════════════════════════════════════════════════════════════════════════════
//  AssetBrowser — thumbnail grid (or list) file/asset browser
//
//  No real filesystem I/O — items are added programmatically.
//  Thumbnails are colored rectangles + icon; real texture thumbnails
//  can be extended via TextureHandle.
//
//  Usage:
//    auto* ab = parent->createChild<AssetBrowser>();
//    ab->setPath("Assets/Textures");
//    ab->addItem({"sky.png",    AssetType::Image,  Color(80,140,200,255)});
//    ab->addItem({"models",     AssetType::Folder, Color(200,160,60,255)});
//    ab->onOpen.connect([](const AssetItem& it){ ... });
// ═════════════════════════════════════════════════════════════════════════════

enum class AssetType { Folder, Image, Audio, Mesh, Script, Material, Scene, Generic };

struct AssetItem
{
    std::string name;
    AssetType   type       = AssetType::Generic;
    Color       thumbColor = Color(60, 64, 72, 255);
    bool        selected   = false;
    TextureHandle thumb;          // optional thumbnail texture
    int           thumbW = 0;     // original image width  (for aspect ratio)
    int           thumbH = 0;     // original image height
};

class AssetBrowser : public Widget
{
public:
    AssetBrowser();

    /// @brief Add an asset item.
    void addItem(const AssetItem& item);
    /// @brief Remove all items.
    void clearItems();
    /// @brief Get the number of items.
    int  itemCount() const { return static_cast<int>(items_.size()); }

    /// @brief Set the displayed path.
    void setPath(const std::string& path) { path_ = path; markDirty(); }
    /// @brief Get the current path.
    const std::string& path() const       { return path_; }

    enum class ViewMode { Grid, List };
    /// @brief Set the view mode (Grid or List).
    void     setViewMode(ViewMode m) { viewMode_ = m; markDirty(); }
    /// @brief Get the current view mode.
    ViewMode viewMode() const        { return viewMode_; }

    /// @brief Set the thumbnail cell size.
    void  setThumbSize(float s)  { thumbSize_ = max2(24.f, s); markDirty(); }
    /// @brief Get the thumbnail cell size.
    float thumbSize() const      { return thumbSize_; }

    /// @brief Emitted when an item is selected.
    Signal<AssetItem> onSelect;
    /// @brief Emitted when an item is opened (double-click).
    Signal<AssetItem> onOpen;
    /// @brief Emitted when an item is right-clicked (for context menus).
    Signal<AssetItem> onRightClick;

    /// @brief Set the type filter (Generic = show all).
    void setFilter(AssetType f) { filter_ = f; rebuildFiltered(); markDirty(); }
    /// @brief Get the current filter.
    AssetType filter() const    { return filter_; }

    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseLeave() override;
    void onMouseScroll(MouseEvent& e) override;

private:
    std::vector<AssetItem> items_;
    std::vector<int>       filtered_;   // indices into items_ (or all if no filter)
    AssetType              filter_      = AssetType::Generic; // Generic = show all
    std::string            path_        = "/";
    ViewMode               viewMode_    = ViewMode::Grid;
    float                  thumbSize_   = 64.f;
    float                  scrollY_     = 0.f;
    int                    selected_    = -1;
    int                    lastClick_   = -1;
    float                  lastClickT_  = 0.f;
    float                  timeAcc_     = 0.f;
    int                    hovered_     = -1;
    float                  hoverX_      = 0.f;
    float                  hoverY_      = 0.f;

    void rebuildFiltered();
    const AssetItem& filteredItem(int i) const { return items_[filtered_[i]]; }
    int filteredCount() const { return static_cast<int>(filtered_.size()); }

    struct FilterBtn { float x, y, w, h; AssetType type; };
    std::vector<FilterBtn> filterBtnRects_;

    IconId typeToIcon(AssetType t) const;
    Color  typeToColor(AssetType t) const;
    int    hitItem(float mx, float my) const;

    struct GridLayout {
        int   cols = 1;
        float cellW = 64, cellH = 80;
        float startX = 0, startY = 0;
        float totalH = 0;
    };
    GridLayout computeGrid(const Rect& b) const;
};

} // namespace BuGUI
