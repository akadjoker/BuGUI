#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "ThumbnailGrid.hpp"
#include <cstdlib>

void registerThumbnailStage(WidgetApp& app)
{
    auto* root = app.addStage("thumbnail");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(4);
    outer->setPadding(8);

    // Header
    auto* header = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSize(0, 32);
    auto* back = header->createChild<Button>("< Back");
    back->setSize(70, 26);
    back->clicked.connect([&app]() { app.setStage("menu", TransitionType::CoverRight); });
    header->createChild<Label>("Thumbnail Grid — click to select");

    // Info label
    auto* info = outer->createChild<Label>("Selected: (none)");

    // ThumbnailGrid
    auto* grid = outer->createChild<ThumbnailGrid>();
    grid->setStretch(1);
    grid->setThumbSize(96, 72);
    grid->setSpacing(10);

    // Add some demo items with varied colours
    const char* names[] = {
        "scene_001.png", "asset_tree.png", "character.png", "env_sunset.png",
        "hdr_sky_01.png", "material_01.png", "texture_diff.png", "texture_norm.png",
        "fx_particle.png", "model_car.png", "model_house.png", "lightmap.png",
        "anim_walk.png", "anim_run.png", "anim_idle.png", "cubemap.png",
        "icon_play.png", "icon_stop.png", "icon_save.png", "icon_load.png",
    };
    for (int i = 0; i < 20; ++i) {
        uint8_t r = 40 + (std::rand() % 150);
        uint8_t g = 40 + (std::rand() % 150);
        uint8_t b = 40 + (std::rand() % 150);
        grid->addItem(names[i], Color(r, g, b, 255));
    }

    grid->onItemClicked.connect([info, grid](int idx) {
        if (idx >= 0 && idx < grid->itemCount()) {
            std::string text = "Selected: " + grid->item(idx).label + " (#" + std::to_string(idx) + ")";
            info->setText(text);
        }
    });
}
