#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "ChartWidgets.hpp"
#include <cmath>
using namespace BuGUI;

void registerChartsStage(WidgetApp& app)
{
    auto* root = app.addStage("charts");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(4);
    outer->setPadding(8);

    // Header
    auto* header = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSize(0, 32);
    auto* back = header->createChild<Button>("< Back");
    back->setSize(70, 26);
    back->clicked.connect([&app]() { app.setStage("menu", TransitionType::CoverRight); });
    header->createChild<Label>("Charts — Gradient · Plot · Histogram · CurveEditor");

    // ── Row 1: GradientEditor ─────────────────────────────────────────────
    outer->createChild<Label>("Gradient Editor");

    auto* gradCol = outer->createChild<BoxLayout>(LayoutDir::Vertical);
    gradCol->setSpacing(4);

    auto* grad = gradCol->createChild<GradientEditor>();
    grad->setSize(0, 58);
    grad->clearStops();
    grad->addStop(0.0f,  Color( 20,  10,  40, 255));
    grad->addStop(0.25f, Color(120,  30,  80, 255));
    grad->addStop(0.5f,  Color(220,  80,  30, 255));
    grad->addStop(0.75f, Color(240, 180,  60, 255));
    grad->addStop(1.0f,  Color(255, 240, 200, 255));

    gradCol->createChild<Label>("Double-click stop to edit · Right-click to remove · Click bar to add");

    outer->createChild<Spacer>(4.0f);

    // ── Row 2: PlotWidget (3 series) ─────────────────────────────────────
    outer->createChild<Label>("Plot Widget (Line / Bar / Scatter)");

    auto* plotRow = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    plotRow->setSpacing(6);
    plotRow->setStretch(2);

    // Line plot — sine waves
    {
        auto* plot = plotRow->createChild<PlotWidget>();
        plot->setStretch(1);
        plot->setTitle("Sine Waves");
        plot->setXLabel("t");
        plot->setYLabel("y");
        int s0 = plot->addSeries("sin(t)",     Color(100, 200, 100, 255), PlotType::Line);
        int s1 = plot->addSeries("sin(2t+1)",  Color(100, 150, 240, 255), PlotType::Line);
        int s2 = plot->addSeries("cos(t)*0.5", Color(230, 130,  80, 255), PlotType::Line);
        for (int i = 0; i <= 100; ++i) {
            float t = i * 0.1f;
            plot->addPoint(s0, std::sin(t));
            plot->addPoint(s1, std::sin(2*t + 1.0f));
            plot->addPoint(s2, std::cos(t) * 0.5f);
        }
        plot->resetView();
    }

    // Bar plot
    {
        auto* plot = plotRow->createChild<PlotWidget>();
        plot->setStretch(1);
        plot->setTitle("Bar Chart");
        int s0 = plot->addSeries("Revenue", Color(80, 180, 120, 255), PlotType::Bar);
        int s1 = plot->addSeries("Costs",   Color(220, 80,  80, 255), PlotType::Bar);
        float rev[] = {4.2f, 5.1f, 3.8f, 6.3f, 7.1f, 5.5f, 8.2f, 6.9f};
        float cos[] = {2.1f, 2.8f, 2.5f, 3.1f, 3.8f, 3.0f, 4.1f, 3.5f};
        for (float v : rev) plot->addPoint(s0, v);
        for (float v : cos) plot->addPoint(s1, v);
        plot->resetView();
    }

    // Scatter plot
    {
        auto* plot = plotRow->createChild<PlotWidget>();
        plot->setStretch(1);
        plot->setTitle("Scatter");
        int s0 = plot->addSeries("Points", Color(200, 160, 80, 255), PlotType::Scatter);
        for (int i = 0; i < 80; ++i) {
            float angle = i * 0.25f;
            float r     = i * 0.12f;
            plot->addPoint(s0, std::sin(angle) * r);
        }
        plot->resetView();
    }

    outer->createChild<Spacer>(4.0f);

    // ── Row 3: Histogram variants ─────────────────────────────────────────
    outer->createChild<Label>("Histogram Widget");

    auto* histRow = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    histRow->setSpacing(6);
    histRow->setStretch(2);

    // Normal distribution
    {
        auto* hist = histRow->createChild<HistogramWidget>();
        hist->setStretch(1);
        hist->setTitle("Normal (mu=0, sigma=1)");
        hist->setBinCount(32);
        hist->setBarColor(Color(100, 160, 230, 200));
        std::vector<float> data;
        data.reserve(500);
        for (int i = 0; i < 250; ++i) {
            float u1 = (float)(i * 7 % 997 + 1) / 997.0f;
            float u2 = (float)(i * 13 % 997 + 1) / 997.0f;
            float z0 = std::sqrt(-2.0f * std::log(u1)) * std::cos(2.0f * 3.14159f * u2);
            float z1 = std::sqrt(-2.0f * std::log(u1)) * std::sin(2.0f * 3.14159f * u2);
            data.push_back(z0);
            data.push_back(z1);
        }
        hist->setData(data);
    }

    // Exponential-like
    {
        auto* hist = histRow->createChild<HistogramWidget>();
        hist->setStretch(1);
        hist->setTitle("Skewed distribution");
        hist->setBinCount(24);
        hist->setBarColor(Color(220, 130, 80, 200));
        std::vector<float> data;
        for (int i = 1; i <= 400; ++i) {
            float v = -std::log((float)i / 400.0f);
            data.push_back(v);
        }
        hist->setData(data);
    }

    // Log scale
    {
        auto* hist = histRow->createChild<HistogramWidget>();
        hist->setStretch(1);
        hist->setTitle("Log scale (same data)");
        hist->setBinCount(24);
        hist->setBarColor(Color(130, 200, 130, 200));
        hist->setLogScale(true);
        std::vector<float> data;
        for (int i = 1; i <= 400; ++i) {
            float v = -std::log((float)i / 400.0f);
            data.push_back(v);
        }
        hist->setData(data);
    }

    outer->createChild<Spacer>(4.0f);

    // ── Row 4: CurveEditor ────────────────────────────────────────────────
    outer->createChild<Label>("Curve Editor (Bezier)");

    auto* curveEd = outer->createChild<CurveEditor>();
    curveEd->setStretch(3);

    int c0 = curveEd->addCurve("Position", Color(220, 60, 60, 255));
    curveEd->addKey(c0, 0.0f, 0.0f);
    curveEd->addKey(c0, 0.5f, 1.2f);
    curveEd->addKey(c0, 1.0f, 0.8f);
    curveEd->addKey(c0, 1.5f, 0.0f);

    int c1 = curveEd->addCurve("Scale", Color(60, 180, 60, 255));
    curveEd->addKey(c1, 0.0f, 1.0f);
    curveEd->addKey(c1, 0.7f, 1.5f);
    curveEd->addKey(c1, 1.5f, 1.0f);

    curveEd->fitView();
}
