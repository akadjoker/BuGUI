#include "Timeline.hpp"
#include "Theme.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  Font helper
// ─────────────────────────────────────────────────────────────────────────────
namespace {
    inline float setupFont(PaintContext& ctx, const Color& color, float size)
    {
        ctx.font.SetFontSize(size);
        ctx.font.SetBatch(&ctx.text);
        ctx.font.SetColor(color);
        return ctx.font.GetAscender();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Timeline
// ═════════════════════════════════════════════════════════════════════════════

Timeline::Timeline() {}

void Timeline::layout()
{
    Widget::layout();
    float contentH = static_cast<float>(tracks_.size()) * kTrackH;
    float viewH    = rect_.h - kRulerH;
    bool  needed   = contentH > viewH + 0.5f;

    if (!vbar_) {
        vbar_ = createChild<ScrollBar>(ScrollBarOrientation::Vertical);
        vbar_->scrolled.connect([this](float v) {
            scrollY_ = v;
            markDirty();
        });
    }
    vbar_->setVisible(needed);
    if (needed) {
        float bw = ScrollBar::kBarThickness;
        vbar_->setRect({rect_.x + rect_.w - bw, rect_.y + kRulerH, bw, viewH});
        vbar_->setContentSize(contentH);
        vbar_->setViewSize(viewH);
        vbar_->setValue(scrollY_);
        vbar_->layout();
        // clamp scroll
        float maxScroll = std::max(0.f, contentH - viewH);
        if (scrollY_ > maxScroll) scrollY_ = maxScroll;
    } else {
        scrollY_ = 0.f;
    }
}

void Timeline::clearSelection()
{
    for (auto& tr : tracks_)
        for (auto& kf : tr.keyframes)
            kf.selected = false;
    markDirty();
}

bool Timeline::hasSelection() const
{
    for (const auto& tr : tracks_)
        for (const auto& kf : tr.keyframes)
            if (kf.selected) return true;
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Track / Keyframe / Clip management
// ─────────────────────────────────────────────────────────────────────────────

int Timeline::addTrack(const std::string& name, const Color& color)
{
    tracks_.push_back({name, color, {}, {}, false, false});
    markDirty();
    return static_cast<int>(tracks_.size()) - 1;
}

void Timeline::removeTrack(int trackId)
{
    if (trackId >= 0 && trackId < static_cast<int>(tracks_.size())) {
        tracks_.erase(tracks_.begin() + trackId);
        markDirty();
    }
}

void Timeline::clearTracks()
{
    tracks_.clear();
    markDirty();
}

int Timeline::addKeyframe(int trackId, float time)
{
    if (trackId < 0 || trackId >= static_cast<int>(tracks_.size())) return -1;
    auto& kfs = tracks_[trackId].keyframes;
    TimelineKeyframe kf;
    kf.time = time;
    auto it = std::lower_bound(kfs.begin(), kfs.end(), kf,
        [](const TimelineKeyframe& a, const TimelineKeyframe& b) { return a.time < b.time; });
    int idx = static_cast<int>(it - kfs.begin());
    kfs.insert(it, kf);
    markDirty();
    return idx;
}

void Timeline::removeKeyframe(int trackId, int keyIdx)
{
    if (trackId < 0 || trackId >= static_cast<int>(tracks_.size())) return;
    auto& kfs = tracks_[trackId].keyframes;
    if (keyIdx >= 0 && keyIdx < static_cast<int>(kfs.size())) {
        kfs.erase(kfs.begin() + keyIdx);
        markDirty();
    }
}

int Timeline::addClip(int trackId, float start, float end,
                      const std::string& label, const Color& color)
{
    if (trackId < 0 || trackId >= static_cast<int>(tracks_.size())) return -1;
    TimelineClip clip;
    clip.start = start;
    clip.end   = end;
    clip.label = label;
    clip.color = color;
    tracks_[trackId].clips.push_back(std::move(clip));
    markDirty();
    return static_cast<int>(tracks_[trackId].clips.size()) - 1;
}

void Timeline::removeClip(int trackId, int clipIdx)
{
    if (trackId < 0 || trackId >= static_cast<int>(tracks_.size())) return;
    auto& clips = tracks_[trackId].clips;
    if (clipIdx >= 0 && clipIdx < static_cast<int>(clips.size())) {
        clips.erase(clips.begin() + clipIdx);
        markDirty();
    }
}

void Timeline::setTimeRange(float start, float end)
{
    viewStart_ = start;
    viewEnd_   = end;
    markDirty();
}

// ─────────────────────────────────────────────────────────────────────────────
//  View clamping
// ─────────────────────────────────────────────────────────────────────────────

void Timeline::clampView()
{
    // Never go before t=0
    if (viewStart_ < 0.f) {
        viewEnd_   -= viewStart_;
        viewStart_  = 0.f;
    }

    // Enforce minimum visible range (prevent zooming to nothing)
    const float kMinRange = 0.05f;
    if (viewEnd_ - viewStart_ < kMinRange)
        viewEnd_ = viewStart_ + kMinRange;

    // If endTime is set, don't pan/zoom so far right that content disappears
    if (endTime_ > 0.f) {
        const float margin = (viewEnd_ - viewStart_) * 0.25f; // allow 25% margin past end
        float maxEnd = endTime_ + margin;
        if (viewEnd_ > maxEnd) {
            float excess = viewEnd_ - maxEnd;
            viewEnd_   -= excess;
            viewStart_  = std::max(0.f, viewStart_ - excess);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Coordinate helpers
// ─────────────────────────────────────────────────────────────────────────────

float Timeline::timeToX(float t) const
{
    Rect b = absoluteRect();
    float contentW = b.w - kHeaderW;
    return b.x + kHeaderW + (t - viewStart_) / (viewEnd_ - viewStart_) * contentW;
}

float Timeline::xToTime(float x) const
{
    Rect b = absoluteRect();
    float contentW = b.w - kHeaderW;
    return viewStart_ + (x - b.x - kHeaderW) / contentW * (viewEnd_ - viewStart_);
}

int Timeline::trackAtY(float y) const
{
    Rect b = absoluteRect();
    float trackArea = y - b.y - kRulerH + scrollY_;
    if (trackArea < 0) return -1;
    int idx = static_cast<int>(trackArea / kTrackH);
    if (idx >= static_cast<int>(tracks_.size())) return -1;
    return idx;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse interaction
// ─────────────────────────────────────────────────────────────────────────────

void Timeline::onMousePress(MouseEvent& e)
{
    Rect b = absoluteRect();

    // Middle button → pan
    if (e.button == 1) {
        dragMode_      = DragMode::Pan;
        dragStartX_    = e.x;
        panStartView_  = viewStart_;
        panStartEnd_   = viewEnd_;
        e.consumed     = true;
        return;
    }
    if (e.button != 0) return;

    // Left-click on ruler → scrub playhead
    if (e.y < b.y + kRulerH && e.x > b.x + kHeaderW) {
        dragMode_ = DragMode::Scrub;
        playhead_ = xToTime(e.x);
        onPlayheadChanged.emit(playhead_);
        markDirty();
        e.consumed = true;
        return;
    }

    int ti = trackAtY(e.y);
    if (ti < 0) return;

    // Left-click on track header → select track (bone selection)
    if (e.x < b.x + kHeaderW) {
        onTrackClicked.emit(ti);
        e.consumed = true;
        return;
    }

    auto& trk = tracks_[ti];

    // Keyframe hit (diamond proximity)
    for (int ki = 0; ki < static_cast<int>(trk.keyframes.size()); ++ki) {
        float kx = timeToX(trk.keyframes[ki].time);
        float ky = b.y + kRulerH + ti * kTrackH + kTrackH * 0.5f;
        if (std::fabs(e.x - kx) + std::fabs(e.y - ky) < 8) {
            trk.keyframes[ki].selected = true;
            dragMode_     = DragMode::MoveKey;
            dragTrack_    = ti;
            dragIndex_    = ki;
            dragOrigTime_ = trk.keyframes[ki].time;
            dragStartX_   = e.x;
            onKeyframeSelected.emit(ti, ki);
            e.consumed = true;
            return;
        }
    }

    // Clip hit
    for (int ci = 0; ci < static_cast<int>(trk.clips.size()); ++ci) {
        float cx0 = timeToX(trk.clips[ci].start);
        float cx1 = timeToX(trk.clips[ci].end);
        float cy  = b.y + kRulerH + ti * kTrackH + 3;
        float ch  = kTrackH - 6;

        if (e.x >= cx0 && e.x <= cx1 && e.y >= cy && e.y <= cy + ch) {
            trk.clips[ci].selected = true;
            onClipSelected.emit(ti, ci);

            if      (e.x < cx0 + 6)  dragMode_ = DragMode::ResizeClipL;
            else if (e.x > cx1 - 6)  dragMode_ = DragMode::ResizeClipR;
            else                      dragMode_ = DragMode::MoveClip;

            dragTrack_    = ti;
            dragIndex_    = ci;
            dragOrigTime_ = trk.clips[ci].start;
            dragOrigEnd_  = trk.clips[ci].end;
            dragStartX_   = e.x;
            e.consumed    = true;
            return;
        }
    }

    // Empty space — wait for drag before starting selection rect
    dragMode_   = DragMode::SelectRectPending;
    selRectX0_  = selRectX1_ = e.x;
    selRectY0_  = selRectY1_ = e.y;
    dragStartX_ = e.x;
    dragStartY_ = e.y;
    e.consumed  = true;
}

void Timeline::onMouseRelease(MouseEvent& e)
{
    if (dragMode_ == DragMode::MoveKey && dragTrack_ >= 0 && dragIndex_ >= 0) {
        float newTime = tracks_[dragTrack_].keyframes[dragIndex_].time;
        onKeyframeMoved.emit(dragTrack_, dragIndex_, newTime);
    }
    if (dragMode_ == DragMode::SelectRectPending) {
        // click without drag — do nothing, selection unchanged
    }
    if (dragMode_ == DragMode::SelectRect) {
        // Finalise rectangle selection: mark all keyframes inside
        Rect b = absoluteRect();
        float t0 = xToTime(std::min(selRectX0_, selRectX1_));
        float t1 = xToTime(std::max(selRectX0_, selRectX1_));
        float y0 = std::min(selRectY0_, selRectY1_);
        float y1 = std::max(selRectY0_, selRectY1_);
        int count = 0;
        for (int ti = 0; ti < (int)tracks_.size(); ++ti) {
            float ky = b.y + kRulerH + ti * kTrackH + kTrackH * 0.5f;
            if (ky < y0 - kTrackH * 0.5f || ky > y1 + kTrackH * 0.5f) continue;
            for (auto& kf : tracks_[ti].keyframes) {
                if (kf.time >= t0 && kf.time <= t1) { kf.selected = true; ++count; }
            }
        }
        onSelectionChanged.emit(count);
        markDirty();
    }
    dragMode_  = DragMode::None;
    dragTrack_ = -1;
    dragIndex_ = -1;
    e.consumed = true;
}

void Timeline::onMouseMove(MouseEvent& e)
{
    switch (dragMode_) {
    case DragMode::Scrub:
        playhead_ = xToTime(e.x);
        onPlayheadChanged.emit(playhead_);
        markDirty();
        e.consumed = true;
        break;

    case DragMode::Pan: {
        float dt  = xToTime(dragStartX_) - xToTime(e.x);
        viewStart_ = panStartView_ + dt;
        viewEnd_   = panStartEnd_  + dt;
        clampView();
        markDirty();
        e.consumed = true;
        break;
    }
    case DragMode::MoveKey: {
        float dt = xToTime(e.x) - xToTime(dragStartX_);
        tracks_[dragTrack_].keyframes[dragIndex_].time = dragOrigTime_ + dt;
        markDirty();
        e.consumed = true;
        break;
    }
    case DragMode::MoveClip: {
        float dt = xToTime(e.x) - xToTime(dragStartX_);
        auto& clip = tracks_[dragTrack_].clips[dragIndex_];
        clip.start = dragOrigTime_ + dt;
        clip.end   = dragOrigEnd_  + dt;
        markDirty();
        e.consumed = true;
        break;
    }
    case DragMode::ResizeClipL: {
        float dt = xToTime(e.x) - xToTime(dragStartX_);
        auto& clip = tracks_[dragTrack_].clips[dragIndex_];
        clip.start = std::min(dragOrigTime_ + dt, clip.end - 0.01f);
        markDirty();
        e.consumed = true;
        break;
    }
    case DragMode::ResizeClipR: {
        float dt = xToTime(e.x) - xToTime(dragStartX_);
        auto& clip = tracks_[dragTrack_].clips[dragIndex_];
        clip.end = std::max(dragOrigEnd_ + dt, clip.start + 0.01f);
        markDirty();
        e.consumed = true;
        break;
    }
    case DragMode::SelectRectPending: {
        float dx = e.x - dragStartX_, dy = e.y - dragStartY_;
        if (dx*dx + dy*dy > 16.f) {
            clearSelection();
            dragMode_ = DragMode::SelectRect;
        }
        e.consumed = true;
        break;
    }
    case DragMode::SelectRect:
        selRectX1_ = e.x;
        selRectY1_ = e.y;
        markDirty();
        e.consumed = true;
        break;
    case DragMode::None:
        break;
    }
}

void Timeline::onMouseScroll(MouseEvent& e)
{
    Rect b = absoluteRect();
    bool inRuler = (e.y < b.y + kRulerH);

    if (inRuler) {
        // Horizontal zoom centred on mouse X
        float tAtMouse = xToTime(e.x);
        float factor   = (e.scrollY > 0) ? 0.9f : 1.1f;
        viewStart_ = tAtMouse + (viewStart_ - tAtMouse) * factor;
        viewEnd_   = tAtMouse + (viewEnd_   - tAtMouse) * factor;
        clampView();
    } else if (vbar_ && vbar_->isVisible()) {
        // Vertical scroll of tracks
        float step  = kTrackH * 2.f;
        scrollY_   -= e.scrollY * step;
        float maxSc = std::max(0.f, static_cast<float>(tracks_.size()) * kTrackH - (b.h - kRulerH));
        if (scrollY_ < 0.f)     scrollY_ = 0.f;
        if (scrollY_ > maxSc)   scrollY_ = maxSc;
        vbar_->setValue(scrollY_);
    } else {
        // No vbar — zoom horizontally anyway
        float tAtMouse = xToTime(e.x);
        float factor   = (e.scrollY > 0) ? 0.9f : 1.1f;
        viewStart_ = tAtMouse + (viewStart_ - tAtMouse) * factor;
        viewEnd_   = tAtMouse + (viewEnd_   - tAtMouse) * factor;
        clampView();
    }
    markDirty();
    e.consumed = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Painting — main entry
// ─────────────────────────────────────────────────────────────────────────────

void Timeline::paint(PaintContext& ctx)
{
    if (!visible_) return;
    Rect b = absoluteRect();

    // Update vbar geometry every frame (track count may have changed)
    {
        float contentH = static_cast<float>(tracks_.size()) * kTrackH;
        float viewH    = b.h - kRulerH;
        bool  needed   = contentH > viewH + 0.5f;
        if (!vbar_) {
            vbar_ = createChild<ScrollBar>(ScrollBarOrientation::Vertical);
            vbar_->scrolled.connect([this](float v) { scrollY_ = v; markDirty(); });
        }
        vbar_->setVisible(needed);
        if (needed) {
            float bw = ScrollBar::kBarThickness;
            vbar_->setRect({b.x + b.w - bw, b.y + kRulerH, bw, viewH});
            vbar_->setContentSize(contentH);
            vbar_->setViewSize(viewH);
            float maxSc = std::max(0.f, contentH - viewH);
            if (scrollY_ > maxSc) scrollY_ = maxSc;
            vbar_->setValue(scrollY_);
            vbar_->layout();
        } else {
            scrollY_ = 0.f;
        }
    }

    ctx.pushClip(b);

    // Background
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    ctx.fillRect(b.x, b.y, b.w, b.h);

    paintRuler(ctx, b);
    paintTracks(ctx, b);
    paintPlayhead(ctx, b);
    paintSelectionRect(ctx, b);

    // Border (4 edges)
    ctx.fill.SetColor(50, 52, 58, 255);
    ctx.fillRect(b.x,           b.y,           b.w, 1);
    ctx.fillRect(b.x,           b.y + b.h - 1, b.w, 1);
    ctx.fillRect(b.x,           b.y,           1,   b.h);
    ctx.fillRect(b.x + b.w - 1, b.y,           1,   b.h);

    ctx.popClip();

    Widget::paint(ctx);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ruler — time ticks with auto-scaled step
// ─────────────────────────────────────────────────────────────────────────────

void Timeline::paintRuler(PaintContext& ctx, const Rect& b)
{
    // Ruler background
    ctx.fill.SetColor(38, 40, 46, 255);
    ctx.fillRect(b.x, b.y, b.w, kRulerH);

    // Compute nice tick step
    float range = viewEnd_ - viewStart_;
    float rough = range / 10.0f;
    float mag   = std::pow(10.0f, std::floor(std::log10(rough)));
    float norm  = rough / mag;
    float step;
    if      (norm < 1.5f) step = mag;
    else if (norm < 3.5f) step = 2 * mag;
    else if (norm < 7.5f) step = 5 * mag;
    else                  step = 10 * mag;

    float asc = setupFont(ctx, Color(150, 152, 160, 200), 10.0f);

    float tStart = std::floor(viewStart_ / step) * step;
    for (float t = tStart; t <= viewEnd_; t += step) {
        float x = timeToX(t);
        if (x < b.x + kHeaderW || x > b.x + b.w) continue;

        // Major tick line
        ctx.fill.SetColor(80, 82, 90, 255);
        ctx.fillRect(x, b.y + kRulerH - 10, 1, 10);

        // Label
        char buf[16];
        if (fps_ > 0 && step < 1.0f)
            snprintf(buf, sizeof(buf), "%d", static_cast<int>(std::round(t * fps_)));
        else
            snprintf(buf, sizeof(buf), "%.2gs", t);

        ctx.font.SetColor(Color(150, 152, 160, 200));
        ctx.font.Print(buf, x + 2, b.y + (kRulerH - 10.0f) * 0.5f + asc);

        // Minor ticks (4 subdivisions)
        float minor = step / 4;
        ctx.fill.SetColor(55, 57, 63, 255);
        for (int m = 1; m < 4; ++m) {
            float mx = timeToX(t + m * minor);
            if (mx < b.x + kHeaderW || mx > b.x + b.w) continue;
            ctx.fillRect(mx, b.y + kRulerH - 5, 1, 5);
        }
    }

    // Separator line at ruler bottom
    ctx.fill.SetColor(60, 62, 68, 255);
    ctx.fillRect(b.x, b.y + kRulerH, b.w, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tracks — header, clips, keyframes
// ─────────────────────────────────────────────────────────────────────────────

void Timeline::paintTracks(PaintContext& ctx, const Rect& b)
{
    float ascName = setupFont(ctx, Color(180, 180, 180, 255), 10.0f);
    float barW = (vbar_ && vbar_->isVisible()) ? ScrollBar::kBarThickness : 0.f;
    float trackW = b.w - barW;
    float viewBottom = b.y + b.h;

    for (int ti = 0; ti < static_cast<int>(tracks_.size()); ++ti) {
        const auto& trk = tracks_[ti];
        float ty = b.y + kRulerH + ti * kTrackH - scrollY_;

        // Skip tracks outside visible area
        if (ty + kTrackH < b.y + kRulerH || ty > viewBottom) continue;

        bool isSel = (ti == selectedTrack_);
        Color bg = isSel ? Color(45, 55, 80, 255)
                         : (ti % 2 == 0) ? Color(34, 36, 40, 255) : Color(30, 32, 36, 255);
        ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
        ctx.fillRect(b.x, ty, trackW, kTrackH);

        // Header area (brighter when selected)
        Color hdrBg = isSel ? Color(55, 70, 110, 255) : Color(36, 38, 44, 255);
        ctx.fill.SetColor(hdrBg.r, hdrBg.g, hdrBg.b, hdrBg.a);
        ctx.fillRect(b.x, ty, kHeaderW, kTrackH);
        // Selection indicator bar on left edge
        if (isSel) {
            ctx.fill.SetColor(100, 160, 255, 255);
            ctx.fillRect(b.x, ty, 3, kTrackH);
        }

        // Track name — vertically centred in track header
        Color tc = trk.muted ? Color(80, 80, 80, 180) : trk.color;
        ctx.font.SetColor(tc);
        float nameY = ty + (kTrackH - 10.0f) * 0.5f + ascName;
        ctx.font.Print(trk.name.c_str(), b.x + 8, nameY);

        // Header separator
        ctx.fill.SetColor(50, 52, 58, 255);
        ctx.fillRect(b.x + kHeaderW, ty, 1, kTrackH);

        // ── Clips ────────────────────────────────────────────────────────
        for (int ci = 0; ci < static_cast<int>(trk.clips.size()); ++ci) {
            const auto& clip = trk.clips[ci];
            float cx0  = timeToX(clip.start);
            float cx1  = timeToX(clip.end);
            float clipY = ty + 3;
            float clipH = kTrackH - 6;

            if (cx1 < b.x + kHeaderW || cx0 > b.x + b.w) continue;

            // Clip body
            Color cc = clip.selected ? Color(255, 200, 60, 200) : clip.color;
            ctx.fill.SetColor(cc.r, cc.g, cc.b, cc.a);
            ctx.fillRect(cx0, clipY, cx1 - cx0, clipH);

            // Clip border (4 edges)
            uint8_t ba = clip.selected ? 255 : 120;
            ctx.fill.SetColor(200, 202, 210, ba);
            ctx.fillRect(cx0,         clipY,             cx1 - cx0, 1);
            ctx.fillRect(cx0,         clipY + clipH - 1, cx1 - cx0, 1);
            ctx.fillRect(cx0,         clipY,             1,         clipH);
            ctx.fillRect(cx1 - 1,     clipY,             1,         clipH);

            // Clip label
            if (!clip.label.empty() && cx1 - cx0 > 30) {
                float ascClip = setupFont(ctx, Color(240, 242, 250, 230), 9.0f);
                ctx.font.Print(clip.label.c_str(), cx0 + 4, clipY + (clipH - 9.0f) * 0.5f + ascClip);
            }
        }

        // ── Keyframes (diamonds) ─────────────────────────────────────────
        for (int ki = 0; ki < static_cast<int>(trk.keyframes.size()); ++ki) {
            const auto& kf = trk.keyframes[ki];
            float kx = timeToX(kf.time);
            float ky = ty + kTrackH * 0.5f;

            if (kx < b.x + kHeaderW || kx > b.x + b.w) continue;

            float ds = kf.selected ? 6.0f : 5.0f;
            Color kc = kf.selected ? Color(255, 200, 60, 255) : trk.color;
            ctx.fill.SetColor(kc.r, kc.g, kc.b, kc.a);
            // Two triangles make a diamond
            ctx.fillTriangle(kx, ky - ds, kx + ds, ky, kx, ky + ds);
            ctx.fillTriangle(kx, ky - ds, kx, ky + ds, kx - ds, ky);
        }

        // Track separator
        ctx.fill.SetColor(45, 47, 52, 255);
        ctx.fillRect(b.x, ty + kTrackH, b.w, 1);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Playhead — red vertical line + triangle marker
// ─────────────────────────────────────────────────────────────────────────────

void Timeline::paintPlayhead(PaintContext& ctx, const Rect& b)
{
    float x = timeToX(playhead_);
    if (x < b.x + kHeaderW || x > b.x + b.w) return;

    float top = b.y;
    float bot = b.y + kRulerH + static_cast<float>(tracks_.size()) * kTrackH;

    ctx.fill.SetColor(255, 80, 80, 220);
    ctx.fillRect(x, top, 1, bot - top);

    ctx.fill.SetColor(255, 80, 80, 240);
    ctx.fillTriangle(x - 6, b.y + kRulerH, x + 6, b.y + kRulerH, x, b.y + kRulerH - 8);

    // Frame / time label above the triangle
    {
        char buf[24];
        if (fps_ > 0.f)
            snprintf(buf, sizeof(buf), "%d", (int)std::roundf(playhead_ * fps_));
        else
            snprintf(buf, sizeof(buf), "%.2f", playhead_);

        float asc = setupFont(ctx, Color(255, 200, 200, 255), 9.f);
        float tw  = ctx.font.GetTextWidth(buf);
        float lx  = x - tw * 0.5f;
        // Keep within bounds
        if (lx < b.x + kHeaderW) lx = b.x + kHeaderW;
        if (lx + tw > b.x + b.w) lx = b.x + b.w - tw;
        // Small bg pill
        ctx.fill.SetColor(180, 50, 50, 200);
        ctx.fillRect(lx - 2, b.y + 2, tw + 4, 11);
        ctx.font.Print(buf, lx, b.y + 2 + asc);
    }
}

void Timeline::paintSelectionRect(PaintContext& ctx, const Rect& /*b*/)
{
    if (dragMode_ != DragMode::SelectRect) return;
    float x0 = std::min(selRectX0_, selRectX1_);
    float y0 = std::min(selRectY0_, selRectY1_);
    float w  = std::abs(selRectX1_ - selRectX0_);
    float h  = std::abs(selRectY1_ - selRectY0_);
    if (w < 1.f || h < 1.f) return;
    // Fill
    ctx.fill.SetColor(80, 160, 255, 40);
    ctx.fillRect(x0, y0, w, h);
    // Border
    ctx.fill.SetColor(80, 160, 255, 180);
    ctx.fillRect(x0,         y0,         w, 1);
    ctx.fillRect(x0,         y0 + h - 1, w, 1);
    ctx.fillRect(x0,         y0,         1, h);
    ctx.fillRect(x0 + w - 1, y0,         1, h);
}
