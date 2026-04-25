#include "WidgetApp.hpp"
#include "Widgets.hpp"

#include <cstdio>

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("Tutorial 02 - Inputs", 1100, 700))
        return 1;

    Widget* root = app.addStage("main");
    auto* col = root->createChild<BoxLayout>(LayoutDir::Vertical);
    col->setPadding(16, 16, 16, 16);
    col->setSpacing(10);

    auto* title = col->createChild<Label>("Tutorial 02: Input Widgets");
    title->setColor(Color(120, 190, 255, 255));

    auto* status = col->createChild<Label>("Try typing, dragging, switching and selecting.");

    auto* input = col->createChild<TextInput>("BuGUI");
    input->setSize(0, 30);
    input->setPlaceholder("Type and press Enter");

    auto* slider = col->createChild<Slider>(0.0f, 100.0f, 35.0f);
    slider->setSize(0, 24);

    auto* sw = col->createChild<Switch>("Live Preview");
    sw->setOn(true);

    auto* combo = col->createChild<ComboBox>();
    combo->addItem("Linear");
    combo->addItem("Nearest");
    combo->addItem("Cubic");
    combo->setSelectedIndex(0);

    auto* out = col->createChild<Label>("Value: 35 | Preview: ON | Filter: Linear");

    input->submitted.connect([status](const std::string& text) {
        status->setText("Submitted: " + text);
    });

    slider->onValueChanged.connect([out, sw, combo](float v) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "Value: %.1f | Preview: %s | Filter: %s",
                      v,
                      sw->isOn() ? "ON" : "OFF",
                      combo->currentText().c_str());
        out->setText(buf);
    });

    sw->toggled.connect([out, slider, combo](bool on) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "Value: %.1f | Preview: %s | Filter: %s",
                      slider->value(),
                      on ? "ON" : "OFF",
                      combo->currentText().c_str());
        out->setText(buf);
    });

    combo->selectionChanged.connect([out, slider, sw, combo](int) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "Value: %.1f | Preview: %s | Filter: %s",
                      slider->value(),
                      sw->isOn() ? "ON" : "OFF",
                      combo->currentText().c_str());
        out->setText(buf);
    });

    app.setStage("main");
    return app.run();
}
