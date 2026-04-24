# BuGUI

A lightweight, retained-mode GUI toolkit for 3D tools, built with **C++17**, **SDL2**, and **OpenGL 3.3**.

Designed for  creative applications where performance and simplicity matter.

## Features

### Core Architecture
- **Retained-mode** widget tree with automatic layout
- **3-batch rendering**: fill (triangles), line (outlines), text (font quads) — drawn in order for correct layering
- **CPU clip stack** (Sutherland-Hodgman polygon clipping) — no GL scissor needed
- **BoxLayout** with 4-pass algorithm: collect → compute main-axis → position with alignment → cross-axis
- **Signal/Slot** template system for widget events
- **Event bubbling** with consumed flag — mouse/key events walk up the parent chain

### Widgets
| Widget | Description |
|---|---|
| **Label** | Text display with alignment (left, center, right) |
| **Button** | Click-able with hover/press/focus states |
| **Panel** | Container with background and border, clips children |
| **CheckBox** | Toggle with visual check mark |
| **Slider** | Horizontal + vertical, thumb drag with fill track |
| **ProgressBar** | Non-interactive bar with optional text overlay, H/V |
| **ScrollBar** | Proportional thumb, drag + click-on-track page jump |
| **ScrollView** | Clipping container with automatic scrollbars, mouse wheel |
| **Canvas** | Custom paint callback + child clipping |
| **ImageView** | Texture display with offset and rotation |
| **Spacer** | Invisible spacing (fixed or stretch) |
| **Line** | Visual separator, auto-adapts to parent layout direction |
| **BoxLayout** | Vertical/horizontal layout with spacing, padding, alignment |

### Transitions & Easing
- **15 transition types**: Slide, Cover, Reveal (4 directions each), ZoomIn/Out, None
- **18 easing functions**: Linear, Quad, Cubic, Expo, Back, Bounce, Elastic (In/Out/InOut)
- Per-call transition + easing override

### Stage System
- Named stages with instant or animated switching
- Navigation between stages with configurable transitions

### Serialization
- Save/load widget trees to JSON via `WidgetSerializer`
- Uses `nlohmann/json` and `SDL_RWops` for I/O

### Other
- **Tags** on widgets for grouping/filtering (`addTag`, `findByTag`)
- **User data** (`std::any` key-value store per widget)
- **Global event dispatcher** (`on`, `onAny`, `fireEvent`)
- **Widget lookup** by ID (`findById<T>`) or tag (`findByTag<T>`)
- **Multi-monitor support** — choose display index at init
- **Theme** — centralized colors and sizes

## Dependencies

| Library | Version | Purpose |
|---|---|---|
| SDL2 | 2.x | Window, input, events |
| OpenGL | 3.3 core | Rendering |
| stb_truetype | — | TTF font rendering (Noto Sans Regular embedded, 621KB) |
| stb_image | — | Image loading |
| stb_rect_pack | — | Font atlas packing |
| nlohmann/json | 3.11.3 | Widget serialization |
| poly2tri | — | Polygon triangulation |
| glm | — | Matrix math (transitions) |

## Building

```bash
mkdir build && cd build
cmake .. -G Ninja
ninja bugui_test
```

The binary is output to `bin/bugui_test`.

### Requirements
- C++17 compiler (GCC 8+, Clang 7+, MSVC 2019+)
- CMake 3.10+
- SDL2 development libraries
- OpenGL 3.3+ capable GPU

## Usage

```cpp
#include "WidgetApp.hpp"
#include "Widgets.hpp"

int main()
{
    auto& app = WidgetApp::instance();

    // init(title, width, height, monitor)
    // monitor: -1 = default, 0 = primary, 1 = secondary, ...
    if (!app.init("MyApp", 1024, 768))
        return 1;

    auto* root = app.addStage("main");
    auto* layout = root->createChild<BoxLayout>(LayoutDir::Vertical);
    layout->setSpacing(8);
    layout->setPadding(16, 16, 16, 16);

    layout->createChild<Label>("Hello BuGUI!");

    auto* btn = layout->createChild<Button>("Click me");
    btn->clicked.connect([]{ printf("Clicked!\n"); });

    auto* slider = layout->createChild<Slider>(0, 100, 50);
    auto* label = layout->createChild<Label>("50");
    slider->onValueChanged.connect([label](float v) {
        char buf[16]; snprintf(buf, sizeof(buf), "%.0f", v);
        label->setText(buf);
    });

    app.setStage("main");
    return app.run();
}
```

## Project Structure

```
BuGUI/
├── core/               # Low-level: Device, Batch, Font, Texture, Shader, Input
│   ├── include/        # Public headers
│   └── src/            # Implementation
├── widgets/            # UI framework: Widget, Widgets, WidgetApp, Serializer
│   ├── include/        # Public headers
│   └── src/            # Implementation
├── vendor/             # Third-party: SDL2, stb, poly2tri, nlohmann, glm
├── app/                # Test application with demo stages
│   ├── main.cpp
│   ├── StageLayout.hpp
│   ├── StageCanvas.hpp
│   ├── StageWidgets.hpp
│   ├── StageScroll.hpp
│   └── StageCommon.hpp
└── bin/                # Build output
```

## License

MIT
