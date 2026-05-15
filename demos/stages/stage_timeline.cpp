#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "Timeline.hpp"
using namespace BuGUI;

void registerTimelineStage(WidgetApp& app)
{
    auto* root = app.addStage("timeline");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(4);
    outer->setPadding(8);

    // Header
    auto* header = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSize(0, 32);
    auto* back = header->createChild<Button>("< Back");
    back->setSize(70, 26);
    back->clicked.connect([&app]() { app.setStage("menu", TransitionType::CoverRight); });
    header->createChild<Label>("Timeline — Click ruler to scrub · Drag clips/keyframes · Middle-drag pan · Scroll zoom");

    // ── Timeline widget ──────────────────────────────────────────────────
    auto* tl = outer->createChild<Timeline>();
    tl->setStretch(1);
    tl->setTimeRange(0.0f, 6.0f);
    tl->setFrameRate(30.0f);

    // ── Track 1: Position ────────────────────────────────────────────────
    int t0 = tl->addTrack("Position", Color(100, 200, 120, 255));
    tl->addKeyframe(t0, 0.0f);
    tl->addKeyframe(t0, 1.0f);
    tl->addKeyframe(t0, 2.5f);
    tl->addKeyframe(t0, 4.0f);

    // ── Track 2: Rotation ────────────────────────────────────────────────
    int t1 = tl->addTrack("Rotation", Color(120, 140, 220, 255));
    tl->addKeyframe(t1, 0.5f);
    tl->addKeyframe(t1, 2.0f);
    tl->addKeyframe(t1, 3.5f);

    // ── Track 3: Scale ───────────────────────────────────────────────────
    int t2 = tl->addTrack("Scale", Color(220, 160, 80, 255));
    tl->addKeyframe(t2, 0.0f);
    tl->addKeyframe(t2, 3.0f);
    tl->addClip(t2, 0.0f, 1.5f, "Grow", Color(180, 120, 40, 180));
    tl->addClip(t2, 2.0f, 3.5f, "Shrink", Color(140, 90, 30, 180));

    // ── Track 4: Opacity ─────────────────────────────────────────────────
    int t3 = tl->addTrack("Opacity", Color(180, 100, 200, 255));
    tl->addClip(t3, 0.0f, 0.8f, "Fade In",  Color(140, 70, 170, 180));
    tl->addClip(t3, 3.5f, 5.0f, "Fade Out", Color(140, 70, 170, 180));

    // ── Track 5: Audio ───────────────────────────────────────────────────
    int t4 = tl->addTrack("Audio", Color(100, 180, 180, 255));
    tl->addClip(t4, 0.2f, 4.8f, "Music.wav", Color(60, 140, 140, 200));
    tl->addKeyframe(t4, 0.2f);
    tl->addKeyframe(t4, 2.5f);
    tl->addKeyframe(t4, 4.8f);

    // ── Track 6: Camera ──────────────────────────────────────────────────
    int t5 = tl->addTrack("Camera", Color(200, 200, 100, 255));
    tl->addClip(t5, 0.0f, 2.0f, "Wide",   Color(160, 160, 60, 180));
    tl->addClip(t5, 2.0f, 4.0f, "Close",  Color(130, 130, 50, 180));
    tl->addClip(t5, 4.0f, 5.5f, "Aerial", Color(100, 100, 40, 180));
    tl->addKeyframe(t5, 0.0f);
    tl->addKeyframe(t5, 2.0f);
    tl->addKeyframe(t5, 4.0f);

    // Playhead at 1 second
    tl->setPlayhead(1.0f);

    // ── Playhead info label ──────────────────────────────────────────────
    auto* info = outer->createChild<Label>("Playhead: 1.000s (frame 30)");
    tl->onPlayheadChanged.connect([info, tl](float t) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Playhead: %.3fs (frame %d)", t,
                 static_cast<int>(std::round(t * tl->frameRate())));
        info->setText(buf);
    });
}
