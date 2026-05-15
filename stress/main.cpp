// BuGUI headers first (before Raylib macros)
#include "RaylibBackend.hpp"
#include "WidgetApp.hpp"
#include "IconAtlas.hpp"

#include <raylib.h>
#include <cstdio>

void registerStressStage(BuGUI::WidgetApp& app);

int main()
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "BuGUI — Stress Test");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(0); // uncapped — show real throughput

    RaylibBackend renderer;
    if (!renderer.init())
        return 1;

    // Font
    BuGUI::FontAtlas fontAtlas;
    const BuGUI::Font* font = nullptr;
    if (fontAtlas.buildDefault()) {
        fontAtlas.setTexture(renderer.createTextureRGBA(
            fontAtlas.width(), fontAtlas.height(), fontAtlas.pixels()));
        if (fontAtlas.texture()) {
            font = &fontAtlas.defaultFont();
            BuGUI::SetWhitePixel(fontAtlas.texture(), fontAtlas.whitePixelUV());
        }
    }

    // Icon atlas (optional)
    BuGUI::IconAtlas iconAtlas;
    {
        auto* img = iconAtlas.buildImage(24);
        if (img && img->width > 0)
            iconAtlas.setTexture(
                renderer.createTextureRGBA(img->width, img->height, img->pixels),
                img->width, img->height);
        delete img;
    }

    BuGUI::WidgetApp& app = BuGUI::WidgetApp::instance();
    app.setTextureUpload([&renderer](const unsigned char* rgba, int w, int h) {
        return renderer.createTextureRGBA(w, h, rgba);
    });
    app.setTextureDestroy([&renderer](BuGUI::TextureHandle tex) {
        renderer.destroyTexture(tex);
    });

    registerStressStage(app);
    app.setStage("stress");

    while (!WindowShouldClose())
    {
        renderer.newFrame();
        BuGUI::NewFrame();

        auto& io = BuGUI::GetIO();
        app.update(io);
        app.paint(*BuGUI::GetDrawData(), font,
                  iconAtlas.ready() ? &iconAtlas : nullptr);

        // ── FPS + draw stats overlay ──────────────────────────────────────
        if (font)
        {
            const auto& st = BuGUI::GetDrawData()->stats;
            auto& ol  = BuGUI::GetOverlayDrawList();
            float lh  = font->lineHeight();
            float asc = font->ascender();
            int   fps = io.deltaTime > 0.0f
                        ? static_cast<int>(1.0f / io.deltaTime) : 0;

            const BuGUI::Color bg   (  0,   0,   0, 160);
            const BuGUI::Color lbl  (140, 160, 180, 200);
            const BuGUI::Color val  (220, 235, 255, 240);
            const BuGUI::Color good ( 80, 220, 120, 240);
            const BuGUI::Color bad  (255, 100,  80, 240);

            float px = io.displayWidth - 210.f;
            float py = 6.f;
            float pw = 202.f;
            float ph = lh * 5 + 10.f;
            ol.addRect({px - 4, py - 4, pw + 8, ph + 8}, bg);

            auto row = [&](const char* label, const char* value,
                           const BuGUI::Color& vc, float& y) {
                ol.addText(*font, {px,      y + asc}, lbl, label);
                ol.addText(*font, {px + 90, y + asc}, vc,  value);
                y += lh;
            };

            char buf[32];
            float y = py + 5.f;
            std::snprintf(buf, sizeof(buf), "%d", fps);
            row("FPS",      buf, fps >= 55 ? good : bad, y);
            std::snprintf(buf, sizeof(buf), "%d", st.drawCalls);
            row("DrawCalls", buf, val, y);
            std::snprintf(buf, sizeof(buf), "%d", st.vertices);
            row("Vertices",  buf, val, y);
            std::snprintf(buf, sizeof(buf), "%.0f x %.0f",
                          io.displayWidth, io.displayHeight);
            row("Size",      buf, val, y);
        }

        BuGUI::Render();
        renderer.render(*BuGUI::GetDrawData());
        EndDrawing();
    }

    renderer.shutdown();
    CloseWindow();
    return 0;
}
