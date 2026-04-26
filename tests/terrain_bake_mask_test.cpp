#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

struct Color
{
    unsigned char r, g, b, a;
};

static Color mix(Color a, Color b, float t)
{
    t = std::max(0.0f, std::min(1.0f, t));
    auto lerp = [t](unsigned char x, unsigned char y) -> unsigned char {
        return static_cast<unsigned char>(x + (y - x) * t);
    };
    return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), 255};
}

static Color sampleRepeat(const std::vector<Color>& img, int w, int h, float u, float v)
{
    u = u - std::floor(u);
    v = v - std::floor(v);
    int x = std::min(w - 1, std::max(0, static_cast<int>(u * w)));
    int y = std::min(h - 1, std::max(0, static_cast<int>(v * h)));
    return img[y * w + x];
}

static bool writePPM(const char* path, const std::vector<Color>& img, int w, int h)
{
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    std::fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (const Color& c : img)
    {
        std::fputc(c.r, f);
        std::fputc(c.g, f);
        std::fputc(c.b, f);
    }
    std::fclose(f);
    return true;
}

int main()
{
    constexpr int detailW = 64;
    constexpr int detailH = 64;
    constexpr int bakedW = 512;
    constexpr int bakedH = 512;
    constexpr float tileCount = 12.0f;

    std::vector<Color> detail(detailW * detailH);
    for (int y = 0; y < detailH; ++y)
    {
        for (int x = 0; x < detailW; ++x)
        {
            bool checker = ((x / 8) + (y / 8)) % 2 == 0;
            detail[y * detailW + x] = checker ? Color{55, 130, 52, 255}
                                              : Color{38, 92, 36, 255};
        }
    }

    std::vector<float> mask(bakedW * bakedH, 0.0f);
    const float brushU = 0.28f;
    const float brushV = 0.62f;
    const float brushRadius = 0.115f;

    for (int y = 0; y < bakedH; ++y)
    {
        for (int x = 0; x < bakedW; ++x)
        {
            float u = (x + 0.5f) / bakedW;
            float v = (y + 0.5f) / bakedH;
            float dx = u - brushU;
            float dy = v - brushV;
            float d = std::sqrt(dx * dx + dy * dy);
            float t = 1.0f - d / brushRadius;
            if (t > 0.0f)
                mask[y * bakedW + x] = std::min(1.0f, t);
        }
    }

    std::vector<Color> baked(bakedW * bakedH);
    Color paintColor{175, 98, 45, 255};

    for (int y = 0; y < bakedH; ++y)
    {
        for (int x = 0; x < bakedW; ++x)
        {
            float worldU = (x + 0.5f) / bakedW;
            float worldV = (y + 0.5f) / bakedH;
            Color base = sampleRepeat(detail, detailW, detailH, worldU * tileCount, worldV * tileCount);
            float m = mask[y * bakedW + x];
            baked[y * bakedW + x] = mix(base, paintColor, m);
        }
    }

    bool ok = true;
    ok = writePPM("/tmp/bugui_terrain_detail_tiled.ppm", detail, detailW, detailH) && ok;
    ok = writePPM("/tmp/bugui_terrain_baked_global_mask.ppm", baked, bakedW, bakedH) && ok;
    return ok ? 0 : 1;
}
