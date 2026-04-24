#include "StageLayout.hpp"
#include "StageCanvas.hpp"
#include "StageWidgets.hpp"
#include "StageScroll.hpp"
#include "StageCalc.hpp"
#include "StageGrid.hpp"
#include "StageBorder.hpp"
#include "StageLayouts2.hpp"
#include "StageTab.hpp"
#include "StageAnchor.hpp"
#include "StageInputs.hpp"
#include "StageDemo.hpp"
#include "StageTreePropColor.hpp"
#include "StageDataGrid.hpp"
#include "StageTreeGrid.hpp"
#include "StageFileDialog.hpp"
#include "StageHome.hpp"
#include "StageFakeBlender.hpp"
#include "StageFakeTiled.hpp"
#include "StageViewport.hpp"
#include "StageGizmo.hpp"
#include "StageEditorWidgets.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include <cstdio>

extern "C" const char *__lsan_default_suppressions()
{
    return "leak:libSDL2\n"
           "leak:SDL_DBus\n"
           "leak:libnvidia-glcore\n";
}


// ═════════════════════════════════════════════════════════════════════════════
//  Procedural textures
// ═════════════════════════════════════════════════════════════════════════════

static Texture* makeCheckerTexture()
{
    constexpr int W = 128, H = 128, TILE = 16;
    unsigned char px[W * H * 4];
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int i = (y * W + x) * 4;
            bool dark = ((x / TILE) + (y / TILE)) % 2 == 0;
            px[i + 0] = dark ?  40 : 220;
            px[i + 1] = dark ? 120 : 180;
            px[i + 2] = dark ? 200 :  50;
            px[i + 3] = 255;
        }
    }
    return CreateTextureFromRGBA("checker", W, H, px);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Stats overlay
// ═════════════════════════════════════════════════════════════════════════════

static void setupOverlay(WidgetApp& app)
{
    app.setOverlayCallback([](PaintContext& ctx) {
        auto& a = WidgetApp::instance();
        const auto& sf = a.fillBatch().stats();
        const auto& sl = a.lineBatch().stats();
        const auto& st = a.textBatch().stats();

        char buf[256];
        snprintf(buf, sizeof(buf),
                 "FPS %d  |  DC %d  Vtx %d  |  %s",
                 a.fps(),
                 sf.drawCalls + sl.drawCalls + st.drawCalls,
                 sf.vertices  + sl.vertices  + st.vertices,
                 a.currentStageName().c_str());

        ctx.font.SetFontSize(13.0f);
        ctx.font.SetColor(Color(100, 100, 105, 255));
        ctx.font.SetBatch(&ctx.text);
        ctx.font.Print(buf, 6.0f, 4.0f);
    });
}

// ═════════════════════════════════════════════════════════════════════════════
//  Entry point
// ═════════════════════════════════════════════════════════════════════════════

int main(int /*argc*/, char* /*argv*/[])
{
    auto& app = WidgetApp::instance();
    if (!app.init("BuGUI", 1024, 768,0))
        return 1;

    Texture* tex = makeCheckerTexture();
    DragState drag;

    app.setTransition(TransitionType::SlideLeft, 0.4f, EaseType::OutBack);

    buildLayoutStage(app);
    buildCanvasStage(app, tex, drag);
    buildWidgetsStage(app);
    buildScrollStage(app);
    buildCalcStage(app);
    buildGridStage(app);
    buildBorderStage(app);
    buildLayouts2Stage(app);
    buildTabStage(app);
    buildAnchorStage(app);
    buildInputsStage(app);
    buildDemoStage(app);
    buildTreePropColorStage(app);
    buildDataGridStage(app);
    buildTreeGridStage(app);
    buildFileDialogStage(app);
    buildHomeStage(app);
    buildFakeBlenderStage(app);
    buildFakeTiledStage(app);
    buildViewportStage(app);
    buildGizmoStage(app);
    buildEditorWidgetsStage(app);
    setupOverlay(app);

    app.setStage("home");

    int rc = app.run();
    delete tex;
    return rc;
}
