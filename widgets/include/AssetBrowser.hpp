#pragma once

#include "Widget.hpp"
#include "Signal.hpp"
#include "IconAtlas.hpp"
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

// ═════════════════════════════════════════════════════════════════════════════
//  AssetBrowser — thumbnail grid (or list) file/asset browser
//
//  No real filesystem I/O — items are added programmatically.
//  Thumbnails are colored rectangles + icon; a custom thumbnail
//  (Texture*) can be set per item for real content.
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
    AssetType   type      = AssetType::Generic;
    Color       thumbColor= Color(60, 64, 72, 255);  // fallback thumbnail bg
    // Extend: Texture* thumbTex = nullptr;           // set for real thumbnails
    bool        selected  = false;
};

class AssetBrowser : public Widget
{
public:
    AssetBrowser();

    // ── Items ─────────────────────────────────────────────────────────────
    void addItem(const AssetItem& item);
    void clearItems();
    int  itemCount() const { return (int)items_.size(); }

    // ── Navigation ────────────────────────────────────────────────────────
    void setPath(const std::string& path) { path_ = path; markDirty(); }
    const std::string& path() const       { return path_; }

    // ── View mode ─────────────────────────────────────────────────────────
    enum class ViewMode { Grid, List };
    void     setViewMode(ViewMode m) { viewMode_ = m; markDirty(); }
    ViewMode viewMode() const        { return viewMode_; }

    void  setThumbSize(float s)  { thumbSize_ = std::max(24.f, s); markDirty(); }
    float thumbSize() const      { return thumbSize_; }

    // ── Signals ───────────────────────────────────────────────────────────
    Signal<AssetItem>  onSelect;   // single click
    Signal<AssetItem>  onOpen;     // double-click

    // ── Widget overrides ──────────────────────────────────────────────────
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;

private:
    std::vector<AssetItem> items_;
    std::string            path_       = "/";
    ViewMode               viewMode_   = ViewMode::Grid;
    float                  thumbSize_  = 64.f;
    float                  scrollY_    = 0.f;
    int                    selected_   = -1;
    int                    lastClick_  = -1;
    uint32_t               lastClickMs_= 0;

    IconId  typeToIcon(AssetType t) const;
    Color   typeToColor(AssetType t) const;
    int     hitItem(float mx, float my) const;

    // Grid layout helpers (computed inside paint)
    struct GridLayout {
        int cols = 1;
        float cellW = 64, cellH = 80;
        float startX = 0, startY = 0;
        float totalH = 0;
    };
    GridLayout computeGrid(const Rect& b) const;
};
