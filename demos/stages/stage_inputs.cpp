#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "InputWidgets.hpp"
using namespace BuGUI;

void registerInputsStage(WidgetApp& app)
{
    auto* root = app.addStage("inputs");
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(12.0f);
    vbox->setPadding(16.0f);

    auto* back = vbox->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
    vbox->createChild<Line>();

    vbox->createChild<Label>("Horizontal Slider");
    auto* sliderH = vbox->createChild<Slider>(0.0f, 100.0f, 30.0f);
    sliderH->setStretch(1.0f);

    vbox->createChild<Label>("Progress Bar");
    auto* pb = vbox->createChild<ProgressBar>(0.0f, 100.0f, 30.0f);
    pb->setText("30%");
    pb->setStretch(1.0f);

    sliderH->onValueChanged.connect([pb](float v) {
        pb->setValue(v);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.0f%%", v);
        pb->setText(buf);
    });

    vbox->createChild<Spacer>(8.0f);
    vbox->createChild<Label>("Vertical Sliders");

    auto* hbox = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    hbox->setSpacing(12.0f);
    hbox->setPadding(4.0f);
    hbox->setStretch(1.0f);
    for (int i = 0; i < 4; ++i)
    {
        auto* s = hbox->createChild<Slider>(0.0f, 100.0f, float(i * 25));
        s->setOrientation(LayoutDir::Vertical);
        s->setStretch(1.0f);
    }
}
