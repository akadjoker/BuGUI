#pragma once

#include "StageCommon.hpp"
#include <string>
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
//  Calculator state — shared between button callbacks
// ═════════════════════════════════════════════════════════════════════════════

struct CalcState
{
    std::string display  = "0";
    std::string expr     = "";
    double      accumulator = 0;
    std::string pendingOp   = "";
    bool        freshInput  = true;  // next digit replaces display

    Label* displayLabel = nullptr;
    Label* exprLabel    = nullptr;

    void updateUI()
    {
        if (displayLabel) displayLabel->setText(display);
        if (exprLabel)    exprLabel->setText(expr);
    }

    void inputDigit(const std::string& d)
    {
        if (freshInput) { display = d; freshInput = false; }
        else if (display == "0" && d != ".") display = d;
        else display += d;
        updateUI();
    }

    void inputOp(const std::string& op)
    {
        evaluate();
        pendingOp = op;
        expr = display + " " + op;
        freshInput = true;
        updateUI();
    }

    void evaluate()
    {
        if (pendingOp.empty()) { accumulator = std::stod(display); return; }
        double b = std::stod(display);
        if      (pendingOp == "+") accumulator += b;
        else if (pendingOp == "-") accumulator -= b;
        else if (pendingOp == "x") accumulator *= b;
        else if (pendingOp == "/") accumulator = (b != 0) ? accumulator / b : 0;
        else if (pendingOp == "mod") accumulator = std::fmod(accumulator, b);

        // Format result
        if (accumulator == static_cast<long long>(accumulator))
            display = std::to_string(static_cast<long long>(accumulator));
        else
        {
            char buf[32]; snprintf(buf, sizeof(buf), "%.10g", accumulator);
            display = buf;
        }
    }

    void pressEquals()
    {
        if (!pendingOp.empty())
        {
            expr = expr + " " + display + " =";
            evaluate();
            pendingOp.clear();
        }
        freshInput = true;
        updateUI();
    }

    void clear()
    {
        display = "0"; expr = ""; accumulator = 0;
        pendingOp.clear(); freshInput = true;
        updateUI();
    }

    void sqrtOp()
    {
        double v = std::stod(display);
        v = std::sqrt(std::abs(v));
        char buf[32]; snprintf(buf, sizeof(buf), "%.10g", v);
        display = buf; freshInput = true;
        updateUI();
    }

    void squareOp()
    {
        double v = std::stod(display);
        v = v * v;
        char buf[32]; snprintf(buf, sizeof(buf), "%.10g", v);
        display = buf; freshInput = true;
        updateUI();
    }

    void percent()
    {
        double v = std::stod(display);
        v = accumulator * v / 100.0;
        char buf[32]; snprintf(buf, sizeof(buf), "%.10g", v);
        display = buf; freshInput = true;
        updateUI();
    }

    void pi()
    {
        display = "3.14159265359"; freshInput = true;
        updateUI();
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  Build calculator stage
// ═════════════════════════════════════════════════════════════════════════════

inline void buildCalcStage(WidgetApp& app)
{
    static CalcState calc;
    calc.clear();

    Widget* root = app.addStage("calc");

    // Full-screen centered layout
    auto* center = root->createChild<BoxLayout>(LayoutDir::Vertical);
    center->setMainAlign(MainAlign::Center);
    center->setSpacing(0);
    center->setPadding(0);

    // Calculator body — fixed width, centered
    auto* body = center->createChild<BoxLayout>(LayoutDir::Vertical);
    body->setSize(360, 0);
    body->setLayoutAlign(Align::Center);
    body->setSpacing(2);
    body->setPadding(12, 12, 12, 12);

    // ── Navigation ───────────────────────────────────────────────────────
    auto* nav = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    auto* back = nav->createChild<Button>("<  Scroll");
    back->clicked.connect([]{ WidgetApp::instance().setStage("scroll", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("Calculator")->setColor(Clr::accent);
    nav->createChild<Spacer>()->setStretch(1.0f);
    auto* fwd = nav->createChild<Button>("Grid  >");
    fwd->clicked.connect([]{ WidgetApp::instance().setStage("grid", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── Expression display ───────────────────────────────────────────────
    auto* exprLbl = body->createChild<Label>("");
    exprLbl->setColor(Clr::dim);
    exprLbl->setAlign(TextAlign::RIGHT);
    exprLbl->setSize(0, 24);
    calc.exprLabel = exprLbl;

    // ── Main display ─────────────────────────────────────────────────────
    auto* dispPanel = body->createChild<Panel>();
    dispPanel->setSize(0, 56);
    dispPanel->setBgColor(Color(38, 40, 44, 255));

    auto* dispLbl = dispPanel->createChild<Label>("0");
    dispLbl->setAlign(TextAlign::RIGHT);
    dispLbl->setColor(Color(240, 240, 245, 255));
    calc.displayLabel = dispLbl;

    body->createChild<Spacer>(8);

    // ── Helper: create a calc button ─────────────────────────────────────
    auto makeBtn = [](BoxLayout* col, const char* text, float stretch = 1.0f,
                      Color bg = Color(0,0,0,0)) -> Button*
    {
        auto* btn = col->createChild<Button>(text);
        btn->setStretch(stretch);
        if (bg.a > 0) btn->setBgColor(bg);
        return btn;
    };

    const Color numBg(70, 70, 75, 255);      // number keys
    const Color opBg(55, 55, 60, 255);        // operators
    const Color funcBg(50, 50, 55, 255);      // functions
    const Color eqBg(230, 120, 30, 255);      // equals — orange

    // ── Column-based grid (5 columns × 5 rows) ──────────────────────────
    auto* grid = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    grid->setSpacing(4);
    grid->setSize(0, 240);

    // Col 0: C, 7, 4, 1, 0
    auto* c0 = grid->createChild<BoxLayout>(LayoutDir::Vertical);
    c0->setSpacing(4); c0->setStretch(1.0f);
    makeBtn(c0, "C", 1, funcBg)->clicked.connect([]{ calc.clear(); });
    makeBtn(c0, "7", 1, numBg)->clicked.connect([]{ calc.inputDigit("7"); });
    makeBtn(c0, "4", 1, numBg)->clicked.connect([]{ calc.inputDigit("4"); });
    makeBtn(c0, "1", 1, numBg)->clicked.connect([]{ calc.inputDigit("1"); });
    makeBtn(c0, "0", 1, numBg)->clicked.connect([]{ calc.inputDigit("0"); });

    // Col 1: (, 8, 5, 2, .
    auto* c1 = grid->createChild<BoxLayout>(LayoutDir::Vertical);
    c1->setSpacing(4); c1->setStretch(1.0f);
    makeBtn(c1, "(", 1, funcBg);
    makeBtn(c1, "8", 1, numBg)->clicked.connect([]{ calc.inputDigit("8"); });
    makeBtn(c1, "5", 1, numBg)->clicked.connect([]{ calc.inputDigit("5"); });
    makeBtn(c1, "2", 1, numBg)->clicked.connect([]{ calc.inputDigit("2"); });
    makeBtn(c1, ".", 1, opBg)->clicked.connect([]{ calc.inputDigit("."); });

    // Col 2: ), 9, 6, 3, %
    auto* c2 = grid->createChild<BoxLayout>(LayoutDir::Vertical);
    c2->setSpacing(4); c2->setStretch(1.0f);
    makeBtn(c2, ")", 1, funcBg);
    makeBtn(c2, "9", 1, numBg)->clicked.connect([]{ calc.inputDigit("9"); });
    makeBtn(c2, "6", 1, numBg)->clicked.connect([]{ calc.inputDigit("6"); });
    makeBtn(c2, "3", 1, numBg)->clicked.connect([]{ calc.inputDigit("3"); });
    makeBtn(c2, "%", 1, opBg)->clicked.connect([]{ calc.percent(); });

    // Col 3: mod, /, x, -, +
    auto* c3 = grid->createChild<BoxLayout>(LayoutDir::Vertical);
    c3->setSpacing(4); c3->setStretch(1.0f);
    makeBtn(c3, "mod", 1, funcBg)->clicked.connect([]{ calc.inputOp("mod"); });
    makeBtn(c3, "/", 1, opBg)->clicked.connect([]{ calc.inputOp("/"); });
    makeBtn(c3, "x", 1, opBg)->clicked.connect([]{ calc.inputOp("x"); });
    makeBtn(c3, "-", 1, opBg)->clicked.connect([]{ calc.inputOp("-"); });
    makeBtn(c3, "+", 1, opBg)->clicked.connect([]{ calc.inputOp("+"); });

    // Col 4: pi, sqrt, x^2, = (tall — stretch=2)
    auto* c4 = grid->createChild<BoxLayout>(LayoutDir::Vertical);
    c4->setSpacing(4); c4->setStretch(1.0f);
    makeBtn(c4, "pi", 1, funcBg)->clicked.connect([]{ calc.pi(); });
    makeBtn(c4, "sqrt", 1, opBg)->clicked.connect([]{ calc.sqrtOp(); });
    makeBtn(c4, "x^2", 1, opBg)->clicked.connect([]{ calc.squareOp(); });
    auto* eq = makeBtn(c4, "=", 2, eqBg);  // stretch=2 → spans 2 row heights
    eq->clicked.connect([]{ calc.pressEquals(); });
}
