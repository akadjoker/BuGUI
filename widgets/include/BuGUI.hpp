#pragma once

#include "Color.hpp"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <unordered_map>

namespace BuGUI
{

    enum class AlignX
    {
        Left,
        Center,
        Right
    };
    enum class AlignY
    {
        Top,
        Middle,
        Bottom
    };

    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Rect
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    struct TextureHandle
    {
        uintptr_t value = 0;

        explicit operator bool() const { return value != 0; }
    };

    struct IO
    {
        float deltaTime = 1.0f / 60.0f;
        float displayWidth = 0.0f;
        float displayHeight = 0.0f;
        float framebufferScaleX = 1.0f;
        float framebufferScaleY = 1.0f;

        float mouseX = 0.0f;
        float mouseY = 0.0f;
        float mouseWheelX = 0.0f;
        float mouseWheelY = 0.0f;
        bool mouseDown[5] = {};

        bool keysDown[512] = {};
        bool keyCtrl = false;
        bool keyShift = false;
        bool keyAlt = false;

        std::vector<uint32_t> inputChars;

        void addInputCharacter(uint32_t c) { inputChars.push_back(c); }
    };

    struct DrawVertex
    {
        float x = 0.0f;
        float y = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
        Color color = Color(255, 255, 255, 255);
    };

    struct DrawCmd
    {
        TextureHandle texture;
        Rect clip;
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
    };

    enum class IconId
    {
        None = 0,
        Check,
        Cross,
        Plus,
        Minus,
        ArrowRight,
        ArrowDown,
        Search
    };

    struct FontGlyph
    {
        uint32_t codepoint = 0;
        Rect uv;
        Vec2 size;
        Vec2 offset;
        float advanceX = 0.0f;
    };

    class Font
    {
    public:
        const FontGlyph *findGlyph(uint32_t codepoint) const;
        TextureHandle texture() const { return texture_; }
        float lineHeight() const { return lineHeight_; }

    private:
        friend class FontAtlas;

        TextureHandle texture_;
        float lineHeight_ = 0.0f;
        std::unordered_map<uint32_t, FontGlyph> glyphs_;
    };

    class FontAtlas
    {
    public:
        bool buildDefault();
        bool buildFromTTFMemory(const unsigned char *data, int dataSize, float pixelHeight = 16.0f);

        void setTexture(TextureHandle texture);
        TextureHandle texture() const { return texture_; }
        const Font &defaultFont() const { return defaultFont_; }

        int width() const { return width_; }
        int height() const { return height_; }
        const unsigned char *pixels() const { return pixels_.data(); }
        size_t pixelSize() const { return pixels_.size(); }

    private:
        int width_ = 0;
        int height_ = 0;
        TextureHandle texture_;
        Font defaultFont_;
        std::vector<unsigned char> pixels_;
    };

    class DrawList
    {
    public:
        void clear();

        void pushClip(const Rect &rect);
        void popClip();

        void addRect(const Rect &rect, const Color &color);
        void addRectOutline(const Rect &rect, const Color &color, float thickness = 1.0f);
        void addRoundRectFilled(const Rect &rect, float radius, const Color &color, int segments = 8);
        void addRoundRect(const Rect &rect, float radius, const Color &color, float thickness = 1.0f, int segments = 8);
        void addCircleFilled(Vec2 center, float radius, const Color &color, int segments = 32);
        void addCircle(Vec2 center, float radius, const Color &color, float thickness = 1.0f, int segments = 32);
        void addTriangleFilled(Vec2 a, Vec2 b, Vec2 c, const Color &color);
        void addTriangle(Vec2 a, Vec2 b, Vec2 c, const Color &color, float thickness = 1.0f);
        void addImage(TextureHandle texture, const Rect &rect, const Rect &uv, const Color &color = Color::WHITE);
        void addText(const Font &font, Vec2 pos, const Color &color, const char *text);


        Vec2 calcTextSize(const Font& font, const char* text);

        void addTextAligned(const Font&  font,
                            const Rect&  bounds,
                            const Color& color,
                            const char*  text,
                            AlignX       ax = AlignX::Left,
                            AlignY       ay = AlignY::Top);

        void addIcon(IconId icon, const Rect &rect, const Color &color, float thickness = 2.0f);
        void addLine(Vec2 a, Vec2 b, const Color &color, float thickness = 1.0f);

        const std::vector<DrawVertex> &vertices() const { return vertices_; }
        const std::vector<uint32_t> &indices() const { return indices_; }
        const std::vector<DrawCmd> &commands() const { return commands_; }

    private:
        Rect currentClip() const;
        void addCmd(uint32_t indexOffset, uint32_t indexCount, TextureHandle texture = {});
        uint32_t addVertex(float x, float y, const Color &color);
        uint32_t addVertex(float x, float y, float u, float v, const Color &color);
        void addConvexPolyFilled(const std::vector<Vec2> &points, const Color &color);
        void addPolyline(const std::vector<Vec2> &points, const Color &color, float thickness, bool closed);

        std::vector<Rect> clipStack_;
        std::vector<DrawVertex> vertices_;
        std::vector<uint32_t> indices_;
        std::vector<DrawCmd> commands_;
           std::unordered_map<uint32_t, FontGlyph> glyphs_;
    };

    struct DrawData
    {
        DrawList *mainList = nullptr;
        float displayWidth = 0.0f;
        float displayHeight = 0.0f;
        float framebufferScaleX = 1.0f;
        float framebufferScaleY = 1.0f;
    };

    struct Context;

    Context *CreateContext();
    void DestroyContext(Context *ctx);
    void SetCurrentContext(Context *ctx);
    Context *GetCurrentContext();

    IO &GetIO();
    DrawList &GetDrawList();

    void NewFrame();
    void Render();
    DrawData *GetDrawData();

} // namespace BuGUI
