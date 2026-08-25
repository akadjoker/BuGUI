// ─────────────────────────────────────────────────────────────────────────────
// demo_raylib/main.cpp  —  BuGUI on top of an existing Raylib project.
//
// BuGUI types live in namespace BuGUI — no global aliases, no name collision
// with Raylib's Color/Rectangle/etc.
//
// RaylibBackend owns everything BuGUI-related:
//   init()     — BuGUI context, IO callbacks, GPU objects
//   newFrame() — IO update (size, dt, input, cursor)
//   render()   — draws BuGUI draw data via rlgl
//   shutdown() — GPU cleanup + BuGUI context destroy
// ─────────────────────────────────────────────────────────────────────────────

// BuGUI headers first — parsed before Raylib's color macros (#define RED, etc.)
#include "RaylibBackend.hpp"
#include "WidgetApp.hpp"
#include "IconAtlas.hpp"

// Raylib after — macros don't affect already-parsed BuGUI headers.
// Color = Raylib's Color here; BuGUI colors use BuGUI::Color explicitly.
#include <raylib.h>

void registerDemoStage(BuGUI::WidgetApp& app);
void registerMenuStage(BuGUI::WidgetApp& app);
void registerBasicStage(BuGUI::WidgetApp& app);
void registerControlsStage(BuGUI::WidgetApp& app);
void registerScrollStage(BuGUI::WidgetApp& app);
void registerInputsStage(BuGUI::WidgetApp& app);
void registerMenusStage(BuGUI::WidgetApp& app);
void registerDialogsStage(BuGUI::WidgetApp& app);
void registerDockStage(BuGUI::WidgetApp& app);
void registerPropertiesStage(BuGUI::WidgetApp& app);

int main()
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "BuGUI + Raylib");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    RaylibBackend renderer;
    if (!renderer.init())
        return 1;

    // Font atlas
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

    // Icon atlas
    BuGUI::IconAtlas iconAtlas;
    {
        auto* img = iconAtlas.buildImage(24);
        if (img && img->width > 0)
            iconAtlas.setTexture(
                renderer.createTextureRGBA(img->width, img->height, img->pixels),
                img->width, img->height);
        delete img;
    }

    // App
    BuGUI::WidgetApp& app = BuGUI::WidgetApp::instance();
    app.setTextureUpload([&renderer](const unsigned char* rgba, int w, int h) {
        return renderer.createTextureRGBA(w, h, rgba);
    });
    app.setTextureDestroy([&renderer](BuGUI::TextureHandle tex) {
        renderer.destroyTexture(tex);
    });
    registerDemoStage(app);
    registerMenuStage(app);
    registerBasicStage(app);
    registerControlsStage(app);
    registerScrollStage(app);
    registerInputsStage(app);
    registerMenusStage(app);
    registerDialogsStage(app);
    registerDockStage(app);
    registerPropertiesStage(app);
    app.setStage("menu");

    while (!WindowShouldClose())
    {
        renderer.newFrame();
        BuGUI::NewFrame();

        auto& io = BuGUI::GetIO();
        app.update(io);
        app.paint(*BuGUI::GetDrawData(), font, iconAtlas.ready() ? &iconAtlas : nullptr);

        // ── Debug overlay ─────────────────────────────────────────────────────
        if (font)
        {
            const auto& st = BuGUI::GetDrawData()->stats;
            auto& ol  = BuGUI::GetOverlayDrawList();
            float lh  = font->lineHeight();
            float asc = font->ascender();
            float W   = io.displayWidth;
            int   fps = io.deltaTime > 0.0f ? static_cast<int>(1.0f / io.deltaTime) : 0;

            const BuGUI::Color bgColor   (  0,   0,   0, 180);
            const BuGUI::Color sepColor  ( 80,  80,  80, 200);
            const BuGUI::Color labelColor(140, 160, 180, 200);
            const BuGUI::Color valueColor(220, 235, 255, 240);
            const BuGUI::Color fpsGood   ( 80, 220, 120, 240);
            const BuGUI::Color fpsBad    (255, 100,  80, 240);
            const BuGUI::Color stageColor(255, 215,  80, 230);
            const BuGUI::Color transColor(255, 160,  50, 220);

            // top-left: FPS + stage
            {
                char stageText[64];
                bool trans = app.isTransitioning();
                snprintf(stageText, sizeof(stageText), "%s%s",
                         app.currentStageName().c_str(), trans ? " \u25ba" : "");
                char fpsStr[16];
                snprintf(fpsStr, sizeof(fpsStr), "%d", fps);

                const char* fpsLabel = "FPS ";
                const char* stgLabel = "  stage: ";
                float fpsLW = ol.calcTextSize(*font, fpsLabel).x;
                float fpsVW = ol.calcTextSize(*font, fpsStr).x;
                float stgLW = ol.calcTextSize(*font, stgLabel).x;
                float stgVW = ol.calcTextSize(*font, stageText).x;

                float pad = 8.0f, vpad = 3.0f;
                float boxW = pad + fpsLW + fpsVW + stgLW + stgVW + pad;
                float boxH = lh + 2.0f * vpad;
                float px = 8.0f, py = 8.0f;
                ol.addRoundRectFilled({px, py, boxW, boxH}, 4.0f, bgColor);
                float tx = px + pad, ty = py + vpad + asc;
                ol.addText(*font, {tx, ty}, labelColor, fpsLabel);          tx += fpsLW;
                ol.addText(*font, {tx, ty}, fps >= 50 ? fpsGood : fpsBad, fpsStr); tx += fpsVW;
                ol.addText(*font, {tx, ty}, labelColor, stgLabel);          tx += stgLW;
                ol.addText(*font, {tx, ty}, trans ? transColor : stageColor, stageText);
            }

            // top-right: DC / clip / tri / vtx / tex
            {
                struct Stat { const char* label; char value[16]; };
                Stat stats[5] = {{"DC ",{}},{"cl ",{}},{"tri ",{}},{"vtx ",{}},{"tex ",{}}};
                snprintf(stats[0].value, 16, "%d", st.drawCalls);
                snprintf(stats[1].value, 16, "%d", st.clipChanges);
                snprintf(stats[2].value, 16, "%d", st.triangles);
                snprintf(stats[3].value, 16, "%d", st.vertices);
                snprintf(stats[4].value, 16, "%d", st.textureSwitches);

                float pad = 8.0f, vpad = 3.0f, sepW = 6.0f;
                float totalW = pad;
                for (int i = 0; i < 5; ++i) {
                    totalW += ol.calcTextSize(*font, stats[i].label).x;
                    totalW += ol.calcTextSize(*font, stats[i].value).x;
                    if (i < 4) totalW += sepW;
                }
                totalW += pad;

                float boxH = lh + 2.0f * vpad;
                float px = W - totalW - 8.0f, py = 8.0f;
                ol.addRoundRectFilled({px, py, totalW, boxH}, 4.0f, bgColor);
                float tx = px + pad, ty = py + vpad + asc;
                for (int i = 0; i < 5; ++i) {
                    float lw = ol.calcTextSize(*font, stats[i].label).x;
                    float vw = ol.calcTextSize(*font, stats[i].value).x;
                    ol.addText(*font, {tx, ty}, labelColor, stats[i].label); tx += lw;
                    ol.addText(*font, {tx, ty}, valueColor, stats[i].value); tx += vw;
                    if (i < 4) {
                        float sx = tx + sepW * 0.5f;
                        ol.addLine({sx, py + vpad}, {sx, py + boxH - vpad}, sepColor);
                        tx += sepW;
                    }
                }
            }
        }

        BuGUI::Render();

        BeginDrawing();
        ClearBackground({24, 26, 30, 255});
        // [your own Raylib draws here]
        renderer.render(*BuGUI::GetDrawData());
        EndDrawing();
    }

    renderer.shutdown();
    CloseWindow();
    return 0;
}
