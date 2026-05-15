// BuGUI headers first (before Raylib macros)
#include "RaylibBackend.hpp"
#include "WidgetApp.hpp"
#include "IconAtlas.hpp"

#include <raylib.h>
#include <cstdio>

void registerAtlasStage(BuGUI::WidgetApp& app);


int main()
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1440, 860, "BuGUI — Atlas Studio");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    RaylibBackend renderer;
    if (!renderer.init())
        return 1;

    // Font
    BuGUI::FontAtlas fontAtlas;
    const BuGUI::Font* font = nullptr;
    if (fontAtlas.buildDefault())
    {
        fontAtlas.setTexture(renderer.createTextureRGBA(
            fontAtlas.width(), fontAtlas.height(), fontAtlas.pixels()));
        if (fontAtlas.texture())
        {
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

    BuGUI::WidgetApp& app = BuGUI::WidgetApp::instance();
    app.setTextureUpload([&renderer](const unsigned char* rgba, int w, int h) {
        return renderer.createTextureRGBA(w, h, rgba);
    });
    app.setTextureDestroy([&renderer](BuGUI::TextureHandle tex) {
        renderer.destroyTexture(tex);
    });

    registerAtlasStage(app);
    app.setStage("atlas");

    while (!WindowShouldClose())
    {
        renderer.newFrame();
        BuGUI::NewFrame();

        auto& io = BuGUI::GetIO();
        app.update(io);
        app.paint(*BuGUI::GetDrawData(), font,
                  iconAtlas.ready() ? &iconAtlas : nullptr);

        BuGUI::Render();
        renderer.render(*BuGUI::GetDrawData());
        EndDrawing();
    }

    renderer.shutdown();
    CloseWindow();
    return 0;
}
