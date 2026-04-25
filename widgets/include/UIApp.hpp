#pragma once

#include "Widget.hpp"

class RenderBatch;
class Font;

// ═════════════════════════════════════════════════════════════════════════════
//  UIApp - manages the widget tree, event dispatch, and rendering
// ═════════════════════════════════════════════════════════════════════════════

class UIApp
{
public:
    UIApp() = default;
    ~UIApp() { delete root_; }

    // Takes ownership of the root widget
    void setRoot(Widget *root)
    {
        if (root_ != root)
        {
            delete root_;
            root_ = root;
        }
    }
    Widget *root() const { return root_; }

    // Call once per frame - dispatches input events to widget tree
    void processInput();

    // Layout + paint the entire tree
    void render(RenderBatch &batch, Font &font);

    // Focus management
    void setFocused(Widget *w);
    Widget *focused() const { return focused_; }

private:
    Widget *root_ = nullptr;
    Widget *focused_ = nullptr;
    Widget *hovered_ = nullptr;
    Widget *pressed_ = nullptr;

    // Recursively find the deepest widget at (x, y)
    Widget *hitTest(Widget *w, float x, float y);

    // Dispatch mouse event to a widget, filling localX/localY
    void dispatchMouse(Widget *target, MouseEvent &e);
};
