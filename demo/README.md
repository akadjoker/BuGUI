# BuGUI Demo

This folder is the SDL/OpenGL demo runner.

For now the backend owns the legacy SDL/OpenGL runner sources (`WidgetApp.cpp`
and `UIApp.cpp`). They are intentionally compiled here, not inside
`bugui_widgets`, so the widget library can be cleaned independently.

Later, this folder should evolve toward the ImGui-style backend split:

```txt
demo/backends/SdlPlatformBackend
demo/backends/OpenGLRenderBackend
```

and the demo should call:

```txt
BuGUI::NewFrame()
root->layout()
root->paint()
BuGUI::Render()
Backend_RenderDrawData()
```

Build:

```sh
cmake -S . -B build -DBUGUI_BUILD_DEMO=ON
cmake --build build --target bugui_demo
```
