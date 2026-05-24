// stage_animation_model.cpp — Tests Animation framework + Model/View DataGrid
#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "DataWidgets.hpp"
#include "TreePropertyColorWidgets.hpp"
#include "TextInputWidgets.hpp"
#include "InputWidgets.hpp"
#include "Animation.hpp"
#include "ItemModel.hpp"
using namespace BuGUI;

static void backToMenu(WidgetApp& app)
{
    app.setStage("menu", TransitionType::CoverRight);
}

void registerAnimModelStage(WidgetApp& app)
{
    Widget* root = app.addStage("animmodel");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setStretch(1);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Top bar ──────────────────────────────────────────────────────────
    auto* topBar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSize(0, 32);
    topBar->setLayoutAlign(Align::Fill);
    topBar->setPadding(4, 8, 4, 8);
    topBar->setSpacing(8);

    auto* backBtn = topBar->createChild<Button>("\u2190 Menu");
    backBtn->clicked.connect([&app]() { backToMenu(app); });

    auto* title = topBar->createChild<Label>("Animation + Model/View Demo");
    title->setStretch(1);

    // ── Main split ───────────────────────────────────────────────────────
    auto* hbox = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    hbox->setStretch(1.0f);
    hbox->setSpacing(12.0f);
    hbox->setMargins(8);

    // ──────────────────────────────────────────────────────────────────────
    //  LEFT: Animation demos
    // ──────────────────────────────────────────────────────────────────────
    auto* leftPanel = hbox->createChild<BoxLayout>(LayoutDir::Vertical);
    leftPanel->setStretch(1.0f);
    leftPanel->setSpacing(6.0f);

    leftPanel->createChild<Label>("── Animation Framework ──");

    // Animated progress bar
    auto* progressLabel = leftPanel->createChild<Label>("Progress: 0%");
    auto* progressBar   = leftPanel->createChild<Slider>();
    progressBar->setRange(0.0f, 100.0f);
    progressBar->setValue(0.0f);
    progressBar->setSize(0, 24);

    // Button: animate progress 0→100 with OutBounce
    auto* animBtn1 = leftPanel->createChild<Button>("Animate 0->100 (OutBounce)");
    animBtn1->clicked.connect([progressBar, progressLabel]() {
        Animator::animate(0.0f, 100.0f, 1.5f, EaseType::OutBounce,
            [progressBar, progressLabel](float v) {
                progressBar->setValue(v);
                char buf[32];
                std::snprintf(buf, sizeof(buf), "Progress: %d%%", static_cast<int>(v));
                progressLabel->setText(buf);
            });
    });

    // Button: animate progress 100→0 with InOutCubic
    auto* animBtn2 = leftPanel->createChild<Button>("Animate 100->0 (InOutCubic)");
    animBtn2->clicked.connect([progressBar, progressLabel]() {
        Animator::animate(100.0f, 0.0f, 1.0f, EaseType::InOutCubic,
            [progressBar, progressLabel](float v) {
                progressBar->setValue(v);
                char buf[32];
                std::snprintf(buf, sizeof(buf), "Progress: %d%%", static_cast<int>(v));
                progressLabel->setText(buf);
            });
    });

    // Color animation (label changes color text)
    leftPanel->createChild<Label>("");
    auto* colorLabel = leftPanel->createChild<Label>("Color transition ─────────────");
    auto* animBtn3 = leftPanel->createChild<Button>("Animate color R→G→B");
    animBtn3->clicked.connect([colorLabel]() {
        // Sequential: Red phase, Green phase, Blue phase
        auto* group = new AnimationGroup(AnimationGroup::Mode::Sequential);

        auto* a1 = new Animation(0.0f, 255.0f, 0.5f, EaseType::InOutQuad);
        a1->onUpdate.connect([colorLabel](float v) {
            colorLabel->setColor(Color(255, static_cast<uint8_t>(v), 0, 255));
        });
        group->add(a1);

        auto* a2 = new Animation(0.0f, 255.0f, 0.5f, EaseType::InOutQuad);
        a2->onUpdate.connect([colorLabel](float v) {
            colorLabel->setColor(Color(static_cast<uint8_t>(255 - v), 255,
                                        static_cast<uint8_t>(v), 255));
        });
        group->add(a2);

        auto* a3 = new Animation(255.0f, 0.0f, 0.5f, EaseType::InOutQuad);
        a3->onUpdate.connect([colorLabel](float v) {
            colorLabel->setColor(Color(0, static_cast<uint8_t>(v), 255, 255));
        });
        group->add(a3);

        group->start();
        Animator::instance().add(group);
    });

    // Easing comparison
    leftPanel->createChild<Label>("");
    leftPanel->createChild<Label>("Easing comparison:");
    auto* easeSliders = leftPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    easeSliders->setSpacing(4.0f);

    struct EaseDemo { const char* name; EaseType ease; };
    static const EaseDemo eases[] = {
        {"Linear",      EaseType::Linear},
        {"OutQuad",     EaseType::OutQuad},
        {"OutCubic",    EaseType::OutCubic},
        {"OutBounce",   EaseType::OutBounce},
        {"OutElastic",  EaseType::OutElastic},
        {"InOutBack",   EaseType::InOutBack},
    };

    std::vector<Slider*> easeSliderVec;
    for (auto& ed : eases) {
        auto* row = easeSliders->createChild<BoxLayout>(LayoutDir::Horizontal);
        row->setSpacing(4.0f);
        auto* lbl = row->createChild<Label>(ed.name);
        lbl->setSize(90, 0);
        auto* sl = row->createChild<Slider>();
        sl->setRange(0.0f, 1.0f);
        sl->setValue(0.0f);
        sl->setStretch(1.0f);
        easeSliderVec.push_back(sl);
    }

    auto* animAllBtn = leftPanel->createChild<Button>("Animate all easings");
    animAllBtn->clicked.connect([easeSliderVec]() {
        for (int i = 0; i < static_cast<int>(easeSliderVec.size()); ++i) {
            auto* sl = easeSliderVec[i];
            sl->setValue(0.0f);
            Animator::animate(0.0f, 1.0f, 1.5f, eases[i].ease,
                [sl](float v) { sl->setValue(v); });
        }
    });

    // ──────────────────────────────────────────────────────────────────────
    //  RIGHT: Model/View demos
    // ──────────────────────────────────────────────────────────────────────
    auto* rightPanel = hbox->createChild<BoxLayout>(LayoutDir::Vertical);
    rightPanel->setStretch(1.0f);
    rightPanel->setSpacing(6.0f);

    rightPanel->createChild<Label>("── Model/View DataGrid ──");

    // Create a ListModel
    static ListModel* stockModel = new ListModel(4);
    stockModel->setHeaders({"Ticker", "Price", "Change", "Volume"});
    stockModel->addRow({"AAPL",  "182.52", "+1.23",  "52.3M"});
    stockModel->addRow({"GOOGL", "141.80", "-0.45",  "28.1M"});
    stockModel->addRow({"MSFT",  "378.91", "+2.10",  "31.7M"});
    stockModel->addRow({"AMZN",  "178.25", "+0.87",  "45.2M"});
    stockModel->addRow({"TSLA",  "248.42", "-3.12",  "92.5M"});
    stockModel->addRow({"META",  "474.99", "+4.56",  "18.9M"});
    stockModel->addRow({"NVDA",  "875.28", "+12.34", "55.8M"});
    stockModel->addRow({"NFLX",  "611.40", "+3.21",  "8.2M"});

    auto* grid = rightPanel->createChild<DataGrid>();
    grid->setModel(stockModel);
    grid->setZebraStripes(true);
    grid->setStretch(1.0f);

    // Filter
    auto* filterRow = rightPanel->createChild<BoxLayout>(LayoutDir::Horizontal);
    filterRow->setSpacing(4.0f);
    filterRow->createChild<Label>("Filter:");
    auto* filterInput = filterRow->createChild<TextInput>();
    filterInput->setStretch(1.0f);
    filterInput->setPlaceholder("Type to filter...");

    static SortFilterProxyModel* proxyModel = new SortFilterProxyModel(stockModel);
    proxyModel->setFilterColumn(0);  // filter on Ticker

    auto* useProxyBtn = rightPanel->createChild<Button>("Switch to filtered view");
    static bool usingProxy = false;
    useProxyBtn->clicked.connect([grid, useProxyBtn]() {
        usingProxy = !usingProxy;
        if (usingProxy) {
            grid->setModel(proxyModel);
            useProxyBtn->setText("Switch to direct view");
        } else {
            grid->setModel(stockModel);
            useProxyBtn->setText("Switch to filtered view");
        }
    });

    filterInput->textChanged.connect([](const std::string& text) {
        proxyModel->setFilterText(text);
    });

    // Add/Remove rows
    auto* rowBtnRow = rightPanel->createChild<BoxLayout>(LayoutDir::Horizontal);
    rowBtnRow->setSpacing(4.0f);

    auto* addRowBtn = rowBtnRow->createChild<Button>("+ Add Row");
    addRowBtn->clicked.connect([]() {
        static int counter = 0;
        char ticker[8];
        std::snprintf(ticker, sizeof(ticker), "NEW%d", counter++);
        stockModel->addRow({ticker, "100.00", "+0.00", "0.0M"});
    });

    auto* delRowBtn = rowBtnRow->createChild<Button>("- Remove Last");
    delRowBtn->clicked.connect([]() {
        int n = stockModel->rowCount();
        if (n > 0) stockModel->removeRow(n - 1);
    });

    // ── TreeView with TreeModel ──────────────────────────────────────────
    rightPanel->createChild<Label>("");
    rightPanel->createChild<Label>("── Model/View TreeView ──");

    static TreeModel* treeModel = new TreeModel(1);
    treeModel->setHeaders({"Name"});

    auto* projRoot = treeModel->addRoot({"Project"});
    auto* srcNode  = projRoot->addChild({"src"});
    srcNode->addChild({"main.cpp"});
    srcNode->addChild({"utils.cpp"});
    srcNode->addChild({"config.hpp"});
    auto* testNode = projRoot->addChild({"tests"});
    testNode->addChild({"test_main.cpp"});
    testNode->addChild({"test_utils.cpp"});
    auto* docNode  = projRoot->addChild({"docs"});
    docNode->addChild({"README.md"});
    docNode->addChild({"API.md"});

    auto* tree = rightPanel->createChild<TreeView>();
    tree->setModel(treeModel);
    tree->setStretch(1.0f);

    auto* infoLabel = rightPanel->createChild<Label>("Selected: (none)");
    tree->selectionChanged.connect([infoLabel](TreeNode* n) {
        if (n) {
            std::string msg = "Selected: " + n->text();
            infoLabel->setText(msg);
        } else {
            infoLabel->setText("Selected: (none)");
        }
    });
}
