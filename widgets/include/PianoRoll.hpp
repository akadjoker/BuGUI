#pragma once

#include "Widget.hpp"
#include <vector>
#include <functional>

// ═════════════════════════════════════════════════════════════════════════════
//  PianoRoll - mini piano keyboard + note grid
//
//  Usage:
//    auto* pr = parent->createChild<PianoRoll>();
//    pr->setRange(36, 84);   // C2 to C6 (MIDI note numbers)
//    pr->addNote(60, 0.0f, 0.5f);  // Middle C, beat 0, length 0.5 beats
//    pr->onNoteAdded.connect([](int note, float start, float len){ ... });
//
//  Layout:
//    Left side: piano keyboard (white/black keys, optionally labelled)
//    Right side: note grid (scrollable horizontally, zoom with wheel)
//
//  Interaction:
//    - Left-click empty grid   → add note
//    - Left-drag note          → move note
//    - Right-click note        → delete note
//    - Middle-drag / Space+drag → pan
//    - Scroll wheel            → zoom time axis
// ═════════════════════════════════════════════════════════════════════════════

struct PianoNote {
    int   pitch;   // MIDI note: 0–127
    float start;   // beat
    float length;  // beats
};

class PianoRoll : public Widget
{
public:
    PianoRoll();

    // ── Note management ───────────────────────────────────────────────────
    void addNote(int pitch, float start, float length);
    void removeNote(int idx);
    void clearNotes();

    const std::vector<PianoNote>& notes() const { return notes_; }

    // ── View range ────────────────────────────────────────────────────────
    void setPitchRange(int lo, int hi);  // MIDI note numbers
    void setTimeRange(float startBeat, float endBeat);
    void setBeatsPerBar(int bpb) { bpb_ = bpb; markDirty(); }

    int   pitchLo() const { return pitchLo_; }
    int   pitchHi() const { return pitchHi_; }

    // ── Playhead ──────────────────────────────────────────────────────────
    void  setPlayhead(float beat)     { playhead_ = beat; markDirty(); }
    float playhead() const            { return playhead_; }

    // ── Appearance ────────────────────────────────────────────────────────
    void setKeyWidth(float w)         { keyW_ = w; markDirty(); }
    void setRowHeight(float h)        { rowH_ = h; markDirty(); }
    void setNoteColor(const Color& c) { noteColor_ = c; markDirty(); }
    void setShowOctaveLabels(bool s)  { showOctave_ = s; markDirty(); }

    // ── Signals ───────────────────────────────────────────────────────────
    Signal<int, float, float> onNoteAdded;    // pitch, start, length
    Signal<int>               onNoteRemoved;  // index
    Signal<int>               onNoteClicked;  // index
    Signal<float>             onScrub;        // beat position

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override { return {400, 200}; }
    void  layout() override;
    void  paint(PaintContext& ctx) override;
    void  onMousePress(MouseEvent& e) override;
    void  onMouseMove(MouseEvent& e) override;
    void  onMouseRelease(MouseEvent& e) override;
    void  onMouseScroll(MouseEvent& e) override;

private:
    // Key/grid geometry helpers
    Rect  gridArea()  const;
    Rect  keyArea()   const;
    float pitchToY(int pitch) const;      // top-left Y of that row in grid
    int   yToPitch(float y)   const;
    float beatToX(float beat) const;
    float xToBeat(float x)    const;
    Rect  noteRect(const PianoNote& n) const;
    bool  isBlackKey(int pitch) const;
    void  paintKeys(PaintContext& ctx, const Rect& ka) const;
    void  paintGrid(PaintContext& ctx, const Rect& ga) const;
    void  paintNotes(PaintContext& ctx, const Rect& ga) const;
    void  paintPlayhead(PaintContext& ctx, const Rect& ga) const;

    std::vector<PianoNote> notes_;
    int   pitchLo_ = 48;   // C3
    int   pitchHi_ = 72;   // C5
    int   bpb_     = 4;    // beats per bar
    float viewStart_ = 0.0f;  // visible beat range start
    float viewEnd_   = 8.0f;  // visible beat range end
    float playhead_  = 0.0f;

    float keyW_    = 36.0f;  // piano keyboard width
    float rowH_    = 12.0f;  // height per pitch row

    Color noteColor_     = Color(100, 160, 240, 220);
    Color noteSelColor_  = Color(240, 200,  60, 255);
    bool  showOctave_    = true;

    // Interaction state
    int   draggingNote_  = -1;
    float dragOffBeat_   = 0.0f;
    int   dragOffPitch_  = 0;
    bool  panning_       = false;
    float panStartX_     = 0.0f;
    float panVS_         = 0.0f;
    float panVE_         = 0.0f;
    // For new-note drawing (drag to set length)
    int   newNotePitch_  = -1;
    float newNoteStart_  = 0.0f;
};
