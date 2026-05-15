#include "SdlOpenGLBackend.hpp"
#include "BuGUI.hpp"
#include "WidgetApp.hpp"
#include "IconAtlas.hpp"
using namespace BuGUI;

extern "C" const char *__lsan_default_suppressions()
{
    return "leak:libSDL2\n"
           "leak:SDL_DBus\n"
           "leak:libnvidia-glcore\n";
}

// Forward declarations — each stage lives in stages/stage_*.cpp
void registerMenuStage(WidgetApp& app);
void registerBasicStage(WidgetApp& app);
void registerInputsStage(WidgetApp& app);
void registerLayoutsStage(WidgetApp& app);
void registerScrollStage(WidgetApp& app);
void registerControlsStage(WidgetApp& app);
void registerInputs2Stage(WidgetApp& app);
void registerMenusStage(WidgetApp& app);
void registerDialogsStage(WidgetApp& app);
void registerTreePropColorStage(WidgetApp& app);
void registerDataGridStage(WidgetApp& app);
void registerFloatWindowStage(WidgetApp& app);
void registerGizmosStage(WidgetApp& app);
void registerAnchorLayoutsStage(WidgetApp& app);
void registerCarouselStage(WidgetApp& app);
void registerSlidePanelStage(WidgetApp& app);
void registerChartsStage(WidgetApp& app);
void registerNodeEditorStage(WidgetApp& app);
void registerTimelineStage(WidgetApp& app);
void registerAudioStage(WidgetApp& app);
void registerThumbnailStage(WidgetApp& app);
void registerFileDialogStage(WidgetApp& app);
void registerConsoleStage(WidgetApp& app);
void registerDockStage(WidgetApp& app);
void registerNewFeaturesStage(WidgetApp& app);
void registerDateTimeStage(WidgetApp& app);
void registerClusterStage(WidgetApp& app);
void registerAnimModelStage(WidgetApp& app);
void registerFakeBlenderStage(WidgetApp& app);
void registerCrmStage(WidgetApp& app);
void registerFakeIdeStage(WidgetApp& app);
void registerFakeUnityStage(WidgetApp& app);
void registerToolBarStage(WidgetApp& app);
void registerCodeEditorStage(WidgetApp& app);
void registerAppWidgetsStage(WidgetApp& app);
void registerFocusStage(WidgetApp& app);

int main()
{
    SdlOpenGLBackend backend;
    if (!backend.init("BuGUI test1", 1280, 720))
        return 1;

    // ── Font ─────────────────────────────────────────────────────────────────
    BuGUI::FontAtlas fontAtlas;
    const BuGUI::Font* font = nullptr;
    if (fontAtlas.buildDefault())
    {
        fontAtlas.setTexture(backend.createTextureRGBA(
            fontAtlas.width(), fontAtlas.height(), fontAtlas.pixels()));
        if (fontAtlas.texture())
        {
            font = &fontAtlas.defaultFont();
            BuGUI::SetWhitePixel(fontAtlas.texture(), fontAtlas.whitePixelUV());
        }
    }

    // ── Icon Atlas ───────────────────────────────────────────────────────────
    IconAtlas iconAtlas;
    {
        auto* img = iconAtlas.buildImage(24);
        if (img && img->width > 0) {
            auto tex = backend.createTextureRGBA(img->width, img->height, img->pixels);
            iconAtlas.setTexture(tex, img->width, img->height);
        }
        delete img;
    }

    // ── Stages ───────────────────────────────────────────────────────────────
    WidgetApp& app = WidgetApp::instance();

    // Register texture services so widgets can upload/free GPU textures
    app.setTextureUpload([&backend](const unsigned char* rgba, int w, int h) {
        return backend.createTextureRGBA(w, h, rgba);
    });
    app.setTextureDestroy([&backend](BuGUI::TextureHandle tex) {
        backend.destroyTexture(tex);
    });

    registerMenuStage(app);
    registerBasicStage(app);
    registerInputsStage(app);
    registerLayoutsStage(app);
    registerScrollStage(app);
    registerControlsStage(app);
    registerInputs2Stage(app);
    registerMenusStage(app);
    registerDialogsStage(app);
    registerTreePropColorStage(app);
    registerDataGridStage(app);
    registerFloatWindowStage(app);
    registerGizmosStage(app);
    registerAnchorLayoutsStage(app);
    registerCarouselStage(app);
    registerSlidePanelStage(app);
    registerChartsStage(app);
    registerNodeEditorStage(app);
    registerTimelineStage(app);
    registerAudioStage(app);
    registerThumbnailStage(app);
    registerFileDialogStage(app);
    registerConsoleStage(app);
    registerDockStage(app);
    registerNewFeaturesStage(app);
    registerDateTimeStage(app);
    registerClusterStage(app);
    registerAnimModelStage(app);
    registerFakeBlenderStage(app);
    registerCrmStage(app);
    registerFakeIdeStage(app);
    registerFakeUnityStage(app);
    registerToolBarStage(app);
    registerCodeEditorStage(app);
    registerAppWidgetsStage(app);
    registerFocusStage(app);
    app.setStage("menu");

    // ── Loop ─────────────────────────────────────────────────────────────────
    while (backend.beginFrame())
    {
        auto& io = BuGUI::GetIO();
        app.update(io);
        app.paint(*BuGUI::GetDrawData(), font, iconAtlas.ready() ? &iconAtlas : nullptr);

        // ── Fixed overlay (always on top, identity camera) ─────────────────
        if (font)
        {
            const auto& st = BuGUI::GetDrawData()->stats;
            auto& ol       = BuGUI::GetOverlayDrawList();
            float lh       = font->lineHeight();
            float asc      = font->ascender();   // baseline offset from top
            float W        = io.displayWidth;
            int   fps      = io.deltaTime > 0.0f ? static_cast<int>(1.0f / io.deltaTime) : 0;

            // Colours
            const BuGUI::Color bgColor   (  0,   0,   0, 180);
            const BuGUI::Color sepColor  ( 80,  80,  80, 200);
            const BuGUI::Color labelColor(140, 160, 180, 200);
            const BuGUI::Color valueColor(220, 235, 255, 240);
            const BuGUI::Color fpsGood   ( 80, 220, 120, 240);
            const BuGUI::Color fpsBad    (255, 100,  80, 240);
            const BuGUI::Color stageColor(255, 215,  80, 230);
            const BuGUI::Color transColor(255, 160,  50, 220);

            // ── Top-left pill: FPS + stage name ─────────────────────────
            {
                char stageText[64];
                bool trans = app.isTransitioning();
                snprintf(stageText, sizeof(stageText), "%s%s",
                         app.currentStageName().c_str(),
                         trans ? " \u25ba" : "");  // ► during transition

                char fpsStr[16];
                snprintf(fpsStr, sizeof(fpsStr), "%d", fps);

                const char* fpsLabel  = "FPS ";
                const char* stgLabel  = "  stage: ";

                float fpsLW = ol.calcTextSize(*font, fpsLabel).x;
                float fpsVW = ol.calcTextSize(*font, fpsStr).x;
                float stgLW = ol.calcTextSize(*font, stgLabel).x;
                float stgVW = ol.calcTextSize(*font, stageText).x;

                float pad = 8.0f;
                float vpad = 3.0f;
                float boxW = pad + fpsLW + fpsVW + stgLW + stgVW + pad;
                float boxH = lh + 2.0f * vpad;
                float px = 8.0f, py = 8.0f;

                ol.addRoundRectFilled({px, py, boxW, boxH}, 4.0f, bgColor);

                float tx = px + pad;
                float ty = py + vpad + asc;  // baseline
                ol.addText(*font, {tx, ty}, labelColor, fpsLabel);              tx += fpsLW;
                ol.addText(*font, {tx, ty}, fps >= 50 ? fpsGood : fpsBad, fpsStr); tx += fpsVW;
                ol.addText(*font, {tx, ty}, labelColor, stgLabel);              tx += stgLW;
                ol.addText(*font, {tx, ty}, trans ? transColor : stageColor, stageText);
            }

            // ── Top-right pill: DC / clip / tri / vtx / tex ──────────────
            {
                struct Stat { const char* label; char value[16]; };
                Stat stats[5] = {
                    {"DC ",  {}}, {"cl ",  {}}, {"tri ",  {}}, {"vtx ",  {}}, {"tex ",  {}}
                };
                snprintf(stats[0].value, 16, "%d", st.drawCalls);
                snprintf(stats[1].value, 16, "%d", st.clipChanges);
                snprintf(stats[2].value, 16, "%d", st.triangles);
                snprintf(stats[3].value, 16, "%d", st.vertices);
                snprintf(stats[4].value, 16, "%d", st.textureSwitches);

                float pad = 8.0f;
                float vpad = 3.0f;
                float sepW = 6.0f;
                float totalW = pad;
                for (int i = 0; i < 5; ++i) {
                    totalW += ol.calcTextSize(*font, stats[i].label).x;
                    totalW += ol.calcTextSize(*font, stats[i].value).x;
                    if (i < 4) totalW += sepW;
                }
                totalW += pad;

                float boxH = lh + 2.0f * vpad;
                float px = W - totalW - 8.0f;
                float py = 8.0f;

                ol.addRoundRectFilled({px, py, totalW, boxH}, 4.0f, bgColor);

                float tx = px + pad;
                float ty = py + vpad + asc;  // baseline
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
        backend.render(*BuGUI::GetDrawData());
        backend.present();
    }

    app.shutdown();
    backend.shutdown();
    return 0;
}


