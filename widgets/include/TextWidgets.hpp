#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include "Font.hpp"
#include <string>
#include <functional>


class TextInput : public Widget
{
public:
    enum class Mode { Normal, Password, NumberOnly, ReadOnly };

    explicit TextInput(const std::string& text = "", Mode mode = Mode::Normal);

    // ── Text ──────────────────────────────────────────────────────────────
    void setText(const std::string& t);
    const std::string& text() const { return text_; }

    void setPlaceholder(const std::string& p) { placeholder_ = p; markDirty(); }
    const std::string& placeholder() const    { return placeholder_; }

    void setMode(Mode m) { mode_ = m; markDirty(); }
    Mode mode() const    { return mode_; }

    // ── Selection ─────────────────────────────────────────────────────────
    void selectAll();
    void clearSelection();
    bool hasSelection() const { return selStart_ != selEnd_; }
    std::string selectedText() const;

    // ── Cursor ────────────────────────────────────────────────────────────
    int cursorPos() const { return cursor_; }
    void setCursorPos(int pos);

    // ── Signals ───────────────────────────────────────────────────────────
    Signal<const std::string&> textChanged;   // text was modified
    Signal<const std::string&> submitted;     // Enter pressed

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onKeyPress(KeyEvent& e) override;
    void onTextInput(KeyEvent& e) override;

private:
    std::string text_;
    std::string placeholder_;
    Mode mode_ = Mode::Normal;

    int cursor_   = 0;   // codepoint index
    int selStart_ = 0;   // selection anchor
    int selEnd_   = 0;   // selection end

    float scrollX_ = 0;  // horizontal scroll offset (pixels)
    float blinkTimer_ = 0;
    bool  dragging_   = false;

    // Helpers
    std::string displayText() const;
    int hitTestChar(PaintContext& ctx, float localX) const;
    float cursorXOffset(PaintContext& ctx, int pos) const;
    void deleteSelection();
    void insertText(const std::string& t);
    void ensureCursorVisible(PaintContext& ctx);
    void clampCursor();
    bool isNumberChar(char c) const;
};

// ═════════════════════════════════════════════════════════════════════════════
//  TextEdit — multi-line text editor
//  Features: line numbers, cursor 2D, selection, scroll, word wrap (opt),
//            tab-to-spaces, clipboard, ready for syntax highlight callback
// ═════════════════════════════════════════════════════════════════════════════

class TextEdit : public Widget
{
public:
    struct TextPos
    {
        int line = 0;
        int col  = 0;

        bool operator==(const TextPos& o) const { return line == o.line && col == o.col; }
        bool operator!=(const TextPos& o) const { return !(*this == o); }
        bool operator<(const TextPos& o) const  { return line < o.line || (line == o.line && col < o.col); }
        bool operator>(const TextPos& o) const  { return o < *this; }
        bool operator<=(const TextPos& o) const { return !(o < *this); }
        bool operator>=(const TextPos& o) const { return !(*this < o); }
    };

    // Color span for syntax highlighting callback
    struct ColorSpan
    {
        int startCol;   // codepoint index
        int endCol;
        Color color;
    };

    using SyntaxCallback = std::function<std::vector<ColorSpan>(int lineIndex, const std::string& lineText)>;

    explicit TextEdit(const std::string& text = "");

    // ── Text ──────────────────────────────────────────────────────────────
    void setText(const std::string& text);
    std::string text() const;
    int lineCount() const { return static_cast<int>(lines_.size()); }
    const std::string& lineAt(int n) const;

    // ── Cursor ────────────────────────────────────────────────────────────
    TextPos cursorPos() const { return cursor_; }
    void setCursorPos(TextPos pos);
    void setCursorPos(int line, int col);

    // ── Selection ─────────────────────────────────────────────────────────
    TextPos selectionStart() const { return std::min(selAnchor_, cursor_); }
    TextPos selectionEnd()   const { return std::max(selAnchor_, cursor_); }
    bool hasSelection() const      { return selAnchor_ != cursor_; }
    std::string selectedText() const;
    void selectAll();
    void selectLine(int n);
    void clearSelection() { selAnchor_ = cursor_; markDirty(); }

    // ── Scroll ────────────────────────────────────────────────────────────
    int firstVisibleLine() const;
    int visibleLineCount() const;
    void scrollToLine(int n);
    void ensureCursorVisible();

    // ── Config ────────────────────────────────────────────────────────────
    void setReadOnly(bool r) { readOnly_ = r; }
    bool isReadOnly() const  { return readOnly_; }

    void setShowLineNumbers(bool s) { showLineNumbers_ = s; markDirty(); }
    bool showLineNumbers() const    { return showLineNumbers_; }

    void setTabSize(int s)  { tabSize_ = s; }
    int tabSize() const     { return tabSize_; }

    void setWordWrap(bool w) { wordWrap_ = w; wrapDirty_ = true; markDirty(); }
    bool wordWrap() const    { return wordWrap_; }

    void setAlign(TextAlign a) { align_ = a; markDirty(); }
    TextAlign align() const    { return align_; }

    // ── Syntax highlight ──────────────────────────────────────────────────
    void setSyntaxCallback(SyntaxCallback cb) { syntaxCb_ = std::move(cb); markDirty(); }

    // ── Signals ───────────────────────────────────────────────────────────
    Signal<>        textChanged;
    Signal<TextPos> cursorMoved;

    // ── Widget overrides ──────────────────────────────────────────────────
    Vec2f sizeHint() const override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseRelease(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;
    void onMouseScroll(MouseEvent& e) override;
    void onKeyPress(KeyEvent& e) override;
    void onTextInput(KeyEvent& e) override;

private:
    std::vector<std::string> lines_;  // one string per line

    TextPos cursor_;
    TextPos selAnchor_;

    float scrollY_  = 0;   // vertical scroll in pixels
    float scrollX_  = 0;   // horizontal scroll in pixels
    float blinkTimer_ = 0;
    bool  dragging_   = false;
    bool  readOnly_   = false;
    bool  showLineNumbers_ = true;
    bool  wordWrap_   = false;
    int   tabSize_    = 4;
    TextAlign align_  = TextAlign::LEFT;

    SyntaxCallback syntaxCb_;

    // ── Word-wrap cache ──────────────────────────────────────────────────
    // Each logical line maps to 1+ visual rows.
    // wrapRows_[i] = list of {startCol, endCol} for each visual row of line i
    struct WrapRow { int startCol; int endCol; };
    std::vector<std::vector<WrapRow>> wrapRows_;
    bool  wrapDirty_ = true;
    float wrapWidth_ = 0;  // width used to compute wrap

    void  rebuildWrap(PaintContext& ctx);
    int   totalVisualRows() const;
    // Map visual row index → {logical line, row within line}
    struct VisualPos { int line; int row; };
    VisualPos visualToLogical(int visualRow) const;
    int       logicalToVisualRow(int line, int row = 0) const;

    // Cached metrics (updated in paint)
    float lineHeight_ = 0;
    float gutterW_    = 0;
    float maxContentW_ = 0;  // widest line in pixels (for horiz scroll)
    static const std::string emptyLine_;

    // Helpers
    float computeGutterWidth(Font& font) const;
    float computeLineHeight(Font& font) const;
    TextPos hitTest(PaintContext& ctx, float localX, float localY) const;
    float colToX(PaintContext& ctx, int line, int col) const;
    int   xToCol(PaintContext& ctx, int line, float x) const;
    int   lineColCount(int line) const;
    void  clampPos(TextPos& p) const;
    void  deleteSelection();
    void  insertTextAtCursor(const std::string& t);
    void  splitLine();
    void  mergeWithPrevLine();
    void  mergeWithNextLine();
};

// ═════════════════════════════════════════════════════════════════════════════
//  MenuItem — a single entry in a Menu
//    Can be a normal item, separator, or submenu header.
// ═════════════════════════════════════════════════════════════════════════════

struct MenuAction
{
    std::string label;
    std::string shortcut;    // display text, e.g. "Ctrl+S"
    bool        enabled = true;
    bool        separator = false;  // if true, draws a horizontal line
    bool        checkable = false;  // if true, draws a check mark
    bool        checked = false;    // state of check mark
    Menu*       submenu = nullptr;  // if non-null, opens submenu on hover
    std::function<void()> callback;

    // Convenience constructors
    static MenuAction Separator() { MenuAction m; m.separator = true; return m; }
};

// ═════════════════════════════════════════════════════════════════════════════
//  Menu — popup list of MenuActions (shown via popup overlay)
//    Used by MenuBar and ContextMenu.
