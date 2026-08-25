#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// RaylibBackend — pure BuGUI → Raylib/rlgl renderer.
//
// Intended use:
//   1. Create your Raylib window normally (InitWindow / SetTargetFPS …)
//   2. Call renderer.init()  — creates GPU shader + VAO/VBO/EBO
//   3. Each frame: BeginDrawing() → ClearBackground() → renderer.render(dd)
//                  → [your own Raylib draws] → EndDrawing()
//   4. Call renderer.shutdown() before CloseWindow()
//
// Window management, input polling and the main loop are entirely the
// caller's responsibility — this class only draws BuGUI DrawData.
// ─────────────────────────────────────────────────────────────────────────────
#include "BuGUI.hpp"
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

class RaylibBackend
{
public:
    // Call after InitWindow() — sets up GPU resources.
    bool init();
    // Call before CloseWindow() — releases GPU resources.
    void shutdown();

    // Call at the start of each frame (before BuGUI::NewFrame).
    // Updates BuGUI IO with Raylib's current display size, delta time and cursor.
    void newFrame();

    // Upload a BuGUI texture (e.g. font atlas) to the GPU.
    BuGUI::TextureHandle createTextureRGBA(int width, int height, const unsigned char* pixels);
    void destroyTexture(BuGUI::TextureHandle texture);

    // Render BuGUI draw data.  Must be called between BeginDrawing/EndDrawing.
    void render(const BuGUI::DrawData& drawData);

private:
    void renderDrawData(const BuGUI::DrawData& drawData);
    void createDeviceObjects();
    void destroyDeviceObjects();

    // GPU resources
    unsigned int shaderId_   = 0;
    int          mvpLoc_     = -1;
    int          tex0Loc_    = -1;
    unsigned int vaoId_      = 0;
    unsigned int vboId_      = 0;
    unsigned int eboId_      = 0;
    int          vboCapacity_= 0;
    int          eboCapacity_= 0;

    std::vector<BuGUI::DrawVertex> xVerts_;

    // Input state
    bool prevKeyDown_[512] = {};
};
