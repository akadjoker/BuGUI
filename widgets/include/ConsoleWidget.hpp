#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include "Signal.hpp"
#include <vector>
#include <string>

// ═════════════════════════════════════════════════════════════════════════════
//  ConsoleWidget - Scrollable log with severity filters + command input
//
//  Usage:
//    auto* con = parent->createChild<ConsoleWidget>();
//    con->log(LogLevel::Info, "Started OK");
//    con->log(LogLevel::Warn, "Low memory");
//    con->log(LogLevel::Error, "File not found");
//    con->onCommand.connect([](const std::string& cmd) { ... });
// ═════════════════════════════════════════════════════════════════════════════

enum class LogLevel { Info, Warn, Error };

struct LogEntry {
    LogLevel    level = LogLevel::Info;
    std::string message;
    float       timestamp = 0;   // optional - seconds since start
};

class ConsoleWidget : public Widget
{
public:
    ConsoleWidget();

    // ── Logging ──────────────────────────────────────────────────────────

    void log(LogLevel level, const std::string& message, float timestamp = 0);
    void clear();

    // ── Filters ──────────────────────────────────────────────────────────

    void setFilter(LogLevel level, bool show);
    bool filter(LogLevel level) const;

    // ── Auto-scroll ──────────────────────────────────────────────────────

    void setAutoScroll(bool v) { autoScroll_ = v; }
    bool autoScroll() const    { return autoScroll_; }

    // ── Signals ──────────────────────────────────────────────────────────

    Signal<const std::string&> onCommand;   // when user presses Enter in input

    // ── Overrides ────────────────────────────────────────────────────────

    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;
    void onKeyPress(KeyEvent& e) override;
    void onTextInput(KeyEvent& e) override;

    void setBgColor(const Color& c) { bgColor_ = c; }

private:
    std::vector<LogEntry> entries_;
    Color bgColor_ = Color(24, 26, 30, 255);

    bool showInfo_  = true;
    bool showWarn_  = true;
    bool showError_ = true;
    bool autoScroll_ = true;
    bool inputFocused_ = false;

    float scrollY_  = 0;        // scroll offset in log area (pixels)
    std::string inputBuf_;

    static constexpr float kLineH    = 16;
    static constexpr float kInputH   = 24;
    static constexpr float kToolbarH = 22;

    // ── Helpers ──────────────────────────────────────────────────────────

    bool passesFilter(LogLevel l) const;
    Color levelColor(LogLevel l) const;
    const char* levelPrefix(LogLevel l) const;

    void paintToolbar(PaintContext& ctx, const Rect& b);
    void paintLog(PaintContext& ctx, const Rect& b);
    void paintInput(PaintContext& ctx, const Rect& b);
};
