#pragma once

#include "Widget.hpp"
#include <vector>

// ═════════════════════════════════════════════════════════════════════════════
//  WaveformView — audio waveform display
//
//  Usage:
//    auto* wv = parent->createChild<WaveformView>();
//    wv->setSamples(buffer.data(), buffer.size(), sampleRate);
//    wv->setPlayhead(0.5f);   // position 0..1
//
//  Features:
//    - Min/max envelope rendering (efficient for large buffers)
//    - Stereo (top/bottom split) or mono (centre)
//    - Zoomable/pannable view (mouse wheel + drag)
//    - Optional playhead scrub
//    - Region selection (click+drag)
//    - Loop markers (setLoopRegion)
// ═════════════════════════════════════════════════════════════════════════════

class WaveformView : public Widget
{
public:
    WaveformView();

    // ── Data ──────────────────────────────────────────────────────────────
    // Interleaved samples if channels>1: [L0,R0, L1,R1, ...]
    void setSamples(const float* data, int count, int channels = 1, float sampleRate = 44100.0f);
    void clearSamples();

    int   sampleCount()  const { return sampleCount_; }
    int   channels()     const { return channels_; }
    float sampleRate()   const { return sampleRate_; }

    // ── Playback ──────────────────────────────────────────────────────────
    void setPlayhead(float t) { playhead_ = t; markDirty(); }  // 0..1
    float playhead() const    { return playhead_; }

    // ── Loop region ───────────────────────────────────────────────────────
    void  setLoopRegion(float start, float end);  // 0..1 normalised
    void  clearLoop() { hasLoop_ = false; markDirty(); }
    float loopStart() const { return loopStart_; }
    float loopEnd()   const { return loopEnd_; }

    // ── View (zoom/pan) ───────────────────────────────────────────────────
    void  setViewRange(float start, float end);  // 0..1 normalised
    float viewStart() const { return viewStart_; }
    float viewEnd()   const { return viewEnd_; }
    void  resetView()       { viewStart_ = 0.0f; viewEnd_ = 1.0f; markDirty(); }

    // ── Appearance ────────────────────────────────────────────────────────
    void setWaveColor(const Color& c)   { waveColor_ = c;   markDirty(); }
    void setBgColor(const Color& c)     { bgColor_   = c;   markDirty(); }
    void setPlayheadColor(const Color& c){ playheadColor_ = c; markDirty(); }
    void setShowCentreLine(bool s)      { showCentre_ = s;  markDirty(); }

    // ── Signals ───────────────────────────────────────────────────────────
    Signal<float> onScrub;      // emitted while dragging playhead

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override { return {300, 100}; }
    void  paint(PaintContext& ctx) override;
    void  onMousePress(MouseEvent& e) override;
    void  onMouseMove(MouseEvent& e) override;
    void  onMouseRelease(MouseEvent& e) override;
    void  onMouseScroll(MouseEvent& e) override;

private:
    // Compressed envelope: min/max per pixel column
    struct EnvPair { float mn, mx; };
    std::vector<EnvPair> buildEnvelope(int ch, float vs, float ve, int pixelW) const;

    std::vector<float> samples_;
    int   sampleCount_ = 0;
    int   channels_    = 1;
    float sampleRate_  = 44100.0f;

    float playhead_    = 0.0f;
    bool  hasLoop_     = false;
    float loopStart_   = 0.0f;
    float loopEnd_     = 1.0f;
    float viewStart_   = 0.0f;
    float viewEnd_     = 1.0f;

    Color waveColor_     = Color(100, 200, 120, 255);
    Color bgColor_       = Color(20,  24,  28,  255);
    Color playheadColor_ = Color(255, 200, 80,  255);
    bool  showCentre_    = true;

    bool  dragging_    = false;
    bool  panning_     = false;
    float panStartX_   = 0.0f;
    float panVS_       = 0.0f;
    float panVE_       = 0.0f;
};
