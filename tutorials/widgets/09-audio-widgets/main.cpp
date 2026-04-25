#include "WidgetApp.hpp"
#include "Widgets.hpp"

#include <cmath>
#include <vector>

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 09 - Audio Widgets", 1280, 820))
        return 1;

    Widget* root = app.addStage("main");
    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(12, 12, 12, 12);
    outer->setSpacing(10);

    auto* title = outer->createChild<Label>("Tutorial 09: ADSR + VUMeter + Waveform + Spectrum + PianoRoll");
    title->setColor(Color(120, 190, 255, 255));
    auto* status = outer->createChild<Label>("Animated fake audio data for widget testing.");

    auto* top = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    top->setSpacing(10);

    auto* adsrPanel = top->createChild<Panel>();
    adsrPanel->setStretch(2);
    auto* adsrCol = adsrPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    adsrCol->setPadding(8, 8, 8, 8);
    adsrCol->createChild<Label>("ADSR");
    auto* adsr = adsrCol->createChild<ADSRWidget>();
    adsr->setStretch(1);

    auto* vuPanel = top->createChild<Panel>();
    vuPanel->setStretch(1);
    auto* vuCol = vuPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    vuCol->setPadding(8, 8, 8, 8);
    vuCol->createChild<Label>("VU Meter");
    auto* vu = vuCol->createChild<VUMeter>();
    vu->setStretch(1);
    vu->setChannelCount(2);

    auto* wavePanel = outer->createChild<Panel>();
    wavePanel->setStretch(1);
    auto* waveCol = wavePanel->createChild<BoxLayout>(LayoutDir::Vertical);
    waveCol->setPadding(8, 8, 8, 8);
    waveCol->createChild<Label>("Waveform");
    auto* wave = waveCol->createChild<WaveformView>();
    wave->setStretch(1);

    auto* spectrumPanel = outer->createChild<Panel>();
    spectrumPanel->setStretch(1);
    auto* specCol = spectrumPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    specCol->setPadding(8, 8, 8, 8);
    specCol->createChild<Label>("Spectrum");
    auto* spec = specCol->createChild<SpectrumAnalyzer>();
    spec->setStretch(1);
    spec->setBinCount(48);

    auto* pianoPanel = outer->createChild<Panel>();
    pianoPanel->setStretch(1);
    auto* prCol = pianoPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    prCol->setPadding(8, 8, 8, 8);
    prCol->createChild<Label>("Piano Roll");
    auto* piano = prCol->createChild<PianoRoll>();
    piano->setStretch(1);
    piano->setPitchRange(48, 72);
    piano->addNote(60, 0.0f, 0.5f);
    piano->addNote(64, 0.5f, 0.5f);
    piano->addNote(67, 1.0f, 0.75f);

    std::vector<float> waveform(44100);
    for (size_t i = 0; i < waveform.size(); ++i)
    {
        float t = static_cast<float>(i) / 44100.0f;
        waveform[i] = 0.55f * std::sin(2.0f * 3.1415926f * 220.0f * t)
                    + 0.25f * std::sin(2.0f * 3.1415926f * 440.0f * t);
    }
    wave->setSamples(waveform.data(), static_cast<int>(waveform.size()), 1, 44100.0f);

    adsr->onChanged.connect([status](float a, float d, float s, float r) {
        status->setText("ADSR: A=" + std::to_string(a) + " D=" + std::to_string(d) +
                        " S=" + std::to_string(s) + " R=" + std::to_string(r));
    });

    piano->onNoteAdded.connect([status](int pitch, float start, float len) {
        status->setText("Note added: pitch=" + std::to_string(pitch) +
                        " start=" + std::to_string(start) +
                        " len=" + std::to_string(len));
    });

    app.setOnIdle([vu, spec, wave, piano](float dt) {
        static float tm = 0.0f;
        tm += dt;

        float l = 0.5f + 0.45f * std::sin(tm * 2.8f);
        float r = 0.5f + 0.45f * std::sin(tm * 3.5f + 0.9f);
        vu->setLevel(0, l);
        vu->setLevel(1, r);
        vu->update(dt);

        constexpr int bins = 48;
        float mags[bins];
        for (int i = 0; i < bins; ++i)
        {
            float phase = tm * (1.8f + i * 0.03f) + i * 0.15f;
            mags[i] = 0.08f + 0.92f * std::fabs(std::sin(phase));
        }
        spec->setMagnitudes(mags, bins);
        spec->update(dt);

        float play = std::fmod(tm * 0.2f, 1.0f);
        wave->setPlayhead(play);
        piano->setPlayhead(std::fmod(tm * 1.2f, 8.0f));
    });

    app.setStage("main");
    return app.run();
}
