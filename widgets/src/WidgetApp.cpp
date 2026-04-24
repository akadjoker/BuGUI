#include "WidgetApp.hpp"
#include "Widgets.hpp"
#include "WidgetSerializer.hpp"
#include "Device.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include "IconAtlas.hpp"
#include "RenderState.hpp"
#include "Opengl.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>

// ═════════════════════════════════════════════════════════════════════════════
//  GL state backup - modelled after Dear ImGui's ImGui_ImplOpenGL3_*
//
//  Saves every piece of GL state that BuGUI touches during render(), then
//  restores it at the end.  This lets an application mix its own 3D
//  rendering with BuGUI's 2D UI without any state pollution.
// ═════════════════════════════════════════════════════════════════════════════

namespace {

struct GLBackup
{
    GLenum  activeTexture;
    GLint   program;
    GLint   texture;
    GLint   arrayBuffer;
    GLint   elementBuffer;
    GLint   vertexArray;
    GLint   viewport[4];
    GLint   scissorBox[4];
    GLint   blendSrcRgb, blendDstRgb;
    GLint   blendSrcAlpha, blendDstAlpha;
    GLint   blendEqRgb, blendEqAlpha;
    GLboolean blendEnabled;
    GLboolean cullEnabled;
    GLboolean depthEnabled;
    GLboolean depthWriteMask;
    GLboolean scissorEnabled;
    GLint   unpackAlignment;
    GLfloat clearColor[4];
};

static void backupGLState(GLBackup& b)
{
    glGetIntegerv(GL_ACTIVE_TEXTURE,              (GLint*)&b.activeTexture);
    glGetIntegerv(GL_CURRENT_PROGRAM,             &b.program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,          &b.texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,        &b.arrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &b.elementBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,        &b.vertexArray);
    glGetIntegerv(GL_VIEWPORT,                    b.viewport);
    glGetIntegerv(GL_SCISSOR_BOX,                 b.scissorBox);
    glGetIntegerv(GL_BLEND_SRC_RGB,               &b.blendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB,               &b.blendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,             &b.blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA,             &b.blendDstAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,          &b.blendEqRgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA,        &b.blendEqAlpha);
    b.blendEnabled   = glIsEnabled(GL_BLEND);
    b.cullEnabled    = glIsEnabled(GL_CULL_FACE);
    b.depthEnabled   = glIsEnabled(GL_DEPTH_TEST);
    b.scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    glGetBooleanv(GL_DEPTH_WRITEMASK,             &b.depthWriteMask);
    glGetIntegerv(GL_UNPACK_ALIGNMENT,            &b.unpackAlignment);
    glGetFloatv(GL_COLOR_CLEAR_VALUE,             b.clearColor);
}

static void restoreGLState(const GLBackup& b)
{
    glUseProgram(b.program);
    glActiveTexture(b.activeTexture);
    glBindTexture(GL_TEXTURE_2D, b.texture);
    glBindVertexArray(b.vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, b.arrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b.elementBuffer);
    glBlendEquationSeparate(b.blendEqRgb, b.blendEqAlpha);
    glBlendFuncSeparate(b.blendSrcRgb, b.blendDstRgb, b.blendSrcAlpha, b.blendDstAlpha);
    if (b.blendEnabled)   glEnable(GL_BLEND);       else glDisable(GL_BLEND);
    if (b.cullEnabled)    glEnable(GL_CULL_FACE);   else glDisable(GL_CULL_FACE);
    if (b.depthEnabled)   glEnable(GL_DEPTH_TEST);  else glDisable(GL_DEPTH_TEST);
    if (b.scissorEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glDepthMask(b.depthWriteMask);
    glViewport(b.viewport[0], b.viewport[1], b.viewport[2], b.viewport[3]);
    glScissor(b.scissorBox[0], b.scissorBox[1], b.scissorBox[2], b.scissorBox[3]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, b.unpackAlignment);
    glClearColor(b.clearColor[0], b.clearColor[1], b.clearColor[2], b.clearColor[3]);
}

} // anonymous namespace
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ═════════════════════════════════════════════════════════════════════════════
//  Singleton
// ═════════════════════════════════════════════════════════════════════════════

WidgetApp& WidgetApp::instance()
{
    // Ensure Device is constructed first so it outlives WidgetApp.
    // Static locals are destroyed in reverse construction order,
    // so WidgetApp will be destroyed while GL context is still alive.
    Device::Instance();
    static WidgetApp app;
    return app;
}

WidgetApp::~WidgetApp()
{
    shutdown();
}

void WidgetApp::shutdown()
{
    if (!inited_) return;
    inited_ = false;

    // Clean up popups BEFORE root - non-owned popups (e.g. MenuBar menus)
    // live inside the root tree and will be freed by their owners.
    if (popupOwned_)
        delete popup_;
    popup_ = nullptr; popupOwner_ = nullptr; popupOwned_ = true;

    delete pendingPopupDelete_; pendingPopupDelete_ = nullptr;

    // Delete all inactive stages
    for (auto& [name, stageRoot] : stages_) {
        if (stageRoot != root_)
            delete stageRoot;
    }
    stages_.clear();

    delete root_;    root_  = nullptr;
    for (auto* fw : floats_) delete fw;
    floats_.clear();
    if (font_)      { font_->Release();      delete font_;      font_      = nullptr; }
    if (iconAtlas_) { delete iconAtlas_; iconAtlas_ = nullptr; }
    if (fillBatch_) { fillBatch_->Release();  delete fillBatch_; fillBatch_ = nullptr; }
    if (lineBatch_) { lineBatch_->Release();  delete lineBatch_; lineBatch_ = nullptr; }
    if (textBatch_) { textBatch_->Release();  delete textBatch_; textBatch_ = nullptr; }

    // Free SDL cursors
    for (auto& c : sdlCursors_) {
        if (c) SDL_FreeCursor(static_cast<SDL_Cursor*>(c));
        c = nullptr;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Init
// ═════════════════════════════════════════════════════════════════════════════

bool WidgetApp::init(const char* title, int width, int height, int monitor, unsigned int flags)
{
    if (inited_) return true;

    // Convert WindowFlags to SDL flags
    Uint32 sdlFlags = 0;
    if (flags & WindowFlag_Resizable)   sdlFlags |= SDL_WINDOW_RESIZABLE;
    if (flags & WindowFlag_Borderless)  sdlFlags |= SDL_WINDOW_BORDERLESS;
    if (flags & WindowFlag_Fullscreen)  sdlFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (flags & WindowFlag_Maximized)   sdlFlags |= SDL_WINDOW_MAXIMIZED;
    if (flags & WindowFlag_AlwaysOnTop) sdlFlags |= SDL_WINDOW_ALWAYS_ON_TOP;

    Device& dev = Device::Instance();
    if (!dev.Create(width, height, title, true, monitor, sdlFlags))
        return false;

    width_  = width;
    height_ = height;
    SDL_GL_GetDrawableSize(dev.GetWindow(), &drawW_, &drawH_);

    fillBatch_ = new RenderBatch();
    fillBatch_->Init(1, BUGUI_FILL_BUFFER_SIZE);

    lineBatch_ = new RenderBatch();
    lineBatch_->Init(1, BUGUI_LINE_BUFFER_SIZE);

    textBatch_ = new RenderBatch();
    textBatch_->Init(1, BUGUI_TEXT_BUFFER_SIZE);

    font_ = new Font();
    font_->LoadDefaultFont();

    iconAtlas_ = new IconAtlas();
    iconAtlas_->init(32);  // 32×32 px per icon cell

    // root_ starts null; set via setStage() or direct assignment
    root_ = nullptr;

    WidgetSerializer::registerBuiltinTypes();

    // Create SDL system cursors
    sdlCursors_[static_cast<int>(CursorType::Arrow)]     = static_cast<void*>(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
    sdlCursors_[static_cast<int>(CursorType::Hand)]      = static_cast<void*>(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND));
    sdlCursors_[static_cast<int>(CursorType::IBeam)]     = static_cast<void*>(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM));
    sdlCursors_[static_cast<int>(CursorType::Crosshair)] = static_cast<void*>(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
    sdlCursors_[static_cast<int>(CursorType::SizeH)]     = static_cast<void*>(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE));
    sdlCursors_[static_cast<int>(CursorType::SizeV)]     = static_cast<void*>(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS));
    sdlCursors_[static_cast<int>(CursorType::SizeAll)]   = static_cast<void*>(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL));
    sdlCursors_[static_cast<int>(CursorType::No)]        = static_cast<void*>(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO));
    activeCursor_ = CursorType::Arrow;

    inited_ = true;
    return true;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Window properties (runtime)
// ═════════════════════════════════════════════════════════════════════════════

bool WidgetApp::isResizable() const
{
    SDL_Window* win = Device::Instance().GetWindow();
    return win ? (SDL_GetWindowFlags(win) & SDL_WINDOW_RESIZABLE) != 0 : false;
}

bool WidgetApp::isMaximized() const
{
    SDL_Window* win = Device::Instance().GetWindow();
    return win ? (SDL_GetWindowFlags(win) & SDL_WINDOW_MAXIMIZED) != 0 : false;
}

void WidgetApp::setResizable(bool r)
{
    SDL_Window* win = Device::Instance().GetWindow();
    if (win) SDL_SetWindowResizable(win, r ? SDL_TRUE : SDL_FALSE);
}

void WidgetApp::setMinSize(int w, int h)
{
    SDL_Window* win = Device::Instance().GetWindow();
    if (win) SDL_SetWindowMinimumSize(win, w, h);
}

void WidgetApp::setMaxSize(int w, int h)
{
    SDL_Window* win = Device::Instance().GetWindow();
    if (win) SDL_SetWindowMaximumSize(win, w, h);
}

void WidgetApp::setTitle(const char* title)
{
    SDL_Window* win = Device::Instance().GetWindow();
    if (win) SDL_SetWindowTitle(win, title);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Root management
// ═════════════════════════════════════════════════════════════════════════════

void WidgetApp::setRoot(Widget* r)
{
    if (root_ != r)
    {
        delete root_;
        root_ = r;
    }
    focused_ = nullptr;
    hovered_ = nullptr;
    pressed_ = nullptr;
    hoverChain_.clear();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Stage management
// ═════════════════════════════════════════════════════════════════════════════

Widget* WidgetApp::addStage(const std::string& name)
{
    auto it = stages_.find(name);
    if (it != stages_.end())
        return it->second;

    auto* stageRoot = new Widget();
    stages_[name] = stageRoot;
    return stageRoot;
}

void WidgetApp::setTransition(TransitionType type, float durationSec, EaseType ease)
{
    transType_     = type;
    transDuration_ = durationSec;
    transEase_     = ease;
}

bool WidgetApp::setStage(const std::string& name)
{
    return setStage(name, transType_, transEase_);
}

bool WidgetApp::setStage(const std::string& name, TransitionType transition)
{
    return setStage(name, transition, transEase_);
}

bool WidgetApp::setStage(const std::string& name, TransitionType transition, EaseType ease)
{
    auto it = stages_.find(name);
    if (it == stages_.end()) return false;
    if (name == currentStage_) return true;

    // Clear interaction state (old tree pointers become invalid)
    focused_ = nullptr;
    hovered_ = nullptr;
    pressed_ = nullptr;
    hoverChain_.clear();
    needsLayout_ = true;

    // Start transition or instant swap
    if (transition != TransitionType::None && root_)
    {
        transActive_   = true;
        transProgress_ = 0.0f;
        transCurrent_  = transition;
        transEaseCur_  = ease;
        transOldRoot_  = root_;
        transOldStage_ = currentStage_;
    }

    // Swap root to the new stage's widget
    root_ = it->second;
    currentStage_ = name;
    return true;
}

Widget* WidgetApp::stage(const std::string& name) const
{
    auto it = stages_.find(name);
    return (it != stages_.end()) ? it->second : nullptr;
}

bool WidgetApp::removeStage(const std::string& name)
{
    if (name == currentStage_) return false;  // cannot remove active stage
    auto it = stages_.find(name);
    if (it == stages_.end()) return false;

    delete it->second;
    stages_.erase(it);
    return true;
}

RenderBatch& WidgetApp::fillBatch() { return *fillBatch_; }
RenderBatch& WidgetApp::lineBatch() { return *lineBatch_; }
RenderBatch& WidgetApp::textBatch() { return *textBatch_; }
Font&        WidgetApp::font()      { return *font_; }
IconAtlas&   WidgetApp::iconAtlas() { return *iconAtlas_; }

// ═════════════════════════════════════════════════════════════════════════════
//  Widget lookup
// ═════════════════════════════════════════════════════════════════════════════

std::vector<Widget*> WidgetApp::widgetsByTag(const std::string& tag)
{
    std::vector<Widget*> result;
    if (root_) root_->findByTag(tag, result);
    return result;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Global event dispatcher
// ═════════════════════════════════════════════════════════════════════════════

void WidgetApp::on(const std::string& event, const std::string& widgetId, EventCallback cb)
{
    globalHandlers_[event + ":" + widgetId].push_back(std::move(cb));
}

void WidgetApp::onAny(const std::string& event, EventCallback cb)
{
    globalHandlers_[event + ":*"].push_back(std::move(cb));
}

void WidgetApp::fireEvent(const std::string& event, Widget* w)
{
    if (!w) return;

    // Targeted: by widget ID
    if (!w->id().empty())
    {
        auto it = globalHandlers_.find(event + ":" + w->id());
        if (it != globalHandlers_.end())
            for (auto& cb : it->second)
                cb(w);
    }

    // Catch-all
    auto it = globalHandlers_.find(event + ":*");
    if (it != globalHandlers_.end())
        for (auto& cb : it->second)
            cb(w);
}

// ═════════════════════════════════════════════════════════════════════════════
//  notifyWidgetRemoved - clear dangling pointers before widget deletion
// ═════════════════════════════════════════════════════════════════════════════

static bool isDescendantOf(Widget* candidate, Widget* root)
{
    Widget* w = candidate;
    while (w) {
        if (w == root) return true;
        w = w->parent();
    }
    return false;
}

void WidgetApp::notifyWidgetRemoved(Widget* w)
{
    if (!w) return;
    if (pressed_ && isDescendantOf(pressed_, w))
        pressed_ = nullptr;
    if (focused_ && isDescendantOf(focused_, w))
        focused_ = nullptr;
    if (hovered_ && isDescendantOf(hovered_, w))
    {
        hovered_ = nullptr;
        hoverChain_.clear();
    }
    else
    {
        // Remove any entries in hover chain that point to the subtree
        hoverChain_.erase(
            std::remove_if(hoverChain_.begin(), hoverChain_.end(),
                           [w](Widget* h) { return isDescendantOf(h, w); }),
            hoverChain_.end());
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Float windows
// ═════════════════════════════════════════════════════════════════════════════

void WidgetApp::addFloatImpl(FloatWindow* fw)
{
    if (!fw) return;
    floats_.push_back(fw);
    fw->layout();
}

void WidgetApp::removeFloat(FloatWindow* fw)
{
    if (!fw) return;
    notifyWidgetRemoved(fw);
    floats_.erase(std::remove(floats_.begin(), floats_.end(), fw), floats_.end());
    // Defer delete - may be called during signal/bubble handling
    pendingFloatDeletes_.push_back(fw);
}

void WidgetApp::bringToFront(FloatWindow* fw)
{
    auto it = std::find(floats_.begin(), floats_.end(), fw);
    if (it != floats_.end() && it != floats_.end() - 1)
    {
        floats_.erase(it);
        floats_.push_back(fw);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Popup overlay
// ═════════════════════════════════════════════════════════════════════════════

void WidgetApp::showPopup(Widget* popup, Widget* owner, bool owned)
{
    closePopup();
    // Flush any pending delete before opening new popup
    delete pendingPopupDelete_;
    pendingPopupDelete_ = nullptr;
    popup_      = popup;
    popupOwner_ = owner;
    popupOwned_ = owned;
}

void WidgetApp::closePopup()
{
    if (popup_)
    {
        // Reset menu state (hovered/submenu) so it opens clean next time
        popup_->resetPopupState();
        notifyWidgetRemoved(popup_);
        // Defer delete only if we own the popup
        if (popupOwned_)
            pendingPopupDelete_ = popup_;
        popup_      = nullptr;
        popupOwner_ = nullptr;
        popupOwned_ = true;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Hit testing
// ═════════════════════════════════════════════════════════════════════════════

Widget* WidgetApp::hitTest(Widget* w, float x, float y)
{
    if (!w || !w->isVisible())
        return nullptr;

    Rect abs = w->absoluteRect();
    if (!abs.contains(x, y))
        return nullptr;

    // Deepest child first (reverse order = topmost drawn last)
    const auto& ch = w->children();

    // If this widget scrolls, clip child hits to the visible viewport
    auto so = w->scrollOffset();
    bool clips = (so.x != 0 || so.y != 0);

    for (int i = static_cast<int>(ch.size()) - 1; i >= 0; --i)
    {
        // Skip children outside the viewport when parent scrolls
        if (clips)
        {
            Rect childAbs = ch[i]->absoluteRect();
            // Child must overlap the parent's visible area
            if (childAbs.right() <= abs.x || childAbs.x >= abs.right() ||
                childAbs.bottom() <= abs.y || childAbs.y >= abs.bottom())
                continue;
        }
        Widget* hit = hitTest(ch[i], x, y);
        if (hit) return hit;
    }

    return w;
}

void WidgetApp::fillLocal(Widget* target, MouseEvent& e)
{
    if (!target) return;
    Rect abs = target->absoluteRect();
    e.localX = e.x - abs.x;
    e.localY = e.y - abs.y;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Event bubbling - call handler on target, then walk up parents
// ═════════════════════════════════════════════════════════════════════════════

template <typename Func>
void WidgetApp::bubble(Widget* target, MouseEvent& e, Func handler)
{
    Widget* w = target;
    while (w && !e.consumed)
    {
        fillLocal(w, e);
        (w->*handler)(e);
        w = w->parent();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Hover chain - track enter/leave for the full widget path
// ═════════════════════════════════════════════════════════════════════════════

std::vector<Widget*> WidgetApp::buildChain(Widget* w)
{
    std::vector<Widget*> chain;
    while (w)
    {
        chain.push_back(w);
        w = w->parent();
    }
    // Reverse so chain goes root → ... → leaf
    std::reverse(chain.begin(), chain.end());
    return chain;
}

void WidgetApp::applyCursor(CursorType type)
{
    if (type == activeCursor_) return;
    activeCursor_ = type;
    int idx = static_cast<int>(type);
    if (idx >= 0 && idx < 8 && sdlCursors_[idx])
        SDL_SetCursor(static_cast<SDL_Cursor*>(sdlCursors_[idx]));
}

void WidgetApp::updateHover(Widget* newLeaf)
{
    std::vector<Widget*> newChain = newLeaf ? buildChain(newLeaf)
                                            : std::vector<Widget*>{};

    // Find common prefix length
    size_t common = 0;
    size_t minLen = std::min(hoverChain_.size(), newChain.size());
    while (common < minLen && hoverChain_[common] == newChain[common])
        ++common;

    // Leave: old widgets that are no longer in the chain (deepest first)
    for (size_t i = hoverChain_.size(); i > common; --i)
    {
        Widget* w = hoverChain_[i - 1];
        w->hovered_ = false;
        w->onMouseLeave();
        w->markDirty();
    }

    // Enter: new widgets that just appeared in the chain (shallowest first)
    for (size_t i = common; i < newChain.size(); ++i)
    {
        Widget* w = newChain[i];
        w->hovered_ = true;
        w->onMouseEnter();
        w->markDirty();
    }

    hoverChain_ = std::move(newChain);
    hovered_ = newLeaf;

    // Update cursor to match deepest hovered widget
    applyCursor(hovered_ ? hovered_->cursor() : CursorType::Arrow);

    // Reset tooltip when hover target changes
    tooltipTimer_   = 0.0f;
    tooltipVisible_ = false;
    tooltipWidget_  = nullptr;
}

// ═════════════════════════════════════════════════════════════════════════════
//  SDL event handlers
// ═════════════════════════════════════════════════════════════════════════════

void WidgetApp::handleMouseMotion(SDL_Event& event)
{
    mouseX_ = static_cast<float>(event.motion.x);
    mouseY_ = static_cast<float>(event.motion.y);

    // During drag (pressed_), skip hover updates - other widgets
    // should not get enter/leave while dragging a splitter/slider/etc.
    if (pressed_)
    {
        applyCursor(pressed_->cursor());

        MouseEvent me;
        me.x = mouseX_;
        me.y = mouseY_;
        bubble(pressed_, me, &Widget::onMouseMove);
        return;
    }

    Widget* hit = popup_ ? hitTest(popup_, mouseX_, mouseY_) : nullptr;
    // If mouse is in submenu area (outside popup_ rect), route to popup
    if (!hit && popup_ && popup_->popupContains(mouseX_, mouseY_))
        hit = popup_;
    if (!hit && popup_)
    {
        // Popup is open but mouse is outside it - don't hover root widgets
        // (prevents I-beam cursor on TextEdit while context menu is open)
        if (hovered_)
            updateHover(nullptr);
        return;
    }
    if (!hit)
    {
        // Check float windows (topmost = last in vector)
        for (int i = static_cast<int>(floats_.size()) - 1; i >= 0; --i)
        {
            auto* fw = floats_[i];
            if (!fw->isVisible()) continue;
            hit = hitTest(fw, mouseX_, mouseY_);
            if (hit) break;
        }
    }
    if (!hit)
        hit = hitTest(root_, mouseX_, mouseY_);

    // Enter / Leave - full chain tracking
    if (hit != hovered_)
        updateHover(hit);

    // Send move to hovered widget (with bubbling)
    if (hovered_)
    {
        MouseEvent me;
        me.x = mouseX_;
        me.y = mouseY_;
        bubble(hovered_, me, &Widget::onMouseMove);
    }
}

void WidgetApp::handleMouseButton(SDL_Event& event, bool down)
{
    int btn = 0;
    if (event.button.button == SDL_BUTTON_RIGHT)  btn = 1;
    if (event.button.button == SDL_BUTTON_MIDDLE) btn = 2;

    mouseX_ = static_cast<float>(event.button.x);
    mouseY_ = static_cast<float>(event.button.y);

    if (down)
    {
        // Check popup first (if open)
        if (popup_)
        {
            Widget* popupHit = hitTest(popup_, mouseX_, mouseY_);
            // If mouse is in submenu area, route to popup
            if (!popupHit && popup_->popupContains(mouseX_, mouseY_))
                popupHit = popup_;
            if (popupHit)
            {
                MouseEvent me;
                me.x = mouseX_;
                me.y = mouseY_;
                me.button = btn;
                bubble(popupHit, me, &Widget::onMousePress);
                // Don't track pressed_ if popup was closed during bubble
                if (popup_)
                    pressed_ = popupHit;
                return;
            }

            // Click outside popup: close it.
            // For left-clicks on the popup owner (MenuBar), let owner handle toggle.
            bool ownerToggle = (btn == 0) && popupOwner_ &&
                               hitTest(popupOwner_, mouseX_, mouseY_) != nullptr;
            if (!ownerToggle)
            {
                closePopup();
                // For right-clicks, fall through so a new context menu can open.
                // For left-clicks, also fall through to normal handling.
            }
        }

        // Check float windows (topmost first)
        for (int i = static_cast<int>(floats_.size()) - 1; i >= 0; --i)
        {
            auto* fw = floats_[i];
            if (!fw->isVisible()) continue;
            Widget* fwHit = hitTest(fw, mouseX_, mouseY_);
            if (fwHit)
            {
                bringToFront(fw);
                MouseEvent me;
                me.x = mouseX_;
                me.y = mouseY_;
                me.button = btn;
                bubble(fwHit, me, &Widget::onMousePress);
                pressed_ = fwHit;
                return;
            }
        }

        Widget* hit = hitTest(root_, mouseX_, mouseY_);

        if (hit)
        {
            // Right-click → check for context menu (bubble up the chain)
            if (btn == 1)
            {
                Widget* w = hit;
                while (w)
                {
                    if (w->contextMenu())
                    {
                        ContextMenu::show(w->contextMenu(), mouseX_, mouseY_, w);
                        return;
                    }
                    w = w->parent();
                }
            }

            // Focus management
            if (hit->acceptsFocus() && hit != focused_)
                setFocused(hit);

            MouseEvent me;
            me.x = mouseX_;
            me.y = mouseY_;
            me.button = btn;
            bubble(hit, me, &Widget::onMousePress);
            pressed_ = hit;
        }
        else
        {
            setFocused(nullptr);
        }
    }
    else // up
    {
        if (pressed_)
        {
            MouseEvent me;
            me.x = mouseX_;
            me.y = mouseY_;
            me.button = btn;
            bubble(pressed_, me, &Widget::onMouseRelease);
            pressed_ = nullptr;

            // Recalculate hover now that drag ended
            Widget* hit2 = hitTest(root_, mouseX_, mouseY_);
            if (hit2 != hovered_)
                updateHover(hit2);
        }
    }
}

void WidgetApp::handleMouseWheel(SDL_Event& event)
{
    if (!hovered_) return;

    MouseEvent me;
    me.x = mouseX_;
    me.y = mouseY_;
    me.scrollX = static_cast<float>(event.wheel.x);
    me.scrollY = static_cast<float>(event.wheel.y);
    bubble(hovered_, me, &Widget::onMouseScroll);
}

void WidgetApp::handleKey(SDL_Event& event, bool down)
{
    if (!focused_) return;

    KeyEvent ke;
    ke.scancode = event.key.keysym.scancode;
    ke.key      = event.key.keysym.sym;
    ke.shift    = (event.key.keysym.mod & KMOD_SHIFT) != 0;
    ke.ctrl     = (event.key.keysym.mod & KMOD_CTRL)  != 0;
    ke.alt      = (event.key.keysym.mod & KMOD_ALT)   != 0;

    // Bubble up from focused widget
    Widget* w = focused_;
    while (w && !ke.consumed)
    {
        if (down)
            w->onKeyPress(ke);
        else
            w->onKeyRelease(ke);
        w = w->parent();
    }
}

void WidgetApp::handleTextInput(SDL_Event& event)
{
    if (!focused_) return;

    KeyEvent ke;
    SDL_strlcpy(ke.text, event.text.text, sizeof(ke.text));

    Widget* w = focused_;
    while (w && !ke.consumed)
    {
        w->onTextInput(ke);
        w = w->parent();
    }
}

void WidgetApp::handleEvent(SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_MOUSEMOTION:
        handleMouseMotion(event);
        break;
    case SDL_MOUSEBUTTONDOWN:
        handleMouseButton(event, true);
        break;
    case SDL_MOUSEBUTTONUP:
        handleMouseButton(event, false);
        break;
    case SDL_MOUSEWHEEL:
        if (onMouseWheel_ && onMouseWheel_(event.wheel.preciseX, event.wheel.preciseY))
            break;
        handleMouseWheel(event);
        break;
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_F1)
        {
            debugLayout_ = !debugLayout_;
            return;
        }
        if (event.key.keysym.sym == SDLK_F9)
        {
            auto& dev = Device::Instance();
            if (dev.IsRecording())
                dev.StopGifRecording();
            else
                dev.StartGifRecording();
            return;
        }
        if (onKeyDown_ && onKeyDown_(event.key.keysym.sym, event.key.keysym.scancode, event.key.keysym.mod))
            return;
        handleKey(event, true);
        break;
    case SDL_KEYUP:
        if (onKeyUp_ && onKeyUp_(event.key.keysym.sym, event.key.keysym.scancode, event.key.keysym.mod))
            return;
        handleKey(event, false);
        break;
    case SDL_TEXTINPUT:
        if (onTextInput_ && onTextInput_(event.text.text))
            return;
        handleTextInput(event);
        break;
    case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
            event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
            event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
            event.window.event == SDL_WINDOWEVENT_RESTORED)
        {
            SDL_Window* win = Device::Instance().GetWindow();
            int nw, nh;
            SDL_GetWindowSize(win, &nw, &nh);
            width_  = nw;
            height_ = nh;
            SDL_GL_GetDrawableSize(win, &drawW_, &drawH_);
            Device::Instance().SetSize(width_, height_);
            needsLayout_ = true;
        }
        break;
    case SDL_QUIT:
        running_ = false;
        break;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Focus
// ═════════════════════════════════════════════════════════════════════════════

void WidgetApp::setFocused(Widget* w)
{
    if (focused_)
        focused_->setFocused(false);
    focused_ = w;
    if (focused_)
        focused_->setFocused(true);
}

// ═════════════════════════════════════════════════════════════════════════════
//  FPS
// ═════════════════════════════════════════════════════════════════════════════

int WidgetApp::fps() const
{
    return Device::Instance().GetFPS();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Debug layout overlay - recursive wireframe drawing
// ═════════════════════════════════════════════════════════════════════════════

namespace {

// Cycle through distinct colors per depth level
static Color debugColor(int depth)
{
    static const Color colors[] = {
        {255, 50,  50,  180},  // red
        {50,  255, 50,  180},  // green
        {80,  80,  255, 180},  // blue
        {255, 255, 50,  180},  // yellow
        {255, 50,  255, 180},  // magenta
        {50,  255, 255, 180},  // cyan
        {255, 160, 50,  180},  // orange
    };
    return colors[depth % 7];
}

void debugPaintWidget(Widget* w, RenderBatch& line, Font& font, int depth)
{
    if (!w || !w->isVisible()) return;

    Rect abs = w->absoluteRect();
    Color c = debugColor(depth);

    // Draw widget rect outline
    line.SetColor(c.r, c.g, c.b, c.a);
    line.Rectangle(static_cast<int>(abs.x), static_cast<int>(abs.y),
                   static_cast<int>(abs.w), static_cast<int>(abs.h), false);

    // Draw margin area as dashed inner outline (if margins > 0)
    const Edges& m = w->margins();
    if (m.top > 0 || m.right > 0 || m.bottom > 0 || m.left > 0)
    {
        // Outer rect = rect + margins (parent allocated this space)
        line.SetColor(c.r, c.g, c.b, 80);
        float ox = abs.x - m.left;
        float oy = abs.y - m.top;
        float ow = abs.w + m.left + m.right;
        float oh = abs.h + m.top + m.bottom;
        line.Rectangle(static_cast<int>(ox), static_cast<int>(oy),
                       static_cast<int>(ow), static_cast<int>(oh), false);
    }

    // Recurse children
    for (auto* child : w->children())
        debugPaintWidget(child, line, font, depth + 1);
}

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
//  Easing functions
// ═════════════════════════════════════════════════════════════════════════════

static constexpr float PI = 3.14159265358979323846f;

static float bounceOut(float t)
{
    if (t < 1.0f / 2.75f)       return 7.5625f * t * t;
    if (t < 2.0f / 2.75f)     { t -= 1.5f   / 2.75f; return 7.5625f * t * t + 0.75f; }
    if (t < 2.5f / 2.75f)     { t -= 2.25f  / 2.75f; return 7.5625f * t * t + 0.9375f; }
    t -= 2.625f / 2.75f;        return 7.5625f * t * t + 0.984375f;
}

float applyEasing(EaseType type, float t)
{
    switch (type)
    {
    case EaseType::Linear:       return t;
    case EaseType::InQuad:       return t * t;
    case EaseType::OutQuad:      return t * (2.0f - t);
    case EaseType::InOutQuad:    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    case EaseType::InCubic:      return t * t * t;
    case EaseType::OutCubic:     { float u = t - 1.0f; return u * u * u + 1.0f; }
    case EaseType::InOutCubic:   return t < 0.5f ? 4.0f * t * t * t
                                     : 1.0f - 0.5f * ((-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f));
    case EaseType::InExpo:       return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
    case EaseType::OutExpo:      return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
    case EaseType::InOutExpo:
        if (t == 0.0f || t == 1.0f) return t;
        return t < 0.5f ? 0.5f * std::pow(2.0f, 20.0f * t - 10.0f)
                        : 1.0f - 0.5f * std::pow(2.0f, -20.0f * t + 10.0f);
    case EaseType::InBack:       { constexpr float s = 1.70158f; return t * t * ((s + 1.0f) * t - s); }
    case EaseType::OutBack:      { constexpr float s = 1.70158f; float u = t - 1.0f; return u * u * ((s + 1.0f) * u + s) + 1.0f; }
    case EaseType::InOutBack:
    {
        constexpr float s = 1.70158f * 1.525f;
        float u = t * 2.0f;
        if (u < 1.0f) return 0.5f * (u * u * ((s + 1.0f) * u - s));
        u -= 2.0f;
        return 0.5f * (u * u * ((s + 1.0f) * u + s) + 2.0f);
    }
    case EaseType::InBounce:     return 1.0f - bounceOut(1.0f - t);
    case EaseType::OutBounce:    return bounceOut(t);
    case EaseType::InOutBounce:  return t < 0.5f ? 0.5f * (1.0f - bounceOut(1.0f - 2.0f * t))
                                                  : 0.5f * bounceOut(2.0f * t - 1.0f) + 0.5f;
    case EaseType::InElastic:
        return t == 0.0f || t == 1.0f ? t
             : -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * (2.0f * PI / 3.0f));
    case EaseType::OutElastic:
        return t == 0.0f || t == 1.0f ? t
             : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * (2.0f * PI / 3.0f)) + 1.0f;
    case EaseType::InOutElastic:
        if (t == 0.0f || t == 1.0f) return t;
        return t < 0.5f
            ? -0.5f * std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * (2.0f * PI / 4.5f))
            :  0.5f * std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * (2.0f * PI / 4.5f)) + 1.0f;
    }
    return t;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Helper: paint a widget tree with offset + scale (for transitions)
// ═════════════════════════════════════════════════════════════════════════════

void WidgetApp::paintTree(Widget* tree, float w, float h,
                          float offsetX, float offsetY, float scale)
{
    if (!tree) return;

    // Layout at full screen size
    tree->setRect({0, 0, w, h});
    for (auto* c : tree->children())
        c->setRect({0, 0, w, h});
    tree->layout();

    // Build projection: ortho shifted by offset, scaled from center
    glm::mat4 ortho = glm::ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
    if (scale != 1.0f)
    {
        // Translate to center, scale, translate back, then apply offset
        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3(w * 0.5f + offsetX, h * 0.5f + offsetY, 0.0f));
        m = glm::scale(m, glm::vec3(scale, scale, 1.0f));
        m = glm::translate(m, glm::vec3(-w * 0.5f, -h * 0.5f, 0.0f));
        ortho = ortho * m;
    }
    else if (offsetX != 0.0f || offsetY != 0.0f)
    {
        ortho = glm::ortho(-offsetX, w - offsetX, h - offsetY, -offsetY, -1.0f, 1.0f);
    }

    fillBatch_->SetMatrix(ortho);
    lineBatch_->SetMatrix(ortho);
    textBatch_->SetMatrix(ortho);

    PaintContext ctx{*fillBatch_, *lineBatch_, *textBatch_, *font_, iconAtlas_};
    tree->paint(ctx);

    fillBatch_->Render();
    lineBatch_->Render();
    textBatch_->Render();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Render
// ═════════════════════════════════════════════════════════════════════════════

void WidgetApp::render()
{
    if (!root_) return;

    // ── Save caller's GL state (like ImGui does) ─────────────────────────
    GLBackup glBackup;
    backupGLState(glBackup);

    // The RenderState cache may be stale (caller changed GL state directly),
    // so invalidate it before we start.
    auto& rs = RenderState::instance();
    rs.resetCache();

    float w = static_cast<float>(width_);
    float h = static_cast<float>(height_);

    rs.setClearColor(bgColor_.r / 255.0f, bgColor_.g / 255.0f,
                     bgColor_.b / 255.0f, bgColor_.a / 255.0f);
    rs.clear(true, true);

    glViewport(0, 0, drawW_, drawH_);

    rs.setDepthTest(false);
    rs.setBlend(true);
    rs.setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    fillBatch_->resetStats();
    lineBatch_->resetStats();
    textBatch_->resetStats();

    // ── Layout pass (always before paint, regardless of transition) ───────
    if (needsLayout_ && root_)
    {
        root_->setRect({0, 0, w, h});
        for (auto* c : root_->children())
            c->setRect({0, 0, w, h});
        root_->layout();

        // Also relayout old stage during transitions
        if (transActive_ && transOldRoot_)
        {
            transOldRoot_->setRect({0, 0, w, h});
            for (auto* c : transOldRoot_->children())
                c->setRect({0, 0, w, h});
            transOldRoot_->layout();
        }
        needsLayout_ = false;
    }

    // ── Transition in progress ───────────────────────────────────────────
    if (transActive_)
    {
        transProgress_ += dt_ / transDuration_;
        if (transProgress_ >= 1.0f)
        {
            transProgress_ = 1.0f;
            transActive_   = false;
            transOldRoot_  = nullptr;
            transOldStage_.clear();
        }

        float t = applyEasing(transEaseCur_, transProgress_);

        switch (transCurrent_)
        {
        // ── Slide: both move together ────────────────────────────────────
        case TransitionType::SlideLeft:
            paintTree(transOldRoot_, w, h, -t * w, 0, 1.0f);
            paintTree(root_,         w, h,  w * (1.0f - t), 0, 1.0f);
            break;

        case TransitionType::SlideRight:
            paintTree(transOldRoot_, w, h,  t * w, 0, 1.0f);
            paintTree(root_,         w, h, -w * (1.0f - t), 0, 1.0f);
            break;

        case TransitionType::SlideUp:
            paintTree(transOldRoot_, w, h, 0, -t * h, 1.0f);
            paintTree(root_,         w, h, 0,  h * (1.0f - t), 1.0f);
            break;

        case TransitionType::SlideDown:
            paintTree(transOldRoot_, w, h, 0,  t * h, 1.0f);
            paintTree(root_,         w, h, 0, -h * (1.0f - t), 1.0f);
            break;

        // ── Cover: old stays, new slides over ────────────────────────────
        case TransitionType::CoverLeft:
            paintTree(transOldRoot_, w, h, 0, 0, 1.0f);
            paintTree(root_,         w, h,  w * (1.0f - t), 0, 1.0f);
            break;

        case TransitionType::CoverRight:
            paintTree(transOldRoot_, w, h, 0, 0, 1.0f);
            paintTree(root_,         w, h, -w * (1.0f - t), 0, 1.0f);
            break;

        case TransitionType::CoverUp:
            paintTree(transOldRoot_, w, h, 0, 0, 1.0f);
            paintTree(root_,         w, h, 0,  h * (1.0f - t), 1.0f);
            break;

        case TransitionType::CoverDown:
            paintTree(transOldRoot_, w, h, 0, 0, 1.0f);
            paintTree(root_,         w, h, 0, -h * (1.0f - t), 1.0f);
            break;

        // ── Reveal: old slides out, new stays behind ─────────────────────
        case TransitionType::RevealLeft:
            paintTree(root_,         w, h, 0, 0, 1.0f);
            paintTree(transOldRoot_, w, h, -t * w, 0, 1.0f);
            break;

        case TransitionType::RevealRight:
            paintTree(root_,         w, h, 0, 0, 1.0f);
            paintTree(transOldRoot_, w, h,  t * w, 0, 1.0f);
            break;

        case TransitionType::RevealUp:
            paintTree(root_,         w, h, 0, 0, 1.0f);
            paintTree(transOldRoot_, w, h, 0, -t * h, 1.0f);
            break;

        case TransitionType::RevealDown:
            paintTree(root_,         w, h, 0, 0, 1.0f);
            paintTree(transOldRoot_, w, h, 0,  t * h, 1.0f);
            break;

        // ── Zoom: scale from/to center ───────────────────────────────────
        case TransitionType::ZoomIn:
            paintTree(transOldRoot_, w, h, 0, 0, 1.0f);
            paintTree(root_,         w, h, 0, 0, t);
            break;

        case TransitionType::ZoomOut:
            paintTree(root_,         w, h, 0, 0, 1.0f);
            paintTree(transOldRoot_, w, h, 0, 0, 1.0f - t);
            break;

        default:
            break;
        }
    }
    else
    {
        // ── Normal rendering (no transition) ─────────────────────────────
        fillBatch_->SetOrtho2D(w, h);
        lineBatch_->SetOrtho2D(w, h);
        textBatch_->SetOrtho2D(w, h);

        PaintContext ctx{*fillBatch_, *lineBatch_, *textBatch_, *font_, iconAtlas_};
        if (root_) root_->paint(ctx);

        fillBatch_->Render();
        lineBatch_->Render();
        textBatch_->Render();

        // Paint float windows (above stage, below popup)
        for (auto* fw : floats_)
        {
            if (!fw->isVisible()) continue;
            fw->layout();
            fw->paint(ctx);
            fillBatch_->Render();
            lineBatch_->Render();
            textBatch_->Render();
        }

        // Paint popup in separate pass - always renders on top
        if (popup_)
        {
            popup_->paint(ctx);
            fillBatch_->Render();
            lineBatch_->Render();
            textBatch_->Render();
        }
    }

    // Debug layout overlay (F1 to toggle)
    if (debugLayout_)
    {
        lineBatch_->SetOrtho2D(w, h);
        debugPaintWidget(root_, *lineBatch_, *font_, 0);
        lineBatch_->Render();
    }

    // Overlay callback (stats, debug, etc.)
    if (overlay_)
    {
        fillBatch_->SetOrtho2D(w, h);
        lineBatch_->SetOrtho2D(w, h);
        textBatch_->SetOrtho2D(w, h);
        PaintContext octx{*fillBatch_, *lineBatch_, *textBatch_, *font_};
        overlay_(octx);
        fillBatch_->Render();
        lineBatch_->Render();
        textBatch_->Render();
    }

    // ── GIF recording indicator ──────────────────────────────────────────
    if (Device::Instance().IsRecording())
    {
        fillBatch_->SetOrtho2D(w, h);
        lineBatch_->SetOrtho2D(w, h);
        textBatch_->SetOrtho2D(w, h);

        float rx = w - 90.0f, ry = 4.0f;

        // Blinking red dot (1Hz)
        float blink = Device::Instance().GifElapsed();
        bool on = (static_cast<int>(blink * 2.0f) % 2) == 0;
        if (on)
        {
            fillBatch_->SetColor(220, 40, 40, 255);
            fillBatch_->Circle(static_cast<int>(rx + 6), static_cast<int>(ry + 8), 5, true);
        }

        // "REC" text + elapsed
        char recBuf[32];
        int secs = static_cast<int>(Device::Instance().GifElapsed());
        snprintf(recBuf, sizeof(recBuf), "REC %d:%02d", secs / 60, secs % 60);
        font_->SetFontSize(13.0f);
        font_->SetColor(Color(220, 40, 40, 255));
        font_->SetBatch(textBatch_);
        font_->Print(recBuf, rx + 14.0f, ry);

        fillBatch_->Render();
        lineBatch_->Render();
        textBatch_->Render();
    }

    // ── Tooltip ──────────────────────────────────────────────────────────
    // Accumulate hover time; show tooltip when delay exceeded
    if (hovered_ && !hovered_->tooltip().empty())
    {
        tooltipTimer_ += dt_;
        if (tooltipTimer_ >= hovered_->tooltipDelay())
        {
            tooltipVisible_ = true;
            tooltipWidget_  = hovered_;
        }
    }
    else
    {
        tooltipTimer_   = 0.0f;
        tooltipVisible_ = false;
        tooltipWidget_  = nullptr;
    }
    if (tooltipVisible_ && tooltipWidget_)
        paintTooltip();

    // ── Toast notifications ───────────────────────────────────────────────
    Toast::tick(dt_);
    if (Toast::hasActive())
    {
        fillBatch_->SetOrtho2D(w, h);
        lineBatch_->SetOrtho2D(w, h);
        textBatch_->SetOrtho2D(w, h);
        PaintContext tctx{*fillBatch_, *lineBatch_, *textBatch_, *font_, iconAtlas_};
        Toast::paint(tctx, static_cast<float>(w), static_cast<float>(h));
        fillBatch_->Render();
        lineBatch_->Render();
        textBatch_->Render();
    }

    // ── Restore caller's GL state ─────────────────────────────────────────
    restoreGLState(glBackup);
    rs.resetCache();  // cache is stale again after raw GL restore
}

// ═════════════════════════════════════════════════════════════════════════════
//  Tooltip rendering
// ═════════════════════════════════════════════════════════════════════════════

void WidgetApp::paintTooltip()
{
    if (!tooltipWidget_ || tooltipWidget_->tooltip().empty()) return;

    const auto& t = Theme::instance();
    const std::string& text = tooltipWidget_->tooltip();

    float w = static_cast<float>(width_);
    float h = static_cast<float>(height_);

    fillBatch_->SetOrtho2D(w, h);
    lineBatch_->SetOrtho2D(w, h);
    textBatch_->SetOrtho2D(w, h);

    font_->SetFontSize(t.fontSize * 0.85f);
    font_->SetBatch(textBatch_);
    float textW = font_->GetTextWidth(text.c_str());
    float textH = t.fontSize * 0.85f;

    float padX = 8.0f, padY = 4.0f;
    float tipW = textW + padX * 2;
    float tipH = textH + padY * 2;

    // Position: below and right of cursor, clamped to screen
    float tx = mouseX_ + 12.0f;
    float ty = mouseY_ + 18.0f;
    if (tx + tipW > w) tx = w - tipW - 2.0f;
    if (ty + tipH > h) ty = mouseY_ - tipH - 4.0f;
    if (tx < 0) tx = 2.0f;
    if (ty < 0) ty = 2.0f;

    // Background
    fillBatch_->SetColor(t.tooltipBg.r, t.tooltipBg.g, t.tooltipBg.b, t.tooltipBg.a);
    fillBatch_->RoundedRectangle(
        static_cast<int>(tx), static_cast<int>(ty),
        static_cast<int>(tipW), static_cast<int>(tipH),
        3.0f, 4, true);

    // Border
    lineBatch_->SetColor(t.tooltipBorder.r, t.tooltipBorder.g, t.tooltipBorder.b, t.tooltipBorder.a);
    lineBatch_->RoundedRectangle(
        static_cast<int>(tx), static_cast<int>(ty),
        static_cast<int>(tipW), static_cast<int>(tipH),
        3.0f, 4, false);

    // Text
    font_->SetColor(t.tooltipText);
    font_->Print(text.c_str(), tx + padX, ty + padY);

    fillBatch_->Render();
    lineBatch_->Render();
    textBatch_->Render();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Main loop
// ═════════════════════════════════════════════════════════════════════════════

int WidgetApp::run()
{
    if (!inited_) return 1;

    running_ = true;
    Device& dev = Device::Instance();

    if (onCreate_) onCreate_();

    while (running_)
    {
 
        SDL_Event event;
        while (SDL_PollEvent(&event))
            handleEvent(event);

 
        if (pendingPopupDelete_)
        {
            delete pendingPopupDelete_;
            pendingPopupDelete_ = nullptr;
        }

        for (auto* fw : pendingFloatDeletes_)
            delete fw;
        pendingFloatDeletes_.clear();

        if (!running_) break;

        // User idle callback (update logic, animations)
        if (onIdle_) onIdle_(dt_);

        // Render
        render();

        // Swap + update frame timing (m_frame, needed for GetFPS)
        dev.Flip();
        dt_ = dev.GetFrameTime();
    }

    if (onDestroy_) onDestroy_();
    shutdown();
    dev.Close();
    return 0;
}
