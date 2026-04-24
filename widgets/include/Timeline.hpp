#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include "Signal.hpp"
#include <vector>
#include <string>

// ═════════════════════════════════════════════════════════════════════════════
//  Timeline — Multi-track timeline with keyframes, clips, scrubbing, zoom
//
//  Features:
//  - Multiple named tracks with independent colors
//  - Keyframe markers (diamond icons) per track
//  - Clip bars (time ranges) per track
//  - Playhead with scrubbing
//  - Zoom (horizontal) and pan
//  - Track header area with labels
//  - Time ruler with tick marks and frame numbers
//
//  Usage:
//    auto* tl = parent->createChild<Timeline>();
//    int t1 = tl->addTrack("Position");
//    tl->addKeyframe(t1, 0.0f);
//    tl->addKeyframe(t1, 1.0f);
//    tl->addClip(t1, 0.5f, 1.5f, "Walk");
//    tl->onPlayheadChanged.connect([](float t) { ... });
// ═════════════════════════════════════════════════════════════════════════════

struct TimelineKeyframe {
    float time = 0;
    bool  selected = false;
};

struct TimelineClip {
    float       start = 0;
    float       end   = 1;
    std::string label;
    Color       color = Color(80, 120, 180, 200);
    bool        selected = false;
};

struct TimelineTrack {
    std::string name;
    Color       color = Color(120, 140, 180, 255);
    std::vector<TimelineKeyframe> keyframes;
    std::vector<TimelineClip>     clips;
    bool muted  = false;
    bool locked = false;
};

class Timeline : public Widget
{
public:
    Timeline();

    // ── Track management ─────────────────────────────────────────────────

    int  addTrack(const std::string& name, const Color& color = Color(120, 140, 180, 255));
    void removeTrack(int trackId);
    void clearTracks();

    int  trackCount() const { return static_cast<int>(tracks_.size()); }
    TimelineTrack&       track(int id)       { return tracks_[id]; }
    const TimelineTrack& track(int id) const { return tracks_[id]; }

    // ── Keyframe management ──────────────────────────────────────────────

    int  addKeyframe(int trackId, float time);
    void removeKeyframe(int trackId, int keyIdx);

    // ── Clip management ──────────────────────────────────────────────────

    int  addClip(int trackId, float start, float end, const std::string& label = "",
                 const Color& color = Color(80, 120, 180, 200));
    void removeClip(int trackId, int clipIdx);

    // ── Playhead ─────────────────────────────────────────────────────────

    void  setPlayhead(float t) { playhead_ = t; markDirty(); }
    float playhead() const     { return playhead_; }

    // ── View ─────────────────────────────────────────────────────────────

    void setTimeRange(float start, float end) { viewStart_ = start; viewEnd_ = end; markDirty(); }
    float viewStart() const { return viewStart_; }
    float viewEnd()   const { return viewEnd_; }

    void setFrameRate(float fps) { fps_ = fps; }
    float frameRate() const      { return fps_; }

    // ── Signals ──────────────────────────────────────────────────────────

    Signal<float>    onPlayheadChanged;
    Signal<int, int> onKeyframeSelected;   // trackId, keyIdx
    Signal<int, int> onClipSelected;       // trackId, clipIdx

    // ── Overrides ────────────────────────────────────────────────────────

    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;

    void setBgColor(const Color& c) { bgColor_ = c; }

private:
    std::vector<TimelineTrack> tracks_;
    Color bgColor_ = Color(30, 32, 36, 255);

    float playhead_  = 0;
    float viewStart_ = 0;
    float viewEnd_   = 5;
    float fps_       = 30.0f;

    static constexpr float kTrackH   = 28;
    static constexpr float kHeaderW  = 100;
    static constexpr float kRulerH   = 24;

    // Interaction
    enum class DragMode { None, Scrub, Pan, MoveKey, MoveClip, ResizeClipL, ResizeClipR };
    DragMode dragMode_    = DragMode::None;
    float    dragStartX_  = 0;
    float    panStartView_ = 0;
    float    panStartEnd_  = 0;
    int      dragTrack_   = -1;
    int      dragIndex_   = -1;
    float    dragOrigTime_ = 0;
    float    dragOrigEnd_  = 0;

    // ── Helpers ──────────────────────────────────────────────────────────

    float timeToX(float t) const;
    float xToTime(float x) const;
    int   trackAtY(float y) const;

    void paintRuler(PaintContext& ctx, const Rect& b);
    void paintTracks(PaintContext& ctx, const Rect& b);
    void paintPlayhead(PaintContext& ctx, const Rect& b);
};
