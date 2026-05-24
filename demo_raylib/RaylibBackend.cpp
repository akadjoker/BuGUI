// ─────────────────────────────────────────────────────────────────────────────
// RaylibBackend.cpp  —  BuGUI → rlgl renderer.
//
// BuGUI types (Color, Vec2f, …) are inside namespace BuGUI — no global aliases.
// Raylib types (Color, Rectangle, …) are at global scope from raylib.h.
// There is no name collision: BuGUI::Color ≠ ::Color.
// ─────────────────────────────────────────────────────────────────────────────

// BuGUI headers first — parsed before Raylib's color macros (#define RED, etc.)
#include "RaylibBackend.hpp"

// Raylib after — its macros don't affect already-parsed BuGUI headers
#include <raylib.h>
#include "rlgl.h"
#include "external/glad.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>

// Raylib key code → BuGUI key constant. Returns -1 if not mapped.
static int raylibKeyToBuGUI(int k)
{
    using namespace BuGUI::Key;
    switch (k) {
    case KEY_ENTER:     return Return;
    case KEY_KP_ENTER:  return KPEnter;
    case KEY_ESCAPE:    return Escape;
    case KEY_BACKSPACE: return Backspace;
    case KEY_TAB:       return Tab;
    case KEY_DELETE:    return Delete;
    case KEY_RIGHT:     return Right;
    case KEY_LEFT:      return Left;
    case KEY_DOWN:      return Down;
    case KEY_UP:        return Up;
    case KEY_HOME:      return Home;
    case KEY_END:       return End;
    case KEY_PAGE_UP:   return PageUp;
    case KEY_PAGE_DOWN: return PageDown;
    case KEY_SPACE:     return Space;
    case KEY_SLASH:     return Slash;
    default:
        if (k >= KEY_A && k <= KEY_Z) return k + 32;
        if (k >= 32 && k < 127)       return k;
        return -1;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
bool RaylibBackend::init()
{
    // Create and configure the BuGUI context — the caller just needs InitWindow.
    BuGUI::SetCurrentContext(BuGUI::CreateContext());
    {
        auto& io = BuGUI::GetIO();
        io.displayWidth      = static_cast<float>(GetScreenWidth());
        io.displayHeight     = static_cast<float>(GetScreenHeight());
        io.framebufferScaleX = 1.0f;
        io.framebufferScaleY = 1.0f;

        io.setClipboardText = [](const char* t) { SetClipboardText(t); };
        io.getClipboardText = []() -> std::string {
            const char* c = GetClipboardText();
            return c ? c : "";
        };
        io.readFile = [](const std::string& path) -> std::string {
            FILE* f = std::fopen(path.c_str(), "rb");
            if (!f) return "";
            std::fseek(f, 0, SEEK_END);
            long sz = std::ftell(f);
            std::rewind(f);
            if (sz <= 0) { std::fclose(f); return ""; }
            std::string buf(static_cast<size_t>(sz), '\0');
            std::fread(&buf[0], 1, buf.size(), f);
            std::fclose(f);
            return buf;
        };
        io.writeFile = [](const std::string& path, const std::string& data) -> bool {
            FILE* f = std::fopen(path.c_str(), "wb");
            if (!f) return false;
            bool ok = std::fwrite(data.c_str(), 1, data.size(), f) == data.size();
            std::fclose(f);
            return ok;
        };
    }

    createDeviceObjects();
    return shaderId_ != 0;
}

void RaylibBackend::shutdown()
{
    destroyDeviceObjects();
    BuGUI::DestroyContext(BuGUI::GetCurrentContext());
}

void RaylibBackend::newFrame()
{
    auto& io = BuGUI::GetIO();
    io.deltaTime         = GetFrameTime();
    io.displayWidth      = static_cast<float>(GetScreenWidth());
    io.displayHeight     = static_cast<float>(GetScreenHeight());
    io.framebufferScaleX = 1.0f;
    io.framebufferScaleY = 1.0f;

    static const MouseCursor cursorMap[] = {
        MOUSE_CURSOR_ARROW, MOUSE_CURSOR_POINTING_HAND, MOUSE_CURSOR_IBEAM,
        MOUSE_CURSOR_CROSSHAIR, MOUSE_CURSOR_RESIZE_EW, MOUSE_CURSOR_RESIZE_NS,
        MOUSE_CURSOR_RESIZE_ALL, MOUSE_CURSOR_NOT_ALLOWED,
    };
    int wc = io.wantedCursor;
    if (wc >= 0 && wc < 8) SetMouseCursor(cursorMap[wc]);

    // Mouse
    Vector2 mp = GetMousePosition();
    io.mouseX = mp.x;
    io.mouseY = mp.y;
    io.mouseDown[0] = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    io.mouseDown[1] = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    io.mouseDown[2] = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
    Vector2 mw = GetMouseWheelMoveV();
    io.mouseWheelX += mw.x;
    io.mouseWheelY += mw.y;

    // Modifiers
    io.keyCtrl  = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    io.keyShift = IsKeyDown(KEY_LEFT_SHIFT)   || IsKeyDown(KEY_RIGHT_SHIFT);
    io.keyAlt   = IsKeyDown(KEY_LEFT_ALT)     || IsKeyDown(KEY_RIGHT_ALT);

    // Key press/release events
    static const int kTracked[] = {
        KEY_ENTER, KEY_KP_ENTER, KEY_ESCAPE, KEY_BACKSPACE, KEY_TAB, KEY_DELETE,
        KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UP,
        KEY_HOME, KEY_END, KEY_PAGE_UP, KEY_PAGE_DOWN, KEY_SPACE,
        KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT, KEY_LEFT_CONTROL, KEY_RIGHT_CONTROL,
        KEY_LEFT_ALT, KEY_RIGHT_ALT,
        KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
        KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
        KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
        KEY_ZERO, KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR,
        KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE,
        KEY_SLASH, KEY_MINUS, KEY_EQUAL, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET,
        KEY_SEMICOLON, KEY_APOSTROPHE, KEY_COMMA, KEY_PERIOD, KEY_BACKSLASH, KEY_GRAVE,
        KEY_F1,  KEY_F2,  KEY_F3,  KEY_F4,  KEY_F5,  KEY_F6,
        KEY_F7,  KEY_F8,  KEY_F9,  KEY_F10, KEY_F11, KEY_F12,
    };
    for (int rlKey : kTracked) {
        if (rlKey < 0 || rlKey >= 512) continue;
        bool now = IsKeyDown(rlKey);
        if (now != prevKeyDown_[rlKey]) {
            int bk = raylibKeyToBuGUI(rlKey);
            if (bk >= 0)
                io.addKeyEvent(bk, rlKey, io.keyShift, io.keyCtrl, io.keyAlt, now);
        }
        prevKeyDown_[rlKey] = now;
    }

    // Text input
    int cp;
    while ((cp = GetCharPressed()) != 0)
        io.addInputCharacter(static_cast<uint32_t>(cp));
}

// ── Texture management via rlgl ───────────────────────────────────────────────

BuGUI::TextureHandle RaylibBackend::createTextureRGBA(int w, int h, const unsigned char* pixels)
{
    if (w <= 0 || h <= 0 || !pixels) return {};
    unsigned int id = rlLoadTexture(pixels, w, h,
                                    RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    if (!id) return {};
    // Switch to linear filtering (rlLoadTexture defaults to nearest).
    rlTextureParameters(id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_LINEAR);
    rlTextureParameters(id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_LINEAR);
    return BuGUI::TextureHandle{ static_cast<uintptr_t>(id) };
}

void RaylibBackend::destroyTexture(BuGUI::TextureHandle texture)
{
    if (texture) rlUnloadTexture(static_cast<unsigned int>(texture.value));
}

void RaylibBackend::render(const BuGUI::DrawData& drawData)
{
    renderDrawData(drawData);
}

// ── Device objects: custom 2D shader + VAO/VBO/EBO via rlgl ─────────────────

void RaylibBackend::createDeviceObjects()
{
    // Minimal 2D shader.
    // Vertex layout matches BuGUI::DrawVertex (stride 20 bytes):
    //   location 0 — pos    vec2 float     offset  0
    //   location 1 — uv     vec2 float     offset  8
    //   location 2 — color  vec4 ubyte/255 offset 16
    const char* vs =
        "#version 330 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "layout(location = 1) in vec2 aUV;\n"
        "layout(location = 2) in vec4 aColor;\n"
        "out vec2 vUV;\n"
        "out vec4 vColor;\n"
        "uniform mat4 mvp;\n"
        "void main() {\n"
        "    gl_Position = mvp * vec4(aPos, 0.0, 1.0);\n"
        "    vUV    = aUV;\n"
        "    vColor = aColor;\n"
        "}\n";

    const char* fs =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "in vec4 vColor;\n"
        "out vec4 outColor;\n"
        "uniform sampler2D tex0;\n"
        "void main() {\n"
        "    outColor = texture(tex0, vUV) * vColor;\n"
        "}\n";

    shaderId_ = rlLoadShaderCode(vs, fs);
    mvpLoc_   = rlGetLocationUniform(shaderId_, "mvp");
    tex0Loc_  = rlGetLocationUniform(shaderId_, "tex0");

    // Pre-allocate large enough buffers; typical UIs stay well under these limits.
    vboCapacity_ = 65536;
    eboCapacity_ = 262144;

    const int vboBytes = vboCapacity_ * (int)sizeof(BuGUI::DrawVertex);
    const int eboBytes = eboCapacity_ * (int)sizeof(uint32_t);

    // Create VAO — stores vertex attribute configuration.
    vaoId_ = rlLoadVertexArray();
    rlEnableVertexArray(vaoId_);

    // VBO (dynamic, initially uninitialized).
    vboId_ = rlLoadVertexBuffer(nullptr, vboBytes, true);
    rlEnableVertexBuffer(vboId_);

    const int stride = (int)sizeof(BuGUI::DrawVertex);
    rlSetVertexAttribute(0, 2, RL_FLOAT,         false, stride,  0);
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 2, RL_FLOAT,         false, stride,  8);
    rlEnableVertexAttribute(1);
    rlSetVertexAttribute(2, 4, RL_UNSIGNED_BYTE, true,  stride, 16);
    rlEnableVertexAttribute(2);

    // EBO — bound inside the VAO so glDrawElements uses it automatically.
    eboId_ = rlLoadVertexBufferElement(nullptr, eboBytes, true);

    // Unbind in reverse order; VAO must be unbound first to preserve its EBO ref.
    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableVertexBufferElement();
}

void RaylibBackend::destroyDeviceObjects()
{
    if (vaoId_)    { rlUnloadVertexArray(vaoId_);      vaoId_    = 0; }
    if (vboId_)    { rlUnloadVertexBuffer(vboId_);     vboId_    = 0; }
    if (eboId_)    { rlUnloadVertexBuffer(eboId_);     eboId_    = 0; }
    if (shaderId_) { rlUnloadShaderProgram(shaderId_); shaderId_ = 0; }
}

// ── Render ───────────────────────────────────────────────────────────────────

void RaylibBackend::renderDrawData(const BuGUI::DrawData& drawData)
{
    if (drawData.passes.empty()) return;
    const float W = drawData.displayWidth;
    const float H = drawData.displayHeight;
    if (W <= 0.0f || H <= 0.0f) return;

    auto& stats = const_cast<BuGUI::DrawData&>(drawData).stats;
    stats.reset();

    // Flush any pending rlgl batch (from ClearBackground etc.) before we take
    // over GL state.  BeginDrawing() already enables blending (RL_BLEND_ALPHA).
    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    rlDisableBackfaceCulling();

    // Orthographic MVP: Y-down, origin top-left.
    Matrix mvp = MatrixOrtho(0.0, (double)W, (double)H, 0.0, 0.0, 1.0);
    const int fbH = GetRenderHeight();  // physical framebuffer height for Y-flip

    rlEnableShader(shaderId_);
    rlSetUniformMatrix(mvpLoc_, mvp);
    int texUnit = 0;
    rlSetUniform(tex0Loc_, &texUnit, RL_SHADER_UNIFORM_INT, 1);

    for (const BuGUI::DrawPass& pass : drawData.passes)
    {
        if (!pass.list) continue;
        const auto& verts = pass.list->vertices();
        const auto& idxs  = pass.list->indices();
        const auto& cmds  = pass.list->commands();
        if (verts.empty() || idxs.empty() || cmds.empty()) continue;

        // Camera transform (same math as SDL backend — done on CPU).
        const float ca  = pass.camera.angle;
        const float sc  = pass.camera.scale;
        const float pvX = pass.camera.pivotX * W;
        const float pvY = pass.camera.pivotY * H;
        const float cc  = std::cos(ca) * sc;
        const float ss  = std::sin(ca) * sc;
        const float ox  = pass.camera.x - pvX;
        const float oy  = pass.camera.y - pvY;
        const float ttx = cc * ox - ss * oy + pvX;
        const float tty = ss * ox + cc * oy + pvY;
        const bool  noRot = (ca == 0.0f);

        // Transform vertices into screen space and upload to VBO.
        xVerts_.resize(verts.size());
        for (size_t i = 0; i < verts.size(); ++i)
        {
            const auto& v = verts[i];
            xVerts_[i]   = v;
            xVerts_[i].x = cc * v.x - ss * v.y + ttx;
            xVerts_[i].y = ss * v.x + cc * v.y + tty;
        }

        rlUpdateVertexBuffer(vboId_,
                             xVerts_.data(),
                             (int)(xVerts_.size() * sizeof(BuGUI::DrawVertex)), 0);
        rlUpdateVertexBufferElements(eboId_,
                                     idxs.data(),
                                     (int)(idxs.size() * sizeof(uint32_t)), 0);

        // Bind VAO — restores vertex attributes + EBO binding.
        // DO NOT call Begin/EndScissorMode inside this loop — they call
        // rlDrawRenderBatchActive() which would rebind rlgl's own VAO, killing
        // our draw calls.  Use rlScissor directly instead.
        rlEnableVertexArray(vaoId_);
        rlEnableScissorTest();

        for (const BuGUI::DrawCmd& cmd : cmds)
        {
            float cx0, cy0, cx1, cy1;
            if (noRot)
            {
                cx0 = cc * cmd.clip.x                + ttx;
                cy0 = cc * cmd.clip.y                + tty;
                cx1 = cc * (cmd.clip.x + cmd.clip.w) + ttx;
                cy1 = cc * (cmd.clip.y + cmd.clip.h) + tty;
                if (cx0 > cx1) std::swap(cx0, cx1);
                if (cy0 > cy1) std::swap(cy0, cy1);
            }
            else
            {
                cx0 = 0.0f; cy0 = 0.0f;
                cx1 = W;    cy1 = H;
            }

            int sx = static_cast<int>(std::max(0.0f, cx0));
            int sy = static_cast<int>(std::max(0.0f, cy0));
            int sw = static_cast<int>(std::min(W, cx1) - sx);
            int sh = static_cast<int>(std::min(H, cy1) - sy);
            if (sw <= 0 || sh <= 0) continue;

            // OpenGL scissor Y is from bottom; BuGUI uses Y from top → flip.
            rlScissor(sx, fbH - (sy + sh), sw, sh);

            unsigned int texId = cmd.texture
                ? static_cast<unsigned int>(cmd.texture.value)
                : rlGetTextureIdDefault();
            rlActiveTextureSlot(0);
            rlEnableTexture(texId);

            glDrawElements(GL_TRIANGLES,
                           static_cast<int>(cmd.indexCount),
                           GL_UNSIGNED_INT,
                           reinterpret_cast<const void*>(
                               static_cast<uintptr_t>(cmd.indexOffset * sizeof(uint32_t))));

            rlDisableTexture();

            stats.drawCalls++;
            stats.triangles += static_cast<int>(cmd.indexCount) / 3;
        }

        rlDisableScissorTest();
        rlDisableVertexArray();

        stats.vertices    += static_cast<int>(verts.size());
        stats.clipChanges += pass.list->pushClipCount();
    }

    rlDisableShader();
    rlEnableDepthTest();
}
