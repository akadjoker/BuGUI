#include "Timeline.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>

// ═════════════════════════════════════════════════════════════════════════════
//  Timeline
// ═════════════════════════════════════════════════════════════════════════════

Timeline::Timeline()
{
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
    if (trackId >= 0 && trackId < (int)tracks_.size()) {
        tracks_.erase(tracks_.begin() + trackId);
        markDirty();
    }
}

void Timeline::clearTracks() { tracks_.clear(); markDirty(); }

int Timeline::addKeyframe(int trackId, float time)
{
    if (trackId < 0 || trackId >= (int)tracks_.size()) return -1;
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
    if (trackId < 0 || trackId >= (int)tracks_.size()) return;
    auto& kfs = tracks_[trackId].keyframes;
    if (keyIdx >= 0 && keyIdx < (int)kfs.size()) {
        kfs.erase(kfs.begin() + keyIdx);
        markDirty();
    }
}

int Timeline::addClip(int trackId, float start, float end, const std::string& label, const Color& color)
{
    if (trackId < 0 || trackId >= (int)tracks_.size()) return -1;
    TimelineClip clip;
    clip.start = start; clip.end = end; clip.label = label; clip.color = color;
    tracks_[trackId].clips.push_back(std::move(clip));
    markDirty();
    return static_cast<int>(tracks_[trackId].clips.size()) - 1;
}

void Timeline::removeClip(int trackId, int clipIdx)
{
    if (trackId < 0 || trackId >= (int)tracks_.size()) return;
    auto& clips = tracks_[trackId].clips;
    if (clipIdx >= 0 && clipIdx < (int)clips.size()) {
        clips.erase(clips.begin() + clipIdx);
        markDirty();
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
    float trackArea = y - b.y - kRulerH;
    if (trackArea < 0) return -1;
    int idx = static_cast<int>(trackArea / kTrackH);
    if (idx >= (int)tracks_.size()) return -1;
    return idx;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse interaction
// ─────────────────────────────────────────────────────────────────────────────

void Timeline::onMousePress(MouseEvent& e)
{
    Rect b = absoluteRect();

    if (e.button == 1) { // middle → pan
        dragMode_      = DragMode::Pan;
        dragStartX_    = e.x;
        panStartView_  = viewStart_;
        panStartEnd_   = viewEnd_;
        e.consumed     = true;
        return;
    }

    if (e.button != 0) return;

    // Click on ruler → scrub playhead
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

    auto& trk = tracks_[ti];

    // Check keyframes (diamond hit)
    for (int ki = 0; ki < (int)trk.keyframes.size(); ++ki) {
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
            e.consumed    = true;
            return;
        }
    }

    // Check clips
    for (int ci = 0; ci < (int)trk.clips.size(); ++ci) {
        float cx0 = timeToX(trk.clips[ci].start);
        float cx1 = timeToX(trk.clips[ci].end);
        float cy  = b.y + kRulerH + ti * kTrackH + 3;
        float ch  = kTrackH - 6;
        if (e.x >= cx0 && e.x <= cx1 && e.y >= cy && e.y <= cy + ch) {
            trk.clips[ci].selected = true;
            onClipSelected.emit(ti, ci);

            // Left edge resize
            if (e.x < cx0 + 6) {
                dragMode_     = DragMode::ResizeClipL;
            }
            // Right edge resize
            else if (e.x > cx1 - 6) {
                dragMode_     = DragMode::ResizeClipR;
            }
            else {
                dragMode_     = DragMode::MoveClip;
            }
            dragTrack_    = ti;
            dragIndex_    = ci;
            dragOrigTime_ = trk.clips[ci].start;
            dragOrigEnd_  = trk.clips[ci].end;
            dragStartX_   = e.x;
            e.consumed    = true;
            return;
        }
    }
}

void Timeline::onMouseRelease(MouseEvent& e)
{
    dragMode_ = DragMode::None;
    dragTrack_ = -1;
    dragIndex_ = -1;
    e.consumed = true;
}

void Timeline::onMouseMove(MouseEvent& e)
{
    if (dragMode_ == DragMode::Scrub) {
        playhead_ = xToTime(e.x);
        onPlayheadChanged.emit(playhead_);
        markDirty();
        e.consumed = true;
    }
    else if (dragMode_ == DragMode::Pan) {
        float dt = xToTime(dragStartX_) - xToTime(e.x);
        viewStart_ = panStartView_ + dt;
        viewEnd_   = panStartEnd_ + dt;
        markDirty();
        e.consumed = true;
    }
    else if (dragMode_ == DragMode::MoveKey) {
        float dt = xToTime(e.x) - xToTime(dragStartX_);
        tracks_[dragTrack_].keyframes[dragIndex_].time = dragOrigTime_ + dt;
        markDirty();
        e.consumed = true;
    }
    else if (dragMode_ == DragMode::MoveClip) {
        float dt = xToTime(e.x) - xToTime(dragStartX_);
        auto& clip = tracks_[dragTrack_].clips[dragIndex_];
        clip.start = dragOrigTime_ + dt;
        clip.end   = dragOrigEnd_ + dt;
        markDirty();
        e.consumed = true;
    }
    else if (dragMode_ == DragMode::ResizeClipL) {
        float dt = xToTime(e.x) - xToTime(dragStartX_);
        auto& clip = tracks_[dragTrack_].clips[dragIndex_];
        clip.start = std::min(dragOrigTime_ + dt, clip.end - 0.01f);
        markDirty();
        e.consumed = true;
    }
    else if (dragMode_ == DragMode::ResizeClipR) {
        float dt = xToTime(e.x) - xToTime(dragStartX_);
        auto& clip = tracks_[dragTrack_].clips[dragIndex_];
        clip.end = std::max(dragOrigEnd_ + dt, clip.start + 0.01f);
        markDirty();
        e.consumed = true;
    }
}

void Timeline::onMouseScroll(MouseEvent& e)
{
    // Zoom centered on mouse X
    float tAtMouse = xToTime(e.x);
    float factor = (e.scrollY > 0) ? 0.9f : 1.1f;

    viewStart_ = tAtMouse + (viewStart_ - tAtMouse) * factor;
    viewEnd_   = tAtMouse + (viewEnd_ - tAtMouse) * factor;

    markDirty();
    e.consumed = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Painting
// ─────────────────────────────────────────────────────────────────────────────

void Timeline::paint(PaintContext& ctx)
{
    if (!visible_) return;
    Rect b = absoluteRect();

    // Background
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    ctx.fill.Rectangle(b.x, b.y, b.w, b.h, true);

    paintRuler(ctx, b);
    paintTracks(ctx, b);
    paintPlayhead(ctx, b);

    // Border
    ctx.line.SetColor(50, 52, 58, 255);
    ctx.line.Rectangle(b.x, b.y, b.w, b.h, false);

    Widget::paint(ctx);
}

void Timeline::paintRuler(PaintContext& ctx, const Rect& b)
{
    // Ruler background
    ctx.fill.SetColor(38, 40, 46, 255);
    ctx.fill.Rectangle(b.x, b.y, b.w, kRulerH, true);

    // Time ticks
    float range = viewEnd_ - viewStart_;
    float rough = range / 10.0f;
    float mag = std::pow(10.0f, std::floor(std::log10(rough)));
    float norm = rough / mag;
    float step;
    if (norm < 1.5f) step = mag;
    else if (norm < 3.5f) step = 2 * mag;
    else if (norm < 7.5f) step = 5 * mag;
    else step = 10 * mag;

    float tStart = std::floor(viewStart_ / step) * step;
    for (float t = tStart; t <= viewEnd_; t += step) {
        float x = timeToX(t);
        if (x < b.x + kHeaderW || x > b.x + b.w) continue;

        // Major tick
        ctx.line.SetColor(80, 82, 90, 255);
        ctx.line.Line2D(x, b.y + kRulerH - 10, x, b.y + kRulerH);

        // Label
        char buf[16];
        if (fps_ > 0 && step < 1.0f)
            snprintf(buf, sizeof(buf), "%d", static_cast<int>(std::round(t * fps_)));
        else
            snprintf(buf, sizeof(buf), "%.2gs", t);
        ctx.font.SetFontSize(9.0f);
        ctx.font.SetBatch(&ctx.text);
        ctx.font.SetColor(Color(150, 152, 160, 200));
        ctx.font.Print(buf, x + 2, b.y + 3);

        // Minor ticks
        float minor = step / 4;
        for (int m = 1; m < 4; ++m) {
            float mx = timeToX(t + m * minor);
            if (mx < b.x + kHeaderW || mx > b.x + b.w) continue;
            ctx.line.SetColor(55, 57, 63, 255);
            ctx.line.Line2D(mx, b.y + kRulerH - 5, mx, b.y + kRulerH);
        }
    }

    // Separator line
    ctx.line.SetColor(60, 62, 68, 255);
    ctx.line.Line2D(b.x, b.y + kRulerH, b.x + b.w, b.y + kRulerH);
}

void Timeline::paintTracks(PaintContext& ctx, const Rect& b)
{
    for (int ti = 0; ti < (int)tracks_.size(); ++ti) {
        auto& trk = tracks_[ti];
        float ty = b.y + kRulerH + ti * kTrackH;

        // Track background (alternating)
        Color bg = (ti % 2 == 0) ? Color(34, 36, 40, 255) : Color(30, 32, 36, 255);
        ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
        ctx.fill.Rectangle(b.x, ty, b.w, kTrackH, true);

        // Header area
        ctx.fill.SetColor(36, 38, 44, 255);
        ctx.fill.Rectangle(b.x, ty, kHeaderW, kTrackH, true);

        // Track name
        ctx.font.SetFontSize(10.0f);
        ctx.font.SetBatch(&ctx.text);
        Color tc = trk.muted ? Color(80, 80, 80, 180) : trk.color;
        ctx.font.SetColor(tc);
        ctx.font.Print(trk.name.c_str(), b.x + 8, ty + 8);

        // Header separator
        ctx.line.SetColor(50, 52, 58, 255);
        ctx.line.Line2D(b.x + kHeaderW, ty, b.x + kHeaderW, ty + kTrackH);

        // Clips
        for (int ci = 0; ci < (int)trk.clips.size(); ++ci) {
            auto& clip = trk.clips[ci];
            float cx0 = timeToX(clip.start);
            float cx1 = timeToX(clip.end);
            float clipY = ty + 3;
            float clipH = kTrackH - 6;

            if (cx1 < b.x + kHeaderW || cx0 > b.x + b.w) continue;

            // Clip body
            Color cc = clip.selected ? Color(255, 200, 60, 200) : clip.color;
            ctx.fill.SetColor(cc.r, cc.g, cc.b, cc.a);
            ctx.fill.Rectangle(cx0, clipY, cx1 - cx0, clipH, true);

            // Clip border
            ctx.line.SetColor(200, 202, 210, clip.selected ? 255 : 120);
            ctx.line.Rectangle(cx0, clipY, cx1 - cx0, clipH, false);

            // Clip label
            if (!clip.label.empty() && cx1 - cx0 > 30) {
                ctx.font.SetFontSize(9.0f);
                ctx.font.SetColor(Color(240, 242, 250, 230));
                ctx.font.Print(clip.label.c_str(), cx0 + 4, clipY + 4);
            }
        }

        // Keyframes
        for (int ki = 0; ki < (int)trk.keyframes.size(); ++ki) {
            auto& kf = trk.keyframes[ki];
            float kx = timeToX(kf.time);
            float ky = ty + kTrackH * 0.5f;

            if (kx < b.x + kHeaderW || kx > b.x + b.w) continue;

            float ds = kf.selected ? 6.0f : 5.0f;
            Color kc = kf.selected ? Color(255, 200, 60, 255) : trk.color;

            // Diamond
            ctx.fill.SetColor(kc.r, kc.g, kc.b, kc.a);
            ctx.fill.Triangle(kx, ky - ds, kx + ds, ky, kx, ky + ds, true);
            ctx.fill.Triangle(kx, ky - ds, kx, ky + ds, kx - ds, ky, true);
        }

        // Track separator
        ctx.line.SetColor(45, 47, 52, 255);
        ctx.line.Line2D(b.x, ty + kTrackH, b.x + b.w, ty + kTrackH);
    }
}

void Timeline::paintPlayhead(PaintContext& ctx, const Rect& b)
{
    float x = timeToX(playhead_);
    if (x < b.x + kHeaderW || x > b.x + b.w) return;

    float top = b.y;
    float bot = b.y + kRulerH + tracks_.size() * kTrackH;

    // Playhead line
    ctx.line.SetColor(255, 80, 80, 220);
    ctx.line.Line2D(x, top, x, bot);

    // Triangle marker at ruler
    ctx.fill.SetColor(255, 80, 80, 240);
    ctx.fill.Triangle(x - 6, b.y + kRulerH, x + 6, b.y + kRulerH, x, b.y + kRulerH - 8, true);
}
