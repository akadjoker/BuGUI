#include "Widgets.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include "Utf8.hpp"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstring>
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
//  TextEdit - multi-line text editor
// ═════════════════════════════════════════════════════════════════════════════

const std::string TextEdit::emptyLine_;

TextEdit::TextEdit(const std::string &text)
{
    acceptsFocus_ = true;
    setCursor(CursorType::IBeam);
    setText(text);
}

void TextEdit::setText(const std::string &text)
{
    lines_.clear();
    std::string line;
    for (char c : text)
    {
        if (c == '\n')
        {
            lines_.push_back(std::move(line));
            line.clear();
        }
        else if (c != '\r')
        {
            line += c;
        }
    }
    lines_.push_back(std::move(line));
    cursor_ = {0, 0};
    selAnchor_ = cursor_;
    scrollX_ = scrollY_ = 0;
    wrapDirty_ = true;
    markDirty();
}

std::string TextEdit::text() const
{
    std::string result;
    for (size_t i = 0; i < lines_.size(); ++i)
    {
        if (i > 0)
            result += '\n';
        result += lines_[i];
    }
    return result;
}

const std::string &TextEdit::lineAt(int n) const
{
    if (n < 0 || n >= static_cast<int>(lines_.size()))
        return emptyLine_;
    return lines_[n];
}

void TextEdit::setCursorPos(TextPos pos)
{
    clampPos(pos);
    cursor_ = pos;
    selAnchor_ = cursor_;
    markDirty();
    cursorMoved.emit(cursor_);
}

void TextEdit::setCursorPos(int line, int col)
{
    setCursorPos({line, col});
}

std::string TextEdit::selectedText() const
{
    if (!hasSelection())
        return {};
    TextPos s = selectionStart();
    TextPos e = selectionEnd();

    if (s.line == e.line)
        return utf8Substr(lines_[s.line], s.col, e.col);

    std::string result = utf8Substr(lines_[s.line], s.col, lineColCount(s.line));
    result += '\n';
    for (int i = s.line + 1; i < e.line; ++i)
    {
        result += lines_[i];
        result += '\n';
    }
    result += utf8Substr(lines_[e.line], 0, e.col);
    return result;
}

void TextEdit::selectAll()
{
    selAnchor_ = {0, 0};
    int last = static_cast<int>(lines_.size()) - 1;
    cursor_ = {last, lineColCount(last)};
    markDirty();
}

void TextEdit::selectLine(int n)
{
    if (n < 0 || n >= static_cast<int>(lines_.size()))
        return;
    selAnchor_ = {n, 0};
    cursor_ = {n, lineColCount(n)};
    markDirty();
}

int TextEdit::firstVisibleLine() const
{
    if (lineHeight_ <= 0)
        return 0;
    return static_cast<int>(scrollY_ / lineHeight_);
}

int TextEdit::visibleLineCount() const
{
    if (lineHeight_ <= 0)
        return 0;
    return static_cast<int>(rect_.h / lineHeight_) + 2;
}

void TextEdit::scrollToLine(int n)
{
    if (lineHeight_ <= 0)
        return;
    scrollY_ = n * lineHeight_;
    float maxY = std::max(0.0f, static_cast<int>(lines_.size()) * lineHeight_ - rect_.h);
    if (scrollY_ > maxY)
        scrollY_ = maxY;
    if (scrollY_ < 0)
        scrollY_ = 0;
    markDirty();
}

void TextEdit::ensureCursorVisible()
{
    if (lineHeight_ <= 0)
        return;

    // Find the visual row of the cursor
    int vRow = 0;
    if (!wrapRows_.empty() && cursor_.line < static_cast<int>(wrapRows_.size()))
    {
        auto &rows = wrapRows_[cursor_.line];
        int rowInLine = 0;
        for (int r = 0; r < static_cast<int>(rows.size()); ++r)
        {
            if (cursor_.col >= rows[r].startCol &&
                (cursor_.col <= rows[r].endCol || r == static_cast<int>(rows.size()) - 1))
            {
                rowInLine = r;
                break;
            }
        }
        vRow = logicalToVisualRow(cursor_.line, rowInLine);
    }

    float cy = vRow * lineHeight_;

    // Vertical
    if (cy < scrollY_)
        scrollY_ = cy;
    else if (cy + lineHeight_ > scrollY_ + rect_.h)
        scrollY_ = cy + lineHeight_ - rect_.h;

    float maxY = std::max(0.0f, totalVisualRows() * lineHeight_ - rect_.h);
    if (scrollY_ > maxY)
        scrollY_ = maxY;
    if (scrollY_ < 0)
        scrollY_ = 0;

    // Horizontal (only when word wrap is off)
    if (!wordWrap_)
    {
        std::string pre = utf8Substr(lines_[cursor_.line], 0, cursor_.col);
        auto &app = WidgetApp::instance();
        app.font().SetFontSize(Theme::instance().fontSize);
        float cx = app.font().GetTextWidth(pre.c_str());
        float textW = rect_.w - gutterW_ - Theme::instance().textEditPadding * 2;

        if (cx - scrollX_ < 0)
            scrollX_ = cx;
        else if (cx - scrollX_ > textW)
            scrollX_ = cx - textW;

        float maxX = std::max(0.0f, maxContentW_ - textW);
        if (scrollX_ > maxX)
            scrollX_ = maxX;
        if (scrollX_ < 0)
            scrollX_ = 0;
    }
    else
    {
        scrollX_ = 0;
    }
}

Widget::Vec2f TextEdit::sizeHint() const
{
    return {300.0f, 200.0f};
}

// ── Helper: gutter width ─────────────────────────────────────────────────
float TextEdit::computeGutterWidth(Font &font) const
{
    if (!showLineNumbers_)
        return 0;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", std::max(static_cast<int>(lines_.size()), 99));
    return font.GetTextWidth(buf) + Theme::instance().gutterPadding * 2;
}

float TextEdit::computeLineHeight(Font &font) const
{
    return font.GetFontSize() + Theme::instance().lineSpacing;
}

int TextEdit::lineColCount(int line) const
{
    if (line < 0 || line >= static_cast<int>(lines_.size()))
        return 0;
    return utf8Length(lines_[line]);
}

void TextEdit::clampPos(TextPos &p) const
{
    int lc = static_cast<int>(lines_.size());
    if (p.line < 0)
        p.line = 0;
    if (p.line >= lc)
        p.line = lc - 1;
    int cc = lineColCount(p.line);
    if (p.col < 0)
        p.col = 0;
    if (p.col > cc)
        p.col = cc;
}

float TextEdit::colToX(PaintContext &ctx, int line, int col) const
{
    if (line < 0 || line >= static_cast<int>(lines_.size()))
        return 0;
    std::string sub = utf8Substr(lines_[line], 0, col);
    return ctx.font.GetTextWidth(sub.c_str());
}

int TextEdit::xToCol(PaintContext &ctx, int line, float x) const
{
    if (line < 0 || line >= static_cast<int>(lines_.size()))
        return 0;
    int len = lineColCount(line);
    float cx = 0;
    for (int i = 0; i < len; ++i)
    {
        std::string ch = utf8Substr(lines_[line], i, i + 1);
        float cw = ctx.font.GetTextWidth(ch.c_str());
        if (x < cx + cw * 0.5f)
            return i;
        cx += cw;
    }
    return len;
}

TextEdit::TextPos TextEdit::hitTest(PaintContext &ctx, float localX, float localY) const
{
    int vRow = static_cast<int>((localY + scrollY_) / lineHeight_);
    int tvr = totalVisualRows();
    if (vRow < 0)
        vRow = 0;
    if (vRow >= tvr)
        vRow = tvr - 1;
    if (vRow < 0)
        return {0, 0};

    auto vp = visualToLogical(vRow);
    int line = vp.line;
    int rowIdx = vp.row;
    if (line < 0 || line >= static_cast<int>(lines_.size()))
        return {0, 0};
    auto &rows = wrapRows_[line];
    if (rowIdx < 0 || rowIdx >= static_cast<int>(rows.size()))
        return {line, 0};

    int startCol = rows[rowIdx].startCol;
    int endCol = rows[rowIdx].endCol;

    float textX = localX - gutterW_ + scrollX_ - Theme::instance().textEditPadding;
    int len = endCol - startCol;
    float cx = 0;
    for (int i = 0; i < len; ++i)
    {
        std::string ch = utf8Substr(lines_[line], startCol + i, startCol + i + 1);
        float cw = ctx.font.GetTextWidth(ch.c_str());
        if (textX < cx + cw * 0.5f)
            return {line, startCol + i};
        cx += cw;
    }
    return {line, endCol};
}

void TextEdit::deleteSelection()
{
    if (!hasSelection())
        return;
    TextPos s = selectionStart();
    TextPos e = selectionEnd();

    if (s.line == e.line)
    {
        size_t byteStart = utf8ByteOffset(lines_[s.line], s.col);
        size_t byteEnd = utf8ByteOffset(lines_[s.line], e.col);
        lines_[s.line].erase(byteStart, byteEnd - byteStart);
    }
    else
    {
        // Keep start of first line + end of last line
        std::string head = utf8Substr(lines_[s.line], 0, s.col);
        std::string tail = utf8Substr(lines_[e.line], e.col, lineColCount(e.line));
        lines_[s.line] = head + tail;
        // Remove intermediate + last lines
        lines_.erase(lines_.begin() + s.line + 1, lines_.begin() + e.line + 1);
    }

    cursor_ = s;
    selAnchor_ = cursor_;
}

void TextEdit::insertTextAtCursor(const std::string &t)
{
    if (readOnly_)
        return;
    deleteSelection();

    // Split inserted text into lines
    std::vector<std::string> insertLines;
    std::string line;
    for (char c : t)
    {
        if (c == '\n')
        {
            insertLines.push_back(std::move(line));
            line.clear();
        }
        else if (c != '\r')
        {
            line += c;
        }
    }
    insertLines.push_back(std::move(line));

    if (insertLines.size() == 1)
    {
        // Single line: insert at cursor
        size_t bytePos = utf8ByteOffset(lines_[cursor_.line], cursor_.col);
        lines_[cursor_.line].insert(bytePos, insertLines[0]);
        cursor_.col += utf8Length(insertLines[0]);
    }
    else
    {
        // Multi-line: split current line, insert in between
        std::string tail = utf8Substr(lines_[cursor_.line], cursor_.col, lineColCount(cursor_.line));
        lines_[cursor_.line] = utf8Substr(lines_[cursor_.line], 0, cursor_.col) + insertLines[0];

        // Insert middle lines
        for (size_t i = 1; i < insertLines.size() - 1; ++i)
            lines_.insert(lines_.begin() + cursor_.line + static_cast<int>(i), insertLines[i]);

        // Last line + tail
        int lastIdx = cursor_.line + static_cast<int>(insertLines.size()) - 1;
        lines_.insert(lines_.begin() + lastIdx, insertLines.back() + tail);

        cursor_.line = lastIdx;
        cursor_.col = utf8Length(insertLines.back());
    }

    selAnchor_ = cursor_;
    wrapDirty_ = true;
    markDirty();
    textChanged.emit();
    cursorMoved.emit(cursor_);
}

void TextEdit::splitLine()
{
    if (readOnly_)
        return;
    deleteSelection();

    std::string tail = utf8Substr(lines_[cursor_.line], cursor_.col, lineColCount(cursor_.line));
    lines_[cursor_.line] = utf8Substr(lines_[cursor_.line], 0, cursor_.col);
    lines_.insert(lines_.begin() + cursor_.line + 1, tail);
    cursor_.line++;
    cursor_.col = 0;
    selAnchor_ = cursor_;
    wrapDirty_ = true;
    markDirty();
    textChanged.emit();
    cursorMoved.emit(cursor_);
}

void TextEdit::mergeWithPrevLine()
{
    if (cursor_.line <= 0)
        return;
    int prevCol = lineColCount(cursor_.line - 1);
    lines_[cursor_.line - 1] += lines_[cursor_.line];
    lines_.erase(lines_.begin() + cursor_.line);
    cursor_.line--;
    cursor_.col = prevCol;
    selAnchor_ = cursor_;
}

void TextEdit::mergeWithNextLine()
{
    if (cursor_.line >= static_cast<int>(lines_.size()) - 1)
        return;
    lines_[cursor_.line] += lines_[cursor_.line + 1];
    lines_.erase(lines_.begin() + cursor_.line + 1);
    wrapDirty_ = true;
}

// ── Word-wrap helpers ────────────────────────────────────────────────────
void TextEdit::rebuildWrap(PaintContext &ctx)
{
    float textW = rect_.w - gutterW_ - Theme::instance().textEditPadding * 2;
    wrapRows_.clear();
    wrapRows_.resize(lines_.size());
    wrapWidth_ = textW;
    maxContentW_ = 0;

    for (int i = 0; i < static_cast<int>(lines_.size()); ++i)
    {
        int len = lineColCount(i);

        // Track widest line for horizontal scrollbar
        float lineW = ctx.font.GetTextWidth(lines_[i].c_str());
        if (lineW > maxContentW_)
            maxContentW_ = lineW;

        if (len == 0 || !wordWrap_ || textW <= 0)
        {
            wrapRows_[i].push_back({0, len});
            continue;
        }

        int start = 0;
        while (start < len)
        {
            // Find how many chars fit in textW
            int end = start;
            int lastSpace = -1;
            float w = 0;
            while (end < len)
            {
                std::string ch = utf8Substr(lines_[i], end, end + 1);
                float cw = ctx.font.GetTextWidth(ch.c_str());
                if (w + cw > textW && end > start)
                    break;
                if (ch[0] == ' ')
                    lastSpace = end;
                w += cw;
                end++;
            }

            // Prefer breaking at space
            if (end < len && lastSpace > start)
                end = lastSpace + 1;

            wrapRows_[i].push_back({start, end});
            start = end;
        }
    }
    wrapDirty_ = false;
}

int TextEdit::totalVisualRows() const
{
    int total = 0;
    for (auto &rows : wrapRows_)
        total += static_cast<int>(rows.size());
    return total;
}

TextEdit::VisualPos TextEdit::visualToLogical(int visualRow) const
{
    int row = 0;
    for (int i = 0; i < static_cast<int>(wrapRows_.size()); ++i)
    {
        int nRows = static_cast<int>(wrapRows_[i].size());
        if (visualRow < row + nRows)
            return {i, visualRow - row};
        row += nRows;
    }
    // Past end
    int last = static_cast<int>(wrapRows_.size()) - 1;
    return {last, static_cast<int>(wrapRows_[last].size()) - 1};
}

int TextEdit::logicalToVisualRow(int line, int row) const
{
    int vr = 0;
    for (int i = 0; i < line && i < static_cast<int>(wrapRows_.size()); ++i)
        vr += static_cast<int>(wrapRows_[i].size());
    return vr + row;
}

// ── Paint ────────────────────────────────────────────────────────────────
void TextEdit::paint(PaintContext &ctx)
{
    auto &t = Theme::instance();
    Rect abs = absoluteRect();
    Rect clipped;

    blinkTimer_ += WidgetApp::instance().deltaTime();
    if (blinkTimer_ > 1.0f)
        blinkTimer_ -= 1.0f;

    ctx.font.SetFontSize(t.fontSize);
    lineHeight_ = computeLineHeight(ctx.font);
    gutterW_ = computeGutterWidth(ctx.font);

    // Rebuild word-wrap cache if needed
    float newWrapW = abs.w - gutterW_ - t.textEditPadding * 2;
    if (wordWrap_ && (wrapDirty_ || std::abs(newWrapW - wrapWidth_) > 1.0f))
        rebuildWrap(ctx);
    else if (!wordWrap_ && wrapDirty_)
        rebuildWrap(ctx); // still builds 1 row per line

    float pad = t.textEditPadding;
    Rect textArea = {abs.x + gutterW_, abs.y, abs.w - gutterW_, abs.h};
    Font::ClipRect textClip{textArea.x, textArea.y, textArea.w, textArea.h};
    Font::ClipRect fullClip{abs.x, abs.y, abs.w, abs.h};

    int totalVRows = totalVisualRows();

    // Background
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.fill.SetColor(t.inputBg.r, t.inputBg.g, t.inputBg.b, t.inputBg.a);
        ctx.fill.RoundedRectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                                  static_cast<int>(clipped.w), static_cast<int>(clipped.h),
                                  t.borderRadius, 6, true);
    }

    // Gutter background
    if (showLineNumbers_ && gutterW_ > 0)
    {
        Rect gutterRect = {abs.x, abs.y, gutterW_, abs.h};
        if (ctx.clipRectIntersect(gutterRect, clipped))
        {
            ctx.fill.SetColor(t.gutterBg.r, t.gutterBg.g, t.gutterBg.b, t.gutterBg.a);
            ctx.fill.RoundedRectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                                      static_cast<int>(clipped.w), static_cast<int>(clipped.h),
                                      0, 1, true);
        }
    }

    ctx.font.SetBatch(&ctx.text);

    // Determine visible visual rows
    int firstVRow = static_cast<int>(scrollY_ / lineHeight_);
    int lastVRow = firstVRow + static_cast<int>(abs.h / lineHeight_) + 2;
    if (firstVRow < 0)
        firstVRow = 0;
    if (lastVRow >= totalVRows)
        lastVRow = totalVRows - 1;

    // Helper lambda: compute X offset for a row substring based on alignment
    auto alignOffset = [&](const std::string &rowText, float areaW) -> float
    {
        if (align_ == TextAlign::LEFT)
            return 0;
        float tw = ctx.font.GetTextWidth(rowText.c_str());
        if (align_ == TextAlign::CENTER)
            return std::max(0.0f, (areaW - tw) * 0.5f);
        if (align_ == TextAlign::RIGHT)
            return std::max(0.0f, areaW - tw);
        return 0; // JUSTIFY handled per-word below
    };

    // Find which logical line the cursor is on (for visual row of cursor)
    int cursorVRow = 0;
    int cursorRowInLine = 0;
    if (!wrapRows_.empty() && cursor_.line < static_cast<int>(wrapRows_.size()))
    {
        auto &rows = wrapRows_[cursor_.line];
        for (int r = 0; r < static_cast<int>(rows.size()); ++r)
        {
            if (cursor_.col >= rows[r].startCol &&
                (cursor_.col <= rows[r].endCol || r == static_cast<int>(rows.size()) - 1))
            {
                cursorRowInLine = r;
                break;
            }
        }
        cursorVRow = logicalToVisualRow(cursor_.line, cursorRowInLine);
    }

    for (int vr = firstVRow; vr <= lastVRow; ++vr)
    {
        auto vp = visualToLogical(vr);
        int i = vp.line;
        int rowIdx = vp.row;
        if (i < 0 || i >= static_cast<int>(lines_.size()))
            continue;
        auto &rows = wrapRows_[i];
        if (rowIdx < 0 || rowIdx >= static_cast<int>(rows.size()))
            continue;

        int startCol = rows[rowIdx].startCol;
        int endCol = rows[rowIdx].endCol;
        std::string rowText = utf8Substr(lines_[i], startCol, endCol);

        float ly = abs.y + vr * lineHeight_ - scrollY_;
        if (ly + lineHeight_ < abs.y || ly > abs.y + abs.h)
            continue;

        float aoff = alignOffset(rowText, textArea.w - pad * 2);

        // ── Selection highlight ──────────────────────────────────────────
        if (isFocused() && hasSelection())
        {
            TextPos s = selectionStart();
            TextPos e = selectionEnd();

            // Does this visual row overlap the selection?
            TextPos rowStart{i, startCol};
            TextPos rowEnd{i, endCol};

            if (rowEnd >= s && rowStart <= e)
            {
                int selS = std::max(startCol, (i == s.line) ? s.col : 0);
                int selE = std::min(endCol, (i == e.line) ? e.col : endCol);
                if (i > s.line && i < e.line)
                {
                    selS = startCol;
                    selE = endCol;
                }
                else if (i == s.line && i == e.line)
                {
                    selS = std::max(startCol, s.col);
                    selE = std::min(endCol, e.col);
                }
                else if (i == s.line)
                {
                    selS = std::max(startCol, s.col);
                    selE = endCol;
                }
                else if (i == e.line)
                {
                    selS = startCol;
                    selE = std::min(endCol, e.col);
                }

                if (selS < selE)
                {
                    std::string pre = utf8Substr(lines_[i], startCol, selS);
                    std::string sel = utf8Substr(lines_[i], startCol, selE);
                    float sx1 = textArea.x + pad + aoff + ctx.font.GetTextWidth(pre.c_str()) - scrollX_;
                    float sx2 = textArea.x + pad + aoff + ctx.font.GetTextWidth(sel.c_str()) - scrollX_;
                    // Extend selection to end of row for multi-line
                    if (i < e.line && selE == endCol)
                        sx2 += 6;

                    float csx = std::max(sx1, textArea.x);
                    float cex = std::min(sx2, textArea.x + textArea.w);
                    if (csx < cex)
                    {
                        ctx.fill.SetColor(t.selectionColor.r, t.selectionColor.g, t.selectionColor.b, t.selectionColor.a);
                        ctx.fill.Rectangle(static_cast<int>(csx), static_cast<int>(ly),
                                           static_cast<int>(cex - csx), static_cast<int>(lineHeight_), true);
                    }
                }
            }
        }

        // ── Line number (only on first row of each logical line) ─────────
        if (showLineNumbers_ && rowIdx == 0)
        {
            char numBuf[16];
            snprintf(numBuf, sizeof(numBuf), "%d", i + 1);
            float numW = ctx.font.GetTextWidth(numBuf);
            Color numColor = (i == cursor_.line && isFocused())
                                 ? t.lineNumberActive
                                 : t.lineNumberColor;
            ctx.font.SetColor(numColor);
            ctx.font.Print(numBuf, abs.x + gutterW_ - numW - t.gutterPadding, ly, &fullClip);
        }

        // ── Row text ─────────────────────────────────────────────────────
        float tx = textArea.x + pad + aoff - scrollX_;
        float ty = ly;

        if (align_ == TextAlign::JUSTIFY && rowIdx < static_cast<int>(rows.size()) - 1 && !rowText.empty())
        {
            // Justify: distribute extra space between words
            // Split row into words
            std::vector<std::string> words;
            std::string word;
            for (size_t ci = 0; ci < rowText.size();)
            {
                int cl = utf8CharLen(rowText.c_str() + ci);
                char ch = rowText[ci];
                if (ch == ' ')
                {
                    if (!word.empty())
                    {
                        words.push_back(std::move(word));
                        word.clear();
                    }
                }
                else
                {
                    word.append(rowText, ci, cl);
                }
                ci += cl;
            }
            if (!word.empty())
                words.push_back(std::move(word));

            if (words.size() > 1)
            {
                float totalWordW = 0;
                for (auto &w : words)
                    totalWordW += ctx.font.GetTextWidth(w.c_str());
                float availW = textArea.w - pad * 2;
                float gap = (availW - totalWordW) / static_cast<float>(words.size() - 1);

                float wx = textArea.x + pad - scrollX_;
                ctx.font.SetColor(t.textColor);
                for (size_t wi = 0; wi < words.size(); ++wi)
                {
                    ctx.font.Print(words[wi].c_str(), wx, ty, &textClip);
                    wx += ctx.font.GetTextWidth(words[wi].c_str()) + gap;
                }
            }
            else
            {
                ctx.font.SetColor(t.textColor);
                ctx.font.Print(rowText.c_str(), tx, ty, &textClip);
            }
        }
        else
        {
            ctx.font.SetColor(t.textColor);
            ctx.font.Print(rowText.c_str(), tx, ty, &textClip);
        }
    }

    // ── Cursor ───────────────────────────────────────────────────────────
    if (isFocused() && blinkTimer_ < 0.5f && !readOnly_)
    {
        float cy = abs.y + cursorVRow * lineHeight_ - scrollY_;
        // Col relative to row start
        int rowStart = 0;
        if (cursor_.line < static_cast<int>(wrapRows_.size()) &&
            cursorRowInLine < static_cast<int>(wrapRows_[cursor_.line].size()))
            rowStart = wrapRows_[cursor_.line][cursorRowInLine].startCol;

        std::string pre = utf8Substr(lines_[cursor_.line], rowStart, cursor_.col);
        float cx = textArea.x + pad + ctx.font.GetTextWidth(pre.c_str()) - scrollX_;

        // Apply alignment offset
        int endCol = wrapRows_[cursor_.line][cursorRowInLine].endCol;
        std::string rowText = utf8Substr(lines_[cursor_.line], rowStart, endCol);
        if (align_ == TextAlign::CENTER)
            cx += std::max(0.0f, (textArea.w - pad * 2 - ctx.font.GetTextWidth(rowText.c_str())) * 0.5f);
        else if (align_ == TextAlign::RIGHT)
            cx += std::max(0.0f, textArea.w - pad * 2 - ctx.font.GetTextWidth(rowText.c_str()));

        if (cx >= textArea.x && cx <= textArea.x + textArea.w &&
            cy >= abs.y && cy + lineHeight_ <= abs.y + abs.h)
        {
            ctx.line.SetColor(t.textColor.r, t.textColor.g, t.textColor.b, t.textColor.a);
            ctx.line.Line2D(static_cast<int>(cx), static_cast<int>(cy + 1),
                            static_cast<int>(cx), static_cast<int>(cy + lineHeight_ - 1));
        }
    }

    // ── Current line highlight ───────────────────────────────────────────
    if (isFocused() && !hasSelection())
    {
        float cy = abs.y + cursorVRow * lineHeight_ - scrollY_;
        if (cy >= abs.y && cy + lineHeight_ <= abs.y + abs.h)
        {
            Rect lineRect = {textArea.x, cy, textArea.w, lineHeight_};
            Rect clippedLine;
            if (ctx.clipRectIntersect(lineRect, clippedLine))
            {
                ctx.fill.SetColor(t.currentLineHighlight.r, t.currentLineHighlight.g, t.currentLineHighlight.b, t.currentLineHighlight.a);
                ctx.fill.Rectangle(static_cast<int>(clippedLine.x), static_cast<int>(clippedLine.y),
                                   static_cast<int>(clippedLine.w), static_cast<int>(clippedLine.h), true);
            }
        }
    }

    // ── Border ───────────────────────────────────────────────────────────
    Color bc = isFocused() ? t.focusColor : t.inputBorder;
    if (ctx.clipRectIntersect(abs, clipped))
    {
        ctx.line.SetColor(bc.r, bc.g, bc.b, bc.a);
        ctx.line.RoundedRectangle(static_cast<int>(clipped.x), static_cast<int>(clipped.y),
                                  static_cast<int>(clipped.w), static_cast<int>(clipped.h),
                                  t.borderRadius, 6, false);
    }

    // ── Scrollbars ────────────────────────────────────────────────────────
    float barW = t.scrollbarWidth;
    float totalH = totalVRows * lineHeight_;
    float textW = textArea.w - pad * 2;
    bool needVBar = totalH > abs.h;
    bool needHBar = !wordWrap_ && maxContentW_ > textW;

    // Vertical scrollbar
    if (needVBar)
    {
        float trackH = needHBar ? abs.h - barW : abs.h;
        float ratio = trackH / totalH;
        float thumbH = std::max(t.scrollbarMinThumb, trackH * ratio);
        float maxScroll = totalH - trackH;
        float thumbY = (maxScroll > 0) ? abs.y + (scrollY_ / maxScroll) * (trackH - thumbH) : abs.y;

        ctx.fill.SetColor(t.scrollbarThumb.r, t.scrollbarThumb.g, t.scrollbarThumb.b, t.scrollbarThumb.a);
        ctx.fill.Rectangle(static_cast<int>(abs.x + abs.w - barW), static_cast<int>(thumbY),
                           static_cast<int>(barW), static_cast<int>(thumbH), true);
    }

    // Horizontal scrollbar
    if (needHBar)
    {
        float trackW = needVBar ? abs.w - gutterW_ - barW : abs.w - gutterW_;
        float ratio = trackW / maxContentW_;
        float thumbW = std::max(t.scrollbarMinThumb, trackW * ratio);
        float maxScroll = maxContentW_ - textW;
        float thumbX = (maxScroll > 0) ? abs.x + gutterW_ + (scrollX_ / maxScroll) * (trackW - thumbW) : abs.x + gutterW_;

        ctx.fill.SetColor(t.scrollbarThumb.r, t.scrollbarThumb.g, t.scrollbarThumb.b, t.scrollbarThumb.a);
        ctx.fill.Rectangle(static_cast<int>(thumbX), static_cast<int>(abs.y + abs.h - barW),
                           static_cast<int>(thumbW), static_cast<int>(barW), true);
    }
}

// ── Mouse events ─────────────────────────────────────────────────────────
void TextEdit::onMousePress(MouseEvent &e)
{
    Rect abs = absoluteRect();
    if (e.localX < 0 || e.localX > abs.w || e.localY < 0 || e.localY > abs.h)
        return;

    auto &app = WidgetApp::instance();
    PaintContext ctx{app.fillBatch(), app.lineBatch(), app.textBatch(), app.font()};
    ctx.font.SetFontSize(Theme::instance().fontSize);

    cursor_ = hitTest(ctx, e.localX, e.localY);
    clampPos(cursor_);
    selAnchor_ = cursor_;
    dragging_ = true;
    blinkTimer_ = 0;
    markDirty();
    e.consumed = true;
}

void TextEdit::onMouseRelease(MouseEvent &e)
{
    if (dragging_)
    {
        dragging_ = false;
        e.consumed = true;
    }
}

void TextEdit::onMouseMove(MouseEvent &e)
{
    if (dragging_)
    {
        auto &app = WidgetApp::instance();
        PaintContext ctx{app.fillBatch(), app.lineBatch(), app.textBatch(), app.font()};
        ctx.font.SetFontSize(Theme::instance().fontSize);

        cursor_ = hitTest(ctx, e.localX, e.localY);
        clampPos(cursor_);
        blinkTimer_ = 0;
        markDirty();
        e.consumed = true;
    }
}

void TextEdit::onMouseScroll(MouseEvent &e)
{
    if (e.scrollX != 0 || (e.scrollY != 0 && SDL_GetModState() & KMOD_SHIFT))
    {
        // Horizontal scroll (Shift+wheel or native horizontal scroll)
        float scroll = (e.scrollX != 0) ? -e.scrollX * 40.0f : -e.scrollY * 40.0f;
        scrollX_ += scroll;
        float textW = rect_.w - gutterW_ - Theme::instance().textEditPadding * 2;
        float maxX = std::max(0.0f, maxContentW_ - textW);
        if (scrollX_ > maxX)
            scrollX_ = maxX;
        if (scrollX_ < 0)
            scrollX_ = 0;
    }
    else
    {
        float scroll = -e.scrollY * lineHeight_ * 3;
        scrollY_ += scroll;
        float maxY = std::max(0.0f, static_cast<float>(totalVisualRows()) * lineHeight_ - rect_.h);
        if (scrollY_ > maxY)
            scrollY_ = maxY;
        if (scrollY_ < 0)
            scrollY_ = 0;
    }
    markDirty();
    e.consumed = true;
}

// ── Keyboard ─────────────────────────────────────────────────────────────
void TextEdit::onKeyPress(KeyEvent &e)
{
    if (readOnly_ && !e.ctrl)
        return;

    blinkTimer_ = 0;
    int lc = static_cast<int>(lines_.size());

    // Ctrl shortcuts
    if (e.ctrl)
    {
        switch (e.key)
        {
        case SDLK_a:
            selectAll();
            e.consumed = true;
            return;
        case SDLK_c:
        {
            std::string sel = selectedText();
            if (!sel.empty())
                SDL_SetClipboardText(sel.c_str());
            e.consumed = true;
            return;
        }
        case SDLK_x:
        {
            if (readOnly_)
            {
                e.consumed = true;
                return;
            }
            std::string sel = selectedText();
            if (!sel.empty())
            {
                SDL_SetClipboardText(sel.c_str());
                deleteSelection();
                markDirty();
                textChanged.emit();
                cursorMoved.emit(cursor_);
            }
            e.consumed = true;
            return;
        }
        case SDLK_v:
        {
            if (readOnly_)
            {
                e.consumed = true;
                return;
            }
            char *clip = SDL_GetClipboardText();
            if (clip && clip[0])
                insertTextAtCursor(std::string(clip));
            SDL_free(clip);
            e.consumed = true;
            return;
        }
        case SDLK_HOME:
            cursor_ = {0, 0};
            if (!e.shift)
                selAnchor_ = cursor_;
            ensureCursorVisible();
            markDirty();
            cursorMoved.emit(cursor_);
            e.consumed = true;
            return;
        case SDLK_END:
            cursor_ = {lc - 1, lineColCount(lc - 1)};
            if (!e.shift)
                selAnchor_ = cursor_;
            ensureCursorVisible();
            markDirty();
            cursorMoved.emit(cursor_);
            e.consumed = true;
            return;
        default:
            break;
        }
    }

    switch (e.key)
    {
    case SDLK_LEFT:
        if (cursor_.col > 0)
            cursor_.col--;
        else if (cursor_.line > 0)
        {
            cursor_.line--;
            cursor_.col = lineColCount(cursor_.line);
        }
        if (!e.shift)
            selAnchor_ = cursor_;
        else
        { /* keep anchor */
        }
        ensureCursorVisible();
        markDirty();
        cursorMoved.emit(cursor_);
        e.consumed = true;
        break;

    case SDLK_RIGHT:
        if (cursor_.col < lineColCount(cursor_.line))
            cursor_.col++;
        else if (cursor_.line < lc - 1)
        {
            cursor_.line++;
            cursor_.col = 0;
        }
        if (!e.shift)
            selAnchor_ = cursor_;
        ensureCursorVisible();
        markDirty();
        cursorMoved.emit(cursor_);
        e.consumed = true;
        break;

    case SDLK_UP:
        if (cursor_.line > 0)
        {
            cursor_.line--;
            int cc = lineColCount(cursor_.line);
            if (cursor_.col > cc)
                cursor_.col = cc;
        }
        if (!e.shift)
            selAnchor_ = cursor_;
        ensureCursorVisible();
        markDirty();
        cursorMoved.emit(cursor_);
        e.consumed = true;
        break;

    case SDLK_DOWN:
        if (cursor_.line < lc - 1)
        {
            cursor_.line++;
            int cc = lineColCount(cursor_.line);
            if (cursor_.col > cc)
                cursor_.col = cc;
        }
        if (!e.shift)
            selAnchor_ = cursor_;
        ensureCursorVisible();
        markDirty();
        cursorMoved.emit(cursor_);
        e.consumed = true;
        break;

    case SDLK_HOME:
        cursor_.col = 0;
        if (!e.shift)
            selAnchor_ = cursor_;
        markDirty();
        cursorMoved.emit(cursor_);
        e.consumed = true;
        break;

    case SDLK_END:
        cursor_.col = lineColCount(cursor_.line);
        if (!e.shift)
            selAnchor_ = cursor_;
        markDirty();
        cursorMoved.emit(cursor_);
        e.consumed = true;
        break;

    case SDLK_PAGEUP:
    {
        int jump = visibleLineCount() - 1;
        if (jump < 1)
            jump = 1;
        cursor_.line = std::max(0, cursor_.line - jump);
        int cc = lineColCount(cursor_.line);
        if (cursor_.col > cc)
            cursor_.col = cc;
        if (!e.shift)
            selAnchor_ = cursor_;
        ensureCursorVisible();
        markDirty();
        cursorMoved.emit(cursor_);
        e.consumed = true;
        break;
    }

    case SDLK_PAGEDOWN:
    {
        int jump = visibleLineCount() - 1;
        if (jump < 1)
            jump = 1;
        cursor_.line = std::min(lc - 1, cursor_.line + jump);
        int cc = lineColCount(cursor_.line);
        if (cursor_.col > cc)
            cursor_.col = cc;
        if (!e.shift)
            selAnchor_ = cursor_;
        ensureCursorVisible();
        markDirty();
        cursorMoved.emit(cursor_);
        e.consumed = true;
        break;
    }

    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        splitLine();
        ensureCursorVisible();
        e.consumed = true;
        break;

    case SDLK_TAB:
    {
        if (readOnly_)
        {
            e.consumed = true;
            break;
        }
        std::string spaces(tabSize_, ' ');
        insertTextAtCursor(spaces);
        ensureCursorVisible();
        e.consumed = true;
        break;
    }

    case SDLK_BACKSPACE:
        if (readOnly_)
        {
            e.consumed = true;
            break;
        }
        if (hasSelection())
        {
            deleteSelection();
        }
        else if (cursor_.col > 0)
        {
            size_t byteEnd = utf8ByteOffset(lines_[cursor_.line], cursor_.col);
            cursor_.col--;
            size_t byteStart = utf8ByteOffset(lines_[cursor_.line], cursor_.col);
            lines_[cursor_.line].erase(byteStart, byteEnd - byteStart);
            selAnchor_ = cursor_;
        }
        else
        {
            mergeWithPrevLine();
        }
        ensureCursorVisible();
        markDirty();
        textChanged.emit();
        cursorMoved.emit(cursor_);
        e.consumed = true;
        break;

    case SDLK_DELETE:
        if (readOnly_)
        {
            e.consumed = true;
            break;
        }
        if (hasSelection())
        {
            deleteSelection();
        }
        else if (cursor_.col < lineColCount(cursor_.line))
        {
            size_t byteStart = utf8ByteOffset(lines_[cursor_.line], cursor_.col);
            size_t byteEnd = utf8ByteOffset(lines_[cursor_.line], cursor_.col + 1);
            lines_[cursor_.line].erase(byteStart, byteEnd - byteStart);
            selAnchor_ = cursor_;
        }
        else
        {
            mergeWithNextLine();
        }
        markDirty();
        textChanged.emit();
        cursorMoved.emit(cursor_);
        e.consumed = true;
        break;

    case SDLK_ESCAPE:
        clearSelection();
        e.consumed = true;
        break;

    default:
        break;
    }
}

void TextEdit::onTextInput(KeyEvent &e)
{
    if (readOnly_)
    {
        e.consumed = true;
        return;
    }

    std::string t(e.text);
    if (t.empty())
        return;

    insertTextAtCursor(t);
    ensureCursorVisible();
    blinkTimer_ = 0;
    e.consumed = true;
}
