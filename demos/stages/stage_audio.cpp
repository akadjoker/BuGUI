#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "AudioWidgets.hpp"
#include <cmath>
#include <cstdlib>
using namespace BuGUI;

void registerAudioStage(WidgetApp& app)
{
    auto* root = app.addStage("audio");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(4);
    outer->setPadding(8);

    // Header
    auto* header = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSize(0, 32);
    auto* back = header->createChild<Button>("< Back");
    back->setSize(70, 26);
    back->clicked.connect([&app]() { app.setStage("menu", TransitionType::CoverRight); });
    header->createChild<Label>("Audio Widgets — Knob · ADSR · VUMeter · Spectrum · Waveform · PianoRoll");

    // ── Row 1: Knobs + ADSR ──────────────────────────────────────────────
    outer->createChild<Label>("Knobs + ADSR Envelope");
    auto* row1 = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    row1->setSpacing(8);
    row1->setSize(0, 130);

    // Rotary knobs
    auto* k1 = row1->createChild<Knob>();
    k1->setSize(56, 72);
    k1->setLabel("Volume");
    k1->setValue(0.75f);
    k1->setArcColor(Color(100, 200, 140, 255));

    auto* k2 = row1->createChild<Knob>();
    k2->setSize(56, 72);
    k2->setLabel("Pan");
    k2->setRange(-1.f, 1.f);
    k2->setValue(0.0f);
    k2->setDefault(0.0f);
    k2->setArcColor(Color(200, 160, 80, 255));

    auto* k3 = row1->createChild<Knob>();
    k3->setSize(56, 72);
    k3->setLabel("Freq");
    k3->setRange(20.f, 20000.f);
    k3->setValue(440.f);
    k3->setArcColor(Color(180, 100, 220, 255));

    auto* k4 = row1->createChild<Knob>();
    k4->setSize(56, 72);
    k4->setLabel("Reso");
    k4->setValue(0.3f);
    k4->setArcColor(Color(220, 80, 80, 255));

    // Linear faders
    auto* lk1 = row1->createChild<LinearKnob>();
    lk1->setSize(28, 120);
    lk1->setOrientation(true); // vertical
    lk1->setLabel("Send A");
    lk1->setValue(0.6f);
    lk1->setFillColor(Color(80, 180, 120, 255));

    auto* lk2 = row1->createChild<LinearKnob>();
    lk2->setSize(28, 120);
    lk2->setOrientation(true);
    lk2->setLabel("Send B");
    lk2->setValue(0.3f);
    lk2->setFillColor(Color(120, 140, 220, 255));

    // ADSR
    auto* adsr = row1->createChild<ADSRWidget>();
    adsr->setStretch(1);
    adsr->setADSR(0.02f, 0.12f, 0.65f, 0.25f);

    // ── Row 2: VU Meters + Spectrum ──────────────────────────────────────
    outer->createChild<Label>("VU Meter + Spectrum Analyzer");
    auto* row2 = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    row2->setSpacing(8);
    row2->setSize(0, 160);

    // VU Meter (stereo, vertical)
    auto* vu = row2->createChild<VUMeter>();
    vu->setSize(80, 150);
    vu->setChannelCount(2);
    vu->setLevel(0, 0.78f);
    vu->setLevel(1, 0.55f);

    // VU Meter (horizontal, 4-channel)
    auto* vuH = row2->createChild<VUMeter>();
    vuH->setSize(180, 120);
    vuH->setOrientation(VUMeter::Orientation::Horizontal);
    vuH->setChannelCount(4);
    vuH->setLevel(0, 0.9f);
    vuH->setLevel(1, 0.6f);
    vuH->setLevel(2, 0.3f);
    vuH->setLevel(3, 0.15f);

    // Spectrum Analyzer
    auto* sa = row2->createChild<SpectrumAnalyzer>();
    sa->setStretch(1);
    sa->setBinCount(48);
    // Generate fake spectrum data (bell curve + some peaks)
    float mags[48];
    float freqs[48];
    for (int i = 0; i < 48; ++i) {
        float f = 20.f + (22000.f - 20.f) * float(i) / 47.f;
        freqs[i] = f;
        float base = std::exp(-0.5f * ((float(i) - 12.f) / 8.f) * ((float(i) - 12.f) / 8.f));
        float harmonic = (i == 6 || i == 18 || i == 30) ? 0.3f : 0.f;
        mags[i] = std::min(1.f, base * 0.7f + harmonic + float(std::rand() % 100) * 0.001f);
    }
    sa->setMagnitudes(mags, 48);
    sa->setFreqLabels(freqs, 48);

    // ── Row 3: Waveform ──────────────────────────────────────────────────
    outer->createChild<Label>("Waveform View (stereo · zoom/pan · scrub)");
    auto* wv = outer->createChild<WaveformView>();
    wv->setSize(0, 100);
    // Generate a fake stereo waveform: L=sine, R=saw
    constexpr int kSR = 44100;
    constexpr int kDur = 2; // seconds
    constexpr int kSamples = kSR * kDur;
    std::vector<float> wavBuf(kSamples * 2);
    for (int i = 0; i < kSamples; ++i) {
        float t = float(i) / kSR;
        float env = (t < 0.01f) ? t / 0.01f : std::exp(-(t - 0.01f) * 2.f);
        wavBuf[i * 2]     = env * std::sin(440.f * 2.f * 3.14159f * t); // L
        wavBuf[i * 2 + 1] = env * (2.f * std::fmod(t * 220.f, 1.f) - 1.f); // R
    }
    wv->setSamples(wavBuf.data(), kSamples * 2, 2, float(kSR));
    wv->setLoopRegion(0.1f, 0.8f);
    wv->setPlayhead(0.25f);

    // ── Row 4: PianoRoll ─────────────────────────────────────────────────
    outer->createChild<Label>("Piano Roll (click to add · middle-drag to move · right-click delete · scroll zoom)");
    auto* pr = outer->createChild<PianoRoll>();
    pr->setStretch(1);
    pr->setPitchRange(48, 76); // C3 to E5
    pr->setTimeRange(0.f, 8.f);
    pr->setPlayhead(1.0f);

    // Add a simple melody
    pr->addNote(60, 0.0f, 0.5f);  // C4
    pr->addNote(64, 0.5f, 0.5f);  // E4
    pr->addNote(67, 1.0f, 0.5f);  // G4
    pr->addNote(72, 1.5f, 1.0f);  // C5
    pr->addNote(71, 2.5f, 0.25f); // B4
    pr->addNote(67, 2.75f, 0.75f);// G4
    pr->addNote(64, 3.5f, 0.5f);  // E4
    pr->addNote(60, 4.0f, 1.5f);  // C4 (held)
    pr->addNote(62, 5.5f, 0.5f);  // D4
    pr->addNote(65, 6.0f, 1.0f);  // F4
    pr->addNote(69, 7.0f, 1.0f);  // A4
}
