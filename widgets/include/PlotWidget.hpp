#pragma once

#include "Widget.hpp"
#include "Signal.hpp"
#include "Color.hpp"
#include <vector>
#include <string>

// ═════════════════════════════════════════════════════════════════════════════
//  PlotWidget — Line / Bar / Scatter charts with pan + zoom
//
//  Usage:
//    auto* plot = parent->createChild<PlotWidget>();
//    int s = plot->addSeries("FPS", Color(100,200,100,255));
//    for (float v : values) plot->addPoint(s, v);
//    plot->setTitle("Frame Time");
//    plot->setXLabel("Frame"); plot->setYLabel("ms");
// ═════════════════════════════════════════════════════════════════════════════

enum class PlotType { Line, Bar, Scatter };

struct PlotSeries {
    std::string name;
    Color       color;
    PlotType    type    = PlotType::Line;
    bool        visible = true;
    float       lineWidth = 2.0f;
    std::vector<float> values;  // Y values; X = index
};

class PlotWidget : public Widget
{
public:
    PlotWidget();

    // ── Series ────────────────────────────────────────────────────────────
    int  addSeries(const std::string& name, const Color& color, PlotType type = PlotType::Line);
    void removeSeries(int idx);
    void clearSeries();

    void addPoint(int seriesIdx, float y);
    void setPoints(int seriesIdx, const std::vector<float>& ys);
    void clearPoints(int seriesIdx);

    int  seriesCount() const { return static_cast<int>(series_.size()); }
    const PlotSeries& series(int idx) const { return series_[idx]; }
    void setSeriesVisible(int idx, bool v) { series_[idx].visible = v; markDirty(); }

    // ── Axes ─────────────────────────────────────────────────────────────
    void setTitle(const std::string& t)  { title_ = t; markDirty(); }
    void setXLabel(const std::string& l) { xLabel_ = l; markDirty(); }
    void setYLabel(const std::string& l) { yLabel_ = l; markDirty(); }

    void setYRange(float yMin, float yMax) { yMin_ = yMin; yMax_ = yMax; autoY_ = false; markDirty(); }
    void setAutoYRange(bool a) { autoY_ = a; markDirty(); }
    void setMaxPoints(int n) { maxPoints_ = n; }   // rolling window (0 = unlimited)

    // ── Grid ─────────────────────────────────────────────────────────────
    void setShowGrid(bool s) { showGrid_ = s; markDirty(); }
    void setShowLegend(bool s) { showLegend_ = s; markDirty(); }

    // ── Pan / zoom ────────────────────────────────────────────────────────
    void resetView();

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override { return {300, 180}; }
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;

private:
    std::vector<PlotSeries> series_;

    std::string title_;
    std::string xLabel_;
    std::string yLabel_;

    float yMin_ = 0, yMax_ = 1;
    bool  autoY_ = true;
    int   maxPoints_ = 0;

    bool showGrid_   = true;
    bool showLegend_ = true;

    // Pan / zoom state (in data space)
    float viewXMin_ = 0, viewXMax_ = 100;
    float viewYMin_ = 0, viewYMax_ = 1;
    bool  panning_ = false;
    float panStartX_ = 0, panStartY_ = 0;
    float panViewX0_ = 0, panViewY0_ = 0;

    // Plot area (computed during paint)
    Rect plotArea_ = {};

    // Helpers
    void  computeAutoRange();
    Rect  plotArea(const Rect& b) const;
    float dataToScreenX(float x, const Rect& pa) const;
    float dataToScreenY(float y, const Rect& pa) const;
    float screenToDataX(float sx, const Rect& pa) const;
    float screenToDataY(float sy, const Rect& pa) const;
    void  paintGrid(PaintContext& ctx, const Rect& pa);
    void  paintSeries(PaintContext& ctx, const Rect& pa);
    void  paintLegend(PaintContext& ctx, const Rect& b);
    void  paintAxes(PaintContext& ctx, const Rect& b, const Rect& pa);
};
