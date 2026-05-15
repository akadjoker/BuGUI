#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "InputWidgets.hpp"
using namespace BuGUI;

void registerDateTimeStage(WidgetApp& app)
{
    auto* root = app.addStage("datetime");
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(0.0f);
    vbox->setPadding(0.0f);

    // ── Back button row ──────────────────────────────────────────────────
    auto* topRow = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    topRow->setSpacing(8.0f);
    topRow->setPadding({4, 8, 4, 8});
    auto* back = topRow->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]() {
        app.setStage("menu", TransitionType::CoverRight);
    });
    topRow->createChild<Label>("Date & Time Pickers");

    vbox->createChild<Line>();

    // ── Main content ─────────────────────────────────────────────────────
    auto* content = vbox->createChild<BoxLayout>(LayoutDir::Vertical);
    content->setSpacing(16.0f);
    content->setPadding(24.0f);

    // ── DatePicker ───────────────────────────────────────────────────────
    content->createChild<Label>("DatePicker — click to open calendar");
    content->createChild<Line>();

    auto* dateForm = content->createChild<FormLayout>();
    dateForm->setSpacing(10.0f, 8.0f);

    auto* dateLabel = content->createChild<Label>("Selected: 2026-04-28");

    auto* dp1 = dateForm->addRow<DatePicker>("Date:");
    dp1->setDate(2026, 4, 28);
    dp1->setSize(170, 28);

    dp1->onDateChanged.connect([dateLabel](int y, int m, int d) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Selected: %04d-%02d-%02d", y, m, d);
        dateLabel->setText(buf);
    });

    auto* dp2 = dateForm->addRow<DatePicker>("Start:");
    dp2->setDate(2026, 1, 1);
    dp2->setSize(170, 28);

    auto* dp3 = dateForm->addRow<DatePicker>("End:");
    dp3->setDate(2026, 12, 31);
    dp3->setSize(170, 28);

    // ── TimePicker ───────────────────────────────────────────────────────
    content->createChild<Spacer>(12.0f);
    content->createChild<Label>("TimePicker — click field, scroll/arrows to adjust");
    content->createChild<Line>();

    auto* timeForm = content->createChild<FormLayout>();
    timeForm->setSpacing(10.0f, 8.0f);

    auto* timeLabel = content->createChild<Label>("Selected: 14:30:00");

    auto* tp1 = timeForm->addRow<TimePicker>("Time:");
    tp1->setTime(14, 30, 0);
    tp1->setSize(150, 28);

    tp1->onTimeChanged.connect([timeLabel](int h, int m, int s) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Selected: %02d:%02d:%02d", h, m, s);
        timeLabel->setText(buf);
    });

    auto* tp2 = timeForm->addRow<TimePicker>("No seconds:");
    tp2->setTime(9, 0, 0);
    tp2->setShowSeconds(false);
    tp2->setSize(110, 28);

    // ── Combined: Date + Time ────────────────────────────────────────────
    content->createChild<Spacer>(12.0f);
    content->createChild<Label>("Combined — Date + Time side by side");
    content->createChild<Line>();

    auto* comboRow = content->createChild<BoxLayout>(LayoutDir::Horizontal);
    comboRow->setSpacing(8.0f);

    auto* dpCombo = comboRow->createChild<DatePicker>();
    dpCombo->setDate(2026, 4, 28);
    dpCombo->setSize(170, 28);

    auto* tpCombo = comboRow->createChild<TimePicker>();
    tpCombo->setTime(10, 15, 0);
    tpCombo->setSize(150, 28);

    auto* dtLabel = content->createChild<Label>("DateTime: 2026-04-28 10:15:00");

    auto updateDT = [dtLabel, dpCombo, tpCombo]() {
        const auto& d = dpCombo->date();
        const auto& t = tpCombo->time();
        char buf[64];
        std::snprintf(buf, sizeof(buf), "DateTime: %04d-%02d-%02d %02d:%02d:%02d",
                      d.year, d.month, d.day, t.hour, t.minute, t.second);
        dtLabel->setText(buf);
    };

    dpCombo->onDateChanged.connect([updateDT](int, int, int) { updateDT(); });
    tpCombo->onTimeChanged.connect([updateDT](int, int, int) { updateDT(); });
}
