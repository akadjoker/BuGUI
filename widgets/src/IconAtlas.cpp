#include "IconAtlas.hpp"
#include "Pixmap.hpp"
#include "Texture.hpp"
#include <cmath>
#include <algorithm>

IconAtlas::~IconAtlas()
{
    if (tex_) { tex_->release(); delete tex_; tex_ = nullptr; }
}

void IconAtlas::init(int cellSize)
{
    cellSize_ = cellSize;

    // Fixed 8-column grid for a stable atlas layout.
    // Grid positions are deterministic: icon index → (col, row).
    // To create a custom atlas PNG, use icon_atlas.png as reference
    // and draw your icons in the same grid slots.
    cols_ = 8;
    int count = static_cast<int>(IconId::COUNT) - 1; // skip None
    int rows = (count + cols_ - 1) / cols_;

    int texW = cols_ * cellSize_;
    int texH = rows  * cellSize_;

    Pixmap pm(texW, texH, 4);
    pm.Fill(0, 0, 0, 0); // fully transparent

    drawAllIcons(pm);

    // Save atlas to file for visual inspection / editing
   // pm.Save("icon_atlas.png");

    tex_ = CreateTextureFromPixmap("icon_atlas", pm);
}

FloatRect IconAtlas::srcRect(IconId id) const
{
    if (id == IconId::None || id == IconId::COUNT || cols_ == 0)
        return {0, 0, 0, 0};

    int idx = static_cast<int>(id) - 1; // None=0, Folder=1 → idx 0
    int col = idx % cols_;
    int row = idx / cols_;
    return {static_cast<float>(col * cellSize_),
            static_cast<float>(row * cellSize_),
            static_cast<float>(cellSize_),
            static_cast<float>(cellSize_)};
}

// ═════════════════════════════════════════════════════════════════════════════
//  Master draw - iterate all icon IDs
// ═════════════════════════════════════════════════════════════════════════════

void IconAtlas::drawAllIcons(Pixmap& pm)
{
    for (int i = 1; i < static_cast<int>(IconId::COUNT); ++i) {
        int idx = i - 1;
        int col = idx % cols_;
        int row = idx / cols_;
        int ox = col * cellSize_;
        int oy = row * cellSize_;
        drawIcon(pm, static_cast<IconId>(i), ox, oy, cellSize_);
    }
}

void IconAtlas::drawIcon(Pixmap& pm, IconId id, int ox, int oy, int sz)
{
    switch (id) {
    case IconId::Folder:      drawFolder(pm, ox, oy, sz); break;
    case IconId::FolderOpen:  drawFolderOpen(pm, ox, oy, sz); break;
    case IconId::File:        drawFile(pm, ox, oy, sz); break;
    case IconId::FileCode:    drawFileCode(pm, ox, oy, sz); break;
    case IconId::Book:        drawBook(pm, ox, oy, sz); break;
    case IconId::Gear:        drawGear(pm, ox, oy, sz); break;
    case IconId::Star:        drawStar(pm, ox, oy, sz); break;
    case IconId::Heart:       drawHeart(pm, ox, oy, sz); break;
    case IconId::Search:      drawSearch(pm, ox, oy, sz); break;
    case IconId::Plus:        drawPlus(pm, ox, oy, sz); break;
    case IconId::Minus:       drawMinus(pm, ox, oy, sz); break;
    case IconId::Check:       drawCheck(pm, ox, oy, sz); break;
    case IconId::Cross:       drawCross(pm, ox, oy, sz); break;
    case IconId::ArrowRight:  drawArrowRight(pm, ox, oy, sz); break;
    case IconId::ArrowDown:   drawArrowDown(pm, ox, oy, sz); break;
    case IconId::ArrowUp:     drawArrowUp(pm, ox, oy, sz); break;
    case IconId::ArrowLeft:   drawArrowLeft(pm, ox, oy, sz); break;
    case IconId::Eye:         drawEye(pm, ox, oy, sz); break;
    case IconId::EyeOff:      drawEyeOff(pm, ox, oy, sz); break;
    case IconId::Lock:        drawLock(pm, ox, oy, sz); break;
    case IconId::Unlock:      drawUnlock(pm, ox, oy, sz); break;
    case IconId::Refresh:     drawRefresh(pm, ox, oy, sz); break;
    case IconId::Trash:       drawTrash(pm, ox, oy, sz); break;
    case IconId::Edit:        drawEdit(pm, ox, oy, sz); break;
    case IconId::Home:        drawHome(pm, ox, oy, sz); break;
    case IconId::User:        drawUser(pm, ox, oy, sz); break;
    case IconId::Warning:     drawWarning(pm, ox, oy, sz); break;
    case IconId::Info:        drawInfo(pm, ox, oy, sz); break;
    case IconId::Error:       drawError(pm, ox, oy, sz); break;
    case IconId::FileImage:   drawFileImage(pm, ox, oy, sz); break;
    case IconId::FileArchive: drawFileArchive(pm, ox, oy, sz); break;
    case IconId::ViewDetail:  drawViewDetail(pm, ox, oy, sz); break;
    case IconId::ViewList:    drawViewList(pm, ox, oy, sz); break;
    case IconId::ViewGrid:    drawViewGrid(pm, ox, oy, sz); break;
    default: break;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Helper: filled rect shortcut (Pixmap::DrawRect with fill=true)
// ═════════════════════════════════════════════════════════════════════════════
static void fRect(Pixmap& pm, int x, int y, int w, int h, const Color& c)
{
    pm.DrawRect(x, y, w, h, c, true);
}

static void oRect(Pixmap& pm, int x, int y, int w, int h, const Color& c)
{
    pm.DrawRect(x, y, w, h, c, false);
}

static void fCirc(Pixmap& pm, int cx, int cy, int r, const Color& c)
{
    pm.DrawCircle(cx, cy, r, c, true);
}

static void oCirc(Pixmap& pm, int cx, int cy, int r, const Color& c)
{
    pm.DrawCircle(cx, cy, r, c, false);
}

static void line(Pixmap& pm, int x1, int y1, int x2, int y2, const Color& c)
{
    pm.DrawLine(x1, y1, x2, y2, c);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Individual icon renderers
//  All coords relative to (ox,oy) with size sz
//  Margin: 2px inside cell for breathing room
// ═════════════════════════════════════════════════════════════════════════════

// Folder: yellow rect with tab
void IconAtlas::drawFolder(Pixmap& pm, int ox, int oy, int sz)
{
    int m = sz / 8; // margin
    Color body(210, 180, 60, 255);
    Color dark(180, 150, 40, 255);
    int bx = ox + m, by = oy + m;
    int bw = sz - m * 2, bh = sz - m * 2;
    // Tab
    fRect(pm, bx, by, bw * 2 / 5, bh / 5, body);
    // Body
    fRect(pm, bx, by + bh / 5, bw, bh * 4 / 5, body);
    // Bottom highlight
    fRect(pm, bx, by + bh - bh / 6, bw, bh / 6, dark);
}

// FolderOpen: folder with slight opening
void IconAtlas::drawFolderOpen(Pixmap& pm, int ox, int oy, int sz)
{
    int m = sz / 8;
    Color body(210, 180, 60, 255);
    Color front(230, 200, 80, 255);
    int bx = ox + m, by = oy + m;
    int bw = sz - m * 2, bh = sz - m * 2;
    // Tab
    fRect(pm, bx, by, bw * 2 / 5, bh / 5, body);
    // Body back
    fRect(pm, bx, by + bh / 5, bw, bh * 4 / 5, body);
    // Front flap (shifted)
    fRect(pm, bx + bw / 8, by + bh * 2 / 5, bw, bh * 3 / 5, front);
}

// File: white rect with corner fold
void IconAtlas::drawFile(Pixmap& pm, int ox, int oy, int sz)
{
    int m = sz / 8;
    Color body(190, 210, 230, 255);
    Color fold(140, 160, 180, 255);
    int bx = ox + m + sz / 8, by = oy + m;
    int bw = sz - m * 2 - sz / 8, bh = sz - m * 2;
    int foldSz = bw / 3;
    fRect(pm, bx, by, bw, bh, body);
    // Corner fold triangle approximation (dark rect in top-right)
    fRect(pm, bx + bw - foldSz, by, foldSz, foldSz, fold);
}

// FileCode: file with < > brackets
void IconAtlas::drawFileCode(Pixmap& pm, int ox, int oy, int sz)
{
    drawFile(pm, ox, oy, sz);
    Color accent(60, 130, 200, 255);
    int cx = ox + sz / 2, cy = oy + sz * 3 / 5;
    int d = sz / 7;
    // < bracket
    line(pm, cx - d * 2, cy, cx - d, cy - d, accent);
    line(pm, cx - d * 2, cy, cx - d, cy + d, accent);
    // > bracket
    line(pm, cx + d * 2, cy, cx + d, cy - d, accent);
    line(pm, cx + d * 2, cy, cx + d, cy + d, accent);
}

// Book: two pages with spine
void IconAtlas::drawBook(Pixmap& pm, int ox, int oy, int sz)
{
    int m = sz / 8;
    Color page(130, 170, 210, 255);
    Color spine(80, 120, 160, 255);
    int bx = ox + m, by = oy + m;
    int bw = sz - m * 2, bh = sz - m * 2;
    int spineW = std::max(2, bw / 7);
    // Left page
    fRect(pm, bx, by, bw / 2 - spineW / 2, bh, page);
    // Right page
    fRect(pm, bx + bw / 2 + spineW / 2, by, bw / 2 - spineW / 2, bh, page);
    // Spine
    fRect(pm, bx + bw / 2 - spineW / 2, by, spineW, bh, spine);
}

// Gear: circle with notches
void IconAtlas::drawGear(Pixmap& pm, int ox, int oy, int sz)
{
    int cx = ox + sz / 2, cy = oy + sz / 2;
    int r = sz * 3 / 8;
    Color col(180, 180, 190, 255);
    Color hole(60, 60, 70, 255);
    fCirc(pm, cx, cy, r, col);
    fCirc(pm, cx, cy, r / 3, hole);
    // Teeth (4 cardinal + 4 diagonal rects)
    int tw = std::max(2, sz / 6), th = std::max(2, sz / 5);
    fRect(pm, cx - tw / 2, cy - r - th / 2, tw, th, col); // top
    fRect(pm, cx - tw / 2, cy + r - th / 2, tw, th, col); // bottom
    fRect(pm, cx - r - th / 2, cy - tw / 2, th, tw, col); // left
    fRect(pm, cx + r - th / 2, cy - tw / 2, th, tw, col); // right
}

// Star: 5-pointed
void IconAtlas::drawStar(Pixmap& pm, int ox, int oy, int sz)
{
    int cx = ox + sz / 2, cy = oy + sz / 2;
    Color col(240, 200, 50, 255);
    // Approximate star: filled circle + some brightening at tips
    int r = sz * 3 / 8;
    fCirc(pm, cx, cy, r, col);
    // Points (small rects protruding)
    int pw = std::max(2, sz / 8), ph = std::max(2, sz / 4);
    fRect(pm, cx - pw / 2, cy - r - ph / 3, pw, ph, col);           // top
    fRect(pm, cx - r - ph / 4, cy - pw, ph, pw * 2, col);           // left
    fRect(pm, cx + r - ph * 3 / 4, cy - pw, ph, pw * 2, col);       // right
    fRect(pm, cx - pw * 2, cy + r - ph / 3, pw * 2, ph * 2 / 3, col); // bottom-left
    fRect(pm, cx + 1,      cy + r - ph / 3, pw * 2, ph * 2 / 3, col); // bottom-right
}

// Heart
void IconAtlas::drawHeart(Pixmap& pm, int ox, int oy, int sz)
{
    int cx = ox + sz / 2, cy = oy + sz / 2;
    Color col(220, 60, 80, 255);
    int r = sz / 5;
    // Two circles at top
    fCirc(pm, cx - r, cy - r / 2, r, col);
    fCirc(pm, cx + r, cy - r / 2, r, col);
    // Triangle below (approximate with rect)
    fRect(pm, cx - r * 2, cy - r / 2, r * 4, r * 2, col);
    // Point
    for (int row = 0; row < r * 2; ++row) {
        int half = r * 2 - row;
        fRect(pm, cx - half, cy + r + row, half * 2, 1, col);
    }
}

// Search: magnifying glass
void IconAtlas::drawSearch(Pixmap& pm, int ox, int oy, int sz)
{
    int m = sz / 6;
    Color col(200, 200, 210, 255);
    int r = sz / 4;
    int cx = ox + sz / 2 - r / 4, cy = oy + sz / 2 - r / 4;
    oCirc(pm, cx, cy, r, col);
    oCirc(pm, cx, cy, r - 1, col);
    // Handle
    int hx = cx + r * 7 / 10, hy = cy + r * 7 / 10;
    line(pm, hx, hy, hx + r, hy + r, col);
    line(pm, hx + 1, hy, hx + r + 1, hy + r, col);
}

// Plus
void IconAtlas::drawPlus(Pixmap& pm, int ox, int oy, int sz)
{
    int cx = ox + sz / 2, cy = oy + sz / 2;
    Color col(100, 200, 100, 255);
    int arm = sz * 3 / 8, th = std::max(2, sz / 5);
    fRect(pm, cx - th / 2, cy - arm, th, arm * 2, col);
    fRect(pm, cx - arm, cy - th / 2, arm * 2, th, col);
}

// Minus
void IconAtlas::drawMinus(Pixmap& pm, int ox, int oy, int sz)
{
    int cx = ox + sz / 2, cy = oy + sz / 2;
    Color col(200, 100, 100, 255);
    int arm = sz * 3 / 8, th = std::max(2, sz / 5);
    fRect(pm, cx - arm, cy - th / 2, arm * 2, th, col);
}

// Check (tick mark)
void IconAtlas::drawCheck(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(80, 220, 100, 255);
    int m = sz / 5;
    // Short down stroke, then long up stroke
    line(pm, ox + m, oy + sz / 2, ox + sz * 2 / 5, oy + sz - m, col);
    line(pm, ox + m + 1, oy + sz / 2, ox + sz * 2 / 5 + 1, oy + sz - m, col);
    line(pm, ox + sz * 2 / 5, oy + sz - m, ox + sz - m, oy + m, col);
    line(pm, ox + sz * 2 / 5 + 1, oy + sz - m, ox + sz - m + 1, oy + m, col);
}

// Cross (X)
void IconAtlas::drawCross(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(220, 80, 80, 255);
    int m = sz / 5;
    line(pm, ox + m, oy + m, ox + sz - m, oy + sz - m, col);
    line(pm, ox + m + 1, oy + m, ox + sz - m + 1, oy + sz - m, col);
    line(pm, ox + sz - m, oy + m, ox + m, oy + sz - m, col);
    line(pm, ox + sz - m - 1, oy + m, ox + m - 1, oy + sz - m, col);
}

// ArrowRight: ▶
void IconAtlas::drawArrowRight(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(200, 200, 210, 255);
    int m = sz / 5;
    int cx = ox + sz / 2, cy = oy + sz / 2;
    for (int row = 0; row < sz - m * 2; ++row) {
        int y = oy + m + row;
        int halfW = (sz / 2 - m) - std::abs(row - (sz / 2 - m));
        fRect(pm, cx - halfW / 3, y, halfW, 1, col);
    }
}

// ArrowDown: ▼
void IconAtlas::drawArrowDown(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(200, 200, 210, 255);
    int m = sz / 5;
    for (int row = 0; row < sz / 2 - m; ++row) {
        int y = oy + sz / 3 + row;
        int halfW = (sz / 2 - m) - row;
        fRect(pm, ox + sz / 2 - halfW, y, halfW * 2, 1, col);
    }
}

// ArrowUp: ▲
void IconAtlas::drawArrowUp(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(200, 200, 210, 255);
    int m = sz / 5;
    for (int row = 0; row < sz / 2 - m; ++row) {
        int y = oy + sz * 2 / 3 - row;
        int halfW = (sz / 2 - m) - row;
        fRect(pm, ox + sz / 2 - halfW, y, halfW * 2, 1, col);
    }
}

// ArrowLeft: ◀
void IconAtlas::drawArrowLeft(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(200, 200, 210, 255);
    int m = sz / 5;
    int cx = ox + sz / 2;
    for (int row = 0; row < sz - m * 2; ++row) {
        int y = oy + m + row;
        int halfW = (sz / 2 - m) - std::abs(row - (sz / 2 - m));
        fRect(pm, cx - halfW * 2 / 3, y, halfW, 1, col);
    }
}

// Eye
void IconAtlas::drawEye(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(180, 200, 220, 255);
    Color pupil(60, 80, 120, 255);
    int cx = ox + sz / 2, cy = oy + sz / 2;
    // Lens shape (horizontal ellipse)
    for (int dx = -sz * 3 / 8; dx <= sz * 3 / 8; ++dx) {
        float t = static_cast<float>(dx) / (sz * 3 / 8);
        int h = static_cast<int>((1.0f - t * t) * sz / 4);
        fRect(pm, cx + dx, cy - h, 1, h * 2, col);
    }
    // Pupil
    fCirc(pm, cx, cy, sz / 8, pupil);
}

// EyeOff
void IconAtlas::drawEyeOff(Pixmap& pm, int ox, int oy, int sz)
{
    drawEye(pm, ox, oy, sz);
    Color slash(220, 80, 80, 255);
    int m = sz / 5;
    line(pm, ox + m, oy + m, ox + sz - m, oy + sz - m, slash);
    line(pm, ox + m + 1, oy + m, ox + sz - m + 1, oy + sz - m, slash);
}

// Lock
void IconAtlas::drawLock(Pixmap& pm, int ox, int oy, int sz)
{
    Color body(180, 160, 80, 255);
    Color ring(160, 140, 60, 255);
    int m = sz / 6;
    int bx = ox + m + sz / 10, by = oy + sz / 2;
    int bw = sz - m * 2 - sz / 5, bh = sz / 2 - m;
    fRect(pm, bx, by, bw, bh, body);
    // Shackle (arc at top)
    int scx = ox + sz / 2, scy = oy + sz / 2;
    int sr = bw / 3;
    oCirc(pm, scx, scy, sr, ring);
    oCirc(pm, scx, scy, sr - 1, ring);
    // Clear bottom half of shackle circle
    fRect(pm, scx - sr - 1, scy, sr * 2 + 2, sr + 2, Color(0, 0, 0, 0));
    // Re-draw body on top
    fRect(pm, bx, by, bw, bh, body);
}

// Unlock
void IconAtlas::drawUnlock(Pixmap& pm, int ox, int oy, int sz)
{
    drawLock(pm, ox, oy, sz);
    // Shift shackle left to indicate "open"
    Color ring(160, 140, 60, 255);
    int scx = ox + sz / 2 - sz / 8;
    int scy = oy + sz / 2;
    int sr = (sz - sz / 3 - sz / 5) / 3;
    oCirc(pm, scx, scy, sr, ring);
}

// Refresh: two curved arrows (simplified as circle + arrows)
void IconAtlas::drawRefresh(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(100, 180, 220, 255);
    int cx = ox + sz / 2, cy = oy + sz / 2;
    int r = sz * 3 / 8;
    oCirc(pm, cx, cy, r, col);
    oCirc(pm, cx, cy, r - 1, col);
    // Arrow tips at top and bottom
    int aw = std::max(2, sz / 6);
    fRect(pm, cx + r - aw, cy - r - aw / 2, aw * 2, aw, col); // top arrow
    fRect(pm, cx - r - aw, cy + r - aw / 2, aw * 2, aw, col); // bottom arrow
}

// Trash: bin shape
void IconAtlas::drawTrash(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(200, 100, 100, 255);
    Color lid(180, 80, 80, 255);
    int m = sz / 6;
    int bx = ox + m + sz / 10, by = oy + sz / 3;
    int bw = sz - m * 2 - sz / 5, bh = sz - sz / 3 - m;
    // Body
    fRect(pm, bx, by, bw, bh, col);
    // Lid
    fRect(pm, ox + m, oy + sz / 4, sz - m * 2, std::max(2, sz / 8), lid);
    // Handle on lid
    fRect(pm, ox + sz * 2 / 5, oy + sz / 6, sz / 5, std::max(2, sz / 8), lid);
    // Lines inside body
    int lw = std::max(1, sz / 12);
    for (int i = 0; i < 3; ++i) {
        int lx = bx + bw * (i + 1) / 4 - lw / 2;
        line(pm, lx, by + 2, lx, by + bh - 2, lid);
    }
}

// Edit: pencil
void IconAtlas::drawEdit(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(200, 180, 100, 255);
    Color tip(100, 80, 60, 255);
    int m = sz / 5;
    // Pencil body (diagonal line)
    line(pm, ox + sz - m, oy + m, ox + m + 2, oy + sz - m - 2, col);
    line(pm, ox + sz - m - 1, oy + m, ox + m + 1, oy + sz - m - 2, col);
    line(pm, ox + sz - m + 1, oy + m, ox + m + 3, oy + sz - m - 2, col);
    // Tip
    fRect(pm, ox + m, oy + sz - m - 1, 3, 3, tip);
}

// Home: house shape
void IconAtlas::drawHome(Pixmap& pm, int ox, int oy, int sz)
{
    Color wall(180, 170, 160, 255);
    Color roof(140, 100, 80, 255);
    int m = sz / 6;
    // Body
    int bx = ox + m + sz / 10, by = oy + sz / 2;
    int bw = sz - m * 2 - sz / 5, bh = sz / 2 - m;
    fRect(pm, bx, by, bw, bh, wall);
    // Roof (triangle via scanlines)
    int rcx = ox + sz / 2;
    int rTop = oy + m;
    int rBot = oy + sz / 2 + 1;
    for (int y = rTop; y < rBot; ++y) {
        float t = static_cast<float>(y - rTop) / (rBot - rTop);
        int halfW = static_cast<int>(t * bw / 2);
        fRect(pm, rcx - halfW, y, halfW * 2, 1, roof);
    }
    // Door
    int dw = bw / 3, dh = bh * 2 / 3;
    fRect(pm, ox + sz / 2 - dw / 2, by + bh - dh, dw, dh, Color(100, 80, 60, 255));
}

// User: head + body silhouette
void IconAtlas::drawUser(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(160, 180, 200, 255);
    int cx = ox + sz / 2, cy = oy + sz / 3;
    // Head
    fCirc(pm, cx, cy, sz / 5, col);
    // Body (half-ellipse below)
    for (int dx = -sz / 3; dx <= sz / 3; ++dx) {
        float t = static_cast<float>(dx) / (sz / 3);
        int h = static_cast<int>((1.0f - t * t) * sz / 4);
        fRect(pm, cx + dx, oy + sz / 2, 1, h, col);
    }
}

// Warning: yellow triangle with !
void IconAtlas::drawWarning(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(240, 200, 50, 255);
    Color bang(60, 50, 10, 255);
    int m = sz / 8;
    int cx = ox + sz / 2;
    int top = oy + m;
    int bot = oy + sz - m;
    for (int y = top; y < bot; ++y) {
        float t = static_cast<float>(y - top) / (bot - top);
        int halfW = static_cast<int>(t * (sz / 2 - m));
        fRect(pm, cx - halfW, y, halfW * 2, 1, col);
    }
    // Exclamation mark
    int ew = std::max(1, sz / 8);
    fRect(pm, cx - ew / 2, oy + sz / 3, ew, sz / 4, bang);
    fRect(pm, cx - ew / 2, oy + sz * 2 / 3, ew, ew, bang);
}

// Info: blue circle with i
void IconAtlas::drawInfo(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(60, 140, 220, 255);
    Color txt(255, 255, 255, 255);
    int cx = ox + sz / 2, cy = oy + sz / 2;
    fCirc(pm, cx, cy, sz * 3 / 8, col);
    int tw = std::max(1, sz / 8);
    fRect(pm, cx - tw / 2, cy - tw / 2, tw, sz / 4, txt);      // stem
    fRect(pm, cx - tw / 2, cy - sz / 5 - tw, tw, tw, txt);      // dot
}

// Error: red circle with X
void IconAtlas::drawError(Pixmap& pm, int ox, int oy, int sz)
{
    Color col(200, 50, 50, 255);
    Color x(255, 255, 255, 255);
    int cx = ox + sz / 2, cy = oy + sz / 2;
    fCirc(pm, cx, cy, sz * 3 / 8, col);
    int m = sz / 4;
    line(pm, cx - m, cy - m, cx + m, cy + m, x);
    line(pm, cx - m + 1, cy - m, cx + m + 1, cy + m, x);
    line(pm, cx + m, cy - m, cx - m, cy + m, x);
    line(pm, cx + m - 1, cy - m, cx - m - 1, cy + m, x);
}

// ═════════════════════════════════════════════════════════════════════════════
//  New icons for FileDialog
// ═════════════════════════════════════════════════════════════════════════════

// FileImage: file with a mountain/sun landscape
void IconAtlas::drawFileImage(Pixmap& pm, int ox, int oy, int sz)
{
    int m = sz / 8;
    Color body(190, 210, 230, 255);
    Color fold(140, 160, 180, 255);
    Color mountain(80, 180, 100, 255);
    Color sun(240, 200, 60, 255);
    int bx = ox + m + sz / 8, by = oy + m;
    int bw = sz - m * 2 - sz / 8, bh = sz - m * 2;
    int foldSz = bw / 3;
    // File body
    fRect(pm, bx, by, bw, bh, body);
    fRect(pm, bx + bw - foldSz, by, foldSz, foldSz, fold);
    // Sun (small circle)
    int sr = std::max(1, bw / 6);
    fCirc(pm, bx + bw / 4, by + bh / 3, sr, sun);
    // Mountain triangle (bottom-left to center to bottom-right)
    int mBot = by + bh - 2;
    int mTop = by + bh / 2;
    int mLeft = bx + 2;
    int mRight = bx + bw - 2;
    int mPeak = bx + bw * 2 / 3;
    for (int row = mTop; row <= mBot; ++row) {
        float t = static_cast<float>(row - mTop) / std::max(1, mBot - mTop);
        int lx = mPeak - static_cast<int>(t * (mPeak - mLeft));
        int rx = mPeak + static_cast<int>(t * (mRight - mPeak));
        fRect(pm, lx, row, rx - lx, 1, mountain);
    }
}

// FileArchive: file with zipper pattern
void IconAtlas::drawFileArchive(Pixmap& pm, int ox, int oy, int sz)
{
    int m = sz / 8;
    Color body(190, 190, 200, 255);
    Color fold(140, 140, 160, 255);
    Color zip(120, 100, 80, 255);
    int bx = ox + m + sz / 8, by = oy + m;
    int bw = sz - m * 2 - sz / 8, bh = sz - m * 2;
    int foldSz = bw / 3;
    fRect(pm, bx, by, bw, bh, body);
    fRect(pm, bx + bw - foldSz, by, foldSz, foldSz, fold);
    // Zipper: alternating small rects down the center
    int zw = std::max(1, bw / 5);
    int zh = std::max(1, bh / 10);
    int zx = bx + bw / 2 - zw / 2;
    for (int i = 0; i < 5; ++i) {
        int zy = by + foldSz + i * (zh + zh / 2);
        if (zy + zh > by + bh - 1) break;
        int xoff = (i % 2 == 0) ? -zw / 2 : zw / 2;
        fRect(pm, zx + xoff, zy, zw, zh, zip);
    }
}

// ViewDetail: 3 horizontal lines with a short prefix column (table icon)
void IconAtlas::drawViewDetail(Pixmap& pm, int ox, int oy, int sz)
{
    int m = sz / 5;
    Color col1(160, 180, 220, 255);
    Color col2(130, 150, 180, 255);
    int lh = std::max(1, sz / 8);
    int gap = (sz - m * 2) / 4;
    int lx = ox + m;
    int lw = sz - m * 2;
    int colW = lw / 4;
    for (int i = 0; i < 3; ++i) {
        int ly = oy + m + gap * (i + 1) - lh / 2;
        fRect(pm, lx, ly, colW - 1, lh, col2);
        fRect(pm, lx + colW + 1, ly, lw - colW - 1, lh, col1);
    }
}

// ViewList: 3 rows of 2 short segments (compact list icon)
void IconAtlas::drawViewList(Pixmap& pm, int ox, int oy, int sz)
{
    int m = sz / 5;
    Color bar(160, 180, 220, 255);
    Color dot(100, 140, 200, 255);
    int lh = std::max(1, sz / 8);
    int gap = (sz - m * 2) / 4;
    int lx = ox + m;
    int lw = sz - m * 2;
    int dotW = std::max(1, lh);
    for (int i = 0; i < 3; ++i) {
        int ly = oy + m + gap * (i + 1) - lh / 2;
        fRect(pm, lx, ly, dotW, lh, dot);
        fRect(pm, lx + dotW + 2, ly, lw - dotW - 2, lh, bar);
    }
}

// ViewGrid: 2x2 grid of squares (grid/icon view)
void IconAtlas::drawViewGrid(Pixmap& pm, int ox, int oy, int sz)
{
    int m = sz / 5;
    Color cell(140, 170, 210, 255);
    int inner = sz - m * 2;
    int gap = std::max(1, inner / 8);
    int cellSz = (inner - gap) / 2;
    int x0 = ox + m, y0 = oy + m;
    fRect(pm, x0,              y0,              cellSz, cellSz, cell);
    fRect(pm, x0 + cellSz + gap, y0,           cellSz, cellSz, cell);
    fRect(pm, x0,              y0 + cellSz + gap, cellSz, cellSz, cell);
    fRect(pm, x0 + cellSz + gap, y0 + cellSz + gap, cellSz, cellSz, cell);
}
