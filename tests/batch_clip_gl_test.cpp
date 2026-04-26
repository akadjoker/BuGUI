#include "Batch.hpp"
#include "Texture.hpp"
#include "Device.hpp"
#include "Opengl.hpp"
#include "Font.hpp"
#include "RenderState.hpp"

#include <cmath>
#include <vector>
#include <string>
#include <array>

int main()
{
    Device& dev = Device::Instance();
    if (!dev.Create(960, 640, "batch_clip_gl_test", false))
        return 2;

    dev.SetTargetFPS(60);

    RenderBatch batch;
    batch.Init(1, 8192);
    batch.SetOrtho2D((float)dev.GetWidth(), (float)dev.GetHeight());

    Font font;
    bool hasFont = false;
    std::array<const char*, 4> fontCandidates = {
        "vendor/stb/NotoSans-Regular.ttf",
        "../vendor/stb/NotoSans-Regular.ttf",
        "../../vendor/stb/NotoSans-Regular.ttf",
        "/media/projectos/projects/cpp/BuGUI/vendor/stb/NotoSans-Regular.ttf"
    };
    for (const char* path : fontCandidates)
    {
        if (font.LoadTTF(path, 18.0f, 512))
        {
            SDL_Log("[batch_clip_gl_test] Loaded TTF: %s", path);
            hasFont = true;
            break;
        }
    }
    if (!hasFont)
    {
        hasFont = font.LoadDefaultFont();
        SDL_Log("[batch_clip_gl_test] TTF failed, default font: %s", hasFont ? "OK" : "FAIL");
    }
    if (hasFont)
    {
        font.SetBatch(&batch);
        font.SetColor(235, 240, 255, 255);
        font.SetFontSize(18.0f);
    }

    const unsigned char px[2 * 2 * 4] = {
        255, 255, 255, 255,
        255,   0,   0, 255,
          0, 255,   0, 255,
          0,   0, 255, 255
    };
    Texture* tex = CreateTextureFromRGBA("test", 2, 2, px);

    int frames = 0;
    while (dev.Run())
    {
        if (dev.IsResize())
            batch.SetOrtho2D((float)dev.GetWidth(), (float)dev.GetHeight());

        const float t = (float)dev.GetTime();
        const float w = (float)dev.GetWidth();
        const float h = (float)dev.GetHeight();

        glViewport(0, 0, dev.GetWidth(), dev.GetHeight());
        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // UI/text friendly state
        auto& rs = RenderState::instance();
        rs.setDepthTest(false);
        rs.setDepthWrite(false);
        rs.setBlend(true);
        rs.setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        

        // Background grid (also clipped to full viewport bounds)
        batch.PushClipRect(0.0f, 0.0f, w, h);
        batch.SetColor(34, 39, 48, 255);
        for (int x = 0; x <= (int)w; x += 32) batch.Line2DClipped((float)x, 0.0f, (float)x, h);
        for (int y = 0; y <= (int)h; y += 32) batch.Line2DClipped(0.0f, (float)y, w, (float)y);
        batch.PopClipRect();

        // Clip viewport (left)
        const float vx = 40.0f;
        const float vy = 80.0f;
        const float vw = w * 0.56f;
        const float vh = h - 140.0f;

        batch.SetColor(22, 26, 33, 255);
        batch.Rectangle((int)vx, (int)vy, (int)vw, (int)vh, true);

        batch.PushClipRect(vx, vy, vw, vh);

        // Moving lines that cross viewport bounds
        batch.SetColor(255, 150, 90, 220);
        for (int i = 0; i < 36; ++i)
        {
            float a = t * 0.8f + i * 0.18f;
            float cx = vx + vw * 0.5f + std::cos(a) * (vw * 0.62f);
            float cy = vy + vh * 0.5f + std::sin(a * 1.3f) * (vh * 0.62f);
            batch.Line2DClipped(vx + vw * 0.5f, vy + vh * 0.5f, cx, cy);
        }

        // Polyline wave extending outside clip rect
        std::vector<glm::vec2> pts;
        pts.reserve(180);
        for (int i = 0; i < 180; ++i)
        {
            float x = vx - 120.0f + i * 9.0f;
            float y = vy + vh * 0.5f + std::sin(t * 2.0f + i * 0.16f) * (vh * 0.32f);
            pts.push_back({x, y});
        }
        batch.SetColor(90, 230, 170, 255);
        for (int i = 0; i < (int)pts.size() - 1; ++i)
            batch.Line2DClipped(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y);

        // Rotating polygon + circle + textured quad
        float px0 = vx + vw * 0.25f + std::cos(t * 0.9f) * 220.0f;
        float py0 = vy + vh * 0.35f + std::sin(t * 1.1f) * 130.0f;

        batch.SetColor(120, 180, 255, 140);
        {
            const int sides = 7;
            const float radius = 92.0f;
            const float rot = t * 90.0f * (3.14159265358979323846f / 180.0f);
            std::vector<glm::vec2> poly;
            poly.reserve(sides);
            for (int i = 0; i < sides; ++i)
            {
                float a = rot + (2.0f * 3.14159265358979323846f * i) / (float)sides;
                poly.push_back({px0 + std::cos(a) * radius, py0 + std::sin(a) * radius});
            }
            for (int i = 1; i < sides - 1; ++i)
                batch.TriangleClipped(poly[0].x, poly[0].y, poly[i].x, poly[i].y, poly[i + 1].x, poly[i + 1].y);
            batch.SetColor(180, 220, 255, 255);
            for (int i = 0; i < sides; ++i)
            {
                const auto& a = poly[i];
                const auto& b = poly[(i + 1) % sides];
                batch.Line2DClipped(a.x, a.y, b.x, b.y);
            }
        }

        batch.SetColor(255, 240, 90, 220);
        batch.CircleClipped(px0 + std::cos(t * 1.8f) * 120.0f, py0 + std::sin(t * 1.4f) * 90.0f, 34.0f, true);

        if (tex)
        {
            batch.SetColor(255, 255, 255, 255);
            batch.DrawImageRegion(tex,
                                  0.0f, 0.0f, 2.0f, 2.0f,
                                  vx + vw - 120.0f, vy + 30.0f, 180.0f, 180.0f,
                                  t * 45.0f, 90.0f, 90.0f,
                                  nullptr);
        }

        batch.PopClipRect();

        // Viewport border
        batch.SetColor(220, 230, 255, 255);
        batch.Rectangle((int)vx, (int)vy, (int)vw, (int)vh, false);

        if (hasFont)
        {
            font.SetBatch(&batch);
            font.SetFontSize(28.0f);
            font.SetColor(255, 255, 255, 255);
            font.Print("FONT OK - CLIP DEMO", 26.0f, 34.0f);

            font.SetFontSize(18.0f);
            Font::ClipRect leftClip{vx, vy, vw, vh};
            std::string msg = "LEFT CLIPPED TEXT t=" + std::to_string(t);
            font.SetColor(240, 245, 255, 255);
            font.Print(msg.c_str(), vx - 120.0f + std::sin(t * 1.4f) * 140.0f, vy + 18.0f, &leftClip);
            font.Print("Text should be clipped by viewport bounds.", vx + 10.0f, vy + vh - 28.0f, &leftClip);
        }

        // Right side: same primitives clipped to right panel
        float rx = vx + vw + 30.0f;
        float rw = w - rx - 30.0f;
        float ry = vy;
        float rh = vh;

        batch.SetColor(28, 32, 40, 255);
        batch.Rectangle((int)rx, (int)ry, (int)rw, (int)rh, true);

        batch.PushClipRect(rx, ry, rw, rh);
        batch.SetColor(255, 80, 80, 220);
        batch.Line2DClipped(rx + 20.0f, ry + 20.0f, rx + rw + 120.0f, ry + rh * 0.7f);
        batch.Line2DClipped(rx - 120.0f, ry + rh - 20.0f, rx + rw - 20.0f, ry + 40.0f);

        batch.SetColor(120, 230, 180, 255);
        for (int i = 0; i < (int)pts.size() - 1; ++i)
            batch.Line2DClipped(pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y);
        batch.PopClipRect();

        batch.SetColor(210, 220, 240, 255);
        batch.Rectangle((int)rx, (int)ry, (int)rw, (int)rh, false);

        if (hasFont)
        {
            font.SetColor(255, 230, 170, 255);
            font.Print("RIGHT PANEL (no text clip)", rx - 50.0f, ry + 18.0f);
        }

        if (hasFont)
        {
            font.SetColor(160, 255, 180, 255);
            font.Print("Batch clip + Font clip demo", 20.0f, 28.0f);
        }

        batch.Render();
        dev.Flip();

        // ++frames;
        // if (frames > 900) // ~15s @ 60fps
        //     break;
    }

    delete tex;
    font.Release();
    batch.Release();
    dev.Close();
    return 0;
}
