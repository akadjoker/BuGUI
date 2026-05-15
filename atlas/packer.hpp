#pragma once
#include <string>
#include <vector>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
//  Atlas packer — shelf-first bin packing
//
//    AtlasPacker packer;
//    packer.reset(1024, 1024);
//    for (auto& sprite : sprites)
//        packer.add(sprite.name, sprite.w, sprite.h);
//    packer.pack();          // fills .x/.y on each entry
//    auto json = packer.toJson();
// ─────────────────────────────────────────────────────────────────────────────

struct PackEntry
{
    std::string name;
    int         srcW   = 0;
    int         srcH   = 0;
    int         x      = 0;   // packed position (filled by pack())
    int         y      = 0;
    int         w      = 0;   // placed width  (may differ from srcW if rotated)
    int         h      = 0;   // placed height (may differ from srcH if rotated)
    bool        packed  = false;
    bool        rotated = false; // true = sprite placed rotated 90°
    uint32_t    previewColor = 0xFF'88'AA'CC; // ABGR placeholder colour
};

struct ShelfPackerConfig
{
    int  atlasW     = 1024;
    int  atlasH     = 1024;
    int  padding    = 1;    // pixels between sprites
    bool sortByH    = true; // sort tallest first for better packing
    bool allowRotate = false; // try 90° rotation for better fit
};

class AtlasPacker
{
public:
    void               reset(const ShelfPackerConfig& cfg);
    void               addEntry(const std::string& name, int w, int h);
    void               removeEntry(int idx);
    void               clear();
    bool               pack();   // returns true if all entries fit

    const std::vector<PackEntry>& entries() const { return entries_; }
    const ShelfPackerConfig&      config()  const { return cfg_; }
    void                          setConfig(const ShelfPackerConfig& c) { cfg_ = c; }

    int  packedCount()  const;
    int  wastePercent() const; // 0..100 — unused area %

    // Serialise to JSON string (nlohmann-compatible layout).
    // Implemented in atlas_stage.cpp when logic is added.
    std::string toJson() const;

private:
    std::vector<PackEntry> entries_;
    ShelfPackerConfig      cfg_;
};
