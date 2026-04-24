#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include "Signal.hpp"
#include <vector>
#include <string>
#include <functional>

// ═════════════════════════════════════════════════════════════════════════════
//  NodeEditor - Visual node graph (for shaders, AI, scripting)
//
//  Features:
//  - Nodes with typed input/output pins
//  - Drag to connect pins (type-checked by color)
//  - Pan (middle-drag) + zoom (scroll wheel)
//  - Selection, multi-select (Shift+click)
//  - Node drag with snap-to-grid
//  - Curved connection lines (Bezier)
//
//  Usage:
//    auto* editor = parent->createChild<NodeEditor>();
//    int n1 = editor->addNode("Add", 100, 100);
//    editor->addPin(n1, "A", PinDir::Input, PinType::Float);
//    editor->addPin(n1, "B", PinDir::Input, PinType::Float);
//    editor->addPin(n1, "Result", PinDir::Output, PinType::Float);
//    editor->addLink(n1, 2, n2, 0); // connect output pin 2 of n1 to input pin 0 of n2
// ═════════════════════════════════════════════════════════════════════════════

enum class PinDir  { Input, Output };
enum class PinType { Float, Vec2, Vec3, Vec4, Color, Bool, Int, String, Any };

struct Pin {
    std::string name;
    PinDir      dir;
    PinType     type;
};

struct Node {
    std::string    title;
    float          x = 0, y = 0;    // position in graph space
    float          w = 140, h = 0;  // width fixed, height computed
    std::vector<Pin> pins;
    Color          headerColor = Color(70, 90, 130, 255);
    bool           selected = false;
    bool           collapsed = false;
};

struct Link {
    int srcNode = -1, srcPin = -1;
    int dstNode = -1, dstPin = -1;
};

class NodeEditor : public Widget
{
public:
    NodeEditor();

    // ── Node management ──────────────────────────────────────────────────

    int  addNode(const std::string& title, float x, float y);
    void removeNode(int nodeId);
    void clearAll();

    int  nodeCount() const { return static_cast<int>(nodes_.size()); }
    Node&       node(int id)       { return nodes_[id]; }
    const Node& node(int id) const { return nodes_[id]; }

    void setNodeHeader(int nodeId, const Color& c) { nodes_[nodeId].headerColor = c; markDirty(); }

    // ── Pin management ───────────────────────────────────────────────────

    int  addPin(int nodeId, const std::string& name, PinDir dir, PinType type);

    // ── Link management ──────────────────────────────────────────────────

    int  addLink(int srcNode, int srcPin, int dstNode, int dstPin);
    void removeLink(int linkId);
    bool canLink(int srcNode, int srcPin, int dstNode, int dstPin) const;

    int  linkCount() const { return static_cast<int>(links_.size()); }

    // ── View ─────────────────────────────────────────────────────────────

    void setGridSnap(float s) { gridSnap_ = s; }
    void fitView();

    float panX() const { return panX_; }
    float panY() const { return panY_; }
    float zoom() const { return zoom_; }

    // ── Signals ──────────────────────────────────────────────────────────

    Signal<int>       onNodeSelected;   // nodeId
    Signal<int, int>  onLinkCreated;    // linkId, (srcNode<<16|dstNode)
    Signal<int>       onLinkRemoved;    // linkId

    // ── Overrides ────────────────────────────────────────────────────────

    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;

    void setBgColor(const Color& c) { bgColor_ = c; }

private:
    std::vector<Node> nodes_;
    std::vector<Link> links_;
    Color bgColor_ = Color(32, 34, 38, 255);

    // View
    float panX_ = 0, panY_ = 0;
    float zoom_ = 1.0f;
    float gridSnap_ = 16.0f;

    // Interaction state
    enum class DragMode { None, Pan, MoveNode, ConnectPin };
    DragMode dragMode_ = DragMode::None;
    float    dragStartX_ = 0, dragStartY_ = 0;
    float    panStartX_ = 0, panStartY_ = 0;
    int      dragNode_ = -1;
    float    nodeStartX_ = 0, nodeStartY_ = 0;
    // For pin connection
    int      connSrcNode_ = -1, connSrcPin_ = -1;
    float    connEndX_ = 0, connEndY_ = 0;

    static constexpr float kNodeHeaderH = 24;
    static constexpr float kPinH        = 20;
    static constexpr float kPinRadius   = 5;

    // ── Helpers ──────────────────────────────────────────────────────────

    // Graph space → screen
    float gx(float x) const;
    float gy(float y) const;
    // Screen → graph
    float sx(float x) const;
    float sy(float y) const;

    float nodeHeight(const Node& n) const;
    void  getPinPos(int nodeId, int pinIdx, float& px, float& py) const;
    Color pinColor(PinType t) const;

    struct NodeHit { int node = -1; int pin = -1; bool onHeader = false; };
    NodeHit hitTest(float mx, float my) const;

    void paintGrid(PaintContext& ctx, const Rect& b);
    void paintLinks(PaintContext& ctx);
    void paintNodes(PaintContext& ctx);
    void paintDragLink(PaintContext& ctx);
};
