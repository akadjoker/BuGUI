#include "BuGUI.hpp"

#include "NotoSans_embedded.h"
#include "stb_truetype.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace BuGUI
{

    struct Context
    {
        IO io;
        DrawList drawList;
        DrawData drawData;
    };

    namespace
    {

        Context *gCurrentContext = nullptr;
        constexpr float Pi = 3.14159265358979323846f;

        Context &current()
        {
            if (!gCurrentContext)
                gCurrentContext = CreateContext();
            return *gCurrentContext;
        }

        int saneSegments(int segments)
        {
            return std::max(3, segments);
        }

        bool intersectRects(const Rect &a, const Rect &b, Rect &out)
        {
            float x0 = std::max(a.x, b.x);
            float y0 = std::max(a.y, b.y);
            float x1 = std::min(a.x + a.w, b.x + b.w);
            float y1 = std::min(a.y + a.h, b.y + b.h);

            if (x1 <= x0 || y1 <= y0)
            {
                out = {x0, y0, 0.0f, 0.0f};
                return false;
            }

            out = {x0, y0, x1 - x0, y1 - y0};
            return true;
        }

        std::vector<Vec2> makeCirclePoints(Vec2 center, float radius, int segments)
        {
            segments = saneSegments(segments);
            std::vector<Vec2> points;
            points.reserve(static_cast<size_t>(segments));

            for (int i = 0; i < segments; ++i)
            {
                float a = (static_cast<float>(i) / static_cast<float>(segments)) * Pi * 2.0f;
                points.push_back({center.x + std::cos(a) * radius,
                                  center.y + std::sin(a) * radius});
            }

            return points;
        }

        uint32_t decodeUtf8(const char *&text)
        {
            const unsigned char *s = reinterpret_cast<const unsigned char *>(text);

            // ASCII
            if (s[0] < 0x80)
            {
                ++text;
                return s[0];
            }

            // 2-byte sequence
            if ((s[0] & 0xE0) == 0xC0 && s[1] != '\0' && (s[1] & 0xC0) == 0x80)
            {
                text += 2;
                return static_cast<uint32_t>(((s[0] & 0x1F) << 6) | (s[1] & 0x3F));
            }

            // 3-byte sequence
            if ((s[0] & 0xF0) == 0xE0 &&
                s[1] != '\0' && (s[1] & 0xC0) == 0x80 &&
                s[2] != '\0' && (s[2] & 0xC0) == 0x80)
            {
                text += 3;
                return static_cast<uint32_t>(((s[0] & 0x0F) << 12) |
                                             ((s[1] & 0x3F) << 6) |
                                             (s[2] & 0x3F));
            }

            // 4-byte sequence
            if ((s[0] & 0xF8) == 0xF0 &&
                s[1] != '\0' && (s[1] & 0xC0) == 0x80 &&
                s[2] != '\0' && (s[2] & 0xC0) == 0x80 &&
                s[3] != '\0' && (s[3] & 0xC0) == 0x80)
            {
                text += 4;
                return static_cast<uint32_t>(((s[0] & 0x07) << 18) |
                                             ((s[1] & 0x3F) << 12) |
                                             ((s[2] & 0x3F) << 6) |
                                             (s[3] & 0x3F));
            }

            ++text;
            return 0xFFFD;
        }

        void appendArc(std::vector<Vec2> &points, Vec2 center, float radius, float start, float end, int segments)
        {
            segments = std::max(1, segments);
            for (int i = 0; i <= segments; ++i)
            {
                float t = static_cast<float>(i) / static_cast<float>(segments);
                float a = start + (end - start) * t;
                points.push_back({center.x + std::cos(a) * radius,
                                  center.y + std::sin(a) * radius});
            }
        }

        std::vector<Vec2> makeRoundRectPoints(const Rect &rect, float radius, int segments)
        {
            radius = std::max(0.0f, std::min(radius, std::min(rect.w, rect.h) * 0.5f));
            segments = std::max(1, segments);

            if (radius <= 0.0001f)
            {
                return {
                    {rect.x, rect.y},
                    {rect.x + rect.w, rect.y},
                    {rect.x + rect.w, rect.y + rect.h},
                    {rect.x, rect.y + rect.h}};
            }

            std::vector<Vec2> points;
            points.reserve(static_cast<size_t>((segments + 1) * 4));

            float x0 = rect.x;
            float y0 = rect.y;
            float x1 = rect.x + rect.w;
            float y1 = rect.y + rect.h;

            appendArc(points, {x1 - radius, y0 + radius}, radius, -Pi * 0.5f, 0.0f, segments);
            appendArc(points, {x1 - radius, y1 - radius}, radius, 0.0f, Pi * 0.5f, segments);
            appendArc(points, {x0 + radius, y1 - radius}, radius, Pi * 0.5f, Pi, segments);
            appendArc(points, {x0 + radius, y0 + radius}, radius, Pi, Pi * 1.5f, segments);

            return points;
        }

    } // namespace

    const FontGlyph *Font::findGlyph(uint32_t codepoint) const
    {
        auto it = glyphs_.find(codepoint);
        if (it != glyphs_.end())
            return &it->second;

        it = glyphs_.find('?');
        if (it != glyphs_.end())
            return &it->second;

        return nullptr;
    }

    bool FontAtlas::buildDefault()
    {
        return buildFromTTFMemory(NotoSans_ttf_data, static_cast<int>(NotoSans_ttf_size), 16.0f);
    }

    bool FontAtlas::buildFromTTFMemory(const unsigned char *data, int dataSize, float pixelHeight)
    {
        if (!data || dataSize <= 0 || pixelHeight <= 0.0f)
            return false;

        width_ = 1024;
        height_ = 1024;
        std::vector<unsigned char> alpha(static_cast<size_t>(width_ * height_), 0);

        constexpr int firstCodepoint = 32;
        constexpr int codepointCount = 224; // Basic Latin + Latin-1 supplement.
        std::vector<stbtt_packedchar> packed(static_cast<size_t>(codepointCount));

        stbtt_pack_context pack = {};
        if (!stbtt_PackBegin(&pack, alpha.data(), width_, height_, 0, 1, nullptr))
            return false;

        stbtt_PackSetOversampling(&pack, 2, 2);
        stbtt_PackSetSkipMissingCodepoints(&pack, 1);
        stbtt_PackFontRange(&pack,
                            data,
                            0,
                            pixelHeight,
                            firstCodepoint,
                            codepointCount,
                            packed.data());
        stbtt_PackEnd(&pack);

        pixels_.assign(static_cast<size_t>(width_ * height_ * 4), 0);
        for (int i = 0; i < width_ * height_; ++i)
        {
            size_t dst = static_cast<size_t>(i) * 4;
            pixels_[dst + 0] = 255;
            pixels_[dst + 1] = 255;
            pixels_[dst + 2] = 255;
            pixels_[dst + 3] = alpha[static_cast<size_t>(i)];
        }

        stbtt_fontinfo info = {};
        if (!stbtt_InitFont(&info, data, stbtt_GetFontOffsetForIndex(data, 0)))
            return false;

        float scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);
        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

        defaultFont_.glyphs_.clear();
        defaultFont_.glyphs_.reserve(static_cast<size_t>(codepointCount));
        defaultFont_.lineHeight_ = static_cast<float>(ascent - descent + lineGap) * scale;
        defaultFont_.texture_ = texture_;

        for (int i = 0; i < codepointCount; ++i)
        {
            const stbtt_packedchar &src = packed[static_cast<size_t>(i)];
            uint32_t codepoint = static_cast<uint32_t>(firstCodepoint + i);
            if (src.xadvance <= 0.0f && codepoint != ' ')
                continue;

            FontGlyph glyph;
            glyph.codepoint = codepoint;
            glyph.uv = {
                static_cast<float>(src.x0) / static_cast<float>(width_),
                static_cast<float>(src.y0) / static_cast<float>(height_),
                static_cast<float>(src.x1 - src.x0) / static_cast<float>(width_),
                static_cast<float>(src.y1 - src.y0) / static_cast<float>(height_)};
            glyph.size = {
                src.xoff2 - src.xoff,
                src.yoff2 - src.yoff};
            glyph.offset = {src.xoff, src.yoff};
            glyph.advanceX = src.xadvance;
            defaultFont_.glyphs_[glyph.codepoint] = glyph;
        }

        return !defaultFont_.glyphs_.empty();
    }

    void FontAtlas::setTexture(TextureHandle texture)
    {
        texture_ = texture;
        defaultFont_.texture_ = texture;
    }

    void DrawList::clear()
    {
        clipStack_.clear();
        vertices_.clear();
        indices_.clear();
        commands_.clear();
    }

    void DrawList::pushClip(const Rect &rect)
    {
        Rect clipped = rect;
        if (!clipStack_.empty())
            intersectRects(clipStack_.back(), rect, clipped);
        clipStack_.push_back(clipped);
    }

    void DrawList::popClip()
    {
        if (!clipStack_.empty())
            clipStack_.pop_back();
    }

    void DrawList::addRect(const Rect &rect, const Color &color)
    {
        uint32_t offset = static_cast<uint32_t>(indices_.size());
        uint32_t v0 = addVertex(rect.x, rect.y, color);
        uint32_t v1 = addVertex(rect.x + rect.w, rect.y, color);
        uint32_t v2 = addVertex(rect.x + rect.w, rect.y + rect.h, color);
        uint32_t v3 = addVertex(rect.x, rect.y + rect.h, color);

        indices_.insert(indices_.end(), {v0, v1, v2, v0, v2, v3});
        addCmd(offset, 6);
    }
    void DrawList::addRectOutline(const Rect &rect, const Color &color, float thickness)
    {
        thickness = std::max(1.0f, thickness);

        addRect({rect.x, rect.y, rect.w, thickness}, color);
        addRect({rect.x, rect.y + rect.h - thickness, rect.w, thickness}, color);

        addRect({rect.x, rect.y + thickness,
                 thickness, rect.h - thickness * 2.0f},
                color);
        addRect({rect.x + rect.w - thickness, rect.y + thickness,
                 thickness, rect.h - thickness * 2.0f},
                color);
    }
    void DrawList::addRoundRectFilled(const Rect &rect, float radius, const Color &color, int segments)
    {
        if (rect.w <= 0.0f || rect.h <= 0.0f)
            return;

        addConvexPolyFilled(makeRoundRectPoints(rect, radius, segments), color);
    }

    void DrawList::addRoundRect(const Rect &rect, float radius, const Color &color, float thickness, int segments)
    {
        if (rect.w <= 0.0f || rect.h <= 0.0f)
            return;

        addPolyline(makeRoundRectPoints(rect, radius, segments), color, thickness, true);
    }

    void DrawList::addCircleFilled(Vec2 center, float radius, const Color &color, int segments)
    {
        if (radius <= 0.0f)
            return;

        addConvexPolyFilled(makeCirclePoints(center, radius, segments), color);
    }

    void DrawList::addCircle(Vec2 center, float radius, const Color &color, float thickness, int segments)
    {
        if (radius <= 0.0f)
            return;

        addPolyline(makeCirclePoints(center, radius, segments), color, thickness, true);
    }

    void DrawList::addTriangleFilled(Vec2 a, Vec2 b, Vec2 c, const Color &color)
    {
        uint32_t offset = static_cast<uint32_t>(indices_.size());
        uint32_t v0 = addVertex(a.x, a.y, color);
        uint32_t v1 = addVertex(b.x, b.y, color);
        uint32_t v2 = addVertex(c.x, c.y, color);

        indices_.insert(indices_.end(), {v0, v1, v2});
        addCmd(offset, 3);
    }

    void DrawList::addTriangle(Vec2 a, Vec2 b, Vec2 c, const Color &color, float thickness)
    {
        addPolyline({a, b, c}, color, thickness, true);
    }

    void DrawList::addImage(TextureHandle texture, const Rect &rect, const Rect &uv, const Color &color)
    {
        if (!texture || rect.w <= 0.0f || rect.h <= 0.0f)
            return;

        uint32_t offset = static_cast<uint32_t>(indices_.size());
        uint32_t v0 = addVertex(rect.x, rect.y, uv.x, uv.y, color);
        uint32_t v1 = addVertex(rect.x + rect.w, rect.y, uv.x + uv.w, uv.y, color);
        uint32_t v2 = addVertex(rect.x + rect.w, rect.y + rect.h, uv.x + uv.w, uv.y + uv.h, color);
        uint32_t v3 = addVertex(rect.x, rect.y + rect.h, uv.x, uv.y + uv.h, color);

        indices_.insert(indices_.end(), {v0, v1, v2, v0, v2, v3});
        addCmd(offset, 6, texture);
    }

    void DrawList::addText(const Font &font, Vec2 pos, const Color &color, const char *text)
    {
        if (!font.texture() || !text)
            return;

        uint32_t offset = static_cast<uint32_t>(indices_.size());
        Vec2 pen = pos;

        while (*text)
        {
            uint32_t codepoint = decodeUtf8(text);
            if (codepoint == '\n')
            {
                pen.x = pos.x;
                pen.y += font.lineHeight();
                continue;
            }

            const FontGlyph *glyph = font.findGlyph(codepoint);
            if (!glyph)
                continue;

            if (glyph->size.x > 0.0f && glyph->size.y > 0.0f)
            {
                Rect rect{
                    pen.x + glyph->offset.x,
                    pen.y + glyph->offset.y,
                    glyph->size.x,
                    glyph->size.y};
                Rect uv = glyph->uv;

                uint32_t v0 = addVertex(rect.x, rect.y, uv.x, uv.y, color);
                uint32_t v1 = addVertex(rect.x + rect.w, rect.y, uv.x + uv.w, uv.y, color);
                uint32_t v2 = addVertex(rect.x + rect.w, rect.y + rect.h, uv.x + uv.w, uv.y + uv.h, color);
                uint32_t v3 = addVertex(rect.x, rect.y + rect.h, uv.x, uv.y + uv.h, color);
                indices_.insert(indices_.end(), {v0, v1, v2, v0, v2, v3});
            }

            pen.x += glyph->advanceX;
        }

        addCmd(offset, static_cast<uint32_t>(indices_.size()) - offset, font.texture());
    }

    Vec2 DrawList::calcTextSize(const Font &font, const char *text)
    {
        if (!text)
            return {0.0f, 0.0f};

        float maxLineWidth = 0.0f;
        float lineWidth = 0.0f;
        int lines = 1;
        const char *cursor = text;

        while (*cursor)
        {
            uint32_t codepoint = decodeUtf8(cursor);
            if (codepoint == '\n')
            {
                maxLineWidth = std::max(maxLineWidth, lineWidth);
                lineWidth = 0.0f;
                ++lines;
                continue;
            }

            const FontGlyph *glyph = font.findGlyph(codepoint);
            if (glyph)
                lineWidth += glyph->advanceX;
        }

        maxLineWidth = std::max(maxLineWidth, lineWidth);
        float height = font.lineHeight() * static_cast<float>(lines);

        return {maxLineWidth, height};
    }

    void DrawList::addTextAligned(const Font &font,
                                  const Rect &bounds,
                                  const Color &color,
                                  const char *text,
                                  AlignX ax,
                                  AlignY ay)
    {
        if (!font.texture() || !text)
            return;

        Vec2 size = calcTextSize(font, text);

        // -- resolve X --
        float x = bounds.x;
        switch (ax)
        {
        case AlignX::Center:
            x = bounds.x + (bounds.w - size.x) * 0.5f;
            break;
        case AlignX::Right:
            x = bounds.x + bounds.w - size.x;
            break;
        default:
            break; // Left
        }

        // -- resolve Y --
        float y = bounds.y;
        switch (ay)
        {
        case AlignY::Middle:
            y = bounds.y + (bounds.h - size.y) * 0.5f;
            break;
        case AlignY::Bottom:
            y = bounds.y + bounds.h - size.y;
            break;
        default:
            break; // Top
        }

        addText(font, {x, y}, color, text);
    }

    void DrawList::addIcon(IconId icon, const Rect &rect, const Color &color, float thickness)
    {
        float x0 = rect.x;
        float y0 = rect.y;
        float x1 = rect.x + rect.w;
        float y1 = rect.y + rect.h;
        float cx = rect.x + rect.w * 0.5f;
        float cy = rect.y + rect.h * 0.5f;
        float s = std::min(rect.w, rect.h);
        float p = s * 0.22f;

        switch (icon)
        {
        case IconId::Check:
            addLine({x0 + p, cy}, {cx - s * 0.08f, y1 - p}, color, thickness);
            addLine({cx - s * 0.08f, y1 - p}, {x1 - p, y0 + p}, color, thickness);
            break;

        case IconId::Cross:
            addLine({x0 + p, y0 + p}, {x1 - p, y1 - p}, color, thickness);
            addLine({x1 - p, y0 + p}, {x0 + p, y1 - p}, color, thickness);
            break;

        case IconId::Plus:
            addLine({cx, y0 + p}, {cx, y1 - p}, color, thickness);
            addLine({x0 + p, cy}, {x1 - p, cy}, color, thickness);
            break;

        case IconId::Minus:
            addLine({x0 + p, cy}, {x1 - p, cy}, color, thickness);
            break;

        case IconId::ArrowRight:
            addTriangleFilled({x1 - p, cy}, {x0 + p, y0 + p}, {x0 + p, y1 - p}, color);
            break;

        case IconId::ArrowDown:
            addTriangleFilled({cx, y1 - p}, {x0 + p, y0 + p}, {x1 - p, y0 + p}, color);
            break;

        case IconId::Search:
            addCircle({cx - s * 0.08f, cy - s * 0.08f}, s * 0.23f, color, thickness);
            addLine({cx + s * 0.12f, cy + s * 0.12f}, {x1 - p, y1 - p}, color, thickness);
            break;

        case IconId::None:
            break;
        }
    }

    void DrawList::addLine(Vec2 a, Vec2 b, const Color &color, float thickness)
    {
        thickness = std::max(1.0f, thickness);
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len <= 0.0001f)
            return;

        float nx = -dy / len * thickness * 0.5f;
        float ny = dx / len * thickness * 0.5f;

        uint32_t offset = static_cast<uint32_t>(indices_.size());
        uint32_t v0 = addVertex(a.x + nx, a.y + ny, color);
        uint32_t v1 = addVertex(b.x + nx, b.y + ny, color);
        uint32_t v2 = addVertex(b.x - nx, b.y - ny, color);
        uint32_t v3 = addVertex(a.x - nx, a.y - ny, color);

        indices_.insert(indices_.end(), {v0, v1, v2, v0, v2, v3});
        addCmd(offset, 6);
    }

    void DrawList::addConvexPolyFilled(const std::vector<Vec2> &points, const Color &color)
    {
        if (points.size() < 3)
            return;

        uint32_t offset = static_cast<uint32_t>(indices_.size());
        uint32_t firstVertex = static_cast<uint32_t>(vertices_.size());

        for (const Vec2 &p : points)
            addVertex(p.x, p.y, color);

        for (uint32_t i = 1; i + 1 < points.size(); ++i)
            indices_.insert(indices_.end(), {firstVertex, firstVertex + i, firstVertex + i + 1});

        addCmd(offset, static_cast<uint32_t>(indices_.size()) - offset);
    }

    void DrawList::addPolyline(const std::vector<Vec2> &points,
                               const Color &color, float thickness, bool closed)
    {
        if (points.size() < 2)
            return;
        thickness = std::max(1.0f, thickness);

        uint32_t offset = static_cast<uint32_t>(indices_.size());

        auto pushSegment = [&](Vec2 a, Vec2 b)
        {
            float dx = b.x - a.x, dy = b.y - a.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len <= 0.0001f)
                return;

            float nx = -dy / len * thickness * 0.5f;
            float ny = dx / len * thickness * 0.5f;

            uint32_t v0 = addVertex(a.x + nx, a.y + ny, color);
            uint32_t v1 = addVertex(b.x + nx, b.y + ny, color);
            uint32_t v2 = addVertex(b.x - nx, b.y - ny, color);
            uint32_t v3 = addVertex(a.x - nx, a.y - ny, color);
            indices_.insert(indices_.end(), {v0, v1, v2, v0, v2, v3});
        };

        for (size_t i = 0; i + 1 < points.size(); ++i)
            pushSegment(points[i], points[i + 1]);

        if (closed)
            pushSegment(points.back(), points.front());

        addCmd(offset, static_cast<uint32_t>(indices_.size()) - offset);
    }

    Rect DrawList::currentClip() const
    {
        if (!clipStack_.empty())
            return clipStack_.back();
        return {0.0f, 0.0f, 100000.0f, 100000.0f};
    }

    void DrawList::addCmd(uint32_t indexOffset, uint32_t indexCount,
                          TextureHandle texture)
    {
        if (indexCount == 0)
            return;

        Rect clip = currentClip();
        if (clip.w <= 0.0f || clip.h <= 0.0f)
            return;

        if (!commands_.empty())
        {
            DrawCmd &prev = commands_.back();
            const bool sameTexture = (prev.texture.value == texture.value);
            const bool sameClip = (prev.clip.x == clip.x &&
                                   prev.clip.y == clip.y &&
                                   prev.clip.w == clip.w &&
                                   prev.clip.h == clip.h);
            // Só faz merge se os índices forem mesmo contíguos
            const bool contiguous = (prev.indexOffset + prev.indexCount == indexOffset);

            if (sameTexture && sameClip && contiguous)
            {
                prev.indexCount += indexCount;
                return;
            }
        }

        commands_.push_back({texture, clip, indexOffset, indexCount});
    }
    uint32_t DrawList::addVertex(float x, float y, const Color &color)
    {
        return addVertex(x, y, 0.0f, 0.0f, color);
    }

    uint32_t DrawList::addVertex(float x, float y, float u, float v, const Color &color)
    {
        uint32_t index = static_cast<uint32_t>(vertices_.size());
        vertices_.push_back({x, y, u, v, color});
        return index;
    }

    Context *CreateContext()
    {
        return new Context();
    }

    void DestroyContext(Context *ctx)
    {
        if (gCurrentContext == ctx)
            gCurrentContext = nullptr;
        delete ctx;
    }

    void SetCurrentContext(Context *ctx)
    {
        gCurrentContext = ctx;
    }

    Context *GetCurrentContext()
    {
        return gCurrentContext;
    }

    IO &GetIO()
    {
        return current().io;
    }

    DrawList &GetDrawList()
    {
        return current().drawList;
    }

    void NewFrame()
    {
        auto &ctx = current();
        ctx.io.inputChars.clear();
        ctx.io.mouseWheelX = 0.0f;
        ctx.io.mouseWheelY = 0.0f;
        ctx.drawList.clear();
    }

    void Render()
    {
        auto &ctx = current();
        ctx.drawData.mainList = &ctx.drawList;
        ctx.drawData.displayWidth = ctx.io.displayWidth;
        ctx.drawData.displayHeight = ctx.io.displayHeight;
        ctx.drawData.framebufferScaleX = ctx.io.framebufferScaleX;
        ctx.drawData.framebufferScaleY = ctx.io.framebufferScaleY;
    }

    DrawData *GetDrawData()
    {
        return &current().drawData;
    }

} // namespace BuGUI
