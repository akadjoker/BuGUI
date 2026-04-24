#include "PlotWidget.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace {
    inline Font::ClipRect toFontClip(const Rect& r)
    { return { r.x, r.y, r.w, r.h }; }

    static constexpr float kPadL = 46.0f;  // left  (y-axis labels)
    static constexpr float kPadR = 10.0f;
    static constexpr float kPadT = 24.0f;  // top   (title)
    static constexpr float kPadB = 28.0f;  // bottom (x-axis labels)
}

// ─────────────────────────────────────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────────────────────────────────────

PlotWidget::PlotWidget()
{
    viewXMin_ = 0; viewXMax_ = 10;
    viewYMin_ = 0; viewYMax_ = 1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Series management
// ─────────────────────────────────────────────────────────────────────────────

int PlotWidget::addSeries(const std::string& name, const Color& color, PlotType type)
{
    PlotSeries s;
    s.name  = name;
    s.color = color;
    s.type  = type;
    series_.push_back(std::move(s));
    markDirty();
    return static_cast<int>(series_.size()) - 1;
}

void PlotWidget::removeSeries(int idx)
{
    if (idx >= 0 && idx < (int)series_.size()) {
        series_.erase(series_.begin() + idx);
        markDirty();
    }
}

void PlotWidget::clearSeries()
{
    series_.clear();
    markDirty();
}

void PlotWidget::addPoint(int idx, float y)
{
    if (idx < 0 || idx >= (int)series_.size()) return;
    series_[idx].values.push_back(y);
    if (maxPoints_ > 0 && (int)series_[idx].values.size() > maxPoints_)
        series_[idx].values.erase(series_[idx].values.begin());
    markDirty();
}

void PlotWidget::setPoints(int idx, const std::vector<float>& ys)
{
    if (idx < 0 || idx >= (int)series_.size()) return;
    series_[idx].values = ys;
    markDirty();
}

void PlotWidget::clearPoints(int idx)
{
    if (idx < 0 || idx >= (int)series_.size()) return;
    series_[idx].values.clear();
    markDirty();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

void PlotWidget::computeAutoRange()
{
    if (!autoY_) return;
    bool first = true;
    float lo = 0, hi = 1;
    for (auto& s : series_) {
        if (!s.visible) continue;
        for (float v : s.values) {
            if (first) { lo = hi = v; first = false; }
            else { lo = std::min(lo, v); hi = std::max(hi, v); }
        }
    }
    if (first) { lo = 0; hi = 1; }
    float span = hi - lo;
    if (span < 1e-6f) span = 1.0f;
    viewYMin_ = lo - span * 0.05f;
    viewYMax_ = hi + span * 0.05f;
}

Rect PlotWidget::plotArea(const Rect& b) const
{
    return { b.x + kPadL, b.y + kPadT, b.w - kPadL - kPadR, b.h - kPadT - kPadB };
}

float PlotWidget::dataToScreenX(float x, const Rect& pa) const
{
    float span = viewXMax_ - viewXMin_;
    if (span < 1e-6f) return pa.x;
    return pa.x + (x - viewXMin_) / span * pa.w;
}

float PlotWidget::dataToScreenY(float y, const Rect& pa) const
{
    float span = viewYMax_ - viewYMin_;
    if (span < 1e-6f) return pa.y + pa.h;
    return pa.y + pa.h - (y - viewYMin_) / span * pa.h;
}

float PlotWidget::screenToDataX(float sx, const Rect& pa) const
{
    return viewXMin_ + (sx - pa.x) / pa.w * (viewXMax_ - viewXMin_);
}

float PlotWidget::screenToDataY(float sy, const Rect& pa) const
{
    return viewYMin_ + (pa.y + pa.h - sy) / pa.h * (viewYMax_ - viewYMin_);
}

void PlotWidget::resetView()
{
    // Count max points across series for X range
    int maxN = 10;
    for (auto& s : series_) maxN = std::max(maxN, (int)s.values.size());
    viewXMin_ = 0; viewXMax_ = (float)maxN;
    computeAutoRange();
    markDirty();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paint
// ─────────────────────────────────────────────────────────────────────────────

void PlotWidget::paintGrid(PaintContext& ctx, const Rect& pa)
{
    auto fc = toFontClip(pa);

    // Y grid lines
    float ySpan = viewYMax_ - viewYMin_;
    float yStep = std::pow(10.0f, std::floor(std::log10(ySpan / 5.0f)));
    float yFirst = std::ceil(viewYMin_ / yStep) * yStep;

    for (float y = yFirst; y <= viewYMax_ + yStep * 0.01f; y += yStep) {
        float sy = dataToScreenY(y, pa);
        if (sy < pa.y || sy > pa.y + pa.h) continue;

        ctx.fill.SetColor(55, 55, 60, 255);
        ctx.fillRect(pa.x, sy, pa.w, 1);

        char buf[16];
        if (std::fabs(y) < 1e4f && std::fabs(y) >= 0.01f)
            snprintf(buf, sizeof(buf), "%.4g", y);
        else
            snprintf(buf, sizeof(buf), "%.2e", y);
        ctx.fill.SetColor(140, 140, 140, 255);
        ctx.font.Print(buf, pa.x - kPadL + 2, sy - 6, &fc);
    }

    // X grid lines
    float xSpan = viewXMax_ - viewXMin_;
    float xStep = std::pow(10.0f, std::floor(std::log10(xSpan / 5.0f)));
    float xFirst = std::ceil(viewXMin_ / xStep) * xStep;

    for (float x = xFirst; x <= viewXMax_ + xStep * 0.01f; x += xStep) {
        float sx = dataToScreenX(x, pa);
        if (sx < pa.x || sx > pa.x + pa.w) continue;

        ctx.fill.SetColor(55, 55, 60, 255);
        ctx.fillRect(sx, pa.y, 1, pa.h);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.4g", x);
        ctx.fill.SetColor(140, 140, 140, 255);
        ctx.font.Print(buf, sx - 10, pa.y + pa.h + 4, &fc);
    }
}

void PlotWidget::paintSeries(PaintContext& ctx, const Rect& pa)
{
    auto fc = toFontClip(pa);

    for (auto& s : series_) {
        if (!s.visible || s.values.empty()) continue;
        int n = (int)s.values.size();

        // Compute X per index based on view
        float xPerIdx = (viewXMax_ - viewXMin_) / std::max(1.0f, (float)(n - 1));
        (void)xPerIdx;

        if (s.type == PlotType::Bar) {
            float barW = std::max(1.0f, pa.w / (float)n - 1.0f);
            for (int i = 0; i < n; ++i) {
                float xi = (float)i;
                float sx = dataToScreenX(xi, pa);
                float sy = dataToScreenY(s.values[i], pa);
                float sy0 = dataToScreenY(0.0f, pa);
                sy0 = std::clamp(sy0, pa.y, pa.y + pa.h);
                float top  = std::min(sy, sy0);
                float h    = std::fabs(sy0 - sy);
                if (sx < pa.x || sx > pa.x + pa.w) continue;
                ctx.fill.SetColor(s.color.r, s.color.g, s.color.b, 180);
                ctx.fillRect(sx - barW * 0.5f, top, barW, h);
            }
        } else if (s.type == PlotType::Scatter) {
            for (int i = 0; i < n; ++i) {
                float sx = dataToScreenX((float)i, pa);
                float sy = dataToScreenY(s.values[i], pa);
                if (sx < pa.x || sx > pa.x + pa.w) continue;
                ctx.fill.SetColor(s.color.r, s.color.g, s.color.b, 220);
                ctx.fillCircle(sx, sy, 3);
            }
        } else {
            // Line - draw thick line segments
            for (int i = 0; i < n - 1; ++i) {
                float sx0 = dataToScreenX((float)i,   pa);
                float sy0 = dataToScreenY(s.values[i], pa);
                float sx1 = dataToScreenX((float)(i+1), pa);
                float sy1 = dataToScreenY(s.values[i+1], pa);

                // Clip to plot area x
                if (sx1 < pa.x || sx0 > pa.x + pa.w) continue;

                // Thick line via rectangle ribbon
                float dx = sx1 - sx0, dy = sy1 - sy0;
                float len = std::sqrt(dx*dx + dy*dy);
                if (len < 0.5f) continue;
                float nx = -dy / len * 1.5f;
                float ny =  dx / len * 1.5f;

                ctx.fill.SetColor(s.color.r, s.color.g, s.color.b, s.color.a);
                ctx.fillTriangle(sx0+nx, sy0+ny, sx0-nx, sy0-ny, sx1+nx, sy1+ny);
                ctx.fillTriangle(sx0-nx, sy0-ny, sx1-nx, sy1-ny, sx1+nx, sy1+ny);
            }
        }
    }
}

void PlotWidget::paintLegend(PaintContext& ctx, const Rect& b)
{
    auto fc = toFontClip(b);
    float lx = b.x + kPadL + 6;
    float ly = b.y + kPadT + 6;

    for (auto& s : series_) {
        if (s.name.empty()) continue;
        ctx.fill.SetColor(s.color.r, s.color.g, s.color.b, 200);
        ctx.fillRect(lx, ly + 3, 12, 3);
        ctx.fill.SetColor(200, 200, 200, 255);
        ctx.font.Print(s.name.c_str(), lx + 16, ly, &fc);
        ly += 14;
    }
}

void PlotWidget::paintAxes(PaintContext& ctx, const Rect& b, const Rect& pa)
{
    auto fc = toFontClip(b);

    // Border around plot area
    ctx.fill.SetColor(70, 70, 75, 255);
    ctx.fillRect(pa.x,          pa.y,          pa.w, 1);
    ctx.fillRect(pa.x,          pa.y+pa.h-1,   pa.w, 1);
    ctx.fillRect(pa.x,          pa.y,          1,    pa.h);
    ctx.fillRect(pa.x+pa.w-1,   pa.y,          1,    pa.h);

    // Title
    if (!title_.empty()) {
        ctx.fill.SetColor(210, 210, 210, 255);
        ctx.font.Print(title_.c_str(), b.x + b.w * 0.5f - title_.size() * 3.5f, b.y + 4, &fc);
    }

    // X label
    if (!xLabel_.empty()) {
        ctx.fill.SetColor(160, 160, 160, 255);
        ctx.font.Print(xLabel_.c_str(), pa.x + pa.w * 0.5f - xLabel_.size() * 3.5f, b.y + b.h - 14, &fc);
    }
}

void PlotWidget::paint(PaintContext& ctx)
{
    const Rect b  = absoluteRect();
    const Rect pa = plotArea(b);
    plotArea_ = pa;

    computeAutoRange();

    // Background
    ctx.fill.SetColor(28, 28, 32, 255);
    ctx.fillRect(b.x, b.y, b.w, b.h);

    // Plot area background
    ctx.fill.SetColor(20, 20, 24, 255);
    ctx.fillRect(pa.x, pa.y, pa.w, pa.h);

    ctx.pushClip(pa);
    if (showGrid_)  paintGrid(ctx, pa);
    paintSeries(ctx, pa);
    ctx.popClip();

    paintAxes(ctx, b, pa);
    if (showLegend_) paintLegend(ctx, b);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Interaction (pan + zoom)
// ─────────────────────────────────────────────────────────────────────────────

void PlotWidget::onMousePress(MouseEvent& e)
{
    if (e.button == 1) {  // middle or right
        panning_   = true;
        panStartX_ = e.x; panStartY_ = e.y;
        panViewX0_ = viewXMin_; panViewY0_ = viewYMin_;
    }
}

void PlotWidget::onMouseRelease(MouseEvent& e)
{
    (void)e;
    panning_ = false;
}

void PlotWidget::onMouseMove(MouseEvent& e)
{
    if (!panning_) return;
    const Rect pa = plotArea_;
    if (pa.w < 1 || pa.h < 1) return;

    float dxData = -(e.x - panStartX_) / pa.w * (viewXMax_ - viewXMin_);
    float dyData =  (e.y - panStartY_) / pa.h * (viewYMax_ - viewYMin_);

    float spanX = viewXMax_ - viewXMin_;
    float spanY = viewYMax_ - viewYMin_;
    viewXMin_ = panViewX0_ + dxData;
    viewXMax_ = viewXMin_ + spanX;
    viewYMin_ = panViewY0_ + dyData;
    viewYMax_ = viewYMin_ + spanY;
    autoY_    = false;
    markDirty();
}

void PlotWidget::onMouseScroll(MouseEvent& e)
{
    const Rect pa = plotArea_;
    if (pa.w < 1) return;

    float factor = (e.scrollY > 0) ? 0.85f : 1.0f / 0.85f;

    // Zoom around mouse position
    float mx = screenToDataX(e.x, pa);
    float my = screenToDataY(e.y, pa);

    viewXMin_ = mx + (viewXMin_ - mx) * factor;
    viewXMax_ = mx + (viewXMax_ - mx) * factor;
    viewYMin_ = my + (viewYMin_ - my) * factor;
    viewYMax_ = my + (viewYMax_ - my) * factor;
    autoY_    = false;
    markDirty();
}
