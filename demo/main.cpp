#include "SdlOpenGLBackend.hpp"
#include "BuGUI.hpp"
#include <cstdio>
#include <string>
#include <vector>
#include <cstring>

// ─────────────────────────────────────────────────────────────────────────────
//  Tiny event log — keeps last N lines
// ─────────────────────────────────────────────────────────────────────────────
static std::vector<std::string> g_log;

static void logEvent(const char* msg)
{
    g_log.push_back(msg);
    if (g_log.size() > 32)
        g_log.erase(g_log.begin());
    std::printf("[event] %s\n", msg);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Draw a simple filled bar (progress / indicator)
// ─────────────────────────────────────────────────────────────────────────────
static void drawBar(BuGUI::DrawList& dl, float x, float y, float w, float h,
                    float t, Color fill, Color bg)
{
    dl.addRoundRectFilled({x, y, w, h}, 4.0f, bg);
    if (t > 0.01f)
        dl.addRoundRectFilled({x, y, w * t, h}, 4.0f, fill);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Draw event log lines from bottom up
// ─────────────────────────────────────────────────────────────────────────────
static void drawLog(BuGUI::DrawList& dl, const BuGUI::Font& font,
                    float x, float y, float w, float h)
{
    float lh = font.lineHeight() + 2.0f;
    float fy = y + h - lh;
    for (int i = static_cast<int>(g_log.size()) - 1; i >= 0 && fy >= y; --i, fy -= lh)
    {
        int age   = static_cast<int>(g_log.size()) - 1 - i;
        uint8_t a = static_cast<uint8_t>(std::max(30, 220 - age * 25));
        dl.addText(font, {x + 4.0f, fy}, Color(190, 205, 230, a), g_log[i].c_str());
    }
}

int main()
{
    SdlOpenGLBackend backend;
    backend.setBackgroundColor(Color(20, 22, 28, 255));

    if (!backend.init("BuGUI – IO & Events Test", 960, 640))
    {
        std::fprintf(stderr, "backend init failed\n");
        return 1;
    }

    // ── Font ──────────────────────────────────────────────────────────────────
    BuGUI::FontAtlas fontAtlas;
    bool fontReady = fontAtlas.buildDefault();
    if (fontReady)
    {
        fontAtlas.setTexture(backend.createTextureRGBA(fontAtlas.width(),
                                                       fontAtlas.height(),
                                                       fontAtlas.pixels()));
        fontReady = static_cast<bool>(fontAtlas.texture());
        if (fontReady)
            BuGUI::SetWhitePixel(fontAtlas.texture(), fontAtlas.whitePixelUV());
    }
    if (!fontReady)
        std::fprintf(stderr, "font atlas failed, continuing without text\n");

    const BuGUI::Font* font = fontReady ? &fontAtlas.defaultFont() : nullptr;

    // ── State ─────────────────────────────────────────────────────────────────
    bool  prevMouseDown[5] = {};
    float wheelAccum = 0.0f;   // decaying accumulator for wheel bar display

    logEvent("Demo started – move mouse, click, type, press keys");

    while (backend.beginFrame())
    {
        auto& io = BuGUI::GetIO();
        auto& dl = BuGUI::GetDrawList();
        float W  = io.displayWidth;
        float H  = io.displayHeight;

        // ── Log mouse button events ───────────────────────────────────────────
        for (int i = 0; i < 3; ++i)
        {
            if (io.mouseDown[i] && !prevMouseDown[i]) {
                char buf[48]; snprintf(buf, sizeof(buf), "Mouse BTN%d press  (%.0f, %.0f)", i, io.mouseX, io.mouseY);
                logEvent(buf);
            } else if (!io.mouseDown[i] && prevMouseDown[i]) {
                char buf[48]; snprintf(buf, sizeof(buf), "Mouse BTN%d release", i);
                logEvent(buf);
            }
            prevMouseDown[i] = io.mouseDown[i];
        }

        // ── Log mouse wheel ───────────────────────────────────────────────────
        if (io.mouseWheelY != 0.0f) {
            char buf[48]; snprintf(buf, sizeof(buf), "Wheel Y = %.1f", io.mouseWheelY);
            logEvent(buf);
        }

        // ── Log key events ────────────────────────────────────────────────────
        for (const auto& ke : io.keyEvents)
        {
            char buf[80];
            snprintf(buf, sizeof(buf), "Key %s  sym=%d scan=%d%s%s%s",
                     ke.down ? "DOWN" : "UP  ",
                     ke.key, ke.scancode,
                     ke.ctrl  ? " Ctrl"  : "",
                     ke.shift ? " Shift" : "",
                     ke.alt   ? " Alt"   : "");
            logEvent(buf);
        }

        // ── Log text input ────────────────────────────────────────────────────
        if (!io.inputChars.empty()) {
            // Convert first codepoint to rough UTF-8 display
            uint32_t cp = io.inputChars[0];
            char buf[48];
            if (cp < 128)
                snprintf(buf, sizeof(buf), "TextInput: '%c'", static_cast<char>(cp));
            else
                snprintf(buf, sizeof(buf), "TextInput: U+%04X", cp);
            logEvent(buf);
        }

        // ══════════════════════════════════════════════════════════════════════
        //  Draw UI
        // ══════════════════════════════════════════════════════════════════════
        constexpr float PAD = 16.0f;
        float panelW = W * 0.45f - PAD * 1.5f;
        float leftX  = PAD;
        float rightX = W * 0.45f + PAD * 0.5f;

        // ── Left panel: IO state ───────────────────────────────────────────────
        dl.addRoundRectFilled({leftX, PAD, panelW, H - PAD * 2.0f}, 10.0f,
                              Color(28, 32, 42, 240));

        if (font)
        {
            float cx = leftX + 12.0f;
            float cy = PAD + 10.0f;
            float lh = font->lineHeight() + 4.0f;

            // Title
            dl.addText(*font, {cx, cy}, Color(180, 210, 255, 255), "IO State");
            cy += lh * 1.4f;

            // Mouse position
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "Mouse  %.0f, %.0f", io.mouseX, io.mouseY);
                dl.addText(*font, {cx, cy}, Color(200, 210, 220, 255), buf);
                cy += lh;
            }

            // Mouse buttons
            for (int i = 0; i < 3; ++i)
            {
                char label[16]; snprintf(label, sizeof(label), "  BTN%d", i);
                Color c = io.mouseDown[i] ? Color(100, 220, 140, 255) : Color(100, 105, 115, 255);
                dl.addText(*font, {cx, cy}, c, label);
                cy += lh;
            }

            cy += lh * 0.5f;

            // Keyboard modifiers
            dl.addText(*font, {cx, cy}, Color(200, 210, 220, 255), "Modifiers:");
            cy += lh;
            bool anyMod = false;
            for (const auto& ke : io.keyEvents) {
                if (ke.down && (ke.ctrl || ke.shift || ke.alt)) { anyMod = true; break; }
            }
            {
                // Show live modifier state via last key events
                static bool lCtrl = false, lShift = false, lAlt = false;
                for (const auto& ke : io.keyEvents) {
                    if (ke.scancode == 224 || ke.scancode == 228) lCtrl  = ke.down;
                    if (ke.scancode == 225 || ke.scancode == 229) lShift = ke.down;
                    if (ke.scancode == 226 || ke.scancode == 230) lAlt   = ke.down;
                }
                Color cCtrl  = lCtrl  ? Color(255, 200, 80, 255) : Color(80, 85, 95, 255);
                Color cShift = lShift ? Color(255, 200, 80, 255) : Color(80, 85, 95, 255);
                Color cAlt   = lAlt   ? Color(255, 200, 80, 255) : Color(80, 85, 95, 255);
                dl.addText(*font, {cx + 8.0f, cy},  cCtrl,  "Ctrl");
                dl.addText(*font, {cx + 60.0f, cy}, cShift, "Shift");
                dl.addText(*font, {cx + 120.0f, cy},cAlt,   "Alt");
                cy += lh;
            }

            cy += lh * 0.5f;

            // Delta time / FPS
            {
                char buf[64];
                int fps = io.deltaTime > 0.0f ? static_cast<int>(1.0f / io.deltaTime) : 0;
                snprintf(buf, sizeof(buf), "dt = %.2f ms   fps = %d", io.deltaTime * 1000.0f, fps);
                dl.addText(*font, {cx, cy}, Color(150, 160, 175, 255), buf);
                cy += lh;
            }

            // Scroll wheel
            {
                // Decay accumulator so bar stays visible after each scroll tick
                wheelAccum += io.mouseWheelY;
                wheelAccum *= 0.85f;  // decay ~15% per frame
                if (std::abs(wheelAccum) < 0.01f) wheelAccum = 0.0f;

                char buf[48];
                snprintf(buf, sizeof(buf), "Wheel  %.2f  (Y raw=%.1f)", wheelAccum, io.mouseWheelY);
                Color wc = std::abs(io.mouseWheelY) > 0.01f ? Color(100, 220, 180, 255)
                                                             : Color(150, 160, 175, 255);
                dl.addText(*font, {cx, cy}, wc, buf);
                cy += lh;

                drawBar(dl, cx, cy, panelW - 24.0f, 10.0f,
                        std::min(1.0f, std::abs(wheelAccum) / 5.0f),
                        Color(100, 180, 255, 200),
                        Color(40, 44, 54, 255));
                cy += 16.0f;
            }

            cy += lh * 0.5f;

            // Last key event
            if (!io.keyEvents.empty())
            {
                const auto& ke = io.keyEvents.back();
                char buf[64];
                snprintf(buf, sizeof(buf), "Last key: sym=%d scan=%d %s",
                         ke.key, ke.scancode, ke.down ? "v" : "^");
                dl.addText(*font, {cx, cy}, Color(220, 180, 100, 255), buf);
            }
        }

        // ── Right panel: event log ─────────────────────────────────────────────
        float logW = W - rightX - PAD;
        dl.addRoundRectFilled({rightX, PAD, logW, H - PAD * 2.0f}, 10.0f,
                              Color(28, 32, 42, 240));

        if (font)
        {
            float lh = font->lineHeight() + 4.0f;
            dl.addText(*font, {rightX + 10.0f, PAD + 10.0f},
                       Color(180, 210, 255, 255), "Event Log");
            dl.addLine({rightX + 8.0f, PAD + lh + 12.0f},
                       {rightX + logW - 8.0f, PAD + lh + 12.0f},
                       Color(50, 58, 78, 255), 1.0f);
            drawLog(dl, *font,
                    rightX, PAD + lh + 14.0f,
                    logW, H - PAD * 2.0f - lh - 16.0f);
        }

        // Push the DrawList as a pass with identity camera before rendering
        BuGUI::GetDrawData()->passes.push_back({&BuGUI::GetDrawList(), BuGUI::Camera2D{}});
        BuGUI::Render();
        backend.render(*BuGUI::GetDrawData());
        backend.present();
    }

    if (fontAtlas.texture())
        backend.destroyTexture(fontAtlas.texture());
    backend.shutdown();
    return 0;
}

