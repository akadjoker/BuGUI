#pragma once

#include "StageCommon.hpp"
#include "VUMeter.hpp"
#include "WaveformView.hpp"
#include "SpectrumAnalyzer.hpp"
#include "ADSRWidget.hpp"
#include "PianoRoll.hpp"
#include "Knob.hpp"
#include <cmath>
#include <vector>

// ═════════════════════════════════════════════════════════════════════════════
//  Sound Widgets demo stage
// ═════════════════════════════════════════════════════════════════════════════

inline void buildSoundStage(WidgetApp& app)
{
    Widget* root = app.addStage("sound");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(6);
    outer->setPadding(8);

    // Header
    {
        auto* hdr = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
        hdr->setSize(0, 32);
        auto* back = hdr->createChild<Button>("< Back");
        back->setSize(70, 26);
        back->clicked.connect([&app]() { app.setStage("home"); });
        hdr->createChild<Label>("Sound Widgets")->setColor(Clr::accent);
    }

    // ── Row 1: VU Meters ─────────────────────────────────────────────────
    outer->createChild<Label>("VU Meter (stereo)")->setColor(Clr::section);
    {
        auto* row = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
        row->setSpacing(8);
        row->setSize(0, 100);

        // Vertical stereo
        auto* vu1 = row->createChild<VUMeter>();
        vu1->setSize(80, 0);
        vu1->setChannelCount(2);
        vu1->setLevel(0, 0.85f);
        vu1->setLevel(1, 0.62f);

        // Horizontal 4-channel
        auto* vu2 = row->createChild<VUMeter>();
        vu2->setStretch(1);
        vu2->setOrientation(VUMeter::Orientation::Horizontal);
        vu2->setChannelCount(4);
        vu2->setLevel(0, 0.90f);
        vu2->setLevel(1, 0.55f);
        vu2->setLevel(2, 0.30f);
        vu2->setLevel(3, 0.75f);
    }

    // ── Row 2: Waveform View ──────────────────────────────────────────────
    outer->createChild<Label>("Waveform View (stereo · scroll+zoom)")->setColor(Clr::section);
    {
        auto* wv = outer->createChild<WaveformView>();
        wv->setStretch(1);
        wv->setSize(0, 90);

        // Generate stereo demo waveform
        const int SR    = 44100;
        const int DUR   = 2;   // seconds
        const int total = SR * DUR * 2;  // stereo interleaved
        std::vector<float> samples(total);
        for (int i = 0; i < SR * DUR; ++i) {
            float t     = (float)i / SR;
            float left  = std::sin(2.0f * 3.14159f * 440.0f * t) * 0.6f
                        + std::sin(2.0f * 3.14159f * 880.0f * t) * 0.2f;
            float right = std::sin(2.0f * 3.14159f * 330.0f * t) * 0.5f
                        + std::sin(2.0f * 3.14159f * 660.0f * t) * 0.3f;
            samples[i * 2]     = left;
            samples[i * 2 + 1] = right;
        }
        wv->setSamples(samples.data(), (int)samples.size(), 2, (float)SR);
        wv->setLoopRegion(0.2f, 0.7f);
        wv->setPlayhead(0.1f);
    }

    // ── Row 3: Spectrum Analyzer ──────────────────────────────────────────
    outer->createChild<Label>("Spectrum Analyzer (32 bins)")->setColor(Clr::section);
    {
        auto* sa = outer->createChild<SpectrumAnalyzer>();
        sa->setStretch(1);
        sa->setSize(0, 100);
        sa->setBinCount(32);
        sa->setLogFrequency(false);
        sa->setShowLabels(true);

        // Demo magnitudes: pink noise-ish (1/f rolloff)
        const int N = 32;
        std::vector<float> mags(N), freqs(N);
        for (int i = 0; i < N; ++i) {
            float f  = 20.0f * std::pow(1000.0f, (float)i / (N - 1));
            freqs[i] = f;
            // Rough 1/f shape + some peaks
            float mag = 0.8f / (1.0f + i * 0.04f);
            if (i == 4)  mag = std::min(1.0f, mag + 0.3f);
            if (i == 10) mag = std::min(1.0f, mag + 0.2f);
            if (i == 20) mag = std::min(1.0f, mag + 0.15f);
            mags[i] = std::clamp(mag, 0.0f, 1.0f);
        }
        sa->setFreqLabels(freqs.data(), N);
        sa->setMagnitudes(mags.data(), N);
    }

    // ── Row 5: Knobs ──────────────────────────────────────────────────────
    outer->createChild<Label>("Knobs (circular)")->setColor(Clr::section);
    {
        auto* row = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
        row->setSpacing(12);
        row->setSize(0, 72);

        struct KnobDef { const char* label; float val; Color col; };
        KnobDef defs[] = {
            {"Volume",    0.80f, Color(100, 180, 240, 255)},
            {"Pan",       0.50f, Color(140, 220, 140, 255)},
            {"Cutoff",    0.65f, Color(240, 160,  80, 255)},
            {"Resonance", 0.30f, Color(200, 100, 200, 255)},
            {"Drive",     0.15f, Color(240,  80,  80, 255)},
        };
        for (auto& d : defs) {
            auto* k = row->createChild<Knob>();
            k->setSize(56, 0);
            k->setLabel(d.label);
            k->setValue(d.val);
            k->setArcColor(d.col);
        }
        row->createChild<Widget>()->setStretch(1); // spacer
    }

    // ── Row 6: Linear Knobs ───────────────────────────────────────────────
    outer->createChild<Label>("Linear Knobs (horizontal)")->setColor(Clr::section);
    {
        auto* row = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
        row->setSpacing(12);
        row->setSize(0, 48);

        struct LDef { const char* label; float val; };
        LDef defs[] = {
            {"Attack",  0.10f},
            {"Decay",   0.30f},
            {"Sustain", 0.70f},
            {"Release", 0.40f},
        };
        for (auto& d : defs) {
            auto* lk = row->createChild<LinearKnob>();
            lk->setStretch(1);
            lk->setLabel(d.label);
            lk->setValue(d.val);
        }
    }

    // ── Row 4: ADSR + PianoRoll side-by-side ─────────────────────────────
    outer->createChild<Label>("ADSR Envelope  ·  Piano Roll")->setColor(Clr::section);
    {
        auto* row = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
        row->setSpacing(8);
        row->setStretch(2);

        // ADSR
        auto* adsr = row->createChild<ADSRWidget>();
        adsr->setSize(240, 0);
        adsr->setADSR(0.02f, 0.15f, 0.65f, 0.25f);
        adsr->onChanged.connect([](float a, float d, float s, float r) {
            // would update synth here
        });

        // Piano Roll
        auto* pr = row->createChild<PianoRoll>();
        pr->setStretch(1);
        pr->setPitchRange(48, 72);   // C3–C5
        pr->setTimeRange(0.0f, 8.0f);
        pr->setBeatsPerBar(4);
        pr->setRowHeight(11.0f);

        // Demo notes — simple C major arpeggio
        int melody[] = {60, 64, 67, 72, 67, 64, 60, 62, 65, 69, 65, 62};
        for (int i = 0; i < 12; ++i)
            pr->addNote(melody[i], i * 0.5f, 0.45f);
    }
}
