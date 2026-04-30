#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "InputWidgets.hpp"

void registerControlsStage(WidgetApp& app)
{
    auto* root = app.addStage("controls");
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(8.0f);
    vbox->setPadding(16.0f);

    // Back button
    auto* back = vbox->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
    vbox->createChild<Line>();

    // TabLayout takes the remaining space
    auto* tabs = vbox->createChild<TabLayout>();
    tabs->setStretch(1.0f);

    // ─────────────────────────────────────────────────────────────────────────
    //  Tab 1 — FormLayout + RadioGroup
    // ─────────────────────────────────────────────────────────────────────────
    auto* formPage = tabs->addTab<BoxLayout>("Form", LayoutDir::Vertical);
    formPage->setSpacing(12.0f);
    formPage->setPadding(12.0f);

    formPage->createChild<Label>("FormLayout — two-column label:widget rows");

    auto* form = formPage->createChild<FormLayout>();
    form->setSpacing(10.0f, 6.0f);
    form->setPadding(4.0f);

    // Slider row
    auto* sliderVal = form->addRow<Label>("Speed:", "0");
    auto* slider    = form->addRow<Slider>("Value:", 0.0f, 100.0f, 40.0f);
    slider->setStretch(1.0f);
    slider->onValueChanged.connect([sliderVal](float v) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.0f", v);
        sliderVal->setText(buf);
    });
    // Fix initial label
    sliderVal->setText("40");

    // CheckBox row
    form->addRow<CheckBox>("Enabled:", "Active");
    form->addRow<Switch>("Dark mode:", "");
    form->addRow<ProgressBar>("Progress:", 0.0f, 100.0f, 60.0f)->setText("60%");

    formPage->createChild<Spacer>(12.0f);
    formPage->createChild<Label>("RadioGroup — mutually exclusive options");

    auto* radioBox = formPage->createChild<BoxLayout>(LayoutDir::Horizontal);
    radioBox->setSpacing(16.0f);
    radioBox->setPadding(4.0f);

    static RadioGroup rg;
    auto* rb1 = radioBox->createChild<RadioButton>("Option A", &rg);
    auto* rb2 = radioBox->createChild<RadioButton>("Option B", &rg);
    auto* rb3 = radioBox->createChild<RadioButton>("Option C", &rg);
    rb1->setSelected(true);
    (void)rb2; (void)rb3;

    auto* selLabel = formPage->createChild<Label>("Selected: Option A");
    rg.selectionChanged.connect([selLabel](int idx) {
        const char* names[] = { "Option A", "Option B", "Option C" };
        if (idx >= 0 && idx < 3)
        {
            char buf[40];
            std::snprintf(buf, sizeof(buf), "Selected: %s", names[idx]);
            selLabel->setText(buf);
        }
    });

    // ─────────────────────────────────────────────────────────────────────────
    //  Tab 2 — FlowLayout
    // ─────────────────────────────────────────────────────────────────────────
    auto* flowPage = tabs->addTab<BoxLayout>("Flow", LayoutDir::Vertical);
    flowPage->setSpacing(10.0f);
    flowPage->setPadding(12.0f);

    flowPage->createChild<Label>("FlowLayout — wraps children like CSS flex-wrap");

    auto* flow = flowPage->createChild<FlowLayout>();
    flow->setSpacing(6.0f, 6.0f);

    const char* tags[] = {
        "C++", "OpenGL", "SDL2", "CMake", "Ninja",
        "Widgets", "BuGUI", "Rendering", "Font Atlas",
        "Unicode", "Signals", "Layouts", "Themes",
        "Scroll", "Tabs", "Forms", "Radio", "Checkbox",
        "Switch", "Slider", "Progress", "Flow"
    };
    for (const char* t : tags)
        flow->createChild<Button>(t);

    flowPage->createChild<Spacer>(12.0f);
    flowPage->createChild<Label>("CheckBox grid");

    auto* checkGrid = flowPage->createChild<FlowLayout>();
    checkGrid->setSpacing(12.0f, 6.0f);
    for (int i = 1; i <= 8; ++i)
    {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "Item %d", i);
        checkGrid->createChild<CheckBox>(buf);
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Tab 3 — StackLayout controlled by RadioGroup
    // ─────────────────────────────────────────────────────────────────────────
    auto* stackPage = tabs->addTab<BoxLayout>("Stack", LayoutDir::Vertical);
    stackPage->setSpacing(10.0f);
    stackPage->setPadding(12.0f);

    stackPage->createChild<Label>("StackLayout — only one page visible at a time");

    // Selector buttons row
    auto* selRow = stackPage->createChild<BoxLayout>(LayoutDir::Horizontal);
    selRow->setSpacing(6.0f);
    auto* btnP1 = selRow->createChild<Button>("Page 1");
    auto* btnP2 = selRow->createChild<Button>("Page 2");
    auto* btnP3 = selRow->createChild<Button>("Page 3");

    stackPage->createChild<Line>();

    auto* stack = stackPage->createChild<StackLayout>();
    stack->setStretch(1.0f);

    // Page 1
    auto* p1 = stack->createChild<BoxLayout>(LayoutDir::Vertical);
    p1->setPadding(12.0f);
    p1->setSpacing(8.0f);
    p1->createChild<Label>("Page 1 — Basic controls");
    p1->createChild<CheckBox>("Option alpha");
    p1->createChild<CheckBox>("Option beta");
    p1->createChild<Switch>("Feature flag");

    // Page 2
    auto* p2 = stack->createChild<BoxLayout>(LayoutDir::Vertical);
    p2->setPadding(12.0f);
    p2->setSpacing(8.0f);
    p2->createChild<Label>("Page 2 — Sliders");
    for (int i = 0; i < 3; ++i)
    {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "Param %d", i + 1);
        p2->createChild<Label>(buf);
        p2->createChild<Slider>(0.0f, 100.0f, float(30 + i * 20))->setStretch(1.0f);
    }

    // Page 3
    auto* p3 = stack->createChild<BoxLayout>(LayoutDir::Vertical);
    p3->setPadding(12.0f);
    p3->setSpacing(6.0f);
    p3->createChild<Label>("Page 3 — RadioGroup");
    static RadioGroup rg3;
    for (int i = 0; i < 5; ++i)
    {
        char buf[20];
        std::snprintf(buf, sizeof(buf), "Choice %d", i + 1);
        p3->createChild<RadioButton>(buf, &rg3);
    }

    stack->setCurrentIndex(0);

    btnP1->clicked.connect([stack](){ stack->setCurrentIndex(0); });
    btnP2->clicked.connect([stack](){ stack->setCurrentIndex(1); });
    btnP3->clicked.connect([stack](){ stack->setCurrentIndex(2); });
    (void)btnP1; (void)btnP2; (void)btnP3;
}
