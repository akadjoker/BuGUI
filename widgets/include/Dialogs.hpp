#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include "ViewWidgets.hpp"
#include <functional>

class BoxLayout;
class Label;
class TextInput;
// ═════════════════════════════════════════════════════════════════════════════
//  MessageBox - modal-style alert/confirm dialog
//    auto* mb = app.addFloat<MessageBox>("Confirm", "Delete this file?",
//                                         MessageBox::YesNo, MessageBox::Question);
//    mb->result.connect([](MessageBox::Result r) { ... });
// ═════════════════════════════════════════════════════════════════════════════


class MessageBox : public FloatWindow
{
public:
    enum Type     { Info, Warning, Error, Question };
    enum Buttons  { Ok, OkCancel, YesNo, YesNoCancel };
    enum Result   { ResultOk, ResultCancel, ResultYes, ResultNo };

    MessageBox(const std::string& title, const std::string& message,
               Buttons buttons = Ok, Type type = Info);

    Signal<Result> result;

private:
    Type     type_;
    Buttons  buttons_;
    std::string message_;

    BoxLayout* mainLayout_ = nullptr;
    Label*     msgLabel_   = nullptr;

    void buildUI();
    void close(Result r);

    void onKeyPress(KeyEvent& e) override;
};

// ═════════════════════════════════════════════════════════════════════════════
//  InputBox - simple text input dialog
//    auto* ib = app.addFloat<InputBox>("Rename", "Enter new name:");
//    ib->setText("old_name.txt");
//    ib->accepted.connect([](const std::string& text) { ... });
// ═════════════════════════════════════════════════════════════════════════════

class InputBox : public FloatWindow
{
public:
    InputBox(const std::string& title, const std::string& prompt = "");

    void setText(const std::string& t);
    std::string text() const;

    void setPlaceholder(const std::string& p);

    Signal<std::string> accepted;
    Signal<>            cancelled;

private:
    std::string prompt_;

    BoxLayout* mainLayout_ = nullptr;
    Label*     promptLabel_ = nullptr;
    TextInput* textInput_   = nullptr;

    void buildUI();
    void onOk();
    void onCancel();

    void onKeyPress(KeyEvent& e) override;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Toast - Android-style notification that auto-dismisses
//    Toast::show("File saved!", Toast::Success);
//    Toast::show("Network error", Toast::Error, 5.0f);  // 5 sec
//
//  Managed by WidgetApp - no manual cleanup needed.
// ═════════════════════════════════════════════════════════════════════════════

class Toast
{
public:
    enum Type { Info, Success, Warning, Error };

    // Show a toast notification. duration in seconds (0 = default 3s).
    static void show(const std::string& message, Type type = Info, float duration = 0);

    // ── Internal (used by WidgetApp) ─────────────────────────────────────
    struct Entry {
        std::string message;
        Type  type     = Info;
        float lifetime = 3.0f;   // total duration
        float elapsed  = 0.0f;   // time since shown
        float alpha    = 0.0f;   // current alpha [0..1]
    };

    static void tick(float dt);
    static void paint(PaintContext& ctx, float screenW, float screenH);
    static bool hasActive();

private:
    static std::vector<Entry>& entries();
    static constexpr float kFadeIn  = 0.25f;
    static constexpr float kFadeOut = 0.5f;
    static constexpr float kDefaultDuration = 3.0f;
};

