#pragma once

#include "StageCommon.hpp"
#include "Batch.hpp"
#include "Font.hpp"
#include "Texture.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  Demo stage — persistent shell with PageView for content
//    Menu bar + drawer + status bar stay fixed
//    Content area swaps via PageView (like Android Fragments)
// ═════════════════════════════════════════════════════════════════════════════

// ── Helper: build the main 4-column SDL_GUI layout ───────────────────────
static Widget* buildMainPage(PageView* pages)
{
    auto* content = pages->addPage<BoxLayout>(LayoutDir::Horizontal);
    pages->setPageName(0, "Main");
    content->setSpacing(0);
    content->setPadding(5, 5, 5, 5);

    // ── COLUMN 1 — 240px ─────────────────────────────────────────────────
    auto* col1 = content->createChild<BoxLayout>(LayoutDir::Vertical);
    col1->setSize(240, 0);
    col1->setStretch(0);
    col1->setSpacing(0);
    col1->setPadding(0, 5, 5, 5);

    auto* label1 = col1->createChild<Label>("Label 1");
    label1->setMargins(10, 0, 0, 0);

    auto* list = col1->createChild<ListBox>();
    list->setStretch(1);
    list->setMargins(10, 0, 0, 0);
    for (int i = 1; i <= 20; i++)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "Simple List Item %d", i);
        list->addItem(buf);
    }

    auto* btn1 = col1->createChild<Button>("Button 1");
    btn1->setMargins(10, 0, 0, 0);

    auto* label2 = col1->createChild<Label>("Label 2");
    label2->setMargins(10, 0, 0, 0);
    label2->setColor(Color(180, 180, 180, 255));

    static RadioGroup demoRadioGroup;
    auto* radioPanel = col1->createChild<Panel>();
    radioPanel->setMargins(10, 0, 0, 0);
    auto* radioBox = radioPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    radioBox->setSpacing(2);
    radioBox->setPadding(5, 10, 5, 10);
    auto* rb1 = radioBox->createChild<RadioButton>("Radio 1", &demoRadioGroup);
    rb1->setSelected(true);
    radioBox->createChild<RadioButton>("Radio 2", &demoRadioGroup);
    radioBox->createChild<RadioButton>("Radio 3", &demoRadioGroup);

    list->selectionChanged.connect([label1, list](int idx){
        if (idx >= 0 && idx < list->itemCount())
            label1->setText(list->itemText(idx));
    });
    demoRadioGroup.selectionChanged.connect([label2](int idx){
        char buf[16];
        snprintf(buf, sizeof(buf), "Radio %d", idx + 1);
        label2->setText(buf);
    });

    // ── COLUMN 2 — 200px ─────────────────────────────────────────────────
    auto* col2 = content->createChild<BoxLayout>(LayoutDir::Vertical);
    col2->setSize(200, 0);
    col2->setStretch(0);
    col2->setSpacing(0);
    col2->setPadding(0, 5, 5, 5);

    auto* cbPanel = col2->createChild<Panel>();
    auto* cbBox = cbPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    cbBox->setSpacing(2);
    cbBox->setPadding(5, 10, 5, 10);
    cbBox->createChild<CheckBox>("Check Box 1");
    auto* cb2 = cbBox->createChild<CheckBox>("Check Box 1");
    cb2->setChecked(true);
    cbBox->createChild<CheckBox>("Check Mix 1");

    auto* iconRow1 = col2->createChild<BoxLayout>(LayoutDir::Horizontal);
    iconRow1->setMargins(10, 0, 0, 0);
    iconRow1->setSpacing(5);
    iconRow1->setMainAlign(MainAlign::Center);
    iconRow1->createChild<Button>("$")->setSize(36, 30);
    iconRow1->createChild<Button>("\xc2\xa5")->setSize(36, 30);
    iconRow1->createChild<Button>("\xc2\xa3")->setSize(36, 30);

    auto* imgPlaceholder = col2->createChild<Canvas>();
    imgPlaceholder->setStretch(1);
    imgPlaceholder->setMargins(10, 0, 0, 0);
    imgPlaceholder->setBgColor(Color(60, 60, 65, 255));

    // Context menu on the canvas (right-click)
    static Menu canvasCtxMenu;
    static bool ctxInited = false;
    if (!ctxInited) {
        ctxInited = true;
        canvasCtxMenu.addAction("Zoom In",   []{ }, "Ctrl++");
        canvasCtxMenu.addAction("Zoom Out",  []{ }, "Ctrl+-");
        canvasCtxMenu.addAction("Fit to Window", []{ });
        canvasCtxMenu.addSeparator();
        canvasCtxMenu.addCheckable("Show Grid", true, [](bool){ });
        canvasCtxMenu.addCheckable("Snap to Grid", false, [](bool){ });
        canvasCtxMenu.addSeparator();
        auto* exportSub = canvasCtxMenu.addSubmenu("Export As...");
        exportSub->addAction("PNG",  []{ });
        exportSub->addAction("JPEG", []{ });
        exportSub->addAction("SVG",  []{ });
        canvasCtxMenu.addSeparator();
        canvasCtxMenu.addAction("Properties...", []{ });
    }
    imgPlaceholder->setContextMenu(&canvasCtxMenu);

    imgPlaceholder->setOnPaint([](PaintContext& ctx, const Rect& b) {
        ctx.font.SetFontSize(13.0f);
        ctx.font.SetColor(Color(120, 120, 130, 255));
        ctx.font.SetBatch(&ctx.text);
        ctx.font.Print("(image placeholder)", b.x + 10, b.y + b.h * 0.5f - 7);
    });

    auto* iconRow2 = col2->createChild<BoxLayout>(LayoutDir::Horizontal);
    iconRow2->setMargins(10, 0, 0, 0);
    iconRow2->setSpacing(5);
    iconRow2->createChild<Button>("$")->setSize(36, 30);
    iconRow2->createChild<Button>("\xc2\xa5")->setSize(36, 30);
    iconRow2->createChild<Button>("\xc2\xa3")->setSize(36, 30);

    auto* testPopupBtn = col2->createChild<Button>("Test Popup");
    testPopupBtn->setMargins(10, 0, 0, 0);
    testPopupBtn->clicked.connect([]{
        auto* dlg = new ConfirmDialog(
            "Confirm Action",
            "Are you sure you want to proceed?\nThis action cannot be undone.",
            "Confirm", "Cancel");
        dlg->confirmed.connect([]{
            // Action confirmed — nothing to do in demo
        });
        WidgetApp::instance().showPopup(dlg);
    });

    // ── COLUMN 3 — fill ──────────────────────────────────────────────────
    auto* col3 = content->createChild<Panel>();
    col3->setStretch(1);
    col3->setMargins(5, 5, 5, 5);

    auto* col3Inner = col3->createChild<BoxLayout>(LayoutDir::Vertical);
    col3Inner->setSpacing(0);
    col3Inner->setPadding(5);

    auto* topRow = col3Inner->createChild<BoxLayout>(LayoutDir::Horizontal);
    topRow->setSpacing(10);
    topRow->setPadding(5);

    auto* leftSub = topRow->createChild<BoxLayout>(LayoutDir::Vertical);
    leftSub->setSize(200, 0);
    leftSub->setStretch(0);
    leftSub->setSpacing(6);

    auto* progress = leftSub->createChild<ProgressBar>(0.0f, 100.0f, 25.0f);
    progress->setMargins(0, 10, 0, 10);
    auto* sliderLabel = leftSub->createChild<Label>("25 %");
    sliderLabel->setMargins(0, 10, 0, 10);
    sliderLabel->setColor(Color(180, 180, 180, 255));
    auto* slider = leftSub->createChild<Slider>();
    slider->setRange(0, 100);
    slider->setValue(25);
    slider->setMargins(0, 2, 0, 2);
    slider->onValueChanged.connect([sliderLabel, progress](float val){
        char buf[32];
        snprintf(buf, sizeof(buf), "%d %%", (int)val);
        sliderLabel->setText(buf);
        progress->setValue(val);
    });

    auto* rightSub = topRow->createChild<BoxLayout>(LayoutDir::Vertical);
    rightSub->setStretch(1);
    rightSub->setSpacing(6);
    rightSub->setMargins(0, 10, 0, 20);

    auto* switchRow = rightSub->createChild<BoxLayout>(LayoutDir::Horizontal);
    switchRow->setSpacing(4);
    switchRow->createChild<Label>("On/Off Switch:");
    switchRow->createChild<Spacer>();
    switchRow->createChild<Switch>("");

    auto* combo = rightSub->createChild<ComboBox>();
    combo->setMargins(10, 0, 0, 0);
    for (int i = 1; i <= 7; i++)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "Combo Box Item #%d", i);
        combo->addItem(buf);
    }

    auto* txtNormal = rightSub->createChild<TextInput>("Hello, BuGUI!");
    txtNormal->setMargins(10, 0, 0, 0);
    txtNormal->setPlaceholder("Type here...");

    auto* txtPass = rightSub->createChild<TextInput>("secret", TextInput::Mode::Password);
    txtPass->setMargins(4, 0, 0, 0);
    txtPass->setPlaceholder("Password");

    auto* txtNum = rightSub->createChild<TextInput>("42", TextInput::Mode::NumberOnly);
    txtNum->setMargins(4, 0, 0, 0);
    txtNum->setPlaceholder("Number only");

    auto* txtRO = rightSub->createChild<TextInput>("Read-only text", TextInput::Mode::ReadOnly);
    txtRO->setMargins(4, 0, 0, 0);

    // ── COLUMN 4 — small ─────────────────────────────────────────────────
    auto* col4 = content->createChild<Panel>();
    col4->setSize(60, 0);
    col4->setStretch(0);
    col4->setMargins(5, 5, 5, 5);
    auto* col4Inner = col4->createChild<BoxLayout>(LayoutDir::Vertical);
    col4Inner->setSpacing(5);
    col4Inner->setPadding(5);
    col4Inner->createChild<Button>("$")->setSize(40, 32);
    col4Inner->createChild<Button>("\xc2\xa5")->setSize(40, 32);
    col4Inner->createChild<Button>("\xc2\xa3")->setSize(40, 32);

    return content;
}

// ── Helper: build the font atlas page ────────────────────────────────────
static Widget* buildAtlasPage(PageView* pages, WidgetApp& app)
{
    auto* page = pages->addPage<BoxLayout>(LayoutDir::Vertical);
    pages->setPageName(pages->pageCount() - 1, "Font Atlas");
    page->setSpacing(0);
    page->setPadding(0);

    auto* canvas = page->createChild<Canvas>();
    canvas->setStretch(1);
    canvas->setBgColor(Color(20, 20, 25, 255));

    canvas->setOnPaint([&app](PaintContext& ctx, const Rect& bounds) {
        Texture* atlas = app.font().GetTexture();
        if (!atlas) return;

        float texW = static_cast<float>(atlas->width);
        float texH = static_cast<float>(atlas->height);
        float padW = 16.0f, padH = 40.0f;
        float maxW = bounds.w - padW;
        float maxH = bounds.h - padH;
        float scale = std::min(maxW / texW, maxH / texH);
        float drawW = texW * scale;
        float drawH = texH * scale;
        float dx = bounds.x + (bounds.w - drawW) * 0.5f;
        float dy = bounds.y + (bounds.h - drawH - 24) * 0.5f;

        ctx.fill.SetColor(255, 255, 255, 255);
        float clip[4] = {bounds.x, bounds.y, bounds.w, bounds.h};
        ctx.fill.DrawImageEx(atlas, dx, dy, drawW, drawH, 0.0f, 0.0f, 0.0f, clip);

        ctx.line.SetColor(80, 80, 90, 255);
        ctx.line.Line2D(dx, dy, dx + drawW, dy);
        ctx.line.Line2D(dx + drawW, dy, dx + drawW, dy + drawH);
        ctx.line.Line2D(dx + drawW, dy + drawH, dx, dy + drawH);
        ctx.line.Line2D(dx, dy + drawH, dx, dy);

        char info[128];
        snprintf(info, sizeof(info), "Atlas: %dx%d  |  Glyphs: %d  |  Scale: %.0f%%",
                 atlas->width, atlas->height, app.font().GetGlyphCount(), scale * 100.0f);
        ctx.font.SetFontSize(14.0f);
        ctx.font.SetColor(Color(180, 180, 190, 255));
        ctx.font.SetBatch(&ctx.text);
        ctx.font.Print(info, bounds.x + 12.0f, bounds.y + bounds.h - 24.0f);
    });

    return page;
}

// ── Helper: build the text editor page ───────────────────────────────────
static Widget* buildEditorPage(PageView* pages)
{
    auto* page = pages->addPage<BoxLayout>(LayoutDir::Vertical);
    pages->setPageName(pages->pageCount() - 1, "Text Editor");
    page->setSpacing(0);
    page->setPadding(8);

    page->createChild<Label>("TextEdit Demo")->setColor(Color(180, 180, 190, 255));
    page->createChild<Spacer>(4);

    auto* editor = page->createChild<TextEdit>(
        "#include <stdio.h>\n"
        "\n"
        "int main(int argc, char* argv[])\n"
        "{\n"
        "    printf(\"Hello, BuGUI!\\n\");\n"
        "    \n"
        "    for (int i = 0; i < 10; i++)\n"
        "    {\n"
        "        printf(\"  Line %d\\n\", i);\n"
        "    }\n"
        "    \n"
        "    return 0;\n"
        "}\n"
        "\n"
        "// This is a multi-line text editor\n"
        "// Features:\n"
        "//   - Line numbers\n"
        "//   - Cursor navigation (arrows, Home, End, PgUp, PgDn)\n"
        "//   - Text selection (Shift + arrows, mouse drag)\n"
        "//   - Clipboard (Ctrl+C, Ctrl+X, Ctrl+V)\n"
        "//   - Tab inserts spaces\n"
        "//   - Scroll (mouse wheel)\n"
        "//   - Ready for syntax highlighting callback\n"
    );
    editor->setStretch(1);
    editor->setShowLineNumbers(true);

    // Context menu on the text editor (right-click)
    static Menu editorCtxMenu;
    static bool edCtxInited = false;
    if (!edCtxInited) {
        edCtxInited = true;
        editorCtxMenu.addAction("Undo",       [editor]{ /* TODO */ }, "Ctrl+Z");
        editorCtxMenu.addAction("Redo",       [editor]{ /* TODO */ }, "Ctrl+Y");
        editorCtxMenu.addSeparator();
        editorCtxMenu.addAction("Cut",        [editor]{ /* TODO */ }, "Ctrl+X");
        editorCtxMenu.addAction("Copy",       [editor]{ /* TODO */ }, "Ctrl+C");
        editorCtxMenu.addAction("Paste",      [editor]{ /* TODO */ }, "Ctrl+V");
        editorCtxMenu.addAction("Select All", [editor]{ editor->selectAll(); }, "Ctrl+A");
        editorCtxMenu.addSeparator();
        editorCtxMenu.addCheckable("Word Wrap", false, [editor](bool on){ editor->setWordWrap(on); });
        editorCtxMenu.addCheckable("Line Numbers", true, [editor](bool on){ editor->setShowLineNumbers(on); });
        editorCtxMenu.addSeparator();
        auto* alignSub = editorCtxMenu.addSubmenu("Alignment");
        alignSub->addAction("Left",    [editor]{ editor->setAlign(TextAlign::LEFT); });
        alignSub->addAction("Center",  [editor]{ editor->setAlign(TextAlign::CENTER); });
        alignSub->addAction("Right",   [editor]{ editor->setAlign(TextAlign::RIGHT); });
        alignSub->addAction("Justify", [editor]{ editor->setAlign(TextAlign::JUSTIFY); });
    }
    editor->setContextMenu(&editorCtxMenu);

    // Toolbar row
    auto* toolbar = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    toolbar->setSize(0, 26);
    toolbar->setSpacing(4);
    toolbar->setPadding(0, 2, 0, 2);

    auto* wrapBtn = toolbar->createChild<Button>("Wrap");
    wrapBtn->setSize(50, 22);
    wrapBtn->clicked.connect([editor]{
        editor->setWordWrap(!editor->wordWrap());
    });

    auto* btnL = toolbar->createChild<Button>("L");
    btnL->setSize(26, 22);
    btnL->clicked.connect([editor]{ editor->setAlign(TextAlign::LEFT); });
    auto* btnC = toolbar->createChild<Button>("C");
    btnC->setSize(26, 22);
    btnC->clicked.connect([editor]{ editor->setAlign(TextAlign::CENTER); });
    auto* btnR = toolbar->createChild<Button>("R");
    btnR->setSize(26, 22);
    btnR->clicked.connect([editor]{ editor->setAlign(TextAlign::RIGHT); });
    auto* btnJ = toolbar->createChild<Button>("J");
    btnJ->setSize(26, 22);
    btnJ->clicked.connect([editor]{ editor->setAlign(TextAlign::JUSTIFY); });

    toolbar->createChild<Spacer>();

    auto* statusRow = page->createChild<BoxLayout>(LayoutDir::Horizontal);
    statusRow->setSize(0, 20);
    statusRow->setSpacing(10);
    statusRow->setPadding(4, 2, 4, 2);

    auto* posLabel = statusRow->createChild<Label>("Ln 1, Col 1");
    posLabel->setColor(Color(140, 140, 150, 255));
    statusRow->createChild<Spacer>();
    auto* linesLabel = statusRow->createChild<Label>("23 lines");
    linesLabel->setColor(Color(140, 140, 150, 255));

    editor->cursorMoved.connect([posLabel](TextEdit::TextPos pos){
        char buf[32];
        snprintf(buf, sizeof(buf), "Ln %d, Col %d", pos.line + 1, pos.col + 1);
        posLabel->setText(buf);
    });

    editor->textChanged.connect([linesLabel, editor](){
        char buf[32];
        snprintf(buf, sizeof(buf), "%d lines", editor->lineCount());
        linesLabel->setText(buf);
    });

    return page;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Main build function
// ═════════════════════════════════════════════════════════════════════════════

inline void buildDemoStage(WidgetApp& app)
{
    Widget* root = app.addStage("demo");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Menu bar (persistent) ────────────────────────────────────────────
    auto* topBar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSize(0, 26);
    topBar->setSpacing(0);
    topBar->setPadding(0);

    auto* burger = topBar->createChild<IconButton>(IconButton::Hamburger);
    burger->setSize(36, 26);
    burger->setAnimated(true);

    auto* menuBar = topBar->createChild<MenuBar>();
    menuBar->setStretch(1);

    // File menu
    auto* fileMenu = menuBar->addMenu("File");
    fileMenu->addAction("New",        []{ /* TODO */ }, "Ctrl+N");
    fileMenu->addAction("Open...",    []{ /* TODO */ }, "Ctrl+O");
    fileMenu->addSeparator();
    fileMenu->addAction("Save",       []{ /* TODO */ }, "Ctrl+S");
    fileMenu->addAction("Save As...", []{ /* TODO */ }, "Ctrl+Shift+S");
    fileMenu->addSeparator();
    fileMenu->addAction("Exit",       []{ WidgetApp::instance().quit(); }, "Alt+F4");

    // Edit menu
    auto* editMenu = menuBar->addMenu("Edit");
    editMenu->addAction("Undo",  []{ /* TODO */ }, "Ctrl+Z");
    editMenu->addAction("Redo",  []{ /* TODO */ }, "Ctrl+Y");
    editMenu->addSeparator();
    editMenu->addAction("Cut",   []{ /* TODO */ }, "Ctrl+X");
    editMenu->addAction("Copy",  []{ /* TODO */ }, "Ctrl+C");
    editMenu->addAction("Paste", []{ /* TODO */ }, "Ctrl+V");
    editMenu->addSeparator();
    editMenu->addAction("Select All", []{ /* TODO */ }, "Ctrl+A");
    editMenu->addAction("Find...",    []{ /* TODO */ }, false, "Ctrl+F");  // disabled

    // View menu
    auto* viewMenu = menuBar->addMenu("View");
    // (actions added after pages are created below)

    // Format menu
    auto* formatMenu = menuBar->addMenu("Format");
    formatMenu->addCheckable("Word Wrap", false, [](bool on){ (void)on; });
    formatMenu->addCheckable("Show Whitespace", false, [](bool on){ (void)on; });
    formatMenu->addSeparator();
    auto* indentSub = formatMenu->addSubmenu("Indentation");
    indentSub->addCheckable("Tabs", true, [](bool){ });
    indentSub->addCheckable("Spaces", false, [](bool){ });
    indentSub->addSeparator();
    indentSub->addAction("2 Spaces", []{ });
    indentSub->addAction("4 Spaces", []{ });
    indentSub->addAction("8 Spaces", []{ });
    auto* eolSub = formatMenu->addSubmenu("Line Endings");
    eolSub->addCheckable("LF (Unix)", true, [](bool){ });
    eolSub->addCheckable("CRLF (Windows)", false, [](bool){ });

    // Help menu
    auto* helpMenu = menuBar->addMenu("Help");
    helpMenu->addAction("About BuGUI", []{ /* TODO */ });

    auto* barSpacer = topBar->createChild<Spacer>();
    (void)barSpacer;

    // Float window toggle button
    auto* floatBtn = topBar->createChild<Button>("Float");
    floatBtn->setSize(50, 26);

    auto* navPrev = topBar->createChild<Button>("< Inputs");
    navPrev->setSize(80, 26);
    navPrev->clicked.connect([]{ WidgetApp::instance().setStage("inputs", TransitionType::SlideRight, EaseType::OutBack); });

    auto* navNext = topBar->createChild<Button>("Tree/Props >");
    navNext->setSize(90, 26);
    navNext->clicked.connect([]{ WidgetApp::instance().setStage("treeprops", TransitionType::SlideLeft, EaseType::OutBack); });

    // ── Input bar (persistent) ───────────────────────────────────────────
    auto* inputBar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    inputBar->setSize(0, 38);
    inputBar->setSpacing(0);
    inputBar->setPadding(5, 10, 5, 10);
    auto* inputPanel = inputBar->createChild<Panel>();
    inputPanel->setStretch(1);
    inputPanel->setBgColor(Color(255, 255, 255, 255));

    // ══════════════════════════════════════════════════════════════════════
    //  PageView — swappable content area
    // ══════════════════════════════════════════════════════════════════════
    auto* pages = outer->createChild<PageView>();
    pages->setStretch(1);
    pages->setTransitionDuration(0.3f);

    // Wire View menu → page switches
    viewMenu->addAction("Main",        [pages]{ pages->setPage(0); });
    viewMenu->addAction("Font Atlas",  [pages]{ pages->setPage(1); });
    viewMenu->addAction("Text Editor", [pages]{ pages->setPage(2); });
    viewMenu->addSeparator();
    viewMenu->addAction("Toggle Debug Layout", []{ auto& a = WidgetApp::instance(); a.setDebugLayout(!a.debugLayout()); }, "F1");

    // Page 0: Main 4-column layout
    buildMainPage(pages);

    // Page 1: Font Atlas viewer
    buildAtlasPage(pages, app);

    // Page 2: Text Editor
    buildEditorPage(pages);

    // ── Sidebar drawer (persistent) ──────────────────────────────────────
    auto* drawer = root->createChild<SidePanel>(SidePanel::Left, 260.0f);
    auto* drawerContent = drawer->setContent<BoxLayout>(LayoutDir::Vertical);
    drawerContent->setSpacing(4);
    drawerContent->setPadding(12);

    auto* drawerHeader = drawerContent->createChild<BoxLayout>(LayoutDir::Horizontal);
    drawerHeader->setSize(0, 28);
    drawerHeader->setSpacing(0);
    auto* drawerTitle = drawerHeader->createChild<Label>("Navigation");
    drawerTitle->setColor(Clr::accent);
    drawerTitle->setStretch(1);
    auto* drawerClose = drawerHeader->createChild<IconButton>(IconButton::Close);
    drawerClose->setSize(28, 28);
    drawerClose->clicked.connect([drawer]{ drawer->close(); });

    drawerContent->createChild<Line>();

    // ── Drawer: internal pages ───────────────────────────────────────────
    drawerContent->createChild<Label>("Pages")->setColor(Clr::dim);

    struct PageLink { const char* label; int idx; };
    PageLink pageLinks[] = {
        {"Main",        0},
        {"Font Atlas",  1},
        {"Text Editor", 2},
    };
    for (auto& pl : pageLinks)
    {
        auto* btn = drawerContent->createChild<Button>(pl.label);
        int idx = pl.idx;
        btn->clicked.connect([pages, drawer, idx]{
            pages->setPage(idx);
            drawer->close();
        });
    }

    drawerContent->createChild<Line>();

    // ── Drawer: theme toggle ─────────────────────────────────────────────
    drawerContent->createChild<Label>("Theme")->setColor(Clr::dim);
    auto* themeBtn = drawerContent->createChild<Button>("Light Theme");
    themeBtn->clicked.connect([themeBtn]{
        static bool dark = true;
        dark = !dark;
        Theme::instance().apply(dark ? Theme::dark() : Theme::light());
        themeBtn->setText(dark ? "Light Theme" : "Dark Theme");
    });

    // ── Drawer: stage navigation ─────────────────────────────────────────
    drawerContent->createChild<Label>("Stages")->setColor(Clr::dim);

    const char* stageNames[] = {
        "layout", "canvas", "widgets", "scroll", "calc",
        "grid", "border", "layouts2", "tabs", "anchor",
        "inputs"
    };
    for (auto* name : stageNames)
    {
        auto* btn = drawerContent->createChild<Button>(name);
        std::string sn = name;
        btn->clicked.connect([sn, drawer]{
            drawer->close();
            WidgetApp::instance().setStage(sn);
        });
    }

    // Wire burger ↔ drawer
    burger->clicked.connect([drawer, burger]{ drawer->toggle(); });
    drawer->openChanged.connect([burger](bool open){
        burger->setIcon(open ? IconButton::Close : IconButton::Hamburger);
    });

    // ── Float window (toggled by "Float" button) ─────────────────────────
    auto* fw = app.addFloat<FloatWindow>("Inspector");
    fw->setFloatPos(500, 150);
    fw->setFloatSize(280, 220);

    auto* fwContent = fw->setContent<BoxLayout>(LayoutDir::Vertical);
    fwContent->setSpacing(6);
    fwContent->setPadding(8);
    fwContent->createChild<Label>("Floating window demo");
    fwContent->createChild<Label>("Drag title bar to move")->setColor(Clr::dim);
    fwContent->createChild<Label>("Resize from corner")->setColor(Clr::dim);

    auto* fwSlider = fwContent->createChild<Slider>();
    fwSlider->setRange(0, 100);
    fwSlider->setValue(50);

    auto* fwLabel = fwContent->createChild<Label>("50 %");
    fwLabel->setColor(Color(180, 180, 180, 255));
    fwSlider->onValueChanged.connect([fwLabel](float v){
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f %%", v);
        fwLabel->setText(buf);
    });

    auto* fwAlertBtn = fwContent->createChild<Button>("Show Alert");
    fwAlertBtn->clicked.connect([]{
        auto* alert = new AlertDialog("Hello!", "This is an alert dialog\nfrom the float window.");
        WidgetApp::instance().showPopup(alert);
    });

    fw->closed.connect([fw]{ fw->setVisible(false); });

    floatBtn->clicked.connect([fw]{
        fw->setVisible(!fw->isVisible());
    });

    // ── Status bar (persistent) ──────────────────────────────────────────
    auto* status = outer->createChild<StatusBar>();
    status->setText("Demo Stage \xe2\x80\x94 SDL_GUI Style");

    // Update status text on page change
    pages->pageChanged.connect([status, pages](int idx){
        char buf[64];
        const auto& name = pages->pageName(idx);
        snprintf(buf, sizeof(buf), "Demo \xe2\x80\x94 %s", name.empty() ? "Unknown" : name.c_str());
        status->setText(buf);
    });
}
