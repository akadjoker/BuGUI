#include "ConsoleWidget.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>

// ═════════════════════════════════════════════════════════════════════════════
//  ConsoleWidget
// ═════════════════════════════════════════════════════════════════════════════

ConsoleWidget::ConsoleWidget()
{
}

// ─────────────────────────────────────────────────────────────────────────────
//  Logging
// ─────────────────────────────────────────────────────────────────────────────

void ConsoleWidget::log(LogLevel level, const std::string& message, float timestamp)
{
    entries_.push_back({level, message, timestamp});
    if (autoScroll_) {
        // Compute total visible height to scroll to bottom (done in paint)
        scrollY_ = 1e9f;   // sentinel — paint will clamp
    }
    markDirty();
}

void ConsoleWidget::clear()
{
    entries_.clear();
    scrollY_ = 0;
    markDirty();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Filters
// ─────────────────────────────────────────────────────────────────────────────

void ConsoleWidget::setFilter(LogLevel level, bool show)
{
    switch (level) {
        case LogLevel::Info:  showInfo_  = show; break;
        case LogLevel::Warn:  showWarn_  = show; break;
        case LogLevel::Error: showError_ = show; break;
    }
    markDirty();
}

bool ConsoleWidget::filter(LogLevel level) const
{
    switch (level) {
        case LogLevel::Info:  return showInfo_;
        case LogLevel::Warn:  return showWarn_;
        case LogLevel::Error: return showError_;
    }
    return true;
}

bool ConsoleWidget::passesFilter(LogLevel l) const { return filter(l); }

Color ConsoleWidget::levelColor(LogLevel l) const
{
    switch (l) {
        case LogLevel::Info:  return Color(180, 200, 220, 255);
        case LogLevel::Warn:  return Color(240, 200, 80, 255);
        case LogLevel::Error: return Color(240, 80, 80, 255);
    }
    return Color(180, 180, 180, 255);
}

const char* ConsoleWidget::levelPrefix(LogLevel l) const
{
    switch (l) {
        case LogLevel::Info:  return "[INF]";
        case LogLevel::Warn:  return "[WRN]";
        case LogLevel::Error: return "[ERR]";
    }
    return "[???]";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse / Keyboard
// ─────────────────────────────────────────────────────────────────────────────

void ConsoleWidget::onMousePress(MouseEvent& e)
{
    Rect b = absoluteRect();

    // Toolbar buttons — toggle filters
    if (e.y < b.y + kToolbarH && e.button == 0) {
        float bx = b.x + 4;
        float bw = 40;
        if (e.x >= bx && e.x < bx + bw) {
            showInfo_ = !showInfo_; markDirty();
        }
        bx += bw + 4;
        if (e.x >= bx && e.x < bx + bw) {
            showWarn_ = !showWarn_; markDirty();
        }
        bx += bw + 4;
        if (e.x >= bx && e.x < bx + bw) {
            showError_ = !showError_; markDirty();
        }
        bx += bw + 8;
        // Clear button
        if (e.x >= bx && e.x < bx + 40) {
            clear();
        }
        e.consumed = true;
        return;
    }

    // Input area — focus
    float inputY = b.y + b.h - kInputH;
    if (e.y >= inputY) {
        inputFocused_ = true;
        markDirty();
        e.consumed = true;
        return;
    }

    inputFocused_ = false;
    e.consumed = true;
}

void ConsoleWidget::onMouseScroll(MouseEvent& e)
{
    scrollY_ -= e.scrollY * kLineH * 3;
    if (scrollY_ < 0) scrollY_ = 0;
    markDirty();
    e.consumed = true;
}

void ConsoleWidget::onKeyPress(KeyEvent& e)
{
    if (!inputFocused_) return;

    if (e.key == 13) { // Enter
        if (!inputBuf_.empty()) {
            log(LogLevel::Info, "> " + inputBuf_);
            onCommand.emit(inputBuf_);
            inputBuf_.clear();
            markDirty();
        }
        e.consumed = true;
    }
    else if (e.key == 8) { // Backspace
        if (!inputBuf_.empty()) {
            inputBuf_.pop_back();
            markDirty();
        }
        e.consumed = true;
    }
}

void ConsoleWidget::onTextInput(KeyEvent& e)
{
    if (!inputFocused_) return;
    inputBuf_ += e.text;
    markDirty();
    e.consumed = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Painting
// ─────────────────────────────────────────────────────────────────────────────

void ConsoleWidget::paint(PaintContext& ctx)
{
    if (!visible_) return;
    Rect b = absoluteRect();

    // Background
    ctx.fill.SetColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    ctx.fill.Rectangle(b.x, b.y, b.w, b.h, true);

    paintToolbar(ctx, b);
    paintLog(ctx, b);
    paintInput(ctx, b);

    // Border
    ctx.line.SetColor(50, 52, 58, 255);
    ctx.line.Rectangle(b.x, b.y, b.w, b.h, false);

    Widget::paint(ctx);
}

void ConsoleWidget::paintToolbar(PaintContext& ctx, const Rect& b)
{
    // Toolbar bg
    ctx.fill.SetColor(34, 36, 42, 255);
    ctx.fill.Rectangle(b.x, b.y, b.w, kToolbarH, true);

    ctx.font.SetFontSize(9.0f);
    ctx.font.SetBatch(&ctx.text);

    float bx = b.x + 4;
    float by = b.y + 4;
    float bw = 40, bh = 14;

    // Info toggle
    Color infoBg = showInfo_ ? Color(60, 80, 110, 255) : Color(44, 46, 52, 255);
    ctx.fill.SetColor(infoBg.r, infoBg.g, infoBg.b, infoBg.a);
    ctx.fill.Rectangle(bx, by, bw, bh, true);
    ctx.font.SetColor(levelColor(LogLevel::Info));
    ctx.font.Print("INF", bx + 8, by + 1);

    bx += bw + 4;
    Color warnBg = showWarn_ ? Color(80, 70, 40, 255) : Color(44, 46, 52, 255);
    ctx.fill.SetColor(warnBg.r, warnBg.g, warnBg.b, warnBg.a);
    ctx.fill.Rectangle(bx, by, bw, bh, true);
    ctx.font.SetColor(levelColor(LogLevel::Warn));
    ctx.font.Print("WRN", bx + 6, by + 1);

    bx += bw + 4;
    Color errBg = showError_ ? Color(80, 40, 40, 255) : Color(44, 46, 52, 255);
    ctx.fill.SetColor(errBg.r, errBg.g, errBg.b, errBg.a);
    ctx.fill.Rectangle(bx, by, bw, bh, true);
    ctx.font.SetColor(levelColor(LogLevel::Error));
    ctx.font.Print("ERR", bx + 7, by + 1);

    bx += bw + 8;
    ctx.fill.SetColor(55, 40, 40, 255);
    ctx.fill.Rectangle(bx, by, bw, bh, true);
    ctx.font.SetColor(Color(180, 140, 140, 200));
    ctx.font.Print("CLR", bx + 8, by + 1);

    // Separator
    ctx.line.SetColor(50, 52, 58, 255);
    ctx.line.Line2D(b.x, b.y + kToolbarH, b.x + b.w, b.y + kToolbarH);
}

void ConsoleWidget::paintLog(PaintContext& ctx, const Rect& b)
{
    float logTop = b.y + kToolbarH;
    float logBot = b.y + b.h - kInputH;
    float logH   = logBot - logTop;

    if (logH <= 0) return;

    // Count visible entries
    int visCount = 0;
    for (auto& e : entries_)
        if (passesFilter(e.level)) ++visCount;

    float totalH = visCount * kLineH;

    // Clamp scrollY
    float maxScroll = std::max(0.0f, totalH - logH);
    if (scrollY_ > maxScroll) scrollY_ = maxScroll;

    ctx.font.SetFontSize(10.0f);
    ctx.font.SetBatch(&ctx.text);

    float y = logTop - scrollY_;
    for (auto& entry : entries_) {
        if (!passesFilter(entry.level)) continue;

        float lineY = y;
        y += kLineH;

        if (lineY + kLineH < logTop || lineY > logBot) continue;

        Color col = levelColor(entry.level);

        // Prefix
        ctx.font.SetColor(Color(col.r, col.g, col.b, 150));
        ctx.font.Print(levelPrefix(entry.level), b.x + 4, lineY + 2);

        // Message
        ctx.font.SetColor(col);
        ctx.font.Print(entry.message.c_str(), b.x + 44, lineY + 2);
    }

    // Scrollbar hint
    if (totalH > logH) {
        float barH = std::max(20.0f, logH * logH / totalH);
        float barY = logTop + (scrollY_ / maxScroll) * (logH - barH);
        ctx.fill.SetColor(80, 82, 90, 120);
        ctx.fill.Rectangle(b.x + b.w - 6, barY, 4, barH, true);
    }
}

void ConsoleWidget::paintInput(PaintContext& ctx, const Rect& b)
{
    float iy = b.y + b.h - kInputH;

    // Input bg
    Color ibg = inputFocused_ ? Color(40, 42, 50, 255) : Color(32, 34, 40, 255);
    ctx.fill.SetColor(ibg.r, ibg.g, ibg.b, ibg.a);
    ctx.fill.Rectangle(b.x, iy, b.w, kInputH, true);

    // Top separator
    ctx.line.SetColor(50, 52, 58, 255);
    ctx.line.Line2D(b.x, iy, b.x + b.w, iy);

    // Prompt
    ctx.font.SetFontSize(10.0f);
    ctx.font.SetBatch(&ctx.text);
    ctx.font.SetColor(Color(100, 180, 100, 220));
    ctx.font.Print(">", b.x + 6, iy + 5);

    // Text
    ctx.font.SetColor(Color(200, 202, 210, 255));
    ctx.font.Print(inputBuf_.c_str(), b.x + 20, iy + 5);

    // Cursor blink (simple: always on)
    if (inputFocused_) {
        float cx = b.x + 20 + inputBuf_.size() * 6.5f;  // rough char width
        ctx.line.SetColor(200, 202, 210, 200);
        ctx.line.Line2D(cx, iy + 4, cx, iy + kInputH - 4);
    }
}
