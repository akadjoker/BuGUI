#include "NodeEditor.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>

// ═════════════════════════════════════════════════════════════════════════════
//  NodeEditor
// ═════════════════════════════════════════════════════════════════════════════

NodeEditor::NodeEditor()
{
}

// ─────────────────────────────────────────────────────────────────────────────
//  Node/Pin/Link management
// ─────────────────────────────────────────────────────────────────────────────

int NodeEditor::addNode(const std::string& title, float x, float y)
{
    Node n;
    n.title = title;
    n.x = x; n.y = y;
    nodes_.push_back(std::move(n));
    markDirty();
    return static_cast<int>(nodes_.size()) - 1;
}

void NodeEditor::removeNode(int nodeId)
{
    if (nodeId < 0 || nodeId >= (int)nodes_.size()) return;
    // Remove links referencing this node
    links_.erase(std::remove_if(links_.begin(), links_.end(),
        [nodeId](const Link& l) { return l.srcNode == nodeId || l.dstNode == nodeId; }),
        links_.end());
    nodes_.erase(nodes_.begin() + nodeId);
    // Fix link indices
    for (auto& l : links_) {
        if (l.srcNode > nodeId) --l.srcNode;
        if (l.dstNode > nodeId) --l.dstNode;
    }
    markDirty();
}

void NodeEditor::clearAll()
{
    nodes_.clear();
    links_.clear();
    markDirty();
}

int NodeEditor::addPin(int nodeId, const std::string& name, PinDir dir, PinType type)
{
    if (nodeId < 0 || nodeId >= (int)nodes_.size()) return -1;
    auto& n = nodes_[nodeId];
    n.pins.push_back({name, dir, type});
    n.h = nodeHeight(n);
    markDirty();
    return static_cast<int>(n.pins.size()) - 1;
}

int NodeEditor::addLink(int srcNode, int srcPin, int dstNode, int dstPin)
{
    if (!canLink(srcNode, srcPin, dstNode, dstPin)) return -1;
    Link l{srcNode, srcPin, dstNode, dstPin};
    links_.push_back(l);
    markDirty();
    int id = static_cast<int>(links_.size()) - 1;
    onLinkCreated.emit(id, (srcNode << 16) | dstNode);
    return id;
}

void NodeEditor::removeLink(int linkId)
{
    if (linkId < 0 || linkId >= (int)links_.size()) return;
    links_.erase(links_.begin() + linkId);
    markDirty();
    onLinkRemoved.emit(linkId);
}

bool NodeEditor::canLink(int srcNode, int srcPin, int dstNode, int dstPin) const
{
    if (srcNode < 0 || srcNode >= (int)nodes_.size()) return false;
    if (dstNode < 0 || dstNode >= (int)nodes_.size()) return false;
    if (srcNode == dstNode) return false;
    auto& sn = nodes_[srcNode];
    auto& dn = nodes_[dstNode];
    if (srcPin < 0 || srcPin >= (int)sn.pins.size()) return false;
    if (dstPin < 0 || dstPin >= (int)dn.pins.size()) return false;
    if (sn.pins[srcPin].dir != PinDir::Output) return false;
    if (dn.pins[dstPin].dir != PinDir::Input) return false;
    // Type compatibility: Any matches anything, otherwise must match
    auto st = sn.pins[srcPin].type, dt = dn.pins[dstPin].type;
    if (st != PinType::Any && dt != PinType::Any && st != dt) return false;
    // Check no duplicate
    for (auto& l : links_)
        if (l.dstNode == dstNode && l.dstPin == dstPin) return false;
    return true;
}

void NodeEditor::fitView()
{
    if (nodes_.empty()) { panX_ = 0; panY_ = 0; zoom_ = 1; return; }
    float cx = 0, cy = 0;
    for (auto& n : nodes_) { cx += n.x + n.w * 0.5f; cy += n.y + nodeHeight(n) * 0.5f; }
    cx /= nodes_.size(); cy /= nodes_.size();
    Rect b = absoluteRect();
    panX_ = cx - (b.w * 0.5f) / zoom_;
    panY_ = cy - (b.h * 0.5f) / zoom_;
    markDirty();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Coordinate helpers
// ─────────────────────────────────────────────────────────────────────────────

float NodeEditor::gx(float x) const { Rect b = absoluteRect(); return b.x + (x - panX_) * zoom_; }
float NodeEditor::gy(float y) const { Rect b = absoluteRect(); return b.y + (y - panY_) * zoom_; }
float NodeEditor::sx(float x) const { Rect b = absoluteRect(); return (x - b.x) / zoom_ + panX_; }
float NodeEditor::sy(float y) const { Rect b = absoluteRect(); return (y - b.y) / zoom_ + panY_; }

float NodeEditor::nodeHeight(const Node& n) const
{
    int inputs = 0, outputs = 0;
    for (auto& p : n.pins) {
        if (p.dir == PinDir::Input) ++inputs;
        else ++outputs;
    }
    int rows = std::max(inputs, outputs);
    return kNodeHeaderH + rows * kPinH + 8;
}

void NodeEditor::getPinPos(int nodeId, int pinIdx, float& px, float& py) const
{
    auto& n = nodes_[nodeId];
    auto& pin = n.pins[pinIdx];

    int inputIdx = 0, outputIdx = 0;
    for (int i = 0; i < pinIdx; ++i) {
        if (n.pins[i].dir == PinDir::Input) ++inputIdx;
        else ++outputIdx;
    }

    if (pin.dir == PinDir::Input) {
        px = gx(n.x);
        py = gy(n.y + kNodeHeaderH + inputIdx * kPinH + kPinH * 0.5f);
    } else {
        px = gx(n.x + n.w);
        py = gy(n.y + kNodeHeaderH + outputIdx * kPinH + kPinH * 0.5f);
    }
}

Color NodeEditor::pinColor(PinType t) const
{
    switch (t) {
        case PinType::Float:  return Color(160, 200, 100, 255);
        case PinType::Vec2:   return Color(100, 200, 160, 255);
        case PinType::Vec3:   return Color(100, 160, 220, 255);
        case PinType::Vec4:   return Color(180, 100, 220, 255);
        case PinType::Color:  return Color(255, 200, 60, 255);
        case PinType::Bool:   return Color(220, 100, 100, 255);
        case PinType::Int:    return Color(100, 220, 200, 255);
        case PinType::String: return Color(200, 180, 140, 255);
        case PinType::Any:    return Color(180, 180, 180, 255);
    }
    return Color(180, 180, 180, 255);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hit testing
// ─────────────────────────────────────────────────────────────────────────────

NodeEditor::NodeHit NodeEditor::hitTest(float mx, float my) const
{
    float pinR = kPinRadius * zoom_ + 4;

    // Check nodes in reverse (topmost first)
    for (int i = (int)nodes_.size() - 1; i >= 0; --i) {
        auto& n = nodes_[i];
        float nh = nodeHeight(n);

        // Check pins first
        for (int pi = 0; pi < (int)n.pins.size(); ++pi) {
            float px, py;
            getPinPos(i, pi, px, py);
            if ((mx - px) * (mx - px) + (my - py) * (my - py) < pinR * pinR)
                return {i, pi, false};
        }

        // Check node body
        float nx = gx(n.x), ny = gy(n.y);
        float nw = n.w * zoom_, nhz = nh * zoom_;
        if (mx >= nx && mx <= nx + nw && my >= ny && my <= ny + nhz) {
            bool onHeader = (my < ny + kNodeHeaderH * zoom_);
            return {i, -1, onHeader};
        }
    }
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse interaction
// ─────────────────────────────────────────────────────────────────────────────

void NodeEditor::onMousePress(MouseEvent& e)
{
    if (e.button == 1) { // middle → pan
        dragMode_   = DragMode::Pan;
        dragStartX_ = e.x;  dragStartY_ = e.y;
        panStartX_  = panX_; panStartY_ = panY_;
        e.consumed  = true;
        return;
    }

    if (e.button == 2) { // right-click on link → delete
        // Check if near a link bezier
        e.consumed = true;
        return;
    }

    if (e.button != 0) return;

    auto hit = hitTest(e.x, e.y);

    if (hit.pin >= 0) {
        // Start connection drag from pin
        dragMode_    = DragMode::ConnectPin;
        connSrcNode_ = hit.node;
        connSrcPin_  = hit.pin;
        connEndX_    = e.x;
        connEndY_    = e.y;
        e.consumed   = true;
        return;
    }

    if (hit.node >= 0) {
        for (auto& n : nodes_) n.selected = false;
        nodes_[hit.node].selected = true;
        onNodeSelected.emit(hit.node);

        dragMode_   = DragMode::MoveNode;
        dragNode_   = hit.node;
        dragStartX_ = e.x;
        dragStartY_ = e.y;
        nodeStartX_ = nodes_[hit.node].x;
        nodeStartY_ = nodes_[hit.node].y;
        e.consumed  = true;
        return;
    }

    // Click on background — deselect
    for (auto& n : nodes_) n.selected = false;
    // Start pan on left-click on empty area
    dragMode_   = DragMode::Pan;
    dragStartX_ = e.x;  dragStartY_ = e.y;
    panStartX_  = panX_; panStartY_ = panY_;
    e.consumed  = true;
}

void NodeEditor::onMouseRelease(MouseEvent& e)
{
    if (dragMode_ == DragMode::ConnectPin) {
        // Try to complete connection
        auto hit = hitTest(e.x, e.y);
        if (hit.pin >= 0 && hit.node != connSrcNode_) {
            // Determine src/dst based on pin direction
            auto& srcPin = nodes_[connSrcNode_].pins[connSrcPin_];
            if (srcPin.dir == PinDir::Output)
                addLink(connSrcNode_, connSrcPin_, hit.node, hit.pin);
            else
                addLink(hit.node, hit.pin, connSrcNode_, connSrcPin_);
        }
    }

    dragMode_ = DragMode::None;
    dragNode_ = -1;
    connSrcNode_ = -1;
    connSrcPin_  = -1;
    markDirty();
    e.consumed = true;
}

void NodeEditor::onMouseMove(MouseEvent& e)
{
    if (dragMode_ == DragMode::Pan) {
        float dx = (e.x - dragStartX_) / zoom_;
        float dy = (e.y - dragStartY_) / zoom_;
        panX_ = panStartX_ - dx;
        panY_ = panStartY_ - dy;
        markDirty();
        e.consumed = true;
    }
    else if (dragMode_ == DragMode::MoveNode && dragNode_ >= 0) {
        float dx = (e.x - dragStartX_) / zoom_;
        float dy = (e.y - dragStartY_) / zoom_;
        float nx = nodeStartX_ + dx;
        float ny = nodeStartY_ + dy;
        if (gridSnap_ > 0) {
            nx = std::round(nx / gridSnap_) * gridSnap_;
            ny = std::round(ny / gridSnap_) * gridSnap_;
        }
        nodes_[dragNode_].x = nx;
        nodes_[dragNode_].y = ny;
        markDirty();
        e.consumed = true;
    }
    else if (dragMode_ == DragMode::ConnectPin) {
        connEndX_ = e.x;
        connEndY_ = e.y;
        markDirty();
        e.consumed = true;
    }
}

void NodeEditor::onMouseScroll(MouseEvent& e)
{
    float oldZoom = zoom_;
    float factor  = (e.scrollY > 0) ? 1.1f : 0.9f;
    zoom_ *= factor;
    zoom_ = std::max(0.2f, std::min(4.0f, zoom_));

    // Zoom towards mouse position
    Rect b = absoluteRect();
    float mx = (e.x - b.x) / oldZoom + panX_;
    float my = (e.y - b.y) / oldZoom + panY_;
    panX_ = mx - (e.x - b.x) / zoom_;
    panY_ = my - (e.y - b.y) / zoom_;

    markDirty();
    e.consumed = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Painting
// ─────────────────────────────────────────────────────────────────────────────

void NodeEditor::paint(PaintContext& ctx)
{
    if (!visible_) return;
    Rect b = absoluteRect();

    ctx.pushClip(b);

    // Background
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    ctx.fill.Rectangle(b.x, b.y, b.w, b.h, true);

    paintGrid(ctx, b);
    paintLinks(ctx);
    paintNodes(ctx);
    if (dragMode_ == DragMode::ConnectPin) paintDragLink(ctx);

    // Border
    ctx.fill.SetColor(50, 52, 58, 255);
    ctx.fill.Rectangle((int)b.x,           (int)b.y,            (int)b.w, 1,      true);
    ctx.fill.Rectangle((int)b.x,           (int)(b.y + b.h - 1),(int)b.w, 1,      true);
    ctx.fill.Rectangle((int)b.x,           (int)b.y,            1,       (int)b.h, true);
    ctx.fill.Rectangle((int)(b.x + b.w -1),(int)b.y,            1,       (int)b.h, true);

    ctx.popClip();

    Widget::paint(ctx);
}

void NodeEditor::paintGrid(PaintContext& ctx, const Rect& b)
{
    float step = gridSnap_ * zoom_;
    if (step < 8) step *= std::ceil(8.0f / step);

    float offX = std::fmod(-(panX_ * zoom_), step);
    float offY = std::fmod(-(panY_ * zoom_), step);
    if (offX < 0) offX += step;
    if (offY < 0) offY += step;

    ctx.fill.SetColor(40, 42, 48, 255);
    for (float x = b.x + offX; x < b.x + b.w; x += step) {
        ctx.fill.Rectangle((int)x, (int)b.y, 1, (int)b.h, true);
    }
    for (float y = b.y + offY; y < b.y + b.h; y += step) {
        ctx.fill.Rectangle((int)b.x, (int)y, (int)b.w, 1, true);
    }

    // Major grid
    float major = step * 4;
    float mOffX = std::fmod(-(panX_ * zoom_), major);
    float mOffY = std::fmod(-(panY_ * zoom_), major);
    if (mOffX < 0) mOffX += major;
    if (mOffY < 0) mOffY += major;

    ctx.fill.SetColor(50, 52, 58, 255);
    for (float x = b.x + mOffX; x < b.x + b.w; x += major) {
        ctx.fill.Rectangle((int)x, (int)b.y, 1, (int)b.h, true);
    }
    for (float y = b.y + mOffY; y < b.y + b.h; y += major) {
        ctx.fill.Rectangle((int)b.x, (int)y, (int)b.w, 1, true);
    }
}

void NodeEditor::paintLinks(PaintContext& ctx)
{
    for (auto& link : links_) {
        float x0, y0, x1, y1;
        getPinPos(link.srcNode, link.srcPin, x0, y0);
        getPinPos(link.dstNode, link.dstPin, x1, y1);

        Color col = pinColor(nodes_[link.srcNode].pins[link.srcPin].type);

        // Bezier curve
        float dist = std::fabs(x1 - x0) * 0.5f;
        if (dist < 30) dist = 30;
        int segs = 24;

        float prevX = x0, prevY = y0;
        for (int i = 1; i <= segs; ++i) {
            float t = float(i) / segs;
            float it = 1 - t;
            // Control points: horizontal tangent
            float cx0 = x0 + dist, cy0 = y0;
            float cx1 = x1 - dist, cy1 = y1;
            float bx = it*it*it*x0 + 3*it*it*t*cx0 + 3*it*t*t*cx1 + t*t*t*x1;
            float by = it*it*it*y0 + 3*it*it*t*cy0 + 3*it*t*t*cy1 + t*t*t*y1;

            // Thick line segment
            float dx = bx - prevX, dy = by - prevY;
            float len = std::sqrt(dx*dx + dy*dy);
            if (len > 0.5f) {
                float thick = 2.0f;
                float nx = -dy/len * thick * 0.5f;
                float ny =  dx/len * thick * 0.5f;
                ctx.fill.SetColor(col.r, col.g, col.b, 200);
                ctx.fill.Triangle(prevX+nx, prevY+ny, prevX-nx, prevY-ny, bx-nx, by-ny, true);
                ctx.fill.Triangle(prevX+nx, prevY+ny, bx-nx, by-ny, bx+nx, by+ny, true);
            }
            prevX = bx; prevY = by;
        }
    }
}

void NodeEditor::paintNodes(PaintContext& ctx)
{
    for (int ni = 0; ni < (int)nodes_.size(); ++ni) {
        auto& n = nodes_[ni];
        float nh = nodeHeight(n);
        float nx = gx(n.x), ny = gy(n.y);
        float nw = n.w * zoom_, nhz = nh * zoom_;
        float hdrH = kNodeHeaderH * zoom_;

        // Shadow
        ctx.fill.SetColor(0, 0, 0, 50);
        ctx.fill.Rectangle(nx + 3, ny + 3, nw, nhz, true);

        // Body
        Color body = n.selected ? Color(50, 55, 65, 240) : Color(40, 42, 48, 240);
        ctx.fill.SetColor(body.r, body.g, body.b, body.a);
        ctx.fill.Rectangle(nx, ny, nw, nhz, true);

        // Header
        Color hdr = n.headerColor;
        if (n.selected) { hdr.r = std::min(255, hdr.r + 30); hdr.g = std::min(255, hdr.g + 30); hdr.b = std::min(255, hdr.b + 30); }
        ctx.fill.SetColor(hdr.r, hdr.g, hdr.b, hdr.a);
        ctx.fill.Rectangle(nx, ny, nw, hdrH, true);

        // Title
        ctx.font.SetFontSize(11.0f * zoom_);
        ctx.font.SetBatch(&ctx.text);
        ctx.font.SetColor(Color(230, 232, 240, 255));
        ctx.font.Print(n.title.c_str(), nx + 8 * zoom_, ny + 5 * zoom_);

        // Border (4 thin fill rects)
        Color border = n.selected ? Color(255, 200, 60, 255) : Color(60, 62, 70, 255);
        ctx.fill.SetColor(border.r, border.g, border.b, border.a);
        ctx.fill.Rectangle((int)nx,          (int)ny,           (int)nw, 1,        true);
        ctx.fill.Rectangle((int)nx,          (int)(ny + nhz -1),(int)nw, 1,        true);
        ctx.fill.Rectangle((int)nx,          (int)ny,           1,       (int)nhz, true);
        ctx.fill.Rectangle((int)(nx + nw -1),(int)ny,           1,       (int)nhz, true);

        // Pins
        int inputIdx = 0, outputIdx = 0;
        for (int pi = 0; pi < (int)n.pins.size(); ++pi) {
            float px, py;
            getPinPos(ni, pi, px, py);
            Color pc = pinColor(n.pins[pi].type);

            // Circle (outline = larger circle behind, both in fill)
            ctx.fill.SetColor(220, 222, 230, 200);
            ctx.fill.Circle(px, py, kPinRadius * zoom_ + 1.0f, true);
            ctx.fill.SetColor(pc.r, pc.g, pc.b, 255);
            ctx.fill.Circle(px, py, kPinRadius * zoom_, true);

            // Label
            ctx.font.SetFontSize(10.0f * zoom_);
            ctx.font.SetColor(Color(190, 192, 200, 220));
            if (n.pins[pi].dir == PinDir::Input)
                ctx.font.Print(n.pins[pi].name.c_str(), px + (kPinRadius + 4) * zoom_, py - 5 * zoom_);
            else
                ctx.font.Print(n.pins[pi].name.c_str(), px - (kPinRadius + 30) * zoom_, py - 5 * zoom_);
        }
    }
}

void NodeEditor::paintDragLink(PaintContext& ctx)
{
    if (connSrcNode_ < 0 || connSrcPin_ < 0) return;

    float x0, y0;
    getPinPos(connSrcNode_, connSrcPin_, x0, y0);
    float x1 = connEndX_, y1 = connEndY_;

    Color col = pinColor(nodes_[connSrcNode_].pins[connSrcPin_].type);

    float dist = std::fabs(x1 - x0) * 0.5f;
    if (dist < 30) dist = 30;
    bool isOutput = (nodes_[connSrcNode_].pins[connSrcPin_].dir == PinDir::Output);
    float cx0 = isOutput ? x0 + dist : x0 - dist;
    float cx1 = isOutput ? x1 - dist : x1 + dist;

    int segs = 20;
    float prevX = x0, prevY = y0;
    for (int i = 1; i <= segs; ++i) {
        float t = float(i) / segs;
        float it = 1 - t;
        float bx = it*it*it*x0 + 3*it*it*t*cx0 + 3*it*t*t*cx1 + t*t*t*x1;
        float by = it*it*it*y0 + 3*it*it*t*y0  + 3*it*t*t*y1  + t*t*t*y1;

        float dx = bx - prevX, dy = by - prevY;
        float len = std::sqrt(dx*dx + dy*dy);
        if (len > 0.5f) {
            float thick = 1.5f;
            float nx = -dy/len * thick * 0.5f;
            float ny =  dx/len * thick * 0.5f;
            ctx.fill.SetColor(col.r, col.g, col.b, 180);
            ctx.fill.Triangle(prevX+nx, prevY+ny, prevX-nx, prevY-ny, bx-nx, by-ny, true);
            ctx.fill.Triangle(prevX+nx, prevY+ny, bx-nx, by-ny, bx+nx, by+ny, true);
        }
        prevX = bx; prevY = by;
    }
}
