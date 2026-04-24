#include "PianoRoll.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace {
    inline Font::ClipRect toFc(const Rect& r) { return {r.x, r.y, r.w, r.h}; }
    static const char* kNoteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
}

PianoRoll::PianoRoll()
{
    setPitchRange(48, 72);
}

// ── Note management ────────────────────────────────────────────────────────

void PianoRoll::addNote(int pitch, float start, float length)
{
    notes_.push_back({pitch, start, std::max(0.01f, length)});
    markDirty();
}

void PianoRoll::removeNote(int idx)
{
    if (idx < 0 || idx >= (int)notes_.size()) return;
    notes_.erase(notes_.begin() + idx);
    if (draggingNote_ == idx) draggingNote_ = -1;
    markDirty();
}

void PianoRoll::clearNotes()
{
    notes_.clear();
    draggingNote_ = -1;
    markDirty();
}

void PianoRoll::setPitchRange(int lo, int hi)
{
    pitchLo_ = std::clamp(lo, 0, 127);
    pitchHi_ = std::clamp(hi, pitchLo_ + 1, 127);
    markDirty();
}

void PianoRoll::setTimeRange(float startBeat, float endBeat)
{
    viewStart_ = startBeat;
    viewEnd_   = std::max(viewStart_ + 0.01f, endBeat);
    markDirty();
}

void PianoRoll::layout() { Widget::layout(); }

// ── Geometry helpers ────────────────────────────────────────────────────────

Rect PianoRoll::keyArea() const
{
    const Rect b = absoluteRect();
    return {b.x, b.y, keyW_, b.h};
}

Rect PianoRoll::gridArea() const
{
    const Rect b = absoluteRect();
    return {b.x + keyW_, b.y, b.w - keyW_, b.h};
}

float PianoRoll::pitchToY(int pitch) const
{
    const Rect ga = gridArea();
    int rows = pitchHi_ - pitchLo_;
    if (rows <= 0) return ga.y;
    int row = pitchHi_ - 1 - pitch;  // top = pitchHi_, bottom = pitchLo_
    return ga.y + row * rowH_;
}

int PianoRoll::yToPitch(float y) const
{
    const Rect ga = gridArea();
    int row = (int)((y - ga.y) / rowH_);
    int pitch = pitchHi_ - 1 - row;
    return std::clamp(pitch, pitchLo_, pitchHi_ - 1);
}

float PianoRoll::beatToX(float beat) const
{
    const Rect ga = gridArea();
    return ga.x + (beat - viewStart_) / (viewEnd_ - viewStart_) * ga.w;
}

float PianoRoll::xToBeat(float x) const
{
    const Rect ga = gridArea();
    return viewStart_ + (x - ga.x) / ga.w * (viewEnd_ - viewStart_);
}

Rect PianoRoll::noteRect(const PianoNote& n) const
{
    float x  = beatToX(n.start);
    float x2 = beatToX(n.start + n.length);
    float y  = pitchToY(n.pitch);
    return {x, y + 1, std::max(4.0f, x2 - x - 1), rowH_ - 2};
}

bool PianoRoll::isBlackKey(int pitch) const
{
    int pc = pitch % 12;
    return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
}

// ── Paint ───────────────────────────────────────────────────────────────────

void PianoRoll::paintKeys(PaintContext& ctx, const Rect& ka) const
{
    auto fc = toFc(ka);
    for (int p = pitchLo_; p < pitchHi_; ++p) {
        float y = pitchToY(p);
        bool black = isBlackKey(p);

        // Key background
        if (black)
            ctx.fill.SetColor(30, 32, 36, 255);
        else
            ctx.fill.SetColor(210, 210, 215, 255);
        ctx.fillRect(ka.x, y, ka.w, rowH_);

        // Row separator
        ctx.fill.SetColor(90, 90, 95, 255);
        ctx.fillRect(ka.x, y + rowH_ - 1, ka.w, 1);

        // Note name label (C notes)
        int pc = p % 12;
        if (pc == 0 && showOctave_) {
            char buf[8];
            snprintf(buf, sizeof(buf), "C%d", p / 12 - 1);
            ctx.fill.SetColor(black ? 180 : 40, black ? 180 : 40, black ? 185 : 45, 255);
            ctx.font.Print(buf, ka.x + 2, y + 1, &fc);
        }
    }
    // Right border of key area
    ctx.fill.SetColor(60, 62, 66, 255);
    ctx.fillRect(ka.x + ka.w - 1, ka.y, 1, ka.h);
}

void PianoRoll::paintGrid(PaintContext& ctx, const Rect& ga) const
{
    
    // Row backgrounds
    for (int p = pitchLo_; p < pitchHi_; ++p) {
        float y = pitchToY(p);
        bool black = isBlackKey(p);
        if (black)
            ctx.fill.SetColor(22, 24, 28, 255);
        else
            ctx.fill.SetColor(30, 32, 36, 255);
        ctx.fillRect(ga.x, y, ga.w, rowH_);

        // Row separator
        ctx.fill.SetColor(40, 42, 46, 255);
        ctx.fillRect(ga.x, y + rowH_ - 1, ga.w, 1);
    }

    // Beat/bar lines
    float beatSpan = viewEnd_ - viewStart_;
    float firstBeat = std::floor(viewStart_);
    for (float beat = firstBeat; beat <= viewEnd_; beat += 1.0f) {
        float x = beatToX(beat);
        if (x < ga.x || x > ga.x + ga.w) continue;
        bool isBar = (std::fmod(beat, (float)bpb_) < 0.001f);
        if (isBar)
            ctx.fill.SetColor(70, 74, 80, 255);
        else
            ctx.fill.SetColor(45, 48, 54, 255);
        ctx.fillRect(x, ga.y, 1, ga.h);
    }

    // Beat labels (bars only, if space allows)
    auto fc = toFc(ga);
    ctx.fill.SetColor(100, 104, 110, 255);
    for (float beat = firstBeat; beat <= viewEnd_; beat += (float)bpb_) {
        float x = beatToX(beat);
        if (x < ga.x || x > ga.x + ga.w) continue;
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", (int)(beat / bpb_) + 1);
        ctx.font.Print(buf, x + 2, ga.y + 1, &fc);
    }
}

void PianoRoll::paintNotes(PaintContext& ctx, const Rect& ga) const
{
    for (int i = 0; i < (int)notes_.size(); ++i) {
        const auto& n = notes_[i];
        if (n.pitch < pitchLo_ || n.pitch >= pitchHi_) continue;
        Rect nr = noteRect(n);
        if (nr.x + nr.w < ga.x || nr.x > ga.x + ga.w) continue;

        bool active = (i == draggingNote_);
        Color c = active ? noteSelColor_ : noteColor_;
        ctx.fill.SetColor(c.r, c.g, c.b, c.a);
        ctx.fillRect(nr.x, nr.y, nr.w, nr.h);

        // Top highlight
        ctx.fill.SetColor(
            std::min(255, c.r + 60),
            std::min(255, c.g + 60),
            std::min(255, c.b + 60), 200);
        ctx.fillRect(nr.x, nr.y, nr.w, 2);

        // Border
        ctx.fill.SetColor(c.r/2, c.g/2, c.b/2, 255);
        ctx.fillRect(nr.x,          nr.y,          nr.w, 1);
        ctx.fillRect(nr.x,          nr.y+nr.h-1,   nr.w, 1);
        ctx.fillRect(nr.x,          nr.y,          1,    nr.h);
        ctx.fillRect(nr.x+nr.w-1,   nr.y,          1,    nr.h);
    }
}

void PianoRoll::paintPlayhead(PaintContext& ctx, const Rect& ga) const
{
    float x = beatToX(playhead_);
    if (x < ga.x || x > ga.x + ga.w) return;
    ctx.fill.SetColor(255, 200, 80, 220);
    ctx.fillRect(x, ga.y, 2, ga.h);
}

void PianoRoll::paint(PaintContext& ctx)
{

    const Rect b  = absoluteRect();
    const Rect ka = keyArea();
    const Rect ga = gridArea();

    ctx.pushClip(b);

    // Overall background
    ctx.fill.SetColor(24, 26, 30, 255);
    ctx.fillRect(b.x, b.y, b.w, b.h);

    ctx.pushClip(ga);
    paintGrid(ctx, ga);
    paintNotes(ctx, ga);
    paintPlayhead(ctx, ga);
    ctx.popClip();

    ctx.pushClip(ka);
    paintKeys(ctx, ka);
    ctx.popClip();

    // Outer border
    ctx.fill.SetColor(55, 58, 65, 255);
    ctx.fillRect(b.x,        b.y,        b.w, 1);
    ctx.fillRect(b.x,        b.y+b.h-1,  b.w, 1);
    ctx.fillRect(b.x,        b.y,        1,   b.h);
    ctx.fillRect(b.x+b.w-1,  b.y,        1,   b.h);

    ctx.popClip();
}

// ── Mouse interaction ────────────────────────────────────────────────────────

void PianoRoll::onMousePress(MouseEvent& e)
{
    const Rect ga = gridArea();
    if (e.x < ga.x || e.x > ga.x + ga.w ||
        e.y < ga.y || e.y > ga.y + ga.h) return;

    if (e.button == 2) {
        // Right-click: delete note under cursor
        for (int i = (int)notes_.size() - 1; i >= 0; --i) {
            Rect nr = noteRect(notes_[i]);
            if (e.x >= nr.x && e.x <= nr.x + nr.w &&
                e.y >= nr.y && e.y <= nr.y + nr.h) {
                onNoteRemoved.emit(i);
                removeNote(i);
                e.consumed = true;
                return;
            }
        }
        return;
    }

    if (e.button == 1) {
        // Try to pick existing note
        for (int i = (int)notes_.size() - 1; i >= 0; --i) {
            Rect nr = noteRect(notes_[i]);
            if (e.x >= nr.x && e.x <= nr.x + nr.w &&
                e.y >= nr.y && e.y <= nr.y + nr.h) {
                draggingNote_ = i;
                dragOffBeat_  = xToBeat(e.x) - notes_[i].start;
                dragOffPitch_ = yToPitch(e.y) - notes_[i].pitch;
                onNoteClicked.emit(i);
                e.consumed = true;
                return;
            }
        }
        // New note: click+drag to set length
        int pitch    = yToPitch(e.y);
        float beat   = xToBeat(e.x);
        // Snap to 0.25 beat
        beat = std::floor(beat * 4.0f) / 4.0f;
        newNotePitch_ = pitch;
        newNoteStart_ = beat;
        addNote(pitch, beat, 0.25f);
        draggingNote_ = (int)notes_.size() - 1;
        dragOffBeat_  = 0.0f;
        dragOffPitch_ = 0;
        e.consumed = true;
    }

    if (e.button == 3) {
        // Middle: pan
        panning_   = true;
        panStartX_ = e.x;
        panVS_     = viewStart_;
        panVE_     = viewEnd_;
        e.consumed = true;
    }
}

void PianoRoll::onMouseMove(MouseEvent& e)
{
    if (panning_) {
        const Rect ga = gridArea();
        float dx    = e.x - panStartX_;
        float dtpp  = (panVE_ - panVS_) / ga.w;
        float shift = -dx * dtpp;
        viewStart_ = panVS_ + shift;
        viewEnd_   = panVE_ + shift;
        markDirty();
        return;
    }

    if (draggingNote_ >= 0 && draggingNote_ < (int)notes_.size()) {
        float beat  = xToBeat(e.x) - dragOffBeat_;
        beat = std::max(0.0f, std::floor(beat * 4.0f) / 4.0f);
        int pitch   = yToPitch(e.y) - dragOffPitch_;
        pitch = std::clamp(pitch, pitchLo_, pitchHi_ - 1);
        auto& n = notes_[draggingNote_];
        n.start = beat;
        n.pitch = pitch;
        markDirty();
        e.consumed = true;
    }
}

void PianoRoll::onMouseRelease(MouseEvent& e)
{
    if (draggingNote_ >= 0) {
        onNoteAdded.emit(notes_[draggingNote_].pitch,
                         notes_[draggingNote_].start,
                         notes_[draggingNote_].length);
    }
    draggingNote_ = -1;
    newNotePitch_ = -1;
    panning_      = false;
}

void PianoRoll::onMouseScroll(MouseEvent& e)
{
    const Rect ga = gridArea();
    float t      = viewStart_ + (e.x - ga.x) / ga.w * (viewEnd_ - viewStart_);
    float span   = viewEnd_ - viewStart_;
    float factor = (e.scrollY > 0) ? 0.8f : 1.0f / 0.8f;
    span = std::clamp(span * factor, 0.25f, 64.0f);

    float ns = t - (e.x - ga.x) / ga.w * span;
    viewStart_ = ns;
    viewEnd_   = ns + span;
    markDirty();
    e.consumed = true;
}
