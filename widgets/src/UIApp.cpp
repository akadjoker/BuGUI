#include "UIApp.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include "Device.hpp"
#include "Input.hpp"
#include "RenderState.hpp"
#include "Opengl.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Hit testing — find deepest widget under mouse
// ═════════════════════════════════════════════════════════════════════════════

Widget* UIApp::hitTest(Widget* w, float x, float y)
{
    if (!w || !w->isVisible() || !w->isEnabled())
        return nullptr;

    Rect abs = w->absoluteRect();
    if (!abs.contains(x, y))
        return nullptr;

    // Check children in reverse (topmost first)
    const auto& children = w->children();
    for (int i = static_cast<int>(children.size()) - 1; i >= 0; --i)
    {
        Widget* hit = hitTest(children[i], x, y);
        if (hit) return hit;
    }

    return w;
}

void UIApp::dispatchMouse(Widget* target, MouseEvent& e)
{
    if (!target) return;
    Rect abs = target->absoluteRect();
    e.localX = e.x - abs.x;
    e.localY = e.y - abs.y;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Input processing — called once per frame
// ═════════════════════════════════════════════════════════════════════════════

void UIApp::processInput()
{
    if (!root_) return;

    float mx = static_cast<float>(Input::GetMouseX());
    float my = static_cast<float>(Input::GetMouseY());

    // ── Mouse move / hover ───────────────────────────────────────────────
    Widget* hit = hitTest(root_, mx, my);

    if (hit != hovered_)
    {
        // Could add onMouseEnter/onMouseLeave later
        hovered_ = hit;
    }

    if (hovered_)
    {
        MouseEvent me;
        me.x = mx;
        me.y = my;
        dispatchMouse(hovered_, me);
        hovered_->onMouseMove(me);
    }

    // Also send move to pressed widget if dragging
    if (pressed_ && pressed_ != hovered_)
    {
        MouseEvent me;
        me.x = mx;
        me.y = my;
        dispatchMouse(pressed_, me);
        pressed_->onMouseMove(me);
    }

    // ── Mouse press ──────────────────────────────────────────────────────
    if (Input::IsMousePressed(LEFT))
    {
        Widget* target = hovered_;
        if (target)
        {
            // Focus management
            if (target->acceptsFocus() && target != focused_)
                setFocused(target);

            MouseEvent me;
            me.x = mx;
            me.y = my;
            me.button = 0;
            dispatchMouse(target, me);
            target->onMousePress(me);
            pressed_ = target;
        }
        else
        {
            setFocused(nullptr);
        }
    }

    if (Input::IsMousePressed(RIGHT))
    {
        if (hovered_)
        {
            MouseEvent me;
            me.x = mx;
            me.y = my;
            me.button = 1;
            dispatchMouse(hovered_, me);
            hovered_->onMousePress(me);
        }
    }

    // ── Mouse release ────────────────────────────────────────────────────
    if (Input::IsMouseReleased(LEFT))
    {
        if (pressed_)
        {
            MouseEvent me;
            me.x = mx;
            me.y = my;
            me.button = 0;
            dispatchMouse(pressed_, me);
            pressed_->onMouseRelease(me);
            pressed_ = nullptr;
        }
    }

    // ── Mouse scroll ─────────────────────────────────────────────────────
    float scrollX = 0, scrollY = 0;
    Input::GetMouseWheel(scrollX, scrollY);
    if (scrollY != 0.0f && hovered_)
    {
        MouseEvent me;
        me.x = mx;
        me.y = my;
        me.scrollY = scrollY;
        dispatchMouse(hovered_, me);
        hovered_->onMouseScroll(me);
    }

    // ── Keyboard ─────────────────────────────────────────────────────────
    // TODO: text input dispatch when Input exposes text buffer
}

// ═════════════════════════════════════════════════════════════════════════════
//  Focus
// ═════════════════════════════════════════════════════════════════════════════

void UIApp::setFocused(Widget* w)
{
    if (focused_)
        focused_->setFocused(false);
    focused_ = w;
    if (focused_)
        focused_->setFocused(true);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Render
// ═════════════════════════════════════════════════════════════════════════════

void UIApp::render(RenderBatch& batch, Font& font)
{
    if (!root_) return;

    Device& dev = Device::Instance();
    float w = static_cast<float>(dev.GetWidth());
    float h = static_cast<float>(dev.GetHeight());

    // Update root to fill window
    root_->setRect({0, 0, w, h});
    root_->layout();

    // Setup 2D orthographic
    batch.SetOrtho2D(w, h);

    auto& rs = RenderState::instance();
    rs.setDepthTest(false);
    rs.setBlend(true);
    rs.setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // UIApp uses a single batch for everything (legacy path)
    PaintContext ctx{batch, batch, batch, font};
    root_->paint(ctx);

    batch.Render();
}
