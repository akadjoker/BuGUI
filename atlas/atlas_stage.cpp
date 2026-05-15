// ─────────────────────────────────────────────────────────────────────────────
//  Atlas Studio — Sprite Atlas Generator
//
//  Layout:
//    ┌─ MenuBar ─────────────────────────────────────────────────────────────┐
//    ├─ Toolbar ─────────────────────────────────────────────────────────────┤
//    ├─ Splitter(H) ─────────────────────────────────────────────────────────┤
//    │  ┌─ Left (Splitter V) ──┐  ┌─ Center (Preview) ┐  ┌─ Right (Props) ─┐│
//    │  │  Sprites tab         │  │  Atlas canvas       │  │  PropertyGrid   ││
//    │  │   DataGrid           │  │  + zoom/pan info    │  │  Pack settings  ││
//    │  ├──────────────────────┤  │                     │  │  Sprite props   ││
//    │  │  Fonts tab           │  │                     │  │                 ││
//    │  │   FormLayout         │  └─────────────────────┘  └─────────────────┘│
//    │  └──────────────────────┘                                              │
//    ├─ StatusBar ───────────────────────────────────────────────────────────┤
//    └───────────────────────────────────────────────────────────────────────┘
// ─────────────────────────────────────────────────────────────────────────────

#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "ScrollWidgets.hpp"
#include "InputWidgets.hpp"
#include "TextInputWidgets.hpp"
#include "ComboBox.hpp"
#include "DataWidgets.hpp"
#include "MenuWidgets.hpp"
#include "TreePropertyColorWidgets.hpp"
#include "FileDialog.hpp"
#include "packer.hpp"

#include <stb_image.h>
#include <cstdio>
#include <string>
#include <vector>
#include <filesystem>

using namespace BuGUI;

// ─────────────────────────────────────────────────────────────────────────────
//  App state  (UI-only, no packing logic yet)
// ─────────────────────────────────────────────────────────────────────────────

static AtlasPacker  g_packer;
static DataGrid*    g_spriteGrid  = nullptr;
static PropertyGrid* g_packProps  = nullptr;
static PropertyGrid* g_spriteProps = nullptr;
static Label*       g_previewInfo = nullptr;   // centre panel info label
static Label*       g_zoomLbl     = nullptr;
static float        g_zoom        = 1.0f;

// PropertyGrid row indices for pack settings
static int g_rowW       = -1;
static int g_rowH       = -1;
static int g_rowPad     = -1;
static int g_rowSort    = -1;
static int g_rowRotate  = -1;
static int g_rowFmt     = -1;

// PropertyGrid row indices for selected sprite
static int g_spName     = -1;
static int g_spW        = -1;
static int g_spH        = -1;
static int g_spPacked   = -1;
static int g_spRotated  = -1;
static int g_spX        = -1;
static int g_spY        = -1;

// Font panel
static TextInput* g_fontPathInput = nullptr;

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers (stubs — logic added later)
// ─────────────────────────────────────────────────────────────────────────────

static void rebuildSpriteGrid()
{
    if (!g_spriteGrid) return;
    g_spriteGrid->clearRows();
    const auto& entries = g_packer.entries();
    for (int i = 0; i < (int)entries.size(); ++i)
    {
        const auto& e = entries[i];
        char posStr[32];
        if (e.packed)
            std::snprintf(posStr, sizeof(posStr), "%d, %d", e.x, e.y);
        else
            std::snprintf(posStr, sizeof(posStr), "—");

        char sizeStr[32];
        std::snprintf(sizeStr, sizeof(sizeStr), "%d × %d", e.srcW, e.srcH);

        g_spriteGrid->addRow({e.name, sizeStr, posStr,
                               e.packed ? (e.rotated ? "Yes (R)" : "Yes") : "No"});
    }
    WidgetApp::instance().requestLayout();
}

static void updateStatus()
{
    auto* sb = WidgetApp::instance().statusBar();
    if (!sb) return;
    const auto& cfg = g_packer.config();
    char buf[128];
    int n   = (int)g_packer.entries().size();
    int ok  = g_packer.packedCount();
    std::snprintf(buf, sizeof(buf),
        "Atlas %d × %d  |  Sprites: %d  |  Packed: %d  |  Waste: %d%%",
        cfg.atlasW, cfg.atlasH, n, ok, g_packer.wastePercent());
    sb->setText(buf);
}

static void updateZoomLabel()
{
    if (!g_zoomLbl) return;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.0f%%", g_zoom * 100.f);
    g_zoomLbl->setText(buf);
}

static void doPack()
{
    g_packer.pack();
    rebuildSpriteGrid();
    updateStatus();
}

// ── Load image dimensions via stb_image (header-only query) ─────────────
static bool queryImageSize(const std::string& path, int& w, int& h)
{
    int channels = 0;
    // stbi_info reads only the header — no pixel data allocated
    return stbi_info(path.c_str(), &w, &h, &channels) != 0;
}

static void doAddSpriteFile()
{
    auto* fd = WidgetApp::instance().addFloat<FileDialog>("Add Sprite");
    fd->setMode(FileDialog::Mode::OpenImage);
    fd->setMultiSelect(true);
    fd->accepted.connect([](const std::string& path) {
        namespace fs = std::filesystem;
        fs::path p(path);
        std::string name = p.stem().string();
        int w = 0, h = 0;
        queryImageSize(path, w, h);
        g_packer.addEntry(name, w, h);
        rebuildSpriteGrid();
        updateStatus();
    });
}

static void doAddFolder()
{
    auto* fd = WidgetApp::instance().addFloat<FileDialog>("Add Sprites from Folder");
    fd->setMode(FileDialog::Mode::SelectFolder);
    fd->accepted.connect([](const std::string& folderPath) {
        namespace fs = std::filesystem;
        static const std::vector<std::string> kImageExts =
            { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif", ".webp" };
        int added = 0;
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(folderPath, ec))
        {
            if (!entry.is_regular_file(ec)) continue;
            std::string ext = entry.path().extension().string();
            // lowercase ext
            for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
            bool isImg = false;
            for (const auto& e : kImageExts) isImg = isImg || (ext == e);
            if (!isImg) continue;
            std::string name = entry.path().stem().string();
            int w = 0, h = 0;
            queryImageSize(entry.path().string(), w, h);
            g_packer.addEntry(name, w, h);
            ++added;
        }
        if (added > 0)
        {
            rebuildSpriteGrid();
            updateStatus();
        }
    });
}

static void doRemoveSelected()
{
    if (!g_spriteGrid) return;
    int sel = g_spriteGrid->selectedRow();
    if (sel < 0) return;
    g_packer.removeEntry(sel);
    rebuildSpriteGrid();
    updateStatus();
}

static void doClear()
{
    g_packer.clear();
    rebuildSpriteGrid();
    updateStatus();
}

static void doExportJson()
{
    auto* fd = WidgetApp::instance().addFloat<FileDialog>("Export JSON");
    fd->setMode(FileDialog::Mode::Save);
    fd->setFilter("*.json");
    fd->accepted.connect([](const std::string& path) {
        std::string json = g_packer.toJson();
        std::string outPath = path;
        // Ensure .json extension
        if (outPath.size() < 5 || outPath.substr(outPath.size()-5) != ".json")
            outPath += ".json";
        FILE* f = std::fopen(outPath.c_str(), "w");
        if (f) { std::fwrite(json.data(), 1, json.size(), f); std::fclose(f); }
    });
}

static void onSpriteSelected(int row)
{
    if (!g_spriteProps) return;
    const auto& entries = g_packer.entries();
    if (row < 0 || row >= (int)entries.size()) return;
    const auto& e = entries[row];
    g_spriteProps->setString(g_spName,   e.name);
    g_spriteProps->setInt   (g_spW,      e.srcW);
    g_spriteProps->setInt   (g_spH,      e.srcH);
    g_spriteProps->setBool  (g_spPacked, e.packed);
    g_spriteProps->setBool  (g_spRotated, e.rotated);
    g_spriteProps->setInt   (g_spX,      e.x);
    g_spriteProps->setInt   (g_spY,      e.y);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Panel builders
// ─────────────────────────────────────────────────────────────────────────────

static Widget* buildSpritesPanel(Widget* parent)
{
    auto* col = parent->createChild<BoxLayout>(LayoutDir::Vertical);
    col->setSpacing(0);
    col->setStretch(1.f);

    // Mini toolbar above the grid
    auto* bar = col->createChild<BoxLayout>(LayoutDir::Horizontal);
    bar->setSpacing(4);
    bar->setMargins(4, 4, 2, 4);
    bar->setSize(0, 30);

    auto* btnAdd = bar->createChild<Button>("+ File");
    btnAdd->setSize(54, 22);
    btnAdd->setTooltip("Add image file(s)");
    btnAdd->clicked.connect([](){ doAddSpriteFile(); });

    auto* btnFolder = bar->createChild<Button>("+ Folder");
    btnFolder->setSize(68, 22);
    btnFolder->setTooltip("Add all images from a folder");
    btnFolder->clicked.connect([](){ doAddFolder(); });

    auto* btnRem = bar->createChild<Button>("Remove");
    btnRem->setSize(60, 22);
    btnRem->clicked.connect([](){ doRemoveSelected(); });

    auto* btnClear = bar->createChild<Button>("Clear");
    btnClear->setSize(54, 22);
    btnClear->clicked.connect([](){ doClear(); });

    // Sprite DataGrid
    g_spriteGrid = col->createChild<DataGrid>();
    g_spriteGrid->setStretch(1.f);
    g_spriteGrid->addColumn("Name",   140, false);
    g_spriteGrid->addColumn("Size",    80, false);
    g_spriteGrid->addColumn("Pos",     80, false);
    g_spriteGrid->addColumn("Packed",  60, false);
    g_spriteGrid->selectionChanged.connect([](int row){ onSpriteSelected(row); });

    return col;
}

static void doOpenFontDialog()
{
    auto* fd = WidgetApp::instance().addFloat<FileDialog>("Select Font");
    fd->setMode(FileDialog::Mode::Open);
    fd->setFilter("*.ttf;*.otf;*.ttc");
    fd->accepted.connect([](const std::string& path) {
        if (g_fontPathInput)
            g_fontPathInput->setText(path);
    });
}

static Widget* buildFontsPanel(Widget* parent)
{
    auto* col = parent->createChild<BoxLayout>(LayoutDir::Vertical);
    col->setStretch(1.f);
    col->setSpacing(0);

    auto* sv = col->createChild<ScrollView>();
    sv->setStretch(1.f);
    auto* form = sv->setContent<FormLayout>();
    form->setLabelWidth(100);
    form->setSpacing(8, 6);
    form->setPadding(Edges(8, 8, 4, 8));

    // Font path row: label + [read-only input + browse button] in a BoxLayout
    // FormLayout doesn't support multi-widget rows, so inject manually:
    // FormLayout children alternate label / widget — add a Label then a BoxLayout
    form->createChild<Label>("Font Path");
    auto* pathRow = form->createChild<BoxLayout>(LayoutDir::Horizontal);
    pathRow->setSpacing(4);
    g_fontPathInput = pathRow->createChild<TextInput>("", TextInput::Mode::ReadOnly);
    g_fontPathInput->setStretch(1.f);
    g_fontPathInput->setPlaceholder("path/to/font.ttf");
    auto* btnBrowse = pathRow->createChild<Button>("…");
    btnBrowse->setSize(28, 22);
    btnBrowse->clicked.connect([](){ doOpenFontDialog(); });

    form->addRow<SpinBox>("Size (px)")->setValue(16.f);
    form->addRow<TextInput>("Characters")->setPlaceholder("32..126 (ASCII)");
    form->addRow<CheckBox>("Bold")->setText("Bold");
    form->addRow<CheckBox>("Antialias")->setText("On");

    auto* btnGen = col->createChild<Button>("Generate Font Atlas");
    btnGen->setMargins(8);
    btnGen->clicked.connect([](){ /* TODO */ });

    return col;
}

static Widget* buildLeftPanel(Widget* parent)
{
    auto* tabs = parent->createChild<TabLayout>();
    tabs->setStretch(1.f);

    auto* spritesHost = new BoxLayout(LayoutDir::Vertical);
    buildSpritesPanel(spritesHost);
    tabs->addTabWidget("Sprites", spritesHost);

    auto* fontsHost = new BoxLayout(LayoutDir::Vertical);
    buildFontsPanel(fontsHost);
    tabs->addTabWidget("Fonts", fontsHost);

    return tabs;
}

static Widget* buildPreviewPanel(Widget* parent)
{
    auto* col = parent->createChild<BoxLayout>(LayoutDir::Vertical);
    col->setStretch(1.f);
    col->setSpacing(0);

    // Zoom toolbar
    auto* zoomBar = col->createChild<BoxLayout>(LayoutDir::Horizontal);
    zoomBar->setSpacing(4);
    zoomBar->setMargins(4, 4, 4, 4);
    zoomBar->setSize(0, 30);

    auto* btnZoomIn  = zoomBar->createChild<Button>("+");
    btnZoomIn->setSize(26, 22);
    btnZoomIn->clicked.connect([](){ g_zoom = std::min(8.f, g_zoom * 1.25f); updateZoomLabel(); });

    auto* btnZoomOut = zoomBar->createChild<Button>("−");
    btnZoomOut->setSize(26, 22);
    btnZoomOut->clicked.connect([](){ g_zoom = std::max(0.1f, g_zoom / 1.25f); updateZoomLabel(); });

    auto* btnFit = zoomBar->createChild<Button>("Fit");
    btnFit->setSize(36, 22);
    btnFit->clicked.connect([](){ g_zoom = 1.f; updateZoomLabel(); });

    g_zoomLbl = zoomBar->createChild<Label>("100%");
    g_zoomLbl->setSize(50, 22);

    // Preview canvas (placeholder — painted manually via overlay in the future)
    auto* canvas = col->createChild<BoxLayout>(LayoutDir::Vertical);
    canvas->setStretch(1.f);
    canvas->setMargins(8);

    g_previewInfo = canvas->createChild<Label>(
        "No atlas packed yet.\n\nAdd sprites and click  Pack  to see the result.");
    g_previewInfo->setStretch(1.f);

    return col;
}

static Widget* buildRightPanel(Widget* parent)
{
    auto* col = parent->createChild<BoxLayout>(LayoutDir::Vertical);
    col->setSpacing(0);

    // ── Pack Settings ──────────────────────────────────────────────────────
    auto* lblPack = col->createChild<Label>("  Pack Settings");
    lblPack->setSize(0, 24);

    auto* svPack = col->createChild<ScrollView>();
    svPack->setSize(0, 200);

    g_packProps = svPack->setContent<PropertyGrid>();

    g_rowW    = g_packProps->addInt  ("Atlas W",  1024, 64, 8192,
                    [](int v){ auto c = g_packer.config(); c.atlasW = v; g_packer.setConfig(c); });
    g_rowH    = g_packProps->addInt  ("Atlas H",  1024, 64, 8192,
                    [](int v){ auto c = g_packer.config(); c.atlasH = v; g_packer.setConfig(c); });
    g_rowPad  = g_packProps->addInt  ("Padding",     1,  0,   16,
                    [](int v){ auto c = g_packer.config(); c.padding = v; g_packer.setConfig(c); });
    g_rowSort = g_packProps->addBool ("Sort by H", true,
                    [](bool v){ auto c = g_packer.config(); c.sortByH = v; g_packer.setConfig(c); });
    g_rowRotate = g_packProps->addBool("Allow Rotate 90°", false,
                    [](bool v){ auto c = g_packer.config(); c.allowRotate = v; g_packer.setConfig(c); });
    g_rowFmt  = g_packProps->addCombo("Format",
                    {"RGBA8", "RGB8", "R8"},
                    0, [](int i){ /* TODO */ });
    g_packProps->addSeparator();
    g_packProps->addButton("Pack Now", [](){ doPack(); });
    g_packProps->addButton("Export JSON…", [](){ doExportJson(); });

    // ── Selected Sprite ────────────────────────────────────────────────────
    auto* lblSp = col->createChild<Label>("  Selected Sprite");
    lblSp->setSize(0, 24);

    auto* svSp = col->createChild<ScrollView>();
    svSp->setStretch(1.f);

    g_spriteProps = svSp->setContent<PropertyGrid>();

    g_spName    = g_spriteProps->addString("Name",    "—");
    g_spW       = g_spriteProps->addInt   ("Width",     0, 0, 8192);
    g_spH       = g_spriteProps->addInt   ("Height",    0, 0, 8192);
    g_spPacked  = g_spriteProps->addBool  ("Packed",  false);
    g_spRotated = g_spriteProps->addBool  ("Rotated 90°", false);
    g_spX       = g_spriteProps->addInt   ("Atlas X",   0, 0, 8192);
    g_spY       = g_spriteProps->addInt   ("Atlas Y",   0, 0, 8192);

    // make read-only (PropertyGrid has no setColumnReadOnly — values are display-only)
    // g_spriteProps fields are non-editable by design

    return col;
}

// ─────────────────────────────────────────────────────────────────────────────
//  MenuBar builder
// ─────────────────────────────────────────────────────────────────────────────

static void buildMenuBar(Widget* root)
{
    auto* mb = root->createChild<MenuBar>();
    mb->setSize(0, 26);

    // File
    auto* file = mb->addMenu("File");
    file->addAction("New Atlas",          [](){ doClear(); });
    file->addSeparator();
    file->addAction("Add Sprites…",       [](){ doAddSpriteFile(); });
    file->addAction("Add Folder…",         [](){ doAddFolder(); });
    file->addSeparator();
    file->addAction("Export JSON…",       [](){ doExportJson(); });
    file->addSeparator();
    file->addAction("Quit", [](){
        // TODO: CloseWindow equivalent via app
    });

    // Edit
    auto* edit = mb->addMenu("Edit");
    edit->addAction("Remove Selected",    [](){ doRemoveSelected(); });
    edit->addAction("Clear All",          [](){ doClear(); });

    // Pack
    auto* pack = mb->addMenu("Pack");
    pack->addAction("Pack Now",           [](){ doPack(); });
    pack->addSeparator();
    auto* pow2 = pack->addCheckable("Power-of-two sizes", false);
    (void)pow2;
    auto* trim = pack->addCheckable("Trim alpha",          false);
    (void)trim;

    // View
    auto* view = mb->addMenu("View");
    view->addAction("Zoom In",   [](){ g_zoom = std::min(8.f, g_zoom * 1.25f); updateZoomLabel(); });
    view->addAction("Zoom Out",  [](){ g_zoom = std::max(0.1f, g_zoom / 1.25f); updateZoomLabel(); });
    view->addAction("Fit 100%",  [](){ g_zoom = 1.f; updateZoomLabel(); });
}

// ─────────────────────────────────────────────────────────────────────────────
//  Stage registration
// ─────────────────────────────────────────────────────────────────────────────

void registerAtlasStage(WidgetApp& app)
{
    // Reset global state
    g_packer        = AtlasPacker{};
    g_spriteGrid    = nullptr;
    g_packProps     = nullptr;
    g_spriteProps   = nullptr;
    g_previewInfo   = nullptr;
    g_zoomLbl       = nullptr;
    g_fontPathInput = nullptr;
    g_zoom          = 1.f;
    g_rowW = g_rowH = g_rowPad = g_rowSort = g_rowRotate = g_rowFmt = -1;
    g_spName = g_spW = g_spH = g_spPacked = g_spRotated = g_spX = g_spY = -1;

    // Initialise packer config defaults
    g_packer.reset({1024, 1024, 1, true});

    auto* root = app.addStage("atlas");

    // ── Root vertical layout ───────────────────────────────────────────
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(0);

    buildMenuBar(vbox);

    // Toolbar
    auto* tb = vbox->createChild<Toolbar>();
    tb->setSize(0, 34);
    tb->addButton("Add Sprites",  IconButton::Plus,  [](){ doAddSpriteFile(); });
    tb->addButton("Add Folder",   IconButton::Plus,  [](){ doAddFolder(); });
    tb->addButton("Remove",       IconButton::Minus, [](){ doRemoveSelected(); });
    tb->addSeparator();
    tb->addButton("Pack",         IconButton::Check, [](){ doPack(); });
    tb->addButton("Export JSON",  IconButton::Check, [](){ doExportJson(); });
    tb->addSeparator();
    tb->addButton("Clear",        IconButton::Close, [](){ doClear(); });

    // ── Main area: outer horizontal splitter ───────────────────────────
    //   Left (panel): ~22%   Centre (preview): flex   Right (props): ~28%
    auto* hSplit = vbox->createChild<Splitter>(LayoutDir::Horizontal);
    hSplit->setStretch(1.f);
    hSplit->setRatio(0.22f);
    hSplit->setMinRatio(0.12f);
    hSplit->setMaxRatio(0.45f);

    buildLeftPanel(hSplit);

    // Right side: another splitter for centre + props
    auto* rSplit = hSplit->createChild<Splitter>(LayoutDir::Horizontal);
    rSplit->setStretch(1.f);
    rSplit->setRatio(0.72f);
    rSplit->setMinRatio(0.40f);
    rSplit->setMaxRatio(0.88f);

    buildPreviewPanel(rSplit);
    buildRightPanel(rSplit);

    // ── Status bar ─────────────────────────────────────────────────────
    app.setShowStatusBar(true);
    updateStatus();
}
