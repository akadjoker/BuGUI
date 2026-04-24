#pragma once

#include "Math.hpp"
#include <vector>

struct Texture;
class Pixmap;

// ═════════════════════════════════════════════════════════════════════════════
//  Icon identifiers
// ═════════════════════════════════════════════════════════════════════════════

enum class IconId
{
    None = 0,
    Folder,
    FolderOpen,
    File,
    FileCode,
    Book,
    Gear,
    Star,
    Heart,
    Search,
    Plus,
    Minus,
    Check,
    Cross,
    ArrowRight,
    ArrowDown,
    ArrowUp,
    ArrowLeft,
    Eye,
    EyeOff,
    Lock,
    Unlock,
    Refresh,
    Trash,
    Edit,
    Home,
    User,
    Warning,
    Info,
    Error,
    FileImage,
    FileArchive,
    ViewDetail,
    ViewList,
    ViewGrid,
    COUNT  // must be last
};

// ═════════════════════════════════════════════════════════════════════════════
//  IconAtlas - renders all icons into a single RGBA Pixmap at startup,
//  uploads as one GL texture.  Widgets draw icons as textured quads.
//
//  Usage:
//      atlas.init(24);                    // 24×24 px per icon cell
//      atlas.texture();                   // the GL texture
//      atlas.srcRect(IconId::Folder);     // FloatRect in pixel coords
//
//  Drawing (through PaintContext):
//      ctx.drawIcon(IconId::Folder, x, y, 16);
// ═════════════════════════════════════════════════════════════════════════════

class IconAtlas
{
public:
    IconAtlas() = default;
    ~IconAtlas();

    // Generate all icons at the given cell size and upload as GL texture.
    // Call once after GL context is ready.
    void init(int cellSize = 24);

    // Has init() been called?
    bool ready() const { return tex_ != nullptr; }

    // The single GPU texture containing all icons.
    Texture* texture() const { return tex_; }

    // Source rect (in pixel coords) for an icon within the atlas texture.
    FloatRect srcRect(IconId id) const;

    // Atlas cell size (icons are square).
    int cellSize() const { return cellSize_; }

private:
    void drawAllIcons(Pixmap& pm);
    void drawIcon(Pixmap& pm, IconId id, int ox, int oy, int sz);

    // Per-icon drawing helpers (ox,oy = cell origin, sz = cell size)
    void drawFolder     (Pixmap& pm, int ox, int oy, int sz);
    void drawFolderOpen (Pixmap& pm, int ox, int oy, int sz);
    void drawFile       (Pixmap& pm, int ox, int oy, int sz);
    void drawFileCode   (Pixmap& pm, int ox, int oy, int sz);
    void drawBook       (Pixmap& pm, int ox, int oy, int sz);
    void drawGear       (Pixmap& pm, int ox, int oy, int sz);
    void drawStar       (Pixmap& pm, int ox, int oy, int sz);
    void drawHeart      (Pixmap& pm, int ox, int oy, int sz);
    void drawSearch     (Pixmap& pm, int ox, int oy, int sz);
    void drawPlus       (Pixmap& pm, int ox, int oy, int sz);
    void drawMinus      (Pixmap& pm, int ox, int oy, int sz);
    void drawCheck      (Pixmap& pm, int ox, int oy, int sz);
    void drawCross      (Pixmap& pm, int ox, int oy, int sz);
    void drawArrowRight (Pixmap& pm, int ox, int oy, int sz);
    void drawArrowDown  (Pixmap& pm, int ox, int oy, int sz);
    void drawArrowUp    (Pixmap& pm, int ox, int oy, int sz);
    void drawArrowLeft  (Pixmap& pm, int ox, int oy, int sz);
    void drawEye        (Pixmap& pm, int ox, int oy, int sz);
    void drawEyeOff     (Pixmap& pm, int ox, int oy, int sz);
    void drawLock       (Pixmap& pm, int ox, int oy, int sz);
    void drawUnlock     (Pixmap& pm, int ox, int oy, int sz);
    void drawRefresh    (Pixmap& pm, int ox, int oy, int sz);
    void drawTrash      (Pixmap& pm, int ox, int oy, int sz);
    void drawEdit       (Pixmap& pm, int ox, int oy, int sz);
    void drawHome       (Pixmap& pm, int ox, int oy, int sz);
    void drawUser       (Pixmap& pm, int ox, int oy, int sz);
    void drawWarning    (Pixmap& pm, int ox, int oy, int sz);
    void drawInfo       (Pixmap& pm, int ox, int oy, int sz);
    void drawError      (Pixmap& pm, int ox, int oy, int sz);
    void drawFileImage  (Pixmap& pm, int ox, int oy, int sz);
    void drawFileArchive(Pixmap& pm, int ox, int oy, int sz);
    void drawViewDetail (Pixmap& pm, int ox, int oy, int sz);
    void drawViewList   (Pixmap& pm, int ox, int oy, int sz);
    void drawViewGrid   (Pixmap& pm, int ox, int oy, int sz);

    Texture* tex_ = nullptr;
    int cellSize_ = 24;
    int cols_ = 0;   // number of columns in the atlas
};
