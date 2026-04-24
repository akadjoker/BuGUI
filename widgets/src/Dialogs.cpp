#include "Widgets.hpp"
#include "WidgetApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include "IconAtlas.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>

namespace {
    inline Font::ClipRect toFontClipDlg(const Rect& r)
    { return {r.x, r.y, r.w, r.h}; }

    // Tiny widget that draws a single atlas icon with a tint
    class IconWidget : public Widget {
    public:
        IconWidget(IconId id, const Color& tint, const Color& bg)
            : id_(id), tint_(tint), bg_(bg) { setSize(40, 40); }
        Vec2f sizeHint() const override { return {40, 40}; }
        void paint(PaintContext& ctx) override {
            auto r = absoluteRect();
            float cx = r.x + r.w * 0.5f;
            float cy = r.y + r.h * 0.5f;
            // Colored circle background
            ctx.fill.SetColor(bg_.r, bg_.g, bg_.b, 50);
            ctx.fill.Circle(static_cast<int>(cx), static_cast<int>(cy), 18, true);
            ctx.line.SetColor(bg_.r, bg_.g, bg_.b, 120);
            ctx.line.Circle(static_cast<int>(cx), static_cast<int>(cy), 18, false);
            // Atlas icon
            if (ctx.icons && id_ != IconId::None) {
                ctx.drawIcon(id_, cx - 12, cy - 12, 24, tint_);
            }
        }
    private:
        IconId id_;
        Color  tint_;
        Color  bg_;
    };
}

// ═════════════════════════════════════════════════════════════════════════════
//  MessageBox
// ═════════════════════════════════════════════════════════════════════════════

MessageBox::MessageBox(const std::string& title, const std::string& message,
                       Buttons buttons, Type type)
    : FloatWindow(title)
    , type_(type)
    , buttons_(buttons)
    , message_(message)
{
    setResizable(false);
    setMinimizable(false);
    buildUI();

    // Center on screen
    auto& app = WidgetApp::instance();
    float fw = 400, fh = 180;
    setFloatSize(fw, fh);
    setFloatPos((app.width() - fw) * 0.5f, (app.height() - fh) * 0.5f);
}

void MessageBox::buildUI()
{
    mainLayout_ = setContent<BoxLayout>(LayoutDir::Vertical);
    mainLayout_->setSpacing(8);
    mainLayout_->setPadding(12);

    // ── Body: icon + message ─────────────────────────────────────────────
    auto* body = mainLayout_->createChild<BoxLayout>(LayoutDir::Horizontal);
    body->setStretch(1);
    body->setSpacing(12);

    // Icon from atlas with color tint matching type
    IconId iconId = IconId::None;
    Color  iconTint(180, 200, 230, 255);
    Color  iconBg(80, 160, 240, 255);
    switch (type_) {
    case Info:     iconId = IconId::Info;    iconTint = Color(80, 160, 240, 255);  iconBg = iconTint; break;
    case Warning:  iconId = IconId::Warning; iconTint = Color(240, 190, 40, 255);  iconBg = iconTint; break;
    case Error:    iconId = IconId::Error;   iconTint = Color(230, 60, 60, 255);   iconBg = iconTint; break;
    case Question: iconId = IconId::Info;    iconTint = Color(80, 160, 240, 255);  iconBg = iconTint; break;
    }
    body->createChild<IconWidget>(iconId, iconTint, iconBg);

    msgLabel_ = body->createChild<Label>(message_);
    msgLabel_->setStretch(1);

    // ── Buttons ──────────────────────────────────────────────────────────
    auto* btnRow = mainLayout_->createChild<BoxLayout>(LayoutDir::Horizontal);
    btnRow->setSize(0, 32);
    btnRow->setSpacing(8);
    btnRow->setMainAlign(MainAlign::End);

    switch (buttons_) {
    case Ok: {
        auto* ok = btnRow->createChild<Button>("OK");
        ok->setSize(80, 0);
        ok->clicked.connect([this] { close(ResultOk); });
        break;
    }
    case OkCancel: {
        auto* cancel = btnRow->createChild<Button>("Cancel");
        cancel->setSize(80, 0);
        cancel->clicked.connect([this] { close(ResultCancel); });
        auto* ok = btnRow->createChild<Button>("OK");
        ok->setSize(80, 0);
        ok->clicked.connect([this] { close(ResultOk); });
        break;
    }
    case YesNo: {
        auto* no = btnRow->createChild<Button>("No");
        no->setSize(80, 0);
        no->clicked.connect([this] { close(ResultNo); });
        auto* yes = btnRow->createChild<Button>("Yes");
        yes->setSize(80, 0);
        yes->clicked.connect([this] { close(ResultYes); });
        break;
    }
    case YesNoCancel: {
        auto* cancel = btnRow->createChild<Button>("Cancel");
        cancel->setSize(80, 0);
        cancel->clicked.connect([this] { close(ResultCancel); });
        auto* no = btnRow->createChild<Button>("No");
        no->setSize(80, 0);
        no->clicked.connect([this] { close(ResultNo); });
        auto* yes = btnRow->createChild<Button>("Yes");
        yes->setSize(80, 0);
        yes->clicked.connect([this] { close(ResultYes); });
        break;
    }
    }
}

void MessageBox::close(Result r)
{
    result.emit(r);
    WidgetApp::instance().removeFloat(this);
}

void MessageBox::onKeyPress(KeyEvent& e)
{
    if (e.key == SDLK_ESCAPE) {
        e.consumed = true;
        if (buttons_ == Ok) close(ResultOk);
        else close(ResultCancel);
        return;
    }
    if (e.key == SDLK_RETURN || e.key == SDLK_KP_ENTER) {
        e.consumed = true;
        if (buttons_ == YesNo || buttons_ == YesNoCancel) close(ResultYes);
        else close(ResultOk);
        return;
    }
    FloatWindow::onKeyPress(e);
}

// ═════════════════════════════════════════════════════════════════════════════
//  InputBox
// ═════════════════════════════════════════════════════════════════════════════

InputBox::InputBox(const std::string& title, const std::string& prompt)
    : FloatWindow(title)
    , prompt_(prompt)
{
    setResizable(false);
    setMinimizable(false);
    buildUI();

    auto& app = WidgetApp::instance();
    float fw = 400, fh = 170;
    setFloatSize(fw, fh);
    setFloatPos((app.width() - fw) * 0.5f, (app.height() - fh) * 0.5f);
}

void InputBox::buildUI()
{
    mainLayout_ = setContent<BoxLayout>(LayoutDir::Vertical);
    mainLayout_->setSpacing(8);
    mainLayout_->setPadding(12);

    if (!prompt_.empty()) {
        promptLabel_ = mainLayout_->createChild<Label>(prompt_);
    }

    textInput_ = mainLayout_->createChild<TextInput>();
    textInput_->setStretch(0);
    textInput_->setSize(0, 28);

    // Spacer
    auto* spacer = mainLayout_->createChild<Widget>();
    spacer->setStretch(1);

    // Buttons
    auto* btnRow = mainLayout_->createChild<BoxLayout>(LayoutDir::Horizontal);
    btnRow->setSize(0, 32);
    btnRow->setSpacing(8);
    btnRow->setMainAlign(MainAlign::End);

    auto* cancel = btnRow->createChild<Button>("Cancel");
    cancel->setSize(80, 0);
    cancel->clicked.connect([this] { onCancel(); });

    auto* ok = btnRow->createChild<Button>("OK");
    ok->setSize(80, 0);
    ok->clicked.connect([this] { onOk(); });
}

void InputBox::setText(const std::string& t)
{
    if (textInput_) textInput_->setText(t);
}

std::string InputBox::text() const
{
    return textInput_ ? textInput_->text() : "";
}

void InputBox::setPlaceholder(const std::string& p)
{
    if (textInput_) textInput_->setPlaceholder(p);
}

void InputBox::onOk()
{
    accepted.emit(text());
    WidgetApp::instance().removeFloat(this);
}

void InputBox::onCancel()
{
    cancelled.emit();
    WidgetApp::instance().removeFloat(this);
}

void InputBox::onKeyPress(KeyEvent& e)
{
    if (e.key == SDLK_ESCAPE) { e.consumed = true; onCancel(); return; }
    if (e.key == SDLK_RETURN || e.key == SDLK_KP_ENTER) { e.consumed = true; onOk(); return; }
    FloatWindow::onKeyPress(e);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Toast - Android-style notifications
// ═════════════════════════════════════════════════════════════════════════════

std::vector<Toast::Entry>& Toast::entries()
{
    static std::vector<Entry> s_entries;
    return s_entries;
}

void Toast::show(const std::string& message, Type type, float duration)
{
    Entry e;
    e.message  = message;
    e.type     = type;
    e.lifetime = duration > 0 ? duration : kDefaultDuration;
    e.elapsed  = 0;
    e.alpha    = 0;
    entries().push_back(std::move(e));
}

bool Toast::hasActive()
{
    return !entries().empty();
}

void Toast::tick(float dt)
{
    auto& v = entries();
    for (auto& e : v) {
        e.elapsed += dt;
        // Fade in
        if (e.elapsed < kFadeIn)
            e.alpha = e.elapsed / kFadeIn;
        // Visible
        else if (e.elapsed < e.lifetime - kFadeOut)
            e.alpha = 1.0f;
        // Fade out
        else if (e.elapsed < e.lifetime)
            e.alpha = (e.lifetime - e.elapsed) / kFadeOut;
        else
            e.alpha = 0;
    }
    // Remove expired
    v.erase(std::remove_if(v.begin(), v.end(),
        [](const Entry& e) { return e.elapsed >= e.lifetime; }), v.end());
}

void Toast::paint(PaintContext& ctx, float screenW, float screenH)
{
    auto& v = entries();
    if (v.empty()) return;

    const auto& t = Theme::instance();
    float toastW = 320;
    float toastH = 36;
    float gap    = 6;
    float baseY  = screenH - 40;

    for (int i = static_cast<int>(v.size()) - 1; i >= 0; --i) {
        auto& e = v[i];
        int a = static_cast<int>(e.alpha * 230);
        if (a <= 0) continue;

        float tx = (screenW - toastW) * 0.5f;
        float ty = baseY - (static_cast<int>(v.size()) - 1 - i) * (toastH + gap);

        // Slide up on fade-in
        if (e.elapsed < kFadeIn) {
            float slideT = e.elapsed / kFadeIn;
            ty += (1.0f - slideT) * 20.0f;
        }

        // Background color based on type
        Color bg;
        switch (e.type) {
        case Info:    bg = Color(50, 60, 80, a); break;
        case Success: bg = Color(30, 80, 50, a); break;
        case Warning: bg = Color(90, 75, 20, a); break;
        case Error:   bg = Color(90, 30, 30, a); break;
        }

        ctx.fill.SetColor(bg.r, bg.g, bg.b, bg.a);
        ctx.fill.RoundedRectangle(
            static_cast<int>(tx), static_cast<int>(ty),
            static_cast<int>(toastW), static_cast<int>(toastH),
            6, 8, true);

        // Border
        ctx.line.SetColor(255, 255, 255, a / 4);
        ctx.line.RoundedRectangle(
            static_cast<int>(tx), static_cast<int>(ty),
            static_cast<int>(toastW), static_cast<int>(toastH),
            6, 8, false);

        // Type indicator bar on left
        Color bar;
        switch (e.type) {
        case Info:    bar = Color(80, 140, 220, a); break;
        case Success: bar = Color(60, 200, 100, a); break;
        case Warning: bar = Color(230, 180, 40, a);  break;
        case Error:   bar = Color(220, 60, 60, a);   break;
        }
        ctx.fill.SetColor(bar.r, bar.g, bar.b, bar.a);
        ctx.fill.RoundedRectangle(
            static_cast<int>(tx), static_cast<int>(ty),
            4, static_cast<int>(toastH),
            2, 4, true);

        // Text
        ctx.font.SetFontSize(t.fontSize * 0.9f);
        ctx.font.SetColor(Color(240, 240, 245, a));
        ctx.font.SetBatch(&ctx.text);
        float textY = ty + (toastH - t.fontSize * 0.9f) * 0.5f;
        Rect clipR = {tx, ty, toastW, toastH};
        auto fc = toFontClipDlg(clipR);
        ctx.font.Print(e.message.c_str(), tx + 14, textY, &fc);
    }
}
