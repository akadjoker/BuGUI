#include "CurveEditor.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace {
    inline Font::ClipRect toFontClip(const Rect& r)
    { return { r.x, r.y, r.w, r.h }; }
}

// ═════════════════════════════════════════════════════════════════════════════
//  CurveEditor
// ═════════════════════════════════════════════════════════════════════════════

static constexpr float kPi = 3.14159265f;

CurveEditor::CurveEditor()
{
}

// ─────────────────────────────────────────────────────────────────────────────
//  Curve management
// ─────────────────────────────────────────────────────────────────────────────

int CurveEditor::addCurve(const std::string& name, const Color& color)
{
    curves_.push_back({name, color, {}, true});
    markDirty();
    return static_cast<int>(curves_.size()) - 1;
}

void CurveEditor::removeCurve(int curveId)
{
    if (curveId >= 0 && curveId < (int)curves_.size()) {
        curves_.erase(curves_.begin() + curveId);
        markDirty();
    }
}

void CurveEditor::clearCurves()
{
    curves_.clear();
    markDirty();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Keyframe management
// ─────────────────────────────────────────────────────────────────────────────

int CurveEditor::addKey(int curveId, float time, float value)
{
    if (curveId < 0 || curveId >= (int)curves_.size()) return -1;

    auto& keys = curves_[curveId].keys;
    CurveKey k;
    k.time = time;
    k.value = value;

    // Insert sorted by time
    auto it = std::lower_bound(keys.begin(), keys.end(), k,
        [](const CurveKey& a, const CurveKey& b) { return a.time < b.time; });
    int idx = static_cast<int>(it - keys.begin());
    keys.insert(it, k);

    // Auto-tangents for neighbors
    autoTangents(curves_[curveId], idx);
    if (idx > 0) autoTangents(curves_[curveId], idx - 1);
    if (idx + 1 < (int)keys.size()) autoTangents(curves_[curveId], idx + 1);

    markDirty();
    onKeyAdded.emit(curveId, idx);
    return idx;
}

void CurveEditor::removeKey(int curveId, int keyIdx)
{
    if (curveId < 0 || curveId >= (int)curves_.size()) return;
    auto& keys = curves_[curveId].keys;
    if (keyIdx < 0 || keyIdx >= (int)keys.size()) return;
    keys.erase(keys.begin() + keyIdx);
    markDirty();
    onKeyRemoved.emit(curveId, keyIdx);
}

void CurveEditor::setKey(int curveId, int keyIdx, float time, float value)
{
    if (curveId < 0 || curveId >= (int)curves_.size()) return;
    auto& keys = curves_[curveId].keys;
    if (keyIdx < 0 || keyIdx >= (int)keys.size()) return;
    keys[keyIdx].time = time;
    keys[keyIdx].value = value;
    autoTangents(curves_[curveId], keyIdx);
    markDirty();
    onKeyChanged.emit(curveId, keyIdx, time, value);
}

// ─────────────────────────────────────────────────────────────────────────────
//  View
// ─────────────────────────────────────────────────────────────────────────────

void CurveEditor::setViewRange(float minT, float maxT, float minV, float maxV)
{
    viewMinT_ = minT; viewMaxT_ = maxT;
    viewMinV_ = minV; viewMaxV_ = maxV;
    markDirty();
}

void CurveEditor::fitView()
{
    if (curves_.empty()) return;
    float tMin = 1e9f, tMax = -1e9f, vMin = 1e9f, vMax = -1e9f;
    for (auto& c : curves_) {
        for (auto& k : c.keys) {
            tMin = std::min(tMin, k.time);
            tMax = std::max(tMax, k.time);
            vMin = std::min(vMin, k.value);
            vMax = std::max(vMax, k.value);
        }
    }
    float tPad = std::max(0.1f, (tMax - tMin) * 0.1f);
    float vPad = std::max(0.1f, (vMax - vMin) * 0.1f);
    setViewRange(tMin - tPad, tMax + tPad, vMin - vPad, vMax + vPad);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Coordinate transforms
// ─────────────────────────────────────────────────────────────────────────────

float CurveEditor::timeToX(float t) const
{
    Rect b = absoluteRect();
    float margin = 40;
    return b.x + margin + (t - viewMinT_) / (viewMaxT_ - viewMinT_) * (b.w - margin * 2);
}

float CurveEditor::valueToY(float v) const
{
    Rect b = absoluteRect();
    float margin = 24;
    // Invert Y: higher value = higher on screen
    return b.y + b.h - margin - (v - viewMinV_) / (viewMaxV_ - viewMinV_) * (b.h - margin * 2);
}

float CurveEditor::xToTime(float x) const
{
    Rect b = absoluteRect();
    float margin = 40;
    return viewMinT_ + (x - b.x - margin) / (b.w - margin * 2) * (viewMaxT_ - viewMinT_);
}

float CurveEditor::yToValue(float y) const
{
    Rect b = absoluteRect();
    float margin = 24;
    return viewMinV_ + (b.y + b.h - margin - y) / (b.h - margin * 2) * (viewMaxV_ - viewMinV_);
}

float CurveEditor::snap(float val, float grid) const
{
    if (grid <= 0) return val;
    return std::round(val / grid) * grid;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Auto tangents (Catmull-Rom-style)
// ─────────────────────────────────────────────────────────────────────────────

void CurveEditor::autoTangents(CurveData& c, int idx)
{
    auto& keys = c.keys;
    if (idx < 0 || idx >= (int)keys.size()) return;
    auto& k = keys[idx];
    if (k.tangentMode != TangentMode::Auto) return;

    if (keys.size() <= 1) {
        k.tanInX = -0.1f; k.tanInY = 0;
        k.tanOutX = 0.1f; k.tanOutY = 0;
        return;
    }

    float dtPrev = 0, dvPrev = 0, dtNext = 0, dvNext = 0;
    if (idx > 0) {
        dtPrev = k.time - keys[idx - 1].time;
        dvPrev = k.value - keys[idx - 1].value;
    }
    if (idx + 1 < (int)keys.size()) {
        dtNext = keys[idx + 1].time - k.time;
        dvNext = keys[idx + 1].value - k.value;
    }

    // Catmull-Rom slope
    float slope = 0;
    if (idx > 0 && idx + 1 < (int)keys.size()) {
        float dt = keys[idx + 1].time - keys[idx - 1].time;
        float dv = keys[idx + 1].value - keys[idx - 1].value;
        if (std::fabs(dt) > 1e-6f) slope = dv / dt;
    } else if (idx > 0) {
        if (std::fabs(dtPrev) > 1e-6f) slope = dvPrev / dtPrev;
    } else if (idx + 1 < (int)keys.size()) {
        if (std::fabs(dtNext) > 1e-6f) slope = dvNext / dtNext;
    }

    float tanLen = 0.33f;
    k.tanInX  = -(dtPrev > 0 ? dtPrev : 0.1f) * tanLen;
    k.tanInY  = k.tanInX * slope;
    k.tanOutX =  (dtNext > 0 ? dtNext : 0.1f) * tanLen;
    k.tanOutY = k.tanOutX * slope;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bezier evaluation
// ─────────────────────────────────────────────────────────────────────────────

float CurveEditor::evalSegment(const CurveKey& k0, const CurveKey& k1, float t) const
{
    // Cubic bezier in the (time, value) plane
    float dt = k1.time - k0.time;
    if (std::fabs(dt) < 1e-8f) return k0.value;

    float u = (t - k0.time) / dt;
    u = std::max(0.0f, std::min(1.0f, u));

    // Control points (value only - time is implicit via u)
    float p0 = k0.value;
    float p1 = k0.value + k0.tanOutY;
    float p2 = k1.value + k1.tanInY;
    float p3 = k1.value;

    float iu = 1.0f - u;
    return iu*iu*iu*p0 + 3*iu*iu*u*p1 + 3*iu*u*u*p2 + u*u*u*p3;
}

float CurveEditor::evaluate(int curveId, float time) const
{
    if (curveId < 0 || curveId >= (int)curves_.size()) return 0;
    auto& keys = curves_[curveId].keys;
    if (keys.empty()) return 0;
    if (keys.size() == 1) return keys[0].value;

    if (time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time)  return keys.back().value;

    for (int i = 0; i + 1 < (int)keys.size(); ++i) {
        if (time >= keys[i].time && time <= keys[i + 1].time)
            return evalSegment(keys[i], keys[i + 1], time);
    }
    return keys.back().value;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hit testing
// ─────────────────────────────────────────────────────────────────────────────

CurveEditor::HitResult CurveEditor::hitTest(float mx, float my) const
{
    float hitR = 8.0f;
    float tanHitR = 6.0f;

    for (int ci = 0; ci < (int)curves_.size(); ++ci) {
        auto& c = curves_[ci];
        if (!c.visible) continue;
        for (int ki = 0; ki < (int)c.keys.size(); ++ki) {
            auto& k = c.keys[ki];
            float kx = timeToX(k.time);
            float ky = valueToY(k.value);

            // Check tangent handles first (if key is selected)
            if (k.selected) {
                float tix = timeToX(k.time + k.tanInX);
                float tiy = valueToY(k.value + k.tanInY);
                if ((mx - tix) * (mx - tix) + (my - tiy) * (my - tiy) < tanHitR * tanHitR)
                    return {ci, ki, true, true};

                float tox = timeToX(k.time + k.tanOutX);
                float toy = valueToY(k.value + k.tanOutY);
                if ((mx - tox) * (mx - tox) + (my - toy) * (my - toy) < tanHitR * tanHitR)
                    return {ci, ki, true, false};
            }

            // Key diamond
            if (std::fabs(mx - kx) + std::fabs(my - ky) < hitR)
                return {ci, ki, false, false};
        }
    }
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse interaction
// ─────────────────────────────────────────────────────────────────────────────

void CurveEditor::onMousePress(MouseEvent& e)
{
    if (e.button == 1) { // middle button → pan
        panning_ = true;
        dragStartX_ = e.x;
        dragStartY_ = e.y;
        panStartMinT_ = viewMinT_;
        panStartMaxT_ = viewMaxT_;
        panStartMinV_ = viewMinV_;
        panStartMaxV_ = viewMaxV_;
        e.consumed = true;
        return;
    }

    if (e.button != 0) return;

    auto hit = hitTest(e.x, e.y);

    if (hit.curve >= 0) {
        // Select key
        auto& keys = curves_[hit.curve].keys;

        // Deselect all
        for (auto& c : curves_)
            for (auto& k : c.keys)
                k.selected = false;
        keys[hit.key].selected = true;

        if (hit.tangent) {
            dragTangent_ = true;
            dragTanIn_   = hit.tanIn;
        } else {
            dragTangent_ = false;
        }

        dragging_   = true;
        dragCurve_  = hit.curve;
        dragKey_    = hit.key;
        dragStartX_ = e.x;
        dragStartY_ = e.y;
        e.consumed  = true;
    } else {
        // Double-click could add key; for now, deselect
        for (auto& c : curves_)
            for (auto& k : c.keys)
                k.selected = false;
        markDirty();
    }
}

void CurveEditor::onMouseRelease(MouseEvent& e)
{
    if (panning_) {
        panning_ = false;
        e.consumed = true;
        return;
    }
    if (dragging_) {
        dragging_ = false;
        dragCurve_ = -1;
        dragKey_ = -1;
        e.consumed = true;
    }
}

void CurveEditor::onMouseMove(MouseEvent& e)
{
    if (panning_) {
        float dx = e.x - dragStartX_;
        float dy = e.y - dragStartY_;

        Rect b = absoluteRect();
        float margin = 40;
        float tPerPx = (panStartMaxT_ - panStartMinT_) / (b.w - margin * 2);
        float vPerPx = (panStartMaxV_ - panStartMinV_) / (b.h - 48);

        viewMinT_ = panStartMinT_ - dx * tPerPx;
        viewMaxT_ = panStartMaxT_ - dx * tPerPx;
        viewMinV_ = panStartMinV_ + dy * vPerPx;
        viewMaxV_ = panStartMaxV_ + dy * vPerPx;
        markDirty();
        e.consumed = true;
        return;
    }

    if (!dragging_) return;
    e.consumed = true;

    float t = xToTime(e.x);
    float v = yToValue(e.y);

    if (snapTime_ > 0)  t = snap(t, snapTime_);
    if (snapValue_ > 0) v = snap(v, snapValue_);

    auto& k = curves_[dragCurve_].keys[dragKey_];

    if (dragTangent_) {
        // Move tangent handle
        float dt = t - k.time;
        float dv = v - k.value;
        if (dragTanIn_) {
            k.tanInX = dt;
            k.tanInY = dv;
            if (k.tangentMode == TangentMode::Aligned) {
                float len = std::sqrt(k.tanOutX * k.tanOutX + k.tanOutY * k.tanOutY);
                float inLen = std::sqrt(dt * dt + dv * dv);
                if (inLen > 1e-6f) {
                    k.tanOutX = -dt / inLen * len;
                    k.tanOutY = -dv / inLen * len;
                }
            }
        } else {
            k.tanOutX = dt;
            k.tanOutY = dv;
            if (k.tangentMode == TangentMode::Aligned) {
                float len = std::sqrt(k.tanInX * k.tanInX + k.tanInY * k.tanInY);
                float outLen = std::sqrt(dt * dt + dv * dv);
                if (outLen > 1e-6f) {
                    k.tanInX = -dt / outLen * len;
                    k.tanInY = -dv / outLen * len;
                }
            }
        }
        k.tangentMode = (k.tangentMode == TangentMode::Auto) ? TangentMode::Free : k.tangentMode;
    } else {
        // Move key
        k.time = t;
        k.value = v;
        autoTangents(curves_[dragCurve_], dragKey_);
    }

    markDirty();
    onKeyChanged.emit(dragCurve_, dragKey_, k.time, k.value);
}

void CurveEditor::onMouseScroll(MouseEvent& e)
{
    // Zoom centered on mouse
    float mx = e.x, my = e.y;
    float tAtMouse = xToTime(mx);
    float vAtMouse = yToValue(my);

    float zoomFactor = (e.scrollY > 0) ? 0.9f : 1.1f;

    viewMinT_ = tAtMouse + (viewMinT_ - tAtMouse) * zoomFactor;
    viewMaxT_ = tAtMouse + (viewMaxT_ - tAtMouse) * zoomFactor;
    viewMinV_ = vAtMouse + (viewMinV_ - vAtMouse) * zoomFactor;
    viewMaxV_ = vAtMouse + (viewMaxV_ - vAtMouse) * zoomFactor;

    markDirty();
    e.consumed = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Painting
// ─────────────────────────────────────────────────────────────────────────────

void CurveEditor::paint(PaintContext& ctx)
{
    if (!visible_) return;
    Rect b = absoluteRect();

    ctx.pushClip(b);

    // Background
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    ctx.fillRect(b.x, b.y, b.w, b.h);

    paintGrid(ctx, b);
    paintCurves(ctx, b);
    paintKeys(ctx, b);
    if (showPlayhead_) paintPlayhead(ctx, b);

    // Border
    ctx.fill.SetColor(50, 52, 58, 255);
    ctx.fillRect(b.x,           b.y,            b.w, 1);
    ctx.fillRect(b.x,           b.y + b.h - 1,  b.w, 1);
    ctx.fillRect(b.x,           b.y,            1,   b.h);
    ctx.fillRect(b.x + b.w - 1, b.y,            1,   b.h);

    ctx.popClip();

    Widget::paint(ctx);
}

void CurveEditor::paintGrid(PaintContext& ctx, const Rect& b)
{
    // Determine nice grid step
    auto niceStep = [](float range, int maxLines) -> float {
        float rough = range / maxLines;
        float mag = std::pow(10.0f, std::floor(std::log10(rough)));
        float norm = rough / mag;
        if (norm < 1.5f) return mag;
        if (norm < 3.5f) return 2 * mag;
        if (norm < 7.5f) return 5 * mag;
        return 10 * mag;
    };

    float tStep = niceStep(viewMaxT_ - viewMinT_, 10);
    float vStep = niceStep(viewMaxV_ - viewMinV_, 8);

    // Time grid (vertical lines)
    float tStart = std::floor(viewMinT_ / tStep) * tStep;
    for (float t = tStart; t <= viewMaxT_; t += tStep) {
        float x = timeToX(t);
        if (x < b.x + 40 || x > b.x + b.w - 40) continue;

        bool isZero = std::fabs(t) < tStep * 0.01f;
        ctx.fill.SetColor(isZero ? 80 : 45, isZero ? 80 : 47, isZero ? 80 : 52, 255);
        ctx.fillRect(x, b.y, 1, b.h - 24);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.2g", t);
        ctx.font.SetFontSize(10.0f);
        ctx.font.SetColor(Color(120, 122, 128, 200));
        ctx.font.SetBatch(&ctx.text);
        auto fc1 = toFontClip(ctx.clipRect());
        ctx.font.Print(buf, x - 8, b.y + b.h - 10, &fc1);
    }

    // Value grid (horizontal lines)
    float vStart = std::floor(viewMinV_ / vStep) * vStep;
    for (float v = vStart; v <= viewMaxV_; v += vStep) {
        float y = valueToY(v);
        if (y < b.y || y > b.y + b.h - 24) continue;

        bool isZero = std::fabs(v) < vStep * 0.01f;
        ctx.fill.SetColor(isZero ? 80 : 45, isZero ? 80 : 47, isZero ? 80 : 52, 255);
        ctx.fillRect(b.x + 40, y, b.w - 80, 1);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.2g", v);
        ctx.font.SetFontSize(10.0f);
        ctx.font.SetColor(Color(120, 122, 128, 200));
        ctx.font.SetBatch(&ctx.text);
        auto fc2 = toFontClip(ctx.clipRect());
        ctx.font.Print(buf, b.x + 4, y - 5, &fc2);
    }
}

void CurveEditor::paintCurves(PaintContext& ctx, const Rect& b)
{
    int segments = std::max(60, static_cast<int>(b.w / 3));

    for (auto& c : curves_) {
        if (!c.visible || c.keys.size() < 2) continue;

        Color col = c.color;
        float prevX = 0, prevY = 0;
        bool first = true;

        for (int s = 0; s <= segments; ++s) {
            float t = viewMinT_ + (viewMaxT_ - viewMinT_) * (float(s) / segments);
            float v = evaluate(static_cast<int>(&c - &curves_[0]), t);
            float x = timeToX(t);
            float y = valueToY(v);

            if (!first) {
                // Thick line (2 triangles)
                float dx = x - prevX, dy = y - prevY;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0.5f) {
                    float thick = 1.5f;
                    float nx = -dy / len * thick * 0.5f;
                    float ny =  dx / len * thick * 0.5f;
                    ctx.fill.SetColor(col.r, col.g, col.b, col.a);
                    ctx.fillTriangle(prevX+nx, prevY+ny, prevX-nx, prevY-ny, x-nx, y-ny);
                    ctx.fillTriangle(prevX+nx, prevY+ny, x-nx, y-ny, x+nx, y+ny);
                }
            }
            prevX = x; prevY = y; first = false;
        }
    }
}

void CurveEditor::paintKeys(PaintContext& ctx, const Rect& b)
{
    for (int ci = 0; ci < (int)curves_.size(); ++ci) {
        auto& c = curves_[ci];
        if (!c.visible) continue;

        for (int ki = 0; ki < (int)c.keys.size(); ++ki) {
            auto& k = c.keys[ki];
            float kx = timeToX(k.time);
            float ky = valueToY(k.value);

            // Tangent handles (only for selected keys)
            if (k.selected) {
                float tix = timeToX(k.time + k.tanInX);
                float tiy = valueToY(k.value + k.tanInY);
                float tox = timeToX(k.time + k.tanOutX);
                float toy = valueToY(k.value + k.tanOutY);

                // Tangent lines as thin fill quads
                ctx.fill.SetColor(c.color.r, c.color.g, c.color.b, 120);
                ctx.fillTriangle(tix - 0.5f, tiy, tix + 0.5f, tiy, kx + 0.5f, ky);
                ctx.fillTriangle(tix - 0.5f, tiy, kx - 0.5f,  ky,  kx + 0.5f, ky);
                ctx.fillTriangle(kx - 0.5f, ky, kx + 0.5f, ky, tox + 0.5f, toy);
                ctx.fillTriangle(kx - 0.5f, ky, tox - 0.5f, toy, tox + 0.5f, toy);

                // Tangent dots
                ctx.fill.SetColor(c.color.r, c.color.g, c.color.b, 200);
                ctx.fillCircle(tix, tiy, 3);
                ctx.fillCircle(tox, toy, 3);
            }

            // Key diamond - outline first (larger), fill on top
            float ds = k.selected ? 6.0f : 5.0f;
            float ods = ds + 1.0f;
            Color oc(255, 255, 255, k.selected ? 255 : 80);
            ctx.fill.SetColor(oc.r, oc.g, oc.b, oc.a);
            ctx.fillTriangle(kx, ky - ods, kx + ods, ky, kx, ky + ods);
            ctx.fillTriangle(kx, ky - ods, kx, ky + ods, kx - ods, ky);

            Color kc = k.selected ? Color(255, 200, 60, 255) : c.color;
            ctx.fill.SetColor(kc.r, kc.g, kc.b, kc.a);
            ctx.fillTriangle(kx, ky - ds, kx + ds, ky, kx, ky + ds);
            ctx.fillTriangle(kx, ky - ds, kx, ky + ds, kx - ds, ky);
        }
    }
}

void CurveEditor::paintPlayhead(PaintContext& ctx, const Rect& b)
{
    float x = timeToX(playhead_);
    if (x < b.x + 40 || x > b.x + b.w - 40) return;

    // Playhead
    ctx.fill.SetColor(255, 80, 80, 200);
    ctx.fillRect(x, b.y, 1, b.h - 24);
    ctx.fill.SetColor(255, 80, 80, 220);
    ctx.fillTriangle(x - 5, b.y, x + 5, b.y, x, b.y + 8);
}
