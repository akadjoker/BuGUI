# BuGUI — Developer Guide

C++17 widget toolkit built on SDL2 + OpenGL 3.3.  
Single-window, immediate-style layout, retained widget tree.

---

## Table of Contents

1. [Quick Start](#1-quick-start)
2. [Architecture](#2-architecture)
3. [Widget Basics](#3-widget-basics)
4. [Layout System](#4-layout-system)
5. [Signals & Events](#5-signals--events)
6. [Theme](#6-theme)
7. [Stages & Transitions](#7-stages--transitions)
8. [Float Windows](#8-float-windows)
9. [Popups & Dialogs](#9-popups--dialogs)
10. [Widget Reference](#10-widget-reference)
11. [Drawing (PaintContext)](#11-drawing-paintcontext)
12. [Build](#12-build)

---

## 1. Quick Start

```cpp
#include "WidgetApp.hpp"
#include "Widgets.hpp"

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("My App", 800, 600))
        return 1;

    Widget* root = app.root();

    auto* col = root->createChild<BoxLayout>(LayoutDir::Vertical);
    col->setSpacing(8);
    col->setPadding(12);

    col->createChild<Label>("Hello BuGUI!");

    auto* btn = col->createChild<Button>("Click me");
    btn->clicked.connect([]{ printf("Clicked!\n"); });

    auto* slider = col->createChild<Slider>(0, 100, 50);
    auto* label  = col->createChild<Label>("50");
    slider->onValueChanged.connect([label](float v){
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f", v);
        label->setText(buf);
    });

    return app.run();
}
```

---

## 2. Architecture

```
┌─────────────────────────────────┐
│         WidgetApp (singleton)   │
│  ┌──────────┐  ┌────────────┐  │
│  │  Stages   │  │  Floats    │  │  ← FloatWindow layer
│  │  (roots)  │  │  (on top)  │  │
│  └──────────┘  └────────────┘  │
│  ┌──────────────────────────┐   │
│  │  Popup (topmost layer)   │   │  ← Dialog / ComboBox dropdown
│  └──────────────────────────┘   │
│  ┌──────────────────────────┐   │
│  │  Overlay callback        │   │  ← FPS counter, debug info
│  └──────────────────────────┘   │
└─────────────────────────────────┘
```

**Render order:** root → floats (each flushed) → popup (flushed) → debug → overlay  
**Hit test order:** popup → floats (topmost first) → root

**3 render batches** (all use `RenderBatch`):
- `fill` — triangles (rectangles, rounded rects, circles, images)
- `line` — lines (borders, separators, icons)
- `text` — quads with font texture

---

## 3. Widget Basics

### Creating widgets

```cpp
// Option 1: createChild (preferred — parent owns the widget)
auto* label = parent->createChild<Label>("text");

// Option 2: new + addChild
auto* label = new Label("text");
parent->addChild(label);
```

`removeChild(w)` deletes the widget. Never delete a widget manually.

### Size and position

```cpp
w->setSize(200, 30);        // fixed size (0 = flexible on that axis)
w->setStretch(1);            // flex factor (like CSS flex-grow)
w->setMargins(10, 5, 10, 5); // top, right, bottom, left
w->setVisible(false);
w->setEnabled(false);
```

### Identity

```cpp
w->setId("myWidget");
w->addTag("sidebar");
w->setTooltip("Help text");
w->setCursor(CursorType::Hand);

// Find later
auto* found = root->findById<Button>("myWidget");
auto items = root->findByTag<Label>("sidebar");

// User data (std::any)
w->setData("score", 42);
int s = w->data<int>("score");
```

---

## 4. Layout System

### BoxLayout (main layout container)

```cpp
auto* row = parent->createChild<BoxLayout>(LayoutDir::Horizontal);
row->setSpacing(8);              // gap between children
row->setPadding(10);             // uniform inner padding
row->setPadding(10, 20, 10, 20); // top, right, bottom, left
row->setMainAlign(MainAlign::SpaceBetween);
```

Children consume space by **stretch** factor:

```cpp
auto* fixed = row->createChild<Button>("OK");
fixed->setSize(80, 30);     // fixed 80px wide
fixed->setStretch(0);        // no flex (default)

auto* fill = row->createChild<Panel>();
fill->setStretch(1);          // takes remaining space
```

**MainAlign options:** `Start`, `Center`, `End`, `SpaceBetween`, `SpaceEvenly`

### GridLayout

```cpp
auto* grid = parent->createChild<GridLayout>(3); // 3 columns
grid->setSpacing(6, 6);  // horizontal, vertical
grid->setPadding(10);
// Children fill cells left-to-right, top-to-bottom
grid->createChild<Button>("A");
grid->createChild<Button>("B");
grid->createChild<Button>("C"); // row 2 starts...
```

### FormLayout (label + widget rows)

```cpp
auto* form = parent->createChild<FormLayout>();
form->setLabelWidth(120);
form->setSpacing(6);
auto* nameInput = form->addRow<Button>("Name:");  // placeholder
auto* slider    = form->addRow<Slider>("Volume:", 0, 100, 50);
```

### BorderLayout (5-region)

```cpp
auto* border = parent->createChild<BorderLayout>();
border->set<StatusBar>(BorderLayout::Bottom);
border->set<Panel>(BorderLayout::Left)->setSize(200, 0);
border->set<Panel>(BorderLayout::Center);
border->setRegionSize(BorderLayout::Left, 200);
```

### Other layouts

| Layout | Purpose |
|--------|---------|
| `FlowLayout` | Wraps children like text (tag clouds) |
| `StackLayout` | Shows one child at a time by index |
| `AnchorLayout` | Positions children with fractional anchors |
| `Overlay` | All children fill full area, stacked |
| `Splitter` | Two children with draggable divider |
| `TabLayout` | Tab bar + content pages |
| `PageView` | Like StackLayout but with slide/fade transitions |

---

## 5. Signals & Events

### Signals (type-safe observer pattern)

```cpp
// Connect
btn->clicked.connect([]{ printf("clicked\n"); });

// Connect with args
slider->onValueChanged.connect([](float val){ printf("%.1f\n", val); });

// Store ID to disconnect later
auto id = btn->clicked.connect(myFunc);
btn->clicked.disconnect(id);
```

### Built-in widget signals

| Widget | Signal | Type |
|--------|--------|------|
| Widget (base) | `clicked` | `Signal<>` |
| Widget (base) | `pressed` / `released` | `Signal<int>` (button id) |
| Widget (base) | `hoverEnter` / `hoverLeave` | `Signal<>` |
| Button | `clicked` | `Signal<>` |
| CheckBox | `toggled` | `Signal<bool>` |
| Switch | `toggled` | `Signal<bool>` |
| RadioGroup | `selectionChanged` | `Signal<int>` |
| Slider | `onValueChanged` | `Signal<float>` |
| ProgressBar | `onValueChanged` | `Signal<float>` |
| ListBox | `selectionChanged` | `Signal<int>` |
| ListBox | `itemActivated` | `Signal<int>` |
| ComboBox | `selectionChanged` | `Signal<int>` |
| TabLayout | `currentChanged` | `Signal<int>` |
| TabLayout | `tabClosed` | `Signal<int>` |
| Splitter | `ratioChanged` | `Signal<float>` |
| Collapsible | `expandedChanged` | `Signal<bool>` |
| FloatWindow | `closed` | `Signal<>` |
| FloatWindow | `minimizedChanged` | `Signal<bool>` |
| SidePanel | `openChanged` | `Signal<bool>` |
| PageView | `pageChanged` | `Signal<int>` |
| Dialog | `buttonClicked` | `Signal<int>` |
| ConfirmDialog | `confirmed` / `cancelled` | `Signal<>` |
| Carousel | `pageChanged` | `Signal<int>` |

### Event overrides (for custom widgets)

```cpp
class MyWidget : public Widget {
    void onMousePress(MouseEvent& e) override {
        printf("click at %.0f, %.0f\n", e.x, e.y);
        e.consumed = true;  // stop bubbling
    }
    void onKeyPress(KeyEvent& e) override {
        if (e.key == SDLK_RETURN) { /* ... */ e.consumed = true; }
    }
    void paint(PaintContext& ctx) override {
        Rect abs = absoluteRect();
        ctx.fill.SetColor(255, 0, 0, 255);
        ctx.fill.Rectangle(abs.x, abs.y, abs.w, abs.h, true);
    }
};
```

### App-level input callbacks

```cpp
app.setOnKeyDown([](int key, int scancode, int mod) -> bool {
    if (key == SDLK_ESCAPE) { WidgetApp::instance().quit(); return true; }
    return false;
});
app.setOnMouseWheel([](float x, float y) -> bool { return false; });
app.setOnTextInput([](const char* text) -> bool { return false; });
```

---

## 6. Theme

All colors are in the `Theme` singleton. Modify before creating widgets:

```cpp
auto& t = Theme::instance();
t.bgColor = Color(20, 20, 25, 255);        // window background
t.panelColor = Color(45, 45, 48, 255);      // panel fill
t.buttonNormal = Color(60, 60, 65, 255);    // button default
t.buttonHover = Color(75, 75, 80, 255);
t.focusColor = Color(100, 150, 255, 255);   // focus ring
t.fontSize = 16.0f;
t.borderRadius = 4.0f;
t.buttonHeight = 28.0f;
```

**Color categories:** Window/Panel, Text, Button, Focus/Selection, Slider, Input/CheckBox, Tooltip, FloatWindow, Drawer, Dialog.

---

## 7. Stages & Transitions

Stages are named widget trees. Only one is active at a time.

```cpp
// Create stages
Widget* menuRoot  = app.addStage("menu");
Widget* gameRoot  = app.addStage("game");
Widget* settingsRoot = app.addStage("settings");

// Build each stage's UI tree on its root widget...

// Switch with transition
app.setStage("game");
app.setStage("menu", TransitionType::SlideRight);
app.setStage("settings", TransitionType::SlideLeft, EaseType::OutBack);

// Set default transition
app.setTransition(TransitionType::SlideLeft, 0.4f, EaseType::OutBack);
```

**Transition types:** `None`, `SlideLeft/Right/Up/Down`, `CoverLeft/Right/Up/Down`, `RevealLeft/Right/Up/Down`, `ZoomIn/Out`

**Easing:** `Linear`, `InQuad/OutQuad/InOutQuad`, `InCubic/OutCubic/InOutCubic`, `InExpo/OutExpo/InOutExpo`, `InBack/OutBack/InOutBack`, `InBounce/OutBounce/InOutBounce`, `InElastic/OutElastic/InOutElastic`

### PageView (in-stage page switching)

For persistent UI shells (menu bar stays, content swaps):

```cpp
auto* pages = parent->createChild<PageView>();
pages->setStretch(1);
pages->setTransitionDuration(0.3f);

auto* mainPage = pages->addPage<BoxLayout>(LayoutDir::Vertical);
// ... build main content ...

auto* settingsPage = pages->addPage<BoxLayout>(LayoutDir::Vertical);
// ... build settings content ...

pages->setPage(0);                          // no animation
pages->setPage(1, PageView::SlideLeft);     // slide
pages->setPage(0, PageView::SlideAuto);     // auto-detect direction
pages->setPage(1, PageView::Fade);          // crossfade
```

---

## 8. Float Windows

Draggable, resizable floating panels above the stage:

```cpp
auto* fw = app.addFloat<FloatWindow>("Inspector");
fw->setFloatPos(100, 100);      // screen position
fw->setFloatSize(300, 250);     // window size
fw->setClosable(true);
fw->setMinimizable(true);
fw->setResizable(true);

auto* content = fw->setContent<BoxLayout>(LayoutDir::Vertical);
content->setSpacing(6);
content->setPadding(8);
content->createChild<Label>("Hello from float!");
content->createChild<Slider>(0, 100, 50);

// Toggle visibility
fw->closed.connect([fw]{ fw->setVisible(false); });
btn->clicked.connect([fw]{ fw->setVisible(!fw->isVisible()); });

// Bring to front (auto on click, or manual)
app.bringToFront(fw);

// Remove entirely
app.removeFloat(fw);  // deletes it
```

---

## 9. Popups & Dialogs

Popups render on top of everything. Only one popup at a time.

### AlertDialog (message + OK)

```cpp
auto* alert = new AlertDialog("Error", "File not found!");
WidgetApp::instance().showPopup(alert);
// Auto-closes on OK click or scrim click
```

### ConfirmDialog (Cancel + OK)

```cpp
auto* dlg = new ConfirmDialog("Delete?", "This cannot be undone.", "Delete", "Cancel");
dlg->confirmed.connect([]{ deleteFile(); });
dlg->cancelled.connect([]{ printf("cancelled\n"); });
WidgetApp::instance().showPopup(dlg);
```

### Custom Dialog

```cpp
auto* dlg = new Dialog("Settings", 400);  // title, width (height=auto)
dlg->setMessage("Choose your preferences:");  // OR use setContent:

auto* content = dlg->setContent<BoxLayout>(LayoutDir::Vertical);
content->setSpacing(8);
content->createChild<CheckBox>("Enable notifications");
content->createChild<CheckBox>("Dark mode");

dlg->addButton("Cancel", Dialog::Default);
dlg->addButton("Apply",  Dialog::Primary);
dlg->addButton("Reset",  Dialog::Danger);

dlg->buttonClicked.connect([](int idx){
    printf("Button %d clicked\n", idx);
});

WidgetApp::instance().showPopup(dlg);
```

### Manual popup

```cpp
auto* custom = new MyCustomWidget();
app.showPopup(custom, ownerWidget);  // owner not closed on click
app.closePopup();
```

---

## 10. Widget Reference

### Display widgets

| Widget | Constructor | Key methods |
|--------|------------|-------------|
| `Label` | `Label("text")` | `setText`, `setColor`, `setAlign` |
| `Spacer` | `Spacer()` or `Spacer(20)` | Use `setStretch(1)` for flex gaps |
| `Line` | `Line(1.0f)` | `setThickness`, `setColor` |
| `ProgressBar` | `ProgressBar(min, max, val)` | `setValue`, `setRange`, `setBarColor` |
| `StatusBar` | `StatusBar()` | `setText`, `setBgColor` |
| `ImageView` | `ImageView()` | `setTexture`, `setOffset`, `setAngle` |
| `Canvas` | `Canvas()` | `setBgColor`, `setOnPaint(callback)` |

### Input widgets

| Widget | Constructor | Signal |
|--------|------------|--------|
| `Button` | `Button("text")` | `clicked` |
| `IconButton` | `IconButton(Icon::Hamburger)` | `clicked` |
| `CheckBox` | `CheckBox("text")` | `toggled(bool)` |
| `RadioButton` | `RadioButton("text", &group)` | `toggled(bool)` |
| `Switch` | `Switch("text")` | `toggled(bool)` |
| `Slider` | `Slider(min, max, val)` | `onValueChanged(float)` |
| `ComboBox` | `ComboBox()` | `selectionChanged(int)` |
| `ListBox` | `ListBox()` | `selectionChanged(int)` |

### Container / Layout widgets

| Widget | Constructor | Purpose |
|--------|------------|---------|
| `Panel` | `Panel()` | Background + border box |
| `BoxLayout` | `BoxLayout(LayoutDir)` | Flex row/column |
| `GridLayout` | `GridLayout(cols)` | Grid of cells |
| `FormLayout` | `FormLayout()` | Label + widget rows |
| `BorderLayout` | `BorderLayout()` | 5-region (NSEW + Center) |
| `FlowLayout` | `FlowLayout()` | Word-wrap flow |
| `StackLayout` | `StackLayout()` | One child at a time |
| `AnchorLayout` | `AnchorLayout()` | Fractional anchors |
| `Overlay` | `Overlay()` | Z-stacked children |
| `Splitter` | `Splitter(dir)` | Draggable divider |
| `ScrollView` | `ScrollView()` | Scrollable container |
| `Collapsible` | `Collapsible("title")` | Expand/collapse section |

### View widgets

| Widget | Constructor | Purpose |
|--------|------------|---------|
| `TabLayout` | `TabLayout()` | Tab bar + content pages |
| `Carousel` | `Carousel()` | Swipeable pages with dots/arrows |
| `PageView` | `PageView()` | Animated page switching |
| `FloatWindow` | `FloatWindow("title")` | Draggable floating panel |
| `SidePanel` | `SidePanel(Side, width)` | Slide-in drawer |
| `Dialog` | `Dialog("title", w, h)` | Modal dialog (popup layer) |
| `AlertDialog` | `AlertDialog(title, msg)` | Alert + OK |
| `ConfirmDialog` | `ConfirmDialog(title, msg)` | Cancel + OK |
| `ListWidget` | `ListWidget()` | Custom widget-per-row list |

### IconButton icons

```
Hamburger  Close  ArrowLeft  ArrowRight  ArrowUp  ArrowDown
Plus  Minus  Check  Chevron
```

---

## 11. Drawing (PaintContext)

For custom painting in `Canvas::setOnPaint` or `Widget::paint` overrides:

```cpp
canvas->setOnPaint([](PaintContext& ctx, const Rect& bounds) {
    // Filled shapes
    ctx.fill.SetColor(255, 0, 0, 255);
    ctx.fill.Rectangle(x, y, w, h, true);                   // filled
    ctx.fill.RoundedRectangle(x, y, w, h, radius, segs, true);
    ctx.fill.Circle(cx, cy, radius, segments, true);
    ctx.fill.DrawImageEx(texture, x, y, w, h, angle, ox, oy, clip);

    // Lines / outlines
    ctx.line.SetColor(200, 200, 200, 255);
    ctx.line.Line2D(x1, y1, x2, y2);
    ctx.line.Rectangle(x, y, w, h, false);                  // outline
    ctx.line.RoundedRectangle(x, y, w, h, radius, segs, false);
    ctx.line.Circle(cx, cy, radius, segments, false);

    // Text
    ctx.font.SetFontSize(16.0f);
    ctx.font.SetColor(Color(220, 220, 220, 255));
    ctx.font.SetBatch(&ctx.text);
    ctx.font.Print("Hello", x, y);
    float w = ctx.font.GetTextWidth("Hello");

    // Clipping
    ctx.pushClip(Rect{x, y, w, h});
    // ... draw clipped content ...
    ctx.popClip();
});
```

### Font info

- **Noto Sans Regular** embedded (TTF)
- 377 glyphs: ASCII, Latin-1, Latin Extended-A, Punctuation, Currency, Arrows, Math, Box Drawing, Geometric, Misc Symbols
- 256×256 atlas, 2-channel (RG8 with GL swizzle)
- Default size: 16px

---

## 12. Build

```bash
# Configure (once)
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build
ninja bugui_test

# Run
../bin/bugui_test
```

**Requirements:** SDL2, OpenGL 3.3, C++17 compiler  
**Vendored:** SDL2, stb (image/truetype/rectpack), poly2tri, Noto Sans TTF  
**Debug:** AddressSanitizer enabled in Debug builds. Press **F1** to toggle layout debug overlay.
