#include "SdlOpenGLBackend.hpp"
#include "BuGUI.hpp"
#include <cstdio>

int main()
{
    SdlOpenGLBackend backend;
    backend.setBackgroundColor(Color(24, 26, 30, 255));

    if (!backend.init("BuGUI Backend Architecture Demo", 1024, 680))
    {
        std::fprintf(stderr, "BuGUI demo: backend init failed\n");
        return 1;
    }

    BuGUI::FontAtlas fontAtlas;
    bool fontReady = fontAtlas.buildDefault();
    if (fontReady)
    {
        fontAtlas.setTexture(backend.createTextureRGBA(fontAtlas.width(),
                                                       fontAtlas.height(),
                                                       fontAtlas.pixels()));
        fontReady = static_cast<bool>(fontAtlas.texture());
    }

    if (!fontReady)
        std::fprintf(stderr, "BuGUI demo: font atlas failed, continuing without text\n");

    while (backend.beginFrame())
    {
        auto& io = BuGUI::GetIO();
        auto& draw = BuGUI::GetDrawList();

        BuGUI::Rect panel{
            32.0f,
            32.0f,
            io.displayWidth > 64.0f ? io.displayWidth - 64.0f : 0.0f,
            120.0f
        };

        draw.addRoundRectFilled(panel, 14.0f, Color(42, 45, 52, 255));
        draw.addRoundRect(panel, 14.0f, Color(90, 96, 110, 255), 2.0f);
        draw.addLine({panel.x + 18.0f, panel.y + 64.0f},
                     {panel.x + panel.w - 18.0f, panel.y + 64.0f},
                     Color(120, 190, 255, 255),
                     3.0f);
        draw.addCircleFilled({panel.x + 42.0f, panel.y + 36.0f}, 10.0f, Color(120, 190, 255, 255));
        draw.addCircle({panel.x + 72.0f, panel.y + 36.0f}, 11.0f, Color(255, 210, 110, 255), 2.0f);
        draw.addTriangleFilled({panel.x + 96.0f, panel.y + 28.0f},
                               {panel.x + 116.0f, panel.y + 42.0f},
                               {panel.x + 96.0f, panel.y + 56.0f},
                               Color(180, 135, 255, 255));
        draw.addTriangle({panel.x + 128.0f, panel.y + 26.0f},
                         {panel.x + 150.0f, panel.y + 58.0f},
                         {panel.x + 106.0f, panel.y + 58.0f},
                         Color(255, 255, 255, 170),
                         2.0f);

        draw.pushClip({panel.x + 120.0f, panel.y + 22.0f, 140.0f, 72.0f});
        draw.addRoundRectFilled({panel.x + 96.0f, panel.y + 10.0f, 190.0f, 96.0f},
                                24.0f,
                                Color(80, 210, 150, 160));
        draw.addCircleFilled({panel.x + 260.0f, panel.y + 58.0f},
                             38.0f,
                             Color(255, 120, 130, 170));
        draw.popClip();

        draw.addIcon(BuGUI::IconId::Check,
                     {panel.x + panel.w - 132.0f, panel.y + 24.0f, 18.0f, 18.0f},
                     Color(120, 230, 160, 255),
                     3.0f);
        draw.addIcon(BuGUI::IconId::Search,
                     {panel.x + panel.w - 100.0f, panel.y + 22.0f, 22.0f, 22.0f},
                     Color(220, 225, 235, 230),
                     2.0f);
        draw.addIcon(BuGUI::IconId::Cross,
                     {panel.x + panel.w - 64.0f, panel.y + 24.0f, 18.0f, 18.0f},
                     Color(255, 120, 130, 255),
                     3.0f);
        if (fontReady)
        {
            draw.addText(fontAtlas.defaultFont(),
                         {panel.x + 18.0f, panel.y + 96.0f},
                         Color(245, 247, 252, 255),
                         "BuGUI:  - ola acao çã áéíóú");
        }

        BuGUI::Render();
        backend.render(*BuGUI::GetDrawData());
        backend.present();
    }

    if (fontAtlas.texture())
        backend.destroyTexture(fontAtlas.texture());
    backend.shutdown();
    return 0;
}
