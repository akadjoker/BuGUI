#pragma once

#include "Widget.hpp"
#include "Theme.hpp"
#include <string>
#include <functional>


// ═════════════════════════════════════════════════════════════════════════════
//  Dialog - modal dialog shown via the popup layer
//    Draws a scrim + centered card with title, content area, and button row.
//    Shown via WidgetApp::instance().showPopup(dialog).
//
//    auto* dlg = new Dialog("Save Changes?", 360, 180);
//    dlg->setMessage("Do you want to save before closing?");
//    dlg->addButton("Cancel", Dialog::Default);
//    dlg->addButton("Save",   Dialog::Primary);
//    dlg->buttonClicked.connect([](int idx){ ... });
//    WidgetApp::instance().showPopup(dlg);
// ═════════════════════════════════════════════════════════════════════════════


class Dialog : public Widget
{
public:
    enum ButtonStyle { Default, Primary, Danger };

    explicit Dialog(const std::string& title = "Dialog",
                    float width = 360.0f, float height = 0.0f);

    void setTitle(const std::string& t) { title_ = t; markDirty(); }
    const std::string& title() const { return title_; }

    void setMessage(const std::string& m) { message_ = m; markDirty(); }
    const std::string& message() const { return message_; }

    // Custom content (replaces message)
    template <typename T, typename... Args>
    T* setContent(Args&&... args)
    {
        auto* w = new T(std::forward<Args>(args)...);
        setContentWidget(w);
        return w;
    }
    void setContentWidget(Widget* w);
    Widget* content() const { return content_; }

    // Button row
    void addButton(const std::string& label, ButtonStyle style = Default);
    int  buttonCount() const { return static_cast<int>(buttons_.size()); }

    // Sizing
    void setDialogWidth(float w)  { dialogW_ = w; markDirty(); }
    void setDialogHeight(float h) { dialogH_ = h; markDirty(); }

    // Signals
    Signal<int> buttonClicked;  // emits button index (0-based, left to right)

    // Overrides
    void layout() override;
    void paint(PaintContext& ctx) override;
    void onMousePress(MouseEvent& e) override;
    void onMouseMove(MouseEvent& e) override;

private:
    struct BtnInfo { std::string label; ButtonStyle style; Rect rect; };

    std::string  title_;
    std::string  message_;
    Widget*      content_     = nullptr;
    float        dialogW_     = 360.0f;
    float        dialogH_     = 0.0f;  // 0 = auto from content
    std::vector<BtnInfo> buttons_;
    int          hoveredBtn_  = -1;

    static constexpr float kTitleBarH  = 36.0f;
    static constexpr float kBtnRowH    = 44.0f;
    static constexpr float kBtnH       = 30.0f;
    static constexpr float kPad        = 16.0f;
    static constexpr float kMsgPad     = 12.0f;

    Rect dialogRect() const;  // centered in parent
    Rect titleBarRect() const;
    Rect contentAreaRect() const;
    Rect buttonRowRect() const;

    void computeButtonRects();
    float autoHeight() const;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Convenience: AlertDialog - message + OK
// ═════════════════════════════════════════════════════════════════════════════

class AlertDialog : public Dialog
{
public:
    explicit AlertDialog(const std::string& title,
                         const std::string& message,
                         const std::string& okText = "OK");
};

// ═════════════════════════════════════════════════════════════════════════════
//  Convenience: ConfirmDialog - message + Cancel/OK
// ═════════════════════════════════════════════════════════════════════════════

class ConfirmDialog : public Dialog
{
public:
    explicit ConfirmDialog(const std::string& title,
                           const std::string& message,
                           const std::string& okText = "OK",
                           const std::string& cancelText = "Cancel");

    Signal<> confirmed;   // OK clicked
    Signal<> cancelled;   // Cancel clicked
};

