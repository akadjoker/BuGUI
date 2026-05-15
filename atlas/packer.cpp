#include "packer.hpp"
#include <stb_rect_pack.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

// ─────────────────────────────────────────────────────────────────────────────
//  AtlasPacker — stb_rect_pack (Skyline Bottom-Left) + optional 90° rotation
//
//  Rotation strategy:
//    Pass 1: pack all sprites in natural orientation.
//    Pass 2: for each sprite that didn't fit, swap w/h (rotate 90°) and
//            repack ALL sprites from scratch with those new dimensions.
//    Repeat up to 2 times until stable (greedy, not optimal).
// ─────────────────────────────────────────────────────────────────────────────

void AtlasPacker::reset(const ShelfPackerConfig& cfg)
{
    cfg_ = cfg;
    entries_.clear();
}

void AtlasPacker::addEntry(const std::string& name, int w, int h)
{
    PackEntry e;
    e.name    = name;
    e.srcW    = w;
    e.srcH    = h;
    e.w       = w;
    e.h       = h;
    e.packed  = false;
    e.rotated = false;
    entries_.push_back(e);
}

void AtlasPacker::removeEntry(int idx)
{
    if (idx >= 0 && idx < (int)entries_.size())
        entries_.erase(entries_.begin() + idx);
}

void AtlasPacker::clear()
{
    entries_.clear();
}

// ── internal: run stb_rect_pack once, fill packed/x/y on entries ─────────
static void runPack(std::vector<PackEntry>& entries,
                    const ShelfPackerConfig& cfg)
{
    const int pad = cfg.padding;
    const int n   = (int)entries.size();
    if (n == 0) return;

    std::vector<stbrp_node> nodes(cfg.atlasW);
    stbrp_context ctx;
    stbrp_init_target(&ctx, cfg.atlasW, cfg.atlasH, nodes.data(), cfg.atlasW);

    std::vector<stbrp_rect> rects(n);
    for (int i = 0; i < n; ++i)
    {
        rects[i].id = i;
        rects[i].w  = (stbrp_coord)(entries[i].w + pad * 2);
        rects[i].h  = (stbrp_coord)(entries[i].h + pad * 2);
    }

    stbrp_pack_rects(&ctx, rects.data(), n);

    for (int i = 0; i < n; ++i)
    {
        auto& e  = entries[rects[i].id];
        e.packed = rects[i].was_packed != 0;
        if (e.packed)
        {
            e.x = rects[i].x + pad;
            e.y = rects[i].y + pad;
        }
    }
}

bool AtlasPacker::pack()
{
    for (auto& e : entries_)
    {
        e.packed  = false;
        e.rotated = false;
        e.w = e.srcW;
        e.h = e.srcH;
    }

    bool anyZero = false;
    for (const auto& e : entries_)
        if (e.srcW <= 0 || e.srcH <= 0) anyZero = true;

    if (!cfg_.allowRotate || entries_.empty())
    {
        runPack(entries_, cfg_);
        return !anyZero && packedCount() == (int)entries_.size();
    }

    // ── Rotation mode: up to 2 improvement passes ────────────────────────
    for (int pass = 0; pass < 2; ++pass)
    {
        for (auto& e : entries_) e.packed = false;
        runPack(entries_, cfg_);

        bool changed = false;
        for (auto& e : entries_)
        {
            if (!e.packed && e.srcW != e.srcH && e.srcW > 0 && e.srcH > 0)
            {
                std::swap(e.w, e.h);
                e.rotated = !e.rotated;
                changed   = true;
            }
        }
        if (!changed) break;

        // Repack with updated dimensions
        for (auto& e : entries_) e.packed = false;
        runPack(entries_, cfg_);
    }

    return !anyZero && packedCount() == (int)entries_.size();
}

int AtlasPacker::packedCount() const
{
    int n = 0;
    for (const auto& e : entries_)
        if (e.packed) ++n;
    return n;
}

int AtlasPacker::wastePercent() const
{
    if (entries_.empty()) return 0;
    int total = cfg_.atlasW * cfg_.atlasH;
    int used  = 0;
    for (const auto& e : entries_)
        if (e.packed) used += e.w * e.h;
    return 100 - (used * 100 / total);
}

std::string AtlasPacker::toJson() const
{
    std::string out;
    out.reserve(512 + entries_.size() * 128);
    out += "{\n  \"meta\": {\n";
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "    \"width\": %d,\n    \"height\": %d,\n    \"count\": %d\n  },\n",
        cfg_.atlasW, cfg_.atlasH, packedCount());
    out += buf;
    out += "  \"frames\": [\n";
    bool first = true;
    for (const auto& e : entries_)
    {
        if (!e.packed) continue;
        if (!first) out += ",\n";
        first = false;
        // rotated=true: sprite was placed 90° CW in the atlas.
        // Game engine reads srcW×srcH from atlas rect (x,y,w,h) rotated back.
        std::snprintf(buf, sizeof(buf),
            "    { \"name\": \"%s\","
            " \"x\": %d, \"y\": %d,"
            " \"w\": %d, \"h\": %d,"
            " \"sourceW\": %d, \"sourceH\": %d,"
            " \"rotated\": %s }",
            e.name.c_str(), e.x, e.y, e.w, e.h,
            e.srcW, e.srcH,
            e.rotated ? "true" : "false");
        out += buf;
    }
    out += "\n  ]\n}\n";
    return out;
}

