#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
//  DockSide - where to dock relative to a panel
// ─────────────────────────────────────────────────────────────────────────────
enum class DockSide { Left, Right, Top, Bottom, Center };

// ─────────────────────────────────────────────────────────────────────────────
//  DockNode - internal tree node (not a Widget)
//  Either a split with two children, or a leaf with N tab entries.
//  Rects stored in DockPanel-local coordinates.
// ─────────────────────────────────────────────────────────────────────────────
struct DockNode
{
    bool isSplit = false;

    // ── Split ─────────────────────────────────────────────────────────────
    LayoutDir  splitDir = LayoutDir::Horizontal;
    float      ratio    = 0.5f;   // fraction for first child
    DockNode*  first    = nullptr;  // owned, delete in freeNode
    DockNode*  second   = nullptr;  // owned, delete in freeNode

    // ── Leaf ──────────────────────────────────────────────────────────────
    struct Tab {
        std::string name;
        Widget*     content      = nullptr;
        float       cachedWidth  = 0.f;   // measured during paint, used in hit-test
    };
    std::vector<Tab> tabs;
    int currentTab = 0;

    // DockPanel-local geometry (assigned in layoutNode)
    Rect rect = {};

    bool isEmpty() const { return !isSplit && tabs.empty(); }

    // Find the leaf that contains 'name' in this subtree.
    // If outIdx != nullptr, writes the tab index there.
    DockNode* findTab(const std::string& name, int* outIdx = nullptr);
};

// ─────────────────────────────────────────────────────────────────────────────
//  DockPanel - docking layout system
//
//  Usage:
//    auto* dock = stage->createChild<DockPanel>();
//    dock->setStretch(1);
//    dock->addPanel("Scene",      sceneWidget);
//    dock->addPanel("Properties", propWidget);
//    dock->addPanel("Console",    consoleWidget);
//    dock->splitOff("Properties", DockSide::Right,  0.28f);
//    dock->splitOff("Console",    DockSide::Bottom,  0.30f);
//
//  At runtime the user can:
//     Click a tab to bring that panel to the front
//     Drag a tab and drop it on another panel to merge or split
//     Drag the splitter handles to resize panels
// ─────────────────────────────────────────────────────────────────────────────
class DockPanel : public Widget
{
public:
    DockPanel() = default;
    ~DockPanel() override;

    // ── Panel management ──────────────────────────────────────────────────

    // Add 'content' as a new panel. All panels start in the root leaf.
    // 'content' is adopted as a child widget of DockPanel.
    void addPanel(const std::string& name, Widget* content);

    // Move the named panel to a new leaf on 'side' of its current leaf.
    // 'ratio' is the fraction of space given to the NEW leaf.
    // No-op if the panel is the only one in its leaf (splitOff only makes
    // sense when at least 2 tabs are present).
    void splitOff(const std::string& tabName, DockSide side, float ratio = 0.5f);

    // Close (delete) a panel by name.
    void closePanel(const std::string& name);

    // Bring a panel to the front (make it the active tab in its leaf).
    void showPanel(const std::string& name);

    // ── Visual settings ───────────────────────────────────────────────────
    void  setTabBarHeight(float h) { tabBarH_ = h; markDirty(); }
    float tabBarHeight()     const { return tabBarH_; }

    void  setHandleSize(float s)   { handleW_ = s;   markDirty(); }
    float handleSize()       const { return handleW_; }

    // ── Signals ───────────────────────────────────────────────────────────
    Signal<std::string> panelActivated;  // tab brought to front
    Signal<std::string> panelClosed;     // panel removed

    // ── Widget overrides ──────────────────────────────────────────────────
    void  layout() override;
    Vec2f sizeHint() const override { return {200, 200}; }
    void  paint(PaintContext& ctx) override;
    void  onMousePress(MouseEvent& e) override;
    void  onMouseRelease(MouseEvent& e) override;
    void  onMouseMove(MouseEvent& e) override;
    void  onMouseLeave() override;

private:
    DockNode* root_    = nullptr;
    float     tabBarH_ = 26.0f;
    float handleW_ =  5.0f;

    // ── Tab drag state ─────────────────────────────────────────────────────
    struct DragState {
        bool      active  = false;
        DockNode* srcNode = nullptr;
        int       srcIdx  = 0;
        float     startX  = 0, startY = 0;
        float     curX    = 0, curY   = 0;
    } drag_;

    // ── Drop target ────────────────────────────────────────────────────────
    DockNode* dropNode_  = nullptr;
    DockSide  dropZone_  = DockSide::Center;
    bool      dropValid_ = false;

    // ── Splitter drag ──────────────────────────────────────────────────────
    struct SplitDrag {
        bool      active     = false;
        DockNode* node       = nullptr;
        float     startRatio = 0;
        float     startMouse = 0;
    } splitDrag_;

    DockNode* hoveredSplit_ = nullptr;

    // ── Internal helpers ───────────────────────────────────────────────────
    void ensureRoot();
    void freeNode(DockNode* n);   // recursive delete

    void layoutNode(DockNode* node, const Rect& r);

    // Paint helpers
    void paintNode(DockNode* node, PaintContext& ctx);
    void paintTabBar(DockNode* leaf, const Rect& barR, PaintContext& ctx);
    void paintDropOverlay(PaintContext& ctx);

    // Convert DockPanel-local rect > screen rect
    Rect toScreen(const Rect& local) const;

    // Hit tests (sx,sy in screen coords)
    DockNode* hitLeaf      (float sx, float sy) const;
    DockNode* hitLeafImpl  (DockNode* n, float ox, float oy, float sx, float sy) const;
    int       hitTab       (DockNode* leaf, float ox, float oy, float sx, float sy) const;
    DockNode* hitSplitter  (float sx, float sy) const;
    DockNode* hitSplitterImpl(DockNode* n, float ox, float oy, float sx, float sy) const;

    // Compute which DockSide (x,y) is over in leaf's content area
    DockSide computeDropZone(DockNode* leaf, float ox, float oy,
                             float sx, float sy) const;

    // Tree mutation - findRef returns pointer-to-pointer so we can replace the slot
    DockNode** findRef  (DockNode* searchFrom, DockNode* target);
    void       performDrop();
    void       pruneNode (DockNode*& slot);   // collapse single-child splits
};
