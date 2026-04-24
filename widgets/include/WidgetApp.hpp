#pragma once

#include "Widget.hpp"
#include "Color.hpp"
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>

// ═════════════════════════════════════════════════════════════════════════════
//  Easing functions for transitions
// ═════════════════════════════════════════════════════════════════════════════

enum class EaseType
{
    Linear,
    InQuad,     OutQuad,     InOutQuad,
    InCubic,    OutCubic,    InOutCubic,
    InExpo,     OutExpo,     InOutExpo,
    InBack,     OutBack,     InOutBack,
    InBounce,   OutBounce,   InOutBounce,
    InElastic,  OutElastic,  InOutElastic,
};

float applyEasing(EaseType type, float t);

// ═════════════════════════════════════════════════════════════════════════════
//  Transition types for stage switching animations
// ═════════════════════════════════════════════════════════════════════════════

enum class TransitionType
{
    None,          // instant swap (no animation)
    SlideLeft,     // old exits left,  new enters from right
    SlideRight,    // old exits right, new enters from left
    SlideUp,       // old exits up,    new enters from bottom
    SlideDown,     // old exits down,  new enters from top
    CoverLeft,     // new slides in from right over stationary old
    CoverRight,    // new slides in from left  over stationary old
    CoverUp,       // new slides in from bottom over stationary old
    CoverDown,     // new slides in from top    over stationary old
    RevealLeft,    // old slides out left,  revealing stationary new
    RevealRight,   // old slides out right, revealing stationary new
    RevealUp,      // old slides out up,    revealing stationary new
    RevealDown,    // old slides out down,  revealing stationary new
    ZoomIn,        // new zooms in from center (small → full)
    ZoomOut,       // old zooms out to center (full → small), new behind
};

class RenderBatch;
class Font;
class FloatWindow;

// ═════════════════════════════════════════════════════════════════════════════
//  Window flags (combinable via |)
// ═════════════════════════════════════════════════════════════════════════════

enum WindowFlags : unsigned int
{
    WindowFlag_None        = 0,
    WindowFlag_Resizable   = 1 << 0,   // window can be resized (default on)
    WindowFlag_Borderless  = 1 << 1,   // no window decorations
    WindowFlag_Fullscreen  = 1 << 2,   // fullscreen desktop
    WindowFlag_Maximized   = 1 << 3,   // start maximized
    WindowFlag_AlwaysOnTop = 1 << 4,   // window stays on top

    WindowFlag_Default = WindowFlag_Resizable,
};

// ═════════════════════════════════════════════════════════════════════════════
//  WidgetApp — singleton that manages window, rendering, events, widget tree
//
//  Usage:
//      auto& app = WidgetApp::instance();
//      app.init("My App", 1024, 768);
//
//      auto* btn = app.root()->createChild<Button>("Click");
//      btn->onClick.connect([]{ ... });
//
//      app.run();   // blocks until window closes
// ═════════════════════════════════════════════════════════════════════════════

class WidgetApp
{
public:
    static WidgetApp& instance();

    // Initialize window + GL + batch + font. Returns false on failure.
    // monitor: -1 = default/centered, 0..N = specific display index
    bool init(const char* title, int width = 1024, int height = 768,
              int monitor = -1, unsigned int flags = WindowFlag_Default);

    // ── Window properties (runtime) ───────────────────────────────────────
    bool isResizable() const;
    bool isMaximized() const;
    void setResizable(bool r);
    void setMinSize(int w, int h);
    void setMaxSize(int w, int h);
    void setTitle(const char* title);

    // Run the main loop (blocks). Returns exit code.
    int run();

    // Release all resources (called automatically by run() and destructor).
    void shutdown();

    // Stop the main loop (call from signal handlers etc.)
    void quit() { running_ = false; }

    // ── Lifecycle callbacks ───────────────────────────────────────────────
    // Called once after init, GL context is alive — create textures, shaders, etc.
    void setOnCreate(std::function<void()> cb)  { onCreate_  = std::move(cb); }
    // Called before shutdown, GL context still alive — free user GL resources.
    void setOnDestroy(std::function<void()> cb) { onDestroy_ = std::move(cb); }
    // Called every frame before render — update logic, animations, etc.
    void setOnIdle(std::function<void(float dt)> cb) { onIdle_ = std::move(cb); }

    // ── Input event callbacks ──────────────────────────────────────────────
    // Return true to consume the event (widgets won't receive it).
    void setOnKeyDown(std::function<bool(int key, int scancode, int mod)> cb)    { onKeyDown_    = std::move(cb); }
    void setOnKeyUp(std::function<bool(int key, int scancode, int mod)> cb)      { onKeyUp_      = std::move(cb); }
    void setOnTextInput(std::function<bool(const char* text)> cb)                { onTextInput_  = std::move(cb); }
    void setOnMouseWheel(std::function<bool(float x, float y)> cb)               { onMouseWheel_ = std::move(cb); }

    // ── Root widget ───────────────────────────────────────────────────────
    // A default root Panel is created by init(). Add children to it.
    Widget* root() const { return root_; }

    // Replace the root widget (takes ownership, deletes old)
    void setRoot(Widget* r);

    // ── Stage management ──────────────────────────────────────────────────
    // A Stage is a named scene (widget tree). Only one stage is active/visible.
    // Inactive stages are kept in memory for instant switching.
    //
    //   app.addStage("menu");
    //   app.stage("menu")->createChild<Label>("Main Menu");
    //   app.addStage("game");
    //   app.stage("game")->createChild<Canvas>(...);
    //   app.setStage("menu");  // shows menu
    //   app.setStage("game");  // switches to game, menu preserved

    // Create a new stage. Returns its root widget (a plain Widget container).
    // If the stage already exists, returns the existing root.
    Widget* addStage(const std::string& name);

    // Switch to a named stage with optional transition and easing.
    // Returns false if stage not found.
    bool setStage(const std::string& name);
    bool setStage(const std::string& name, TransitionType transition);
    bool setStage(const std::string& name, TransitionType transition, EaseType ease);

    // Set default transition type, easing and duration
    void setTransition(TransitionType type, float durationSec = 0.3f, EaseType ease = EaseType::InOutCubic);
    TransitionType transitionType() const { return transType_; }
    EaseType       transitionEase() const { return transEase_; }
    float transitionDuration() const { return transDuration_; }
    bool  isTransitioning()   const { return transActive_; }

    // Get a stage's root widget (nullptr if not found). Works for any stage.
    Widget* stage(const std::string& name) const;

    // Current stage name (empty if no stage system used)
    const std::string& currentStageName() const { return currentStage_; }

    // Remove a stage (deletes its widget tree). Cannot remove active stage.
    bool removeStage(const std::string& name);

    // Request a layout pass on the next frame
    void requestLayout() { needsLayout_ = true; }

    // Clear any internal references to a widget about to be destroyed
    // (called automatically by Widget::removeChild)
    void notifyWidgetRemoved(Widget* w);

    // ── Float windows ───────────────────────────────────────────────────
    // Floating windows always render on top of the stage (below popups).
    template <typename T = FloatWindow, typename... Args>
    T* addFloat(Args&&... args)
    {
        auto* fw = new T(std::forward<Args>(args)...);
        addFloatImpl(fw);
        return fw;
    }
    void removeFloat(FloatWindow* fw);    // deletes it
    void bringToFront(FloatWindow* fw);   // re-stack to top
    const std::vector<FloatWindow*>& floats() const { return floats_; }

    // ── Accessors ─────────────────────────────────────────────────────────
    int  width()  const { return width_; }
    int  height() const { return height_; }
    int  fps()    const;
    float deltaTime() const { return dt_; }
    float mouseX() const { return mouseX_; }
    float mouseY() const { return mouseY_; }

    // Background color
    void setBgColor(const Color& c) { bgColor_ = c; }

    // Debug layout visualization (toggle with F1)
    bool debugLayout() const { return debugLayout_; }
    void setDebugLayout(bool d) { debugLayout_ = d; }

    // Optional per-frame callback (called after widget paint, before swap)
    void setOverlayCallback(std::function<void(PaintContext&)> cb) { overlay_ = std::move(cb); }

    // ── Focus ─────────────────────────────────────────────────────────────
    void setFocused(Widget* w);
    Widget* focused() const { return focused_; }

    // ── Widget lookup shortcuts ───────────────────────────────────────────
    // Find by ID in current root (active stage)
    Widget* widget(const std::string& id) { return root_ ? root_->findById(id) : nullptr; }

    template <typename T>
    T* widget(const std::string& id) { return root_ ? root_->findById<T>(id) : nullptr; }

    // Find all widgets with a tag in current root
    std::vector<Widget*> widgetsByTag(const std::string& tag);

    template <typename T>
    std::vector<T*> widgetsByTag(const std::string& tag)
    {
        return root_ ? root_->findByTag<T>(tag) : std::vector<T*>{};
    }

    // ── Global event dispatcher ───────────────────────────────────────────
    // Register callbacks for widgets by ID. Works with JSON-loaded trees.
    //   app.on("click",   "save-btn",       [](Widget* w){ ... });
    //   app.on("press",   "my-button",      [](Widget* w, int btn){ ... });
    //   app.on("release", "my-button",      [](Widget* w, int btn){ ... });
    //   app.on("change",  "opacity-slider", [](Widget* w, float v){ ... });
    //   app.on("toggle",  "wireframe-chk",  [](Widget* w, bool v){ ... });
    //   app.on("hover",   "panel",          [](Widget* w){ ... });
    //   app.on("leave",   "panel",          [](Widget* w){ ... });
    //   app.on("move",    "canvas",         [](Widget* w, float lx, float ly){ ... });
    using EventCallback = std::function<void(Widget*)>;
    void on(const std::string& event, const std::string& widgetId, EventCallback cb);

    // Global catch-all: fires for every widget on that event
    //   app.onAny("click", [](Widget* w){ if (w->hasTag("btn")) ... });
    void onAny(const std::string& event, EventCallback cb);

    // Fire a global event (called internally by WidgetApp after signal emit)
    void fireEvent(const std::string& event, Widget* w);

    // ── Popup overlay ─────────────────────────────────────────────────
    // Show a popup widget on top of everything.
    // owner = the widget that opened it (clicks on owner won't close it).
    // owned = if true (default), popup is deleted on close.
    void showPopup(Widget* popup, Widget* owner = nullptr, bool owned = true);
    void closePopup();
    Widget* popup() const { return popup_; }
    Widget* popupOwner() const { return popupOwner_; }

    // ── Internal access (for widgets that need batch/font) ────────────────
    RenderBatch& fillBatch();
    RenderBatch& lineBatch();
    RenderBatch& textBatch();
    Font& font();
    IconAtlas& iconAtlas();

private:
    WidgetApp() = default;
    void addFloatImpl(FloatWindow* fw);
    ~WidgetApp();

    WidgetApp(const WidgetApp&) = delete;
    WidgetApp& operator=(const WidgetApp&) = delete;

    // Event handling — called per SDL event
    void handleEvent(union SDL_Event& event);
    void handleMouseButton(union SDL_Event& event, bool down);
    void handleMouseMotion(union SDL_Event& event);
    void handleMouseWheel(union SDL_Event& event);
    void handleKey(union SDL_Event& event, bool down);
    void handleTextInput(union SDL_Event& event);

    // Hit test — find deepest widget under (x,y)
    Widget* hitTest(Widget* w, float x, float y);

    // Fill localX/localY on a MouseEvent for a target widget
    void fillLocal(Widget* target, MouseEvent& e);

    // Bubble an event up the parent chain until consumed
    template <typename Func>
    void bubble(Widget* target, MouseEvent& e, Func handler);

    // Build chain from root to widget (inclusive)
    static std::vector<Widget*> buildChain(Widget* w);

    // Update hover chain, fire enter/leave
    void updateHover(Widget* newLeaf);

    // Apply SDL cursor
    void applyCursor(CursorType type);

    // Render
    void render();

    // Paint a widget tree with offset + scale (for transitions)
    void paintTree(Widget* tree, float w, float h,
                   float offsetX, float offsetY, float scale);

    // State
    bool  inited_       = false;
    bool  running_      = false;
    bool  needsLayout_  = true;
    int   width_   = 0;
    int   height_  = 0;
    int   drawW_   = 0;   // GL drawable size (for glViewport, may differ on HiDPI)
    int   drawH_   = 0;
    float dt_      = 0.0f;
    Color bgColor_ = Color(30, 30, 30, 255);
    bool  debugLayout_ = false;

    // Subsystems (pointers to avoid including headers)
    RenderBatch* fillBatch_  = nullptr;  // filled shapes
    RenderBatch* lineBatch_  = nullptr;  // outlines, borders
    RenderBatch* textBatch_  = nullptr;  // font quads
    Font*        font_       = nullptr;
    IconAtlas*   iconAtlas_  = nullptr;  // icon sprite sheet

    // Widget tree
    Widget* root_    = nullptr;
    Widget* focused_ = nullptr;
    Widget* hovered_ = nullptr;
    Widget* pressed_ = nullptr;
    std::vector<Widget*> hoverChain_;   // root → ... → deepest hovered
    float   mouseX_  = 0;
    float   mouseY_  = 0;

    // Popup overlay
    Widget* popup_              = nullptr;   // popup widget (may or may not be owned)
    Widget* popupOwner_         = nullptr;   // widget that opened it (not owned)
    Widget* pendingPopupDelete_ = nullptr;   // deferred delete (safe after event loop)
    bool    popupOwned_         = true;      // if true, popup is deleted on close

    // Float windows (painted above stage, below popup)
    std::vector<FloatWindow*> floats_;
    std::vector<FloatWindow*> pendingFloatDeletes_;  // deferred delete (safe after event loop)

    // Tooltip
    float   tooltipTimer_   = 0.0f;   // seconds hovered on current widget
    bool    tooltipVisible_ = false;
    Widget* tooltipWidget_  = nullptr; // widget whose tooltip is showing
    void    paintTooltip();

    // Cursors (SDL system cursors, indexed by CursorType)
    void*       sdlCursors_[8] = {};
    CursorType  activeCursor_  = CursorType::Arrow;

    // Stage system
    std::unordered_map<std::string, Widget*> stages_;  // name → root widget
    std::string currentStage_;

    // Transition
    TransitionType transType_     = TransitionType::SlideLeft;
    EaseType       transEase_     = EaseType::InOutCubic;
    float          transDuration_ = 0.3f;   // seconds
    bool           transActive_   = false;
    float          transProgress_ = 0.0f;   // 0..1
    TransitionType transCurrent_  = TransitionType::None; // type of active transition
    EaseType       transEaseCur_  = EaseType::InOutCubic; // ease of active transition
    Widget*        transOldRoot_  = nullptr;
    std::string    transOldStage_;

    // Lifecycle callbacks
    std::function<void()> onCreate_;
    std::function<void()> onDestroy_;
    std::function<void(float)> onIdle_;

    // Input event callbacks
    std::function<bool(int, int, int)> onKeyDown_;
    std::function<bool(int, int, int)> onKeyUp_;
    std::function<bool(const char*)>   onTextInput_;
    std::function<bool(float, float)>  onMouseWheel_;

    // Optional overlay callback
    std::function<void(PaintContext&)> overlay_;

    // Global event dispatcher
    // Key = "event:widgetId" for targeted, "event:*" for catch-all
    std::unordered_map<std::string, std::vector<EventCallback>> globalHandlers_;
};
