#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "ConsoleWidget.hpp"

void registerConsoleStage(WidgetApp& app)
{
    auto* root = app.addStage("console");

    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setStretch(1);
    vbox->setSpacing(8);
    vbox->setPadding(20);

    vbox->createChild<Label>("ConsoleWidget Demo");
    vbox->createChild<Spacer>(6);

    auto* con = vbox->createChild<ConsoleWidget>();
    con->setStretch(1);

    // Pre-populate with some entries
    con->log(LogLevel::Info,  "Console initialized");
    con->log(LogLevel::Trace, "Loading assets...");
    con->log(LogLevel::Info,  "Texture atlas built (1024x1024)");
    con->log(LogLevel::Warn,  "Texture 'grass.png' not found, using fallback");
    con->log(LogLevel::Error, "Shader compile failed: frag.glsl line 42");
    con->log(LogLevel::Info,  "Server started on port 8080");
    con->log(LogLevel::Trace, "Frame budget: 16.6ms");
    con->log(LogLevel::Warn,  "Audio buffer underrun detected");
    con->log(LogLevel::Info,  "Player spawned at (0, 0, 0)");
    con->log(LogLevel::Info,  "Player spawned at (0, 0, 0)");  // duplicate to test collapse
    con->log(LogLevel::Info,  "Player spawned at (0, 0, 0)");
    con->log(LogLevel::Error, "Out of memory: failed to allocate 2GB");

    // Wire command callback
    con->onCommand.connect([con](const std::string& cmd) {
        if (cmd == "clear") {
            con->clear();
        } else if (cmd == "help") {
            con->log(LogLevel::Info, "Commands: clear, help, spam, timestamp");
        } else if (cmd == "spam") {
            for (int i = 0; i < 50; i++)
                con->logf(LogLevel::Trace, "Spam line %d", i);
        } else if (cmd == "timestamp") {
            con->setShowTimestamp(!con->showTimestamp());
        } else {
            con->logf(LogLevel::Warn, "Unknown command: %s", cmd.c_str());
        }
    });
}
