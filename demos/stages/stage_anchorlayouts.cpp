#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
using namespace BuGUI;

// ─────────────────────────────────────────────────────────────────────────────
//  Stage 13 — AnchorLayout + Panel + Overlay
//  Demonstrates:
//    • Panel         — coloured container widget
//    • AnchorLayout  — children placed by anchor fractions + pixel offsets
//    • Overlay       — Z-stacked layers; layer 1 acts as a dismissible modal
// ─────────────────────────────────────────────────────────────────────────────

static Widget*    s_overlayLayer = nullptr;
static StatusBar* s_ancStatus    = nullptr;

static void showOverlay(bool visible)
{
    if (s_overlayLayer)
        s_overlayLayer->setVisible(visible);
}

// ─────────────────────────────────────────────────────────────────────────────

void registerAnchorLayoutsStage(WidgetApp& app)
{
    s_overlayLayer = nullptr;
    s_ancStatus    = nullptr;

    auto* root = app.addStage("anchorlayouts");
    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(0.0f);
    vbox->setPadding(0.0f);

    // Top bar
    auto* topBar = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSpacing(8.0f);
    topBar->setPadding(Edges(8.0f, 12.0f));

    auto* back = topBar->createChild<Button>("\u2190 Menu");
    back->clicked.connect([&app]()
    {
        showOverlay(false);
        app.setStage("menu", TransitionType::CoverRight);
    });

    topBar->createChild<Spacer>(0.0f)->setStretch(1.0f);
    topBar->createChild<Label>("13 \u2014 AnchorLayout + Panel + Overlay");
    topBar->createChild<Spacer>(0.0f)->setStretch(1.0f);

    auto* btnOverlay = topBar->createChild<Button>("Show Overlay");
    btnOverlay->clicked.connect([](){ showOverlay(true); });

    // Overlay container (two stacked AnchorLayout layers)
    auto* ov = vbox->createChild<Overlay>();
    ov->setStretch(1.0f);

    // ── Layer 0: AnchorLayout with coloured Panel blocks ──────────────────────
    auto* layer0 = ov->createChild<AnchorLayout>();

    // Top-left: fixed 200x120 anchored to top-left corner
    auto* pTL = layer0->createChild<Panel>();
    pTL->setBgColor(Color(60, 80, 120, 255));
    AnchorLayout::setAnchors(pTL, AnchorRule{
        0.0f, 0.0f, 0.0f, 0.0f,
        12.0f, 212.0f, 12.0f, 132.0f
    });
    {
        auto* b = pTL->createChild<BoxLayout>(LayoutDir::Vertical);
        b->setSpacing(4.0f);
        b->setPadding(Edges(10.0f));
        b->createChild<Label>("Top-Left | anchors: left+top");
        auto* btn = b->createChild<Button>("Click");
        btn->clicked.connect([](){ if (s_ancStatus) s_ancStatus->setText("Clicked: Top-Left panel"); });
    }

    // Top-right: fixed 200x120 anchored to top-right corner
    auto* pTR = layer0->createChild<Panel>();
    pTR->setBgColor(Color(80, 60, 120, 255));
    AnchorLayout::setAnchors(pTR, AnchorRule{
        1.0f, 1.0f, 0.0f, 0.0f,
        -212.0f, -12.0f, 12.0f, 132.0f
    });
    {
        auto* b = pTR->createChild<BoxLayout>(LayoutDir::Vertical);
        b->setSpacing(4.0f);
        b->setPadding(Edges(10.0f));
        b->createChild<Label>("Top-Right | anchors: right+top");
        auto* btn = b->createChild<Button>("Click");
        btn->clicked.connect([](){ if (s_ancStatus) s_ancStatus->setText("Clicked: Top-Right panel"); });
    }

    // Bottom-left: fixed 200x120 anchored to bottom-left corner
    auto* pBL = layer0->createChild<Panel>();
    pBL->setBgColor(Color(60, 120, 80, 255));
    AnchorLayout::setAnchors(pBL, AnchorRule{
        0.0f, 0.0f, 1.0f, 1.0f,
        12.0f, 212.0f, -132.0f, -12.0f
    });
    {
        auto* b = pBL->createChild<BoxLayout>(LayoutDir::Vertical);
        b->setSpacing(4.0f);
        b->setPadding(Edges(10.0f));
        b->createChild<Label>("Bottom-Left | anchors: left+bottom");
        auto* btn = b->createChild<Button>("Click");
        btn->clicked.connect([](){ if (s_ancStatus) s_ancStatus->setText("Clicked: Bottom-Left panel"); });
    }

    // Bottom-right: fixed 200x120 anchored to bottom-right corner
    auto* pBR = layer0->createChild<Panel>();
    pBR->setBgColor(Color(120, 80, 60, 255));
    AnchorLayout::setAnchors(pBR, AnchorRule{
        1.0f, 1.0f, 1.0f, 1.0f,
        -212.0f, -12.0f, -132.0f, -12.0f
    });
    {
        auto* b = pBR->createChild<BoxLayout>(LayoutDir::Vertical);
        b->setSpacing(4.0f);
        b->setPadding(Edges(10.0f));
        b->createChild<Label>("Bottom-Right | anchors: right+bottom");
        auto* btn = b->createChild<Button>("Click");
        btn->clicked.connect([](){ if (s_ancStatus) s_ancStatus->setText("Clicked: Bottom-Right panel"); });
    }

    // Center panel: stretches with the window (margins all sides)
    auto* pCenter = layer0->createChild<Panel>();
    pCenter->setBgColor(Color(45, 50, 65, 255));
    AnchorLayout::setAnchors(pCenter, AnchorRule{
        0.0f, 1.0f, 0.0f, 1.0f,
        230.0f, -230.0f, 150.0f, -150.0f
    });
    {
        auto* b = pCenter->createChild<BoxLayout>(LayoutDir::Vertical);
        b->setSpacing(8.0f);
        b->setPadding(Edges(20.0f));
        b->createChild<Label>("Center panel \u2014 stretches with window");
        b->createChild<Label>("fractions 0..1 each side, offsets \u00b1230/\u00b1150 px");
        auto* row = b->createChild<BoxLayout>(LayoutDir::Horizontal);
        row->setSpacing(8.0f);
        auto* btnOvr = row->createChild<Button>("Open Overlay");
        btnOvr->clicked.connect([](){ showOverlay(true); });
        auto* btnClear = row->createChild<Button>("Clear status");
        btnClear->clicked.connect([](){ if (s_ancStatus) s_ancStatus->setText("Status cleared."); });
    }

    (void)pTL; (void)pTR; (void)pBL; (void)pBR; (void)pCenter;

    // ── Layer 1: modal overlay layer (initially hidden) ───────────────────────
    //  dimPanel fills layer1 completely.
    //  cardPanel is centred inside layer1.
    auto* layer1 = ov->createChild<AnchorLayout>();
    layer1->setVisible(false);
    s_overlayLayer = layer1;

    // Dimmer: full-fill
    auto* dimPanel = layer1->createChild<Panel>();
    dimPanel->setBgColor(Color(0, 0, 0, 160));
    AnchorLayout::setAnchors(dimPanel, AnchorRule{
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    });

    // Modal card: centred 340 x 210
    auto* cardPanel = layer1->createChild<Panel>();
    cardPanel->setBgColor(Color(48, 50, 68, 255));
    AnchorLayout::setAnchors(cardPanel, AnchorRule{
        0.5f, 0.5f, 0.5f, 0.5f,
        -170.0f, 170.0f, -105.0f, 105.0f
    });
    {
        auto* b = cardPanel->createChild<BoxLayout>(LayoutDir::Vertical);
        b->setSpacing(10.0f);
        b->setPadding(Edges(24.0f));
        b->createChild<Label>("Overlay \u2014 Z-Stacked Layer");
        b->createChild<Label>("This AnchorLayout layer sits on top of\nthe normal UI inside the Overlay.");
        auto* btnClose = b->createChild<Button>("Close");
        btnClose->clicked.connect([](){ showOverlay(false); });
    }

    (void)dimPanel; (void)cardPanel;

    // ── Status bar (fixed at bottom of vbox) ─────────────────────────────
    s_ancStatus = vbox->createChild<StatusBar>();
    s_ancStatus->setText("Resize the window to see panels re-anchor.");
}
