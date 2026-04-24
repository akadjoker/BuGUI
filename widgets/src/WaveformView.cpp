#include "WaveformView.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace {
    inline Font::ClipRect toFc(const Rect& r) { return {r.x, r.y, r.w, r.h}; }
}

WaveformView::WaveformView() {}

void WaveformView::setSamples(const float* data, int count, int channels, float sampleRate)
{
    if (!data || count <= 0) { clearSamples(); return; }
    channels_    = std::max(1, channels);
    sampleCount_ = count / channels_;
    sampleRate_  = sampleRate;
    samples_.assign(data, data + count);
    markDirty();
}

void WaveformView::clearSamples()
{
    samples_.clear();
    sampleCount_ = 0;
    channels_    = 1;
    markDirty();
}

void WaveformView::setLoopRegion(float start, float end)
{
    loopStart_ = std::clamp(start, 0.0f, 1.0f);
    loopEnd_   = std::clamp(end,   0.0f, 1.0f);
    hasLoop_   = true;
    markDirty();
}

void WaveformView::setViewRange(float start, float end)
{
    viewStart_ = std::clamp(start, 0.0f, 1.0f);
    viewEnd_   = std::clamp(end,   viewStart_ + 0.001f, 1.0f);
    markDirty();
}

std::vector<WaveformView::EnvPair> WaveformView::buildEnvelope(int ch, float vs, float ve, int pixelW) const
{
    std::vector<EnvPair> env(pixelW, {0.0f, 0.0f});
    if (sampleCount_ <= 0 || pixelW <= 0) return env;

    for (int px = 0; px < pixelW; ++px) {
        float t0 = vs + (ve - vs) * ((float)px       / pixelW);
        float t1 = vs + (ve - vs) * ((float)(px + 1) / pixelW);
        int s0 = (int)(t0 * sampleCount_);
        int s1 = (int)(t1 * sampleCount_);
        s0 = std::clamp(s0, 0, sampleCount_ - 1);
        s1 = std::clamp(s1, s0, sampleCount_ - 1);

        float mn = 0.0f, mx = 0.0f;
        bool first = true;
        for (int si = s0; si <= s1; ++si) {
            int idx = si * channels_ + ch;
            if (idx >= (int)samples_.size()) break;
            float v = samples_[idx];
            if (first) { mn = mx = v; first = false; }
            else { mn = std::min(mn, v); mx = std::max(mx, v); }
        }
        env[px] = {mn, mx};
    }
    return env;
}

void WaveformView::paint(PaintContext& ctx)
{
    
    const Rect b = absoluteRect();
    ctx.pushClip(b);

    // Background
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, 255);
    ctx.fillRect(b.x, b.y, b.w, b.h);

    const int nch = std::max(1, channels_);
    float chH = b.h / nch;

    for (int ch = 0; ch < nch; ++ch) {
        float chY0 = b.y + ch * chH;
        float mid  = chY0 + chH * 0.5f;

        // Centre line
        if (showCentre_) {
            ctx.line.SetColor(50, 55, 60, 255);
            ctx.drawLine(b.x, mid, b.x + b.w, mid);
        }

        if (sampleCount_ > 0) {
            // Loop region
            if (hasLoop_) {
                float lx0 = b.x + (loopStart_ - viewStart_) / (viewEnd_ - viewStart_) * b.w;
                float lx1 = b.x + (loopEnd_   - viewStart_) / (viewEnd_ - viewStart_) * b.w;
                ctx.fill.SetColor(waveColor_.r, waveColor_.g, waveColor_.b, 40);
                ctx.fillRect(lx0, chY0, lx1 - lx0, chH);
            }

            // Envelope - draw vertical lines via line batch (2 verts each, not 6)
            // Cap at 512 columns - line batch is 2048*4=8192 verts, 512*2ch*2=2048 fits
            const int maxCols = 512;
            int pw = std::min(maxCols, (int)b.w);
            float colW = b.w / pw;
            auto env = buildEnvelope(ch, viewStart_, viewEnd_, pw);
            ctx.line.SetColor(waveColor_.r, waveColor_.g, waveColor_.b, 220);
            for (int px = 0; px < pw; ++px) {
                float top    = mid - env[px].mx * chH * 0.5f;
                float bottom = mid - env[px].mn * chH * 0.5f;
                if (bottom <= top) bottom = top + 1.0f;
                float cx = b.x + (px + 0.5f) * colW;
                ctx.drawLine(cx, top, cx, bottom);
            }
        } else {
            // No data - show placeholder
            ctx.fill.SetColor(50, 55, 60, 255);
            ctx.fillRect(b.x, mid, b.w, 1);
        }

        // Channel separator
        if (ch > 0) {
            ctx.line.SetColor(40, 44, 50, 255);
            ctx.drawLine(b.x, chY0, b.x + b.w, chY0);
        }
    }

    // Playhead
    {
        float t = (playhead_ - viewStart_) / (viewEnd_ - viewStart_);
        if (t >= 0.0f && t <= 1.0f) {
            float px = b.x + t * b.w;
            ctx.fill.SetColor(playheadColor_.r, playheadColor_.g, playheadColor_.b, 255);
            ctx.fillRect(px, b.y, 2, b.h);
        }
    }

    // Border
    ctx.line.SetColor(60, 65, 70, 255);
    ctx.drawLine(b.x,        b.y,        b.x+b.w,   b.y);
    ctx.drawLine(b.x,        b.y+b.h-1,  b.x+b.w,   b.y+b.h-1);
    ctx.drawLine(b.x,        b.y,        b.x,        b.y+b.h);
    ctx.drawLine(b.x+b.w-1,  b.y,        b.x+b.w-1, b.y+b.h);

    ctx.popClip();
}

void WaveformView::onMousePress(MouseEvent& e)
{
    if (e.button == 1) {
        const Rect b = absoluteRect();
        float t = viewStart_ + (e.x - b.x) / b.w * (viewEnd_ - viewStart_);
        t = std::clamp(t, 0.0f, 1.0f);
        playhead_ = t;
        dragging_ = true;
        onScrub.emit(t);
        markDirty();
        e.consumed = true;
    } else if (e.button == 2) {
        // Middle button pan
        const Rect b = absoluteRect();
        panning_   = true;
        panStartX_ = e.x - b.x;
        panVS_     = viewStart_;
        panVE_     = viewEnd_;
        e.consumed = true;
    }
}

void WaveformView::onMouseMove(MouseEvent& e)
{
    const Rect b = absoluteRect();
    if (dragging_) {
        float t = viewStart_ + (e.x - b.x) / b.w * (viewEnd_ - viewStart_);
        t = std::clamp(t, 0.0f, 1.0f);
        playhead_ = t;
        onScrub.emit(t);
        markDirty();
    } else if (panning_) {
        float dx    = (e.x - b.x) - panStartX_;
        float dtpp  = (panVE_ - panVS_) / b.w;  // delta t per pixel
        float shift = -dx * dtpp;
        float span  = panVE_ - panVS_;
        float ns    = panVS_ + shift;
        float ne    = panVE_ + shift;
        if (ns < 0.0f) { ns = 0.0f; ne = span; }
        if (ne > 1.0f) { ne = 1.0f; ns = 1.0f - span; }
        viewStart_ = ns;
        viewEnd_   = ne;
        markDirty();
    }
}

void WaveformView::onMouseRelease(MouseEvent& e)
{
    dragging_ = false;
    panning_  = false;
}

void WaveformView::onMouseScroll(MouseEvent& e)
{
    const Rect b = absoluteRect();
    float t    = viewStart_ + (e.x - b.x) / b.w * (viewEnd_ - viewStart_);
    float span = viewEnd_ - viewStart_;
    float factor = (e.scrollY > 0) ? 0.85f : 1.0f / 0.85f;
    span = std::clamp(span * factor, 0.001f, 1.0f);

    float ns = t - (e.x - b.x) / b.w * span;
    float ne = ns + span;
    if (ns < 0.0f) { ns = 0.0f; ne = span; }
    if (ne > 1.0f) { ne = 1.0f; ns = 1.0f - span; }
    viewStart_ = ns;
    viewEnd_   = ne;
    markDirty();
    e.consumed = true;
}
