#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "InputWidgets.hpp"
#include "TextInputWidgets.hpp"
#include "ScrollWidgets.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  Simple syntax callback for C-like code (keywords, strings, comments)
// ─────────────────────────────────────────────────────────────────────────────
static std::vector<TextEdit::ColorSpan> cppSyntax(int /*lineIndex*/, const std::string& line)
{
    std::vector<TextEdit::ColorSpan> spans;
    static const Color kwColor   = {200, 120, 220, 255}; // purple
    static const Color strColor  = {180, 210, 110, 255}; // green
    static const Color numColor  = {180, 140, 100, 255}; // orange
    static const Color cmtColor  = {100, 120, 100, 255}; // grey-green

    static const char* keywords[] = {
        "int", "float", "void", "return", "if", "else", "for", "while",
        "class", "struct", "const", "auto", "bool", "true", "false",
        "include", "nullptr", "new", "delete", nullptr
    };

    int len = static_cast<int>(line.size());

    // Comment: // to end of line
    for (int i = 0; i + 1 < len; ++i) {
        if (line[i] == '/' && line[i + 1] == '/') {
            spans.push_back({i, len, cmtColor});
            return spans;
        }
    }

    // String literals
    for (int i = 0; i < len; ++i) {
        if (line[i] == '"') {
            int start = i++;
            while (i < len && line[i] != '"') {
                if (line[i] == '\\' && i + 1 < len) ++i; // skip escaped
                ++i;
            }
            if (i < len) ++i; // closing quote
            spans.push_back({start, i, strColor});
        }
    }

    // Keywords
    for (int i = 0; i < len; ) {
        if (std::isalpha(line[i]) || line[i] == '_') {
            int start = i;
            while (i < len && (std::isalnum(line[i]) || line[i] == '_')) ++i;
            std::string word = line.substr(start, i - start);
            for (const char** kw = keywords; *kw; ++kw) {
                if (word == *kw) {
                    spans.push_back({start, i, kwColor});
                    break;
                }
            }
        } else if (std::isdigit(line[i])) {
            int start = i;
            while (i < len && (std::isdigit(line[i]) || line[i] == '.')) ++i;
            spans.push_back({start, i, numColor});
        } else {
            ++i;
        }
    }

    return spans;
}

// ─────────────────────────────────────────────────────────────────────────────
//  DnD test: a draggable color swatch
// ─────────────────────────────────────────────────────────────────────────────
class ColorSwatch : public Widget
{
public:
    explicit ColorSwatch(const Color& c, const std::string& name)
        : color_(c), name_(name)
    {
        setSize(60, 60);
        setTooltip("Drag me: " + name);
    }

    bool isDragSource() const override { return true; }

    DragPayload onDragBegin() override
    {
        DragPayload p;
        p.type = "color";
        p.data = color_;
        return p;
    }

    void paint(PaintContext& ctx) override
    {
        if (!visible_) return;
        Rect abs = absoluteRect();
        ctx.fill.SetColor(color_.r, color_.g, color_.b, color_.a);
        ctx.fillRoundedRect(abs.x, abs.y, abs.w, abs.h, 6, 8);

        // Border
        Color bc = isHovered() ? Color(200,200,200,255) : Color(80,80,80,255);
        ctx.line.SetColor(bc.r, bc.g, bc.b, bc.a);
        ctx.lineRoundedRect(abs.x, abs.y, abs.w, abs.h, 6, 8);

        // Label
        ctx.font.SetColor(Color(255,255,255,255));
        float tw = ctx.font.GetTextWidth(name_.c_str());
        float asc = ctx.font.GetAscender();
        ctx.font.Print(name_.c_str(), abs.x + (abs.w - tw) * 0.5f,
                        abs.y + (abs.h + asc) * 0.5f);
    }

private:
    Color color_;
    std::string name_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  DnD test: a drop target panel that changes color
// ─────────────────────────────────────────────────────────────────────────────
class ColorDropTarget : public Widget
{
public:
    ColorDropTarget()
    {
        setSize(200, 80);
    }

    bool acceptsDrop(const DragPayload& p) override
    {
        return p.type == "color";
    }

    void onDropReceive(const DragPayload& p) override
    {
        if (p.type == "color") {
            color_ = std::any_cast<Color>(p.data);
            markDirty();
        }
    }

    void paint(PaintContext& ctx) override
    {
        if (!visible_) return;
        Rect abs = absoluteRect();
        ctx.fill.SetColor(color_.r, color_.g, color_.b, color_.a);
        ctx.fillRoundedRect(abs.x, abs.y, abs.w, abs.h, 6, 8);

        const char* txt = "Drop color here";
        ctx.font.SetColor(Color(255,255,255,255));
        float tw = ctx.font.GetTextWidth(txt);
        float asc = ctx.font.GetAscender();
        ctx.font.Print(txt, abs.x + (abs.w - tw) * 0.5f,
                        abs.y + (abs.h + asc) * 0.5f);

        // Dashed border
        Color bc = Color(160, 160, 160, 255);
        ctx.line.SetColor(bc.r, bc.g, bc.b, bc.a);
        ctx.lineRoundedRect(abs.x, abs.y, abs.w, abs.h, 6, 8);
    }

private:
    Color color_ = Color(50, 50, 55, 255);
};

// ═════════════════════════════════════════════════════════════════════════════
void registerNewFeaturesStage(WidgetApp& app)
{
    auto* root = app.addStage("newfeatures");
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
    topRow->createChild<Label>("New Features Test Stage");

    vbox->createChild<Line>();

    // ── Toolbar ──────────────────────────────────────────────────────────
    auto* toolbar = vbox->createChild<Toolbar>(34.0f);
    toolbar->setSpacing(3.0f);

    auto* statusLabel = vbox->createChild<Label>("Toolbar: (click buttons above)");
    statusLabel->setMargins(8.0f);

    toolbar->addButton("Save", IconButton::Check, [statusLabel]() {
        statusLabel->setText("Toolbar: Save clicked!");
    });
    toolbar->addButton("Undo", IconButton::ArrowLeft, [statusLabel]() {
        statusLabel->setText("Toolbar: Undo clicked!");
    });
    toolbar->addButton("Redo", IconButton::ArrowRight, [statusLabel]() {
        statusLabel->setText("Toolbar: Redo clicked!");
    });
    toolbar->addSeparator();
    toolbar->addButton("Add", IconButton::Plus, [statusLabel]() {
        statusLabel->setText("Toolbar: Add clicked!");
    });
    toolbar->addButton("Remove", IconButton::Minus, [statusLabel]() {
        statusLabel->setText("Toolbar: Remove clicked!");
    });
    toolbar->addSeparator();
    toolbar->addButton("Menu", IconButton::Hamburger, [statusLabel]() {
        statusLabel->setText("Toolbar: Menu clicked!");
    });

    vbox->createChild<Line>();

    // ── Main content in a scroll view ────────────────────────────────────
    auto* sv = vbox->createChild<ScrollView>();
    sv->setStretch(1.0f);
    auto* content = sv->setContent<BoxLayout>(LayoutDir::Vertical);
    content->setSpacing(16.0f);
    content->setPadding(16.0f);

    // ── Section: SpinBox ─────────────────────────────────────────────────
    content->createChild<Label>("SpinBox");
    content->createChild<Line>();

    auto* spinRow = content->createChild<BoxLayout>(LayoutDir::Horizontal);
    spinRow->setSpacing(16.0f);

    auto* formSpin = spinRow->createChild<FormLayout>();
    formSpin->setSpacing(10.0f, 6.0f);

    auto* spin1 = formSpin->addRow<SpinBox>("Integer:", 0.0f, 100.0f, 50.0f, 1.0f);
    spin1->setDecimals(0);

    auto* spin2 = formSpin->addRow<SpinBox>("Float:", 0.0f, 1.0f, 0.5f, 0.01f);
    spin2->setDecimals(3);

    auto* spin3 = formSpin->addRow<SpinBox>("Angle:", -360.0f, 360.0f, 45.0f, 5.0f);
    spin3->setDecimals(1);
    spin3->setSuffix("°");

    auto* spin4 = formSpin->addRow<SpinBox>("Percent:", 0.0f, 100.0f, 75.0f, 1.0f);
    spin4->setDecimals(0);
    spin4->setSuffix("%");

    auto* spinValueLabel = spinRow->createChild<Label>("Value: 50");
    spin1->onValueChanged.connect([spinValueLabel](float v) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Value: %.0f", v);
        spinValueLabel->setText(buf);
    });

    // ── Section: TextEdit with Syntax + Markers ──────────────────────────
    content->createChild<Spacer>(8.0f);
    content->createChild<Label>("TextEdit — Syntax Highlighting + Gutter Markers");
    content->createChild<Line>();

    auto* editor = content->createChild<TextEdit>(
        "// C++ syntax highlighting demo\n"
        "#include <iostream>\n"
        "\n"
        "int main() {\n"
        "    const float pi = 3.14159;\n"
        "    auto msg = \"Hello, BuGUI!\";\n"
        "    for (int i = 0; i < 10; i = i + 1) {\n"
        "        std::cout << msg << std::endl;\n"
        "    }\n"
        "    return 0;\n"
        "}\n"
        "// Click the gutter to toggle breakpoints\n"
    );
    editor->setSize(0, 260);
    editor->setSyntaxCallback(cppSyntax);

    // Pre-set some markers
    editor->setMarker(0, TextEdit::MarkerType::Info);
    editor->setMarker(3, TextEdit::MarkerType::Breakpoint);
    editor->setMarker(6, TextEdit::MarkerType::Warning);
    editor->setMarker(9, TextEdit::MarkerType::Error);

    // Clicking gutter toggles breakpoints
    editor->markerClicked.connect([editor](int line, TextEdit::MarkerType type) {
        if (editor->hasMarker(line, TextEdit::MarkerType::Breakpoint)) {
            editor->clearMarker(line, TextEdit::MarkerType::Breakpoint);
        } else {
            editor->setMarker(line, TextEdit::MarkerType::Breakpoint);
        }
    });

    // ── Section: Drag & Drop ─────────────────────────────────────────────
    content->createChild<Spacer>(8.0f);
    content->createChild<Label>("Drag & Drop — drag a color swatch onto the target");
    content->createChild<Line>();

    auto* dndRow = content->createChild<BoxLayout>(LayoutDir::Horizontal);
    dndRow->setSpacing(12.0f);

    dndRow->createChild<ColorSwatch>(Color(220,  60,  60, 255), "Red");
    dndRow->createChild<ColorSwatch>(Color( 60, 180,  60, 255), "Green");
    dndRow->createChild<ColorSwatch>(Color( 60,  80, 220, 255), "Blue");
    dndRow->createChild<ColorSwatch>(Color(220, 180,  40, 255), "Yellow");
    dndRow->createChild<ColorSwatch>(Color(180,  60, 220, 255), "Purple");

    auto* target = content->createChild<ColorDropTarget>();
    target->setSize(300, 80);

    // ── Section: Tab Navigation ──────────────────────────────────────────
    content->createChild<Spacer>(8.0f);
    content->createChild<Label>("Tab Navigation — press Tab/Shift+Tab to cycle focus");
    content->createChild<Line>();

    auto* tabRow = content->createChild<BoxLayout>(LayoutDir::Horizontal);
    tabRow->setSpacing(8.0f);

    for (int i = 1; i <= 5; ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Input %d", i);
        auto* inp = tabRow->createChild<TextInput>(buf);
        inp->setSize(120, 28);
    }

    // ── Section: OS File Drop ────────────────────────────────────────────
    content->createChild<Spacer>(8.0f);
    content->createChild<Label>("OS File Drop — drag files from your file manager here");
    content->createChild<Line>();

    auto* dropLabel = content->createChild<Label>("(drop files anywhere on the window)");
    dropLabel->setColor(Color(140, 140, 140, 255));

    // Listen for OS file drops on the root
    root->fileDrop.connect([dropLabel](const std::vector<std::string>& paths) {
        std::string msg = "Dropped " + std::to_string(paths.size()) + " file(s): ";
        for (size_t i = 0; i < paths.size() && i < 3; ++i) {
            if (i > 0) msg += ", ";
            // Show just the filename, not the full path
            auto pos = paths[i].find_last_of("/\\");
            msg += (pos != std::string::npos) ? paths[i].substr(pos + 1) : paths[i];
        }
        if (paths.size() > 3) msg += "...";
        dropLabel->setText(msg);
    });
}
