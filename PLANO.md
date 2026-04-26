# Plano de Refatoracao BuGUI

## Objetivo

Separar os widgets de SDL, OpenGL, `RenderBatch`, `Texture` e `Device`, mantendo a BuGUI como uma UI retained-mode.

A direcao e ficar perto do modelo do ImGui:

- O core nao cria janela.
- O core nao inicializa renderer.
- O core nao sabe se esta a desenhar com OpenGL, raylib, SDL_Renderer, DirectX ou uma engine propria.
- Os widgets geram `DrawData`.
- Backends opcionais alimentam input e desenham o `DrawData`.

## Arquitetura Alvo

```txt
App / Engine / Demo
        |
        v
Platform backend opcional
  SDL2, GLFW, raylib, engine propria
  - mouse
  - teclado
  - texto
  - clipboard
  - cursor
  - delta time
        |
        v
BuGUI core
  - Context
  - IO
  - Widget tree
  - Layout
  - Focus / hover / capture
  - PaintContext
  - DrawList / DrawData
        |
        v
Render backend opcional
  OpenGL, SDL_Renderer, raylib, DirectX, engine propria
```

## Principios

1. `WidgetApp` passa a ser helper/demo runner, nao a base obrigatoria da biblioteca.
2. Widgets nao devem incluir `Batch.hpp`, `Device.hpp`, `Opengl.hpp` ou `Texture.hpp`.
3. Widgets nao devem chamar GL, SDL ou APIs de renderer diretamente.
4. `PaintContext` deve expor comandos de alto nivel, nao batches internos.
5. Rendering deve respeitar ordem visual.
6. Otimizacao de draw calls fica dentro do backend renderer.

## API Core Estilo ImGui

Exemplo de API desejada:

```cpp
namespace BuGUI {

struct IO {
    float deltaTime = 1.0f / 60.0f;
    float displayWidth = 0.0f;
    float displayHeight = 0.0f;

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    bool mouseDown[5] = {};

    bool keysDown[512] = {};
    bool keyCtrl = false;
    bool keyShift = false;
    bool keyAlt = false;

    void addInputCharacter(unsigned int c);
};

struct Context;

Context* CreateContext();
void DestroyContext(Context* ctx);
void SetCurrentContext(Context* ctx);
Context* GetCurrentContext();

IO& GetIO();

void NewFrame();
void Render();
DrawData* GetDrawData();

}
```

Uso dentro de uma app externa:

```cpp
auto& io = BuGUI::GetIO();
io.displayWidth = engine.width();
io.displayHeight = engine.height();
io.deltaTime = engine.deltaTime();
io.mouseX = engine.input.mouseX();
io.mouseY = engine.input.mouseY();

BuGUI::NewFrame();

root->setRect({0, 0, io.displayWidth, io.displayHeight});
root->layout();
root->paint(ctx);

BuGUI::Render();
MyEngine_RenderDrawData(BuGUI::GetDrawData());
```

## Backends Opcionais

Backends devem ser ficheiros/adapters separados, nao classes virtuais obrigatorias.

Exemplos:

```cpp
BuGUI_ImplSDL2_ProcessEvent(&event);
BuGUI_ImplOpenGL3_RenderDrawData(BuGUI::GetDrawData());
```

```cpp
BuGUI_ImplRaylib_NewFrame();
BuGUI_ImplRaylib_RenderDrawData(BuGUI::GetDrawData());
```

Possiveis backends:

- `bugui_impl_sdl2`
- `bugui_impl_glfw`
- `bugui_impl_opengl3`
- `bugui_impl_sdlrenderer`
- `bugui_impl_raylib`
- `bugui_impl_dx11`
- backend custom de uma engine

## TextureHandle

Texturas devem ser handles opacos. O core nao interpreta o valor.

```cpp
struct TextureHandle {
    uintptr_t value = 0;

    explicit operator bool() const { return value != 0; }
};
```

Mapeamento por backend:

```txt
OpenGL       -> GLuint guardado em value
SDL_Renderer -> SDL_Texture* reinterpretado para uintptr_t
raylib       -> Texture2D* ou id conforme o backend escolher
DirectX      -> ID3D11ShaderResourceView*
Engine       -> id interno da engine
```

Regra: widgets podem guardar e passar `TextureHandle`, mas nunca podem chamar APIs especificas do backend.

## DrawList e DrawData

O modelo atual de 3 passes globais:

```txt
todos os fills -> todas as linhas -> todos os textos/images
```

reduz draw calls, mas quebra a ordem visual.

O novo modelo deve ser:

```txt
DrawList ordenado
```

Cada widget emite comandos na ordem em que quer desenhar:

```cpp
ctx.fillRect(bg, bgColor);
ctx.line(x1, y1, x2, y2, gridColor);
ctx.text(label, x, y, textColor);
ctx.image(icon, dst);
```

Internamente, o `DrawList` guarda arrays lineares:

```cpp
struct DrawVertex {
    float x, y;
    float u, v;
    uint32_t color;
};

struct DrawCmd {
    TextureHandle texture;
    Rect clip;
    uint32_t indexOffset;
    uint32_t indexCount;
};

struct DrawList {
    std::vector<DrawVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<DrawCmd> commands;
};
```

O backend pode agrupar comandos consecutivos compativeis, mas nunca pode mudar a ordem visual.

## PaintContext Novo

`PaintContext` nao deve expor isto:

```cpp
RenderBatch& fill;
RenderBatch& line;
RenderBatch& text;
Font& font;
```

Deve expor comandos UI-level:

```cpp
struct PaintContext {
    DrawList& draw;
    UIContext& ui;

    void pushClip(const Rect& r);
    void popClip();
    bool isClipped(const Rect& r) const;

    void fillRect(const Rect& r, const Color& c);
    void lineRect(const Rect& r, const Color& c, float thickness = 1.0f);
    void roundedRect(const Rect& r, float radius, const Color& c);
    void roundedRectOutline(const Rect& r, float radius, const Color& c, float thickness = 1.0f);
    void line(float x1, float y1, float x2, float y2, const Color& c, float thickness = 1.0f);
    void triangle(...);
    void text(const char* s, float x, float y, const Color& c);
    void image(TextureHandle tex, const Rect& dst, const Rect& uv, const Color& tint = Color(255, 255, 255, 255));
};
```

Durante a migracao, este contexto pode ser um adapter por cima do `RenderBatch` atual. Depois troca para `DrawList`.

## UIContext e Input

Input nao deve viver dentro do `PaintContext`.

Separar servicos de UI:

```cpp
struct UIContext {
    void requestFocus(Widget* w);
    void captureMouse(Widget* w);
    void releaseMouse(Widget* w);
    void setCursor(CursorType cursor);

    std::string clipboardText() const;
    void setClipboardText(const std::string& text);

    const IO& io() const;
};
```

Eventos entram no core atraves de `IO` ou helpers de backend. A arvore de widgets consome eventos ja normalizados.

Fluxo:

```txt
SDL / raylib / engine
        |
        v
Backend platform atualiza BuGUI::IO
        |
        v
BuGUI::NewFrame()
        |
        v
Widget tree processa input
        |
        v
Widgets recebem MouseEvent / KeyEvent / TextInput
```

## WidgetApp

`WidgetApp` continua util, mas deve virar opcional.

Uso futuro:

- demos
- exemplos rapidos
- runner default SDL2 + OpenGL

Nao deve ser necessario usar `WidgetApp` dentro de raylib, DirectX, SDL_Renderer ou outra engine.

## Fase 1: Quarentena dos Widgets Pesados

Adicionar opcao no CMake:

```cmake
option(BUGUI_BUILD_EXTRA_WIDGETS "Build experimental/heavy widgets" OFF)
```

Core inicial:

```txt
Widget.cpp
BasicWidgets.cpp
InputWidgets.cpp
TextEdit.cpp
ScrollWidgets.cpp
LayoutWidgets.cpp
MenuWidgets.cpp
Dialogs.cpp
IconAtlas.cpp, se ainda for necessario
WidgetApp.cpp, como runner legado
WidgetSerializer.cpp, se nao bloquear
```

Extras temporarios:

```txt
ViewportWidget.cpp
Gizmo2D.cpp
Gizmo3D.cpp
Timeline.cpp
NodeEditor.cpp
CurveEditor.cpp
VUMeter.cpp
WaveformView.cpp
SpectrumAnalyzer.cpp
ADSRWidget.cpp
PianoRoll.cpp
PlotWidget.cpp
HistogramWidget.cpp
AssetBrowser.cpp
FileDialog.cpp
DockPanel.cpp
```

Criar dois umbrella headers:

```txt
Widgets.hpp       -> core/stable
WidgetsExtra.hpp  -> extras
```

## Fase 2: Criar DrawList

Criar tipos base:

```txt
DrawVertex
DrawCmd
DrawList
DrawData
TextureHandle
FontHandle
```

Primeira versao pode suportar apenas:

- clip rect
- rect fill
- rect outline
- line
- triangle
- text
- image

## Fase 3: Adapter Temporario

Antes de trocar tudo, criar um adapter que preserve o renderer atual:

```txt
PaintContext novo -> RenderBatch antigo
```

Isto permite converter widgets um por um sem mudar logo o backend OpenGL.

## Fase 4: Converter Widgets Core

Ordem recomendada:

1. `BasicWidgets.cpp`
2. `LayoutWidgets.cpp`
3. `InputWidgets.cpp`
4. `ScrollWidgets.cpp`
5. `TextEdit.cpp`
6. `MenuWidgets.cpp`
7. `Dialogs.cpp`

Trocas desejadas:

```cpp
ctx.fill.SetColor(...);
ctx.fill.Rectangle(...);
ctx.line.Line2D(...);
ctx.font.SetBatch(&ctx.text);
ctx.font.Print(...);
```

para:

```cpp
ctx.fillRect(...);
ctx.line(...);
ctx.text(...);
```

## Fase 5: Renderer por DrawData

Criar renderer OpenGL que consome `DrawData`.

Depois disso, `RenderBatch` deixa de ser dependencia dos widgets e pode ficar como:

- backend legado
- helper interno
- ou ser removido gradualmente

## Fase 6: Platform Backends

Criar helpers opcionais para alimentar `BuGUI::IO`.

Primeiro backend:

```txt
bugui_impl_sdl2
```

Depois:

```txt
bugui_impl_raylib
bugui_impl_sdlrenderer
bugui_impl_glfw
```

## Fase 7: Voltar aos Extras

Depois do core estar limpo, reativar extras um por um:

1. `Timeline`
2. `PlotWidget`
3. `HistogramWidget`
4. `VUMeter`
5. `WaveformView`
6. `SpectrumAnalyzer`
7. `ADSRWidget`
8. `PianoRoll`
9. `NodeEditor`
10. `CurveEditor`
11. `Gizmo2D`
12. `Gizmo3D`
13. `ViewportWidget`
14. `AssetBrowser`
15. `FileDialog`
16. `DockPanel`

Widgets com textura, viewport ou GL direto ficam para o fim.

## Criterio de Sucesso

No final, o core de widgets nao deve depender de:

```cpp
#include "Batch.hpp"
#include "Device.hpp"
#include "Opengl.hpp"
#include "Texture.hpp"
```

Backends podem depender dessas coisas. Widgets nao.

O uso final deve permitir:

```cpp
BuGUI::CreateContext();

while (running) {
    FeedBuGUIInputFromMyEngine();

    BuGUI::NewFrame();
    root->layout();
    root->paint(ctx);
    BuGUI::Render();

    MyRenderer_DrawBuGUI(BuGUI::GetDrawData());
}
```

Isto permite usar BuGUI com OpenGL, raylib, SDL_Renderer, DirectX ou outra engine sem prender os widgets a uma tecnologia especifica.
