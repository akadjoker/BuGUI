# BuGUI

A **retained-mode**, backend-agnostic C++17 GUI toolkit. Widgets generate abstract `DrawData` — the host application picks a platform backend (SDL2, GLFW, raylib, custom engine) to feed input and render the output.

```
Your App / Engine
       │
       ▼
  Platform Backend          ◄── SDL2, GLFW, raylib, …
  (input + window)
       │
       ▼
  BuGUI Core                ◄── widgets, layout, focus, paint
  (no platform deps)
       │
       ▼
  Render Backend            ◄── OpenGL, Vulkan, SDL_Renderer, …
  (draws DrawData)
```

## Features

- **60+ widget classes** — from `Label` and `Button` to `NodeEditor`, `DockPanel`, `Timeline`, and `PianoRoll`
- **Retained-mode** — build the widget tree once, update properties; no per-frame widget calls
- **Backend-agnostic** — the `widgets/` library has zero SDL/GL/platform includes
- **DrawData pipeline** — widgets paint into `DrawList` command buffers; backends submit them to GPU
- **Layout system** — `BoxLayout`, `GridLayout`, `BorderLayout`, `AnchorLayout`, `FlowLayout`, `FormLayout`, `Splitter`, `DockPanel`
- **Animation** — tweening engine with 20 ease types, delays, loops, groups (parallel/sequential)
- **Model/View** — `AbstractItemModel`, `ListModel`, `TreeModel` for `DataGrid` and `TreeView`
- **Theme** — dark/light presets with 70+ color tokens and 20+ size fields
- **Signal/Slot** — type-safe `Signal<Args...>` for decoupled widget communication
- **Stage system** — multi-screen apps with animated transitions (15 types × 20 easings)
- **Float windows** — draggable/resizable panels with minimize, stack order
- **Drag & drop** — widget-to-widget and OS file drops

## Requirements

- **C++17** compiler (Clang, GCC, MSVC)
- **CMake 3.10+**
- **SDL2** (for the included demo/test backends)
- **OpenGL 3.3** (for the included render backend)

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

Executables go to `bin/`. The default build produces `bugui_demo` (backend IO test) and `bugui_demos` (widget showcase with 30+ stages).

### CMake options

| Option | Default | Description |
|---|---|---|
| `BUGUI_BUILD_APP` | ON | Main application |
| `BUGUI_BUILD_DEMO` | ON | Backend IO/events demo |
| `BUGUI_BUILD_EDITOR` | ON | Editor executable |
| `BUGUI_BUILD_TUTORIALS` | OFF | Tutorial examples |

## Project Structure

```
widgets/            Core library (zero platform deps)
  include/          Public headers
  src/              Implementation
vendor/             Third-party (poly2tri, stb, glad, nlohmann)
demo/               Minimal backend demo (SDL2 + OpenGL)
test1/              Widget showcase app
  main.cpp          App entry, stage registration
  backends/         SdlOpenGLBackend (reference implementation)
  stages/           One file per demo stage (~30 stages)
```

## Architecture

### The Widget Tree

All UI is a tree of `Widget` objects. The root is created by `WidgetApp::addStage()`. You build the tree with `createChild<T>(args...)`:

```cpp
void registerMyStage(WidgetApp& app) {
    Widget* root = app.addStage("my-stage");

    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(8);
    vbox->setPadding(12);

    vbox->createChild<Label>("Hello, BuGUI!");

    auto* btn = vbox->createChild<Button>("Click me");
    btn->clicked.connect([]() {
        Toast::show("Clicked!", Toast::Type::Info);
    });
}
```

### Backend Integration

A backend implements two responsibilities:

1. **Platform input** — poll OS events and fill `BuGUI::IO` (mouse, keyboard, text, clipboard, display size, delta time)
2. **Rendering** — iterate `DrawData` passes and submit vertex/index buffers to GPU

```cpp
// Main loop (simplified)
SdlOpenGLBackend backend;
backend.init("My App", 1280, 720);

WidgetApp& app = WidgetApp::instance();

while (backend.beginFrame()) {
    auto& io = BuGUI::GetIO();
    app.update(io);
    app.paint(*BuGUI::GetDrawData(), font, &iconAtlas);
    BuGUI::Render();
    backend.render(*BuGUI::GetDrawData());
    backend.present();
}
```

### IO Struct

The backend fills this every frame before `app.update()`:

```cpp
BuGUI::IO& io = BuGUI::GetIO();
io.deltaTime       = 1.0f / 60.0f;
io.displayWidth    = 1280.0f;
io.displayHeight   = 720.0f;
io.mouseX          = mouseX;
io.mouseY          = mouseY;
io.mouseDown[0]    = leftPressed;
io.keysDown[key]   = pressed;
io.keyCtrl         = ctrlHeld;
io.keyShift        = shiftHeld;
io.keyAlt          = altHeld;
io.addInputCharacter(codepoint);
io.setClipboardText = [](const char* t) { /* OS clipboard */ };
io.getClipboardText = []() -> std::string { /* OS clipboard */ };
```

### Key Constants

Platform-agnostic key codes in `BuGUI::Key`:

```cpp
BuGUI::Key::Return, Escape, Backspace, Tab, Delete, Space
BuGUI::Key::Left, Right, Up, Down, Home, End, PageUp, PageDown
BuGUI::Key::F1 .. F12, KPEnter
BuGUI::Key::A .. Z   // lowercase ASCII
```

### DrawData Pipeline

```
Widget::paint()  →  PaintContext  →  DrawList  →  DrawData
                                      │
                                      ├─ vertices (pos, uv, color)
                                      ├─ indices
                                      └─ commands (texture, clip rect, count)
```

`DrawData` contains `vector<DrawPass>`, each with a `DrawList*` and a `Camera2D`. The backend iterates passes and submits GL draw calls.

## Widget Catalogue

### Basic

| Widget | Description |
|---|---|
| `Label` | Static text with color and alignment |
| `Button` | Clickable button with text |
| `IconButton` | Built-in icon with optional animation |
| `ImageButton` | Atlas-textured button |
| `CheckBox` | Toggle with text label |
| `RadioButton` / `RadioGroup` | Mutual-exclusion radio options |
| `Switch` | On/off toggle |
| `Panel` | Container with background color |
| `Line` | Visual separator |
| `Spacer` | Invisible spacing |
| `Toolbar` | Horizontal button/icon bar |

### Layout

| Widget | Description |
|---|---|
| `BoxLayout` | H/V box with spacing, padding, alignment |
| `GridLayout` | Uniform N-column grid |
| `BorderLayout` | 5-region (Top/Bottom/Left/Right/Center) |
| `FlowLayout` | Wrapping flex layout |
| `FormLayout` | Two-column label:widget pairs |
| `AnchorLayout` | Anchor-based positioning |
| `Splitter` | Two panels with draggable divider |
| `TabLayout` | Tabbed container with closable tabs |
| `StackLayout` | One visible child at a time |
| `Collapsible` | Expandable/collapsible section |
| `Overlay` | Z-ordered full-area stack |
| `Carousel` | Animated page viewer with auto-play |
| `SlidePanel` | Drawer from left/right edge |
| `StatusBar` | Bottom bar |
| `DockPanel` | Docking layout with split/tab/float |

### Input

| Widget | Description |
|---|---|
| `TextInput` | Single-line text (normal, password, number) |
| `TextEdit` | Multi-line editor with line numbers, word wrap, syntax callback, gutter markers |
| `Slider` | Draggable value with H/V orientation |
| `SpinBox` | Numeric ± with drag, decimals, prefix/suffix |
| `ProgressBar` | Non-interactive bar |
| `ComboBox` | Dropdown selector |
| `DatePicker` | Calendar popup |
| `TimePicker` | Hour:min:sec spinners |
| `ListBox` | String list with selection |
| `ListWidget` | Widget-per-row list |

### Data

| Widget | Description |
|---|---|
| `DataGrid` | Spreadsheet table — columns, rows, sort, multi-select, checkboxes, edit, model |
| `TreeGrid` | Hierarchical table with expand/collapse |
| `TreeView` | Tree hierarchy with icons and model support |
| `PropertyGrid` | Inspector panel — sections, string/float/int/bool/color/combo/vec2-4/button/range |
| `ColorPicker` | HSV wheel + bars with alpha |

### Charts

| Widget | Description |
|---|---|
| `PlotWidget` | Line/bar/scatter chart with pan, zoom, legend |
| `HistogramWidget` | Value distribution with bin count and log scale |
| `GradientEditor` | Draggable color stops |
| `CurveEditor` | Bezier animation curves with keyframes |

### Dialog

| Widget | Description |
|---|---|
| `Dialog` | Modal overlay with role-based buttons |
| `AlertDialog` | Quick alert popup |
| `ConfirmDialog` | Confirm/cancel popup |
| `MessageBox` | Modal with Ok/Cancel/Yes/No |
| `InputBox` | Text input dialog |
| `Toast` | Brief notification (Info/Success/Warning/Error) |
| `FileDialog` | File/folder picker with bookmarks, filters, view modes |

### Specialty

| Widget | Description |
|---|---|
| `FloatWindow` | Draggable/resizable floating panel |
| `Canvas` | Custom paint callback |
| `ImageView` | Texture display |
| `PageView` | Page container with fade/slide transitions |
| `NodeEditor` | Visual node graph with typed pins and links |
| `Timeline` | Multi-track timeline with keyframes and clips |
| `ConsoleWidget` | Log viewer with filter, search, command input |
| `AssetBrowser` | Thumbnail asset browser with folder navigation |
| `ThumbnailGrid` | Responsive image/color grid |
| `Gizmo2D` | 2D translate/rotate/scale handles |
| `Gizmo3D` | 3D projected transform handles |

### Audio

| Widget | Description |
|---|---|
| `Knob` | Rotary 270° arc knob |
| `LinearKnob` | H/V slider with thumb |
| `ADSRWidget` | ADSR envelope editor |
| `VUMeter` | Multi-channel volume meter |
| `SpectrumAnalyzer` | Frequency spectrum bars |
| `WaveformView` | Audio waveform display with scrub |
| `PianoRoll` | MIDI note grid editor |

### Automotive

| Widget | Description |
|---|---|
| `RadialGauge` | Arc-style gauge (speedometer) |
| `PowerBar` | Center-zero bar gauge |
| `DigitalSpeed` | Large numeric speed display |
| `BatteryGauge` | Battery bar with charge state |
| `InfoTile` | Icon + value + suffix status tile |
| `DriveMode` | Mode selector (Normal/Sport/Eco) |

## Signals

Type-safe observer pattern:

```cpp
Signal<float> valueChanged;

// Connect
auto id = valueChanged.connect([](float v) {
    printf("New value: %f\n", v);
});

// Emit
valueChanged.emit(42.0f);

// Disconnect
valueChanged.disconnect(id);
```

## Theming

```cpp
auto& theme = Theme::instance();
theme.dark();   // dark preset
theme.light();  // light preset

// Customize
theme.buttonBg      = Color(60, 65, 75, 255);
theme.accentColor   = Color(100, 150, 255, 255);
theme.fontSize       = 14.0f;
```

## Animation

```cpp
auto& anim = Animator::instance();

// Simple tween
anim.animate(widget, "opacity", 0.0f, 1.0f, 300, EaseType::OutCubic);

// Convenience
anim.fadeIn(widget, 200);
anim.slideIn(widget, SlideDir::Left, 400, EaseType::OutBack);
```

## Model/View

For large datasets, use a model instead of adding rows manually:

```cpp
auto* model = new ListModel();
model->setHeaders({"Name", "Age", "Country"});
model->appendRow({"Alice", "30", "Portugal"});
model->appendRow({"Bob", "25", "Brazil"});
// ... thousands of rows ...

auto* grid = parent->createChild<DataGrid>();
grid->setModel(model);
```

## License

See [LICENSE](LICENSE) for details.
