#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "DataWidgets.hpp"
#include "TextInputWidgets.hpp"
#include "ComboBox.hpp"
#include "InputWidgets.hpp"
#include "ScrollWidgets.hpp"
#include "TreePropertyColorWidgets.hpp"
#include "DialogWidgets.hpp"
#include "ChartWidgets.hpp"
#include "MenuWidgets.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ─────────────────────────────────────────────────────────────────────────────
//  Stage — CRM (Client Relationship Manager)
//
//  Stress tests:
//  - DataGrid with 500 rows (sortable, editable, multi-select, checkboxes)
//  - TabLayout with multiple views (Clients, Deals, Tasks, Reports)
//  - FormLayout with all input types
//  - PropertyGrid for client details
//  - Dialogs (add/edit client)
//  - ComboBox, TextInput, DatePicker, ProgressBar
//  - PlotWidget with sales data
//  - MenuBar with app actions
// ─────────────────────────────────────────────────────────────────────────────

static const char* firstNames[] = {
    "Alice", "Bob", "Carlos", "Diana", "Elena", "Frank", "Grace", "Hector",
    "Iris", "James", "Karen", "Leo", "Maria", "Nathan", "Olivia", "Paul",
    "Quinn", "Rachel", "Steve", "Tina", "Uma", "Victor", "Wendy", "Xavier",
    "Yara", "Zach", "Ana", "Bruno", "Clara", "David", "Eva", "Felix",
    "Gina", "Hugo", "Ines", "Jorge", "Lara", "Marco", "Nina", "Oscar",
    "Pedro", "Rita", "Sofia", "Tomas", "Ursula", "Vera", "Walter", "Ximena"
};
static const char* lastNames[] = {
    "Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller",
    "Davis", "Rodriguez", "Martinez", "Hernandez", "Lopez", "Gonzalez",
    "Wilson", "Anderson", "Thomas", "Taylor", "Moore", "Jackson", "Martin",
    "Lee", "Perez", "Thompson", "White", "Harris", "Sanchez", "Clark",
    "Ramirez", "Lewis", "Robinson", "Walker", "Young", "Allen", "King",
    "Wright", "Scott", "Torres", "Nguyen", "Hill", "Flores", "Green",
    "Adams", "Nelson", "Baker", "Hall", "Rivera", "Campbell", "Mitchell"
};
static const char* companies[] = {
    "Acme Corp", "GlobalTech", "InnoSoft", "MegaData", "CloudNine",
    "DataDynamics", "TechVista", "CyberLink", "NovaSys", "AlphaWave",
    "ByteForge", "CodeMasters", "DigiPulse", "EdgeWorks", "FutureStack",
    "GridIron", "HyperScale", "InfoPlex", "JetStream", "KernelOps",
    "LogicHub", "MetaCore", "NetPrime", "OptiFlow", "PixelCraft",
    "QuantumBit", "RapidDev", "SkyNet AI", "TurboCode", "UltraNode"
};
static const char* statuses[] = {"Lead", "Prospect", "Active", "Churned", "VIP"};
static const char* countries[] = {
    "USA", "Brazil", "Germany", "Japan", "UK", "France", "Canada",
    "Australia", "Mexico", "Spain", "Italy", "Portugal", "India", "China"
};
static const char* dealStages[] = {
    "Qualification", "Proposal", "Negotiation", "Closed Won", "Closed Lost"
};

static unsigned int crmRng = 42;
static int crmRand(int maxVal) {
    crmRng = crmRng * 1103515245 + 12345;
    return static_cast<int>((crmRng >> 16) & 0x7FFF) % maxVal;
}

static void populateClientGrid(DataGrid* grid, int count)
{
    int nFirst = sizeof(firstNames) / sizeof(firstNames[0]);
    int nLast  = sizeof(lastNames) / sizeof(lastNames[0]);
    int nComp  = sizeof(companies) / sizeof(companies[0]);
    int nStat  = sizeof(statuses) / sizeof(statuses[0]);
    int nCtry  = sizeof(countries) / sizeof(countries[0]);

    for (int i = 0; i < count; ++i) {
        char name[64];
        snprintf(name, sizeof(name), "%s %s",
                 firstNames[crmRand(nFirst)], lastNames[crmRand(nLast)]);

        char email[80];
        // lowercase first letter + last name
        snprintf(email, sizeof(email), "%c.%s@%s.com",
                 static_cast<char>(name[0] + 32), lastNames[crmRand(nLast)],
                 i % 3 == 0 ? "gmail" : (i % 3 == 1 ? "outlook" : "company"));

        char phone[20];
        snprintf(phone, sizeof(phone), "+%d %d-%d-%04d",
                 1 + crmRand(80), 100 + crmRand(900),
                 100 + crmRand(900), crmRand(10000));

        char revenue[16];
        int rev = 1000 + crmRand(500000);
        snprintf(revenue, sizeof(revenue), "$%d,%03d", rev / 1000, rev % 1000);

        char deals[8];
        snprintf(deals, sizeof(deals), "%d", crmRand(12));

        char lastContact[12];
        snprintf(lastContact, sizeof(lastContact), "2026-%02d-%02d",
                 1 + crmRand(4), 1 + crmRand(28));

        char score[8];
        snprintf(score, sizeof(score), "%d", 10 + crmRand(91));

        grid->addRow({
            name,
            companies[crmRand(nComp)],
            email,
            phone,
            statuses[crmRand(nStat)],
            countries[crmRand(nCtry)],
            revenue,
            deals,
            lastContact,
            score
        });
    }
}

static void populateDealsGrid(DataGrid* grid, int count)
{
    int nFirst = sizeof(firstNames) / sizeof(firstNames[0]);
    int nLast  = sizeof(lastNames) / sizeof(lastNames[0]);
    int nComp  = sizeof(companies) / sizeof(companies[0]);
    int nStage = sizeof(dealStages) / sizeof(dealStages[0]);

    for (int i = 0; i < count; ++i) {
        char client[64];
        snprintf(client, sizeof(client), "%s %s",
                 firstNames[crmRand(nFirst)], lastNames[crmRand(nLast)]);

        char value[16];
        int val = 5000 + crmRand(200000);
        snprintf(value, sizeof(value), "$%d,%03d", val / 1000, val % 1000);

        char prob[8];
        snprintf(prob, sizeof(prob), "%d%%", 10 + crmRand(91));

        char closeDate[12];
        snprintf(closeDate, sizeof(closeDate), "2026-%02d-%02d",
                 4 + crmRand(9), 1 + crmRand(28));

        char owner[48];
        snprintf(owner, sizeof(owner), "%s %s",
                 firstNames[crmRand(nFirst)], lastNames[crmRand(nLast)]);

        grid->addRow({
            client,
            companies[crmRand(nComp)],
            dealStages[crmRand(nStage)],
            value,
            prob,
            closeDate,
            owner
        });
    }
}

void registerCrmStage(WidgetApp& app)
{
    auto* root = app.addStage("crm");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setStretch(1);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Menu Bar ─────────────────────────────────────────────────────────
    auto* menuBar = outer->createChild<MenuBar>();
    {
        auto* fileMenu = menuBar->addMenu("CRM");
        fileMenu->addAction("New Client")->setShortcut("Ctrl+N");
        fileMenu->addAction("New Deal")->setShortcut("Ctrl+D");
        fileMenu->addSeparator();
        fileMenu->addAction("Import CSV...")->setShortcut("Ctrl+I");
        fileMenu->addAction("Export CSV...")->setShortcut("Ctrl+E");
        fileMenu->addSeparator();
        fileMenu->addAction("Exit");

        auto* viewMenu = menuBar->addMenu("View");
        viewMenu->addAction("All Clients");
        viewMenu->addCheckable("Active Only", true);
        viewMenu->addCheckable("VIP Only");
        viewMenu->addSeparator();
        viewMenu->addAction("Refresh")->setShortcut("F5");

        auto* helpMenu = menuBar->addMenu("Help");
        helpMenu->addAction("Documentation");
        helpMenu->addAction("About CRM (fake)");
    }

    // ── Toolbar ──────────────────────────────────────────────────────────
    auto* toolRow = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    toolRow->setSize(0, 36);
    toolRow->setPadding(Edges(4, 6, 4, 6));
    toolRow->setSpacing(6);
    {
        auto* back = toolRow->createChild<Button>("\u2190 Menu");
        back->setSize(80, 24);
        back->clicked.connect([&app]() {
            app.setStage("menu", TransitionType::CoverRight);
        });
        toolRow->createChild<Line>();
        toolRow->createChild<Label>("CRM — Client Management");
        toolRow->createChild<Spacer>(0.f)->setStretch(1);

        auto* searchInput = toolRow->createChild<TextInput>();
        searchInput->setPlaceholder("Search clients...");
        searchInput->setSize(200, 24);

        toolRow->createChild<Button>("+ New Client")->setSize(100, 24);
        toolRow->createChild<Button>("+ New Deal")->setSize(90, 24);
    }
    outer->createChild<Line>();

    // ── Main content: TabLayout ──────────────────────────────────────────
    auto* tabs = outer->createChild<TabLayout>();
    tabs->setStretch(1);

    // ═══════════════════════════════════════════════════════════════════════
    //  TAB 1: Clients
    // ═══════════════════════════════════════════════════════════════════════
    {
        auto* clientPage = tabs->addTab<BoxLayout>("Clients (500)", LayoutDir::Vertical);
        auto* hbox = clientPage->createChild<BoxLayout>(LayoutDir::Horizontal);
        hbox->setSpacing(0);
        hbox->setPadding(0);
        hbox->setStretch(1);

        // Left: Grid
        auto* leftCol = hbox->createChild<BoxLayout>(LayoutDir::Vertical);
        leftCol->setSpacing(4);
        leftCol->setPadding(Edges(8, 4, 8, 4));
        leftCol->setStretch(1);

        // Filter row
        auto* filterRow = leftCol->createChild<BoxLayout>(LayoutDir::Horizontal);
        filterRow->setSpacing(8);
        filterRow->setPadding(Edges(0, 4, 0, 4));
        filterRow->createChild<Label>("Status:");
        filterRow->createChild<ComboBox>()->setItems({"All", "Lead", "Prospect", "Active", "Churned", "VIP"});
        filterRow->createChild<Label>("Country:");
        filterRow->createChild<ComboBox>()->setItems({"All", "USA", "Brazil", "Germany", "Japan", "UK", "France", "Canada"});
        filterRow->createChild<Spacer>(0.f)->setStretch(1);
        filterRow->createChild<Label>("500 clients");

        auto* grid = leftCol->createChild<DataGrid>();
        grid->setStretch(1);
        grid->setShowCheckboxes(true);
        grid->setZebraStripes(true);
        grid->setMultiSelect(true);

        grid->addColumn("Name",          150.f, true);
        grid->addColumn("Company",       130.f, true);
        grid->addColumn("Email",         180.f, true);
        grid->addColumn("Phone",         120.f, false);
        grid->addColumn("Status",         80.f, true);
        grid->addColumn("Country",        80.f, true);
        grid->addColumn("Revenue",       100.f, true);
        grid->addColumn("Deals",          50.f, true);
        grid->addColumn("Last Contact",  100.f, true);
        grid->addColumn("Score",          50.f, true);

        populateClientGrid(grid, 500);

        // Right: Detail panel (PropertyGrid)
        auto* rightCol = hbox->createChild<BoxLayout>(LayoutDir::Vertical);
        rightCol->setSpacing(4);
        rightCol->setPadding(Edges(8, 4, 8, 4));
        rightCol->setSize(280, 0);

        rightCol->createChild<Label>("Client Details")->setColor(Color(140, 180, 220, 255));
        rightCol->createChild<Line>();

        auto* detailPg = rightCol->createChild<PropertyGrid>();
        detailPg->setStretch(1);
        detailPg->addSection("Contact");
        detailPg->addString("Name",    "Alice Smith");
        detailPg->addString("Email",   "a.smith@gmail.com");
        detailPg->addString("Phone",   "+1 555-123-4567");
        detailPg->addString("Company", "Acme Corp");
        detailPg->addCombo("Status",   {"Lead", "Prospect", "Active", "Churned", "VIP"}, 2);

        detailPg->addSection("Address");
        detailPg->addString("Street",  "123 Main St");
        detailPg->addString("City",    "San Francisco");
        detailPg->addString("State",   "CA");
        detailPg->addString("ZIP",     "94102");
        detailPg->addCombo("Country",  {"USA", "Brazil", "Germany", "Japan", "UK"}, 0);

        detailPg->addSection("Business");
        detailPg->addInt("Score",           85, 0, 100);
        detailPg->addFloat("Revenue",       125000.f, 0.f, 10000000.f);
        detailPg->addInt("Active Deals",    3, 0, 50);
        detailPg->addInt("Closed Deals",    7, 0, 200);
        detailPg->addString("Last Contact", "2026-04-15");
        detailPg->addString("Created",      "2024-03-20");

        detailPg->addSection("Notes");
        detailPg->addString("Tags", "enterprise, priority, west-coast");
        detailPg->addButton("Send Email");
        detailPg->addButton("Schedule Call");
        detailPg->addButton("View History");

        // Wire selection
        static PropertyGrid* s_detailPg = detailPg;
        static DataGrid*     s_clientGrid = grid;
        grid->selectionChanged.connect([](int row) {
            if (!s_detailPg || !s_clientGrid || row < 0) return;
            s_detailPg->setString(1, s_clientGrid->cell(row, 0));
            s_detailPg->setString(2, s_clientGrid->cell(row, 2));
            s_detailPg->setString(3, s_clientGrid->cell(row, 3));
            s_detailPg->setString(4, s_clientGrid->cell(row, 1));
        });
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  TAB 2: Deals Pipeline
    // ═══════════════════════════════════════════════════════════════════════
    {
        auto* dealsPage = tabs->addTab<BoxLayout>("Deals (300)", LayoutDir::Vertical);
        auto* vbox = dealsPage->createChild<BoxLayout>(LayoutDir::Vertical);
        vbox->setSpacing(6);
        vbox->setPadding(8);
        vbox->setStretch(1);

        // Pipeline summary
        auto* summRow = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
        summRow->setSpacing(12);
        summRow->setPadding(Edges(4, 8, 4, 8));

        auto makeStatCard = [](BoxLayout* parent, const char* label, const char* value, const Color& c) {
            auto* card = parent->createChild<BoxLayout>(LayoutDir::Vertical);
            card->setSpacing(2);
            card->setPadding(Edges(12, 8, 12, 8));
            card->setStretch(1);
            auto* vLbl = card->createChild<Label>(value);
            vLbl->setColor(c);
            card->createChild<Label>(label)->setColor(Color(140, 140, 160, 255));
        };

        makeStatCard(summRow, "Total Pipeline",   "$4,235,000", Color(100, 180, 255, 255));
        makeStatCard(summRow, "Closed Won",        "$1,850,000", Color(100, 220, 120, 255));
        makeStatCard(summRow, "Closed Lost",       "$620,000",   Color(255, 100, 80, 255));
        makeStatCard(summRow, "Avg Deal Size",     "$28,400",    Color(220, 200, 100, 255));
        makeStatCard(summRow, "Win Rate",          "64%",        Color(160, 120, 220, 255));

        vbox->createChild<Line>();

        // Deal stage filter
        auto* dealFilter = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
        dealFilter->setSpacing(8);
        dealFilter->setPadding(Edges(0, 4, 0, 4));
        dealFilter->createChild<Label>("Stage:");
        dealFilter->createChild<ComboBox>()->setItems(
            {"All", "Qualification", "Proposal", "Negotiation", "Closed Won", "Closed Lost"});
        dealFilter->createChild<Label>("Owner:");
        dealFilter->createChild<ComboBox>()->setItems({"All", "Me", "Team A", "Team B"});
        dealFilter->createChild<Spacer>(0.f)->setStretch(1);

        auto* dealsGrid = vbox->createChild<DataGrid>();
        dealsGrid->setStretch(1);
        dealsGrid->setShowCheckboxes(true);
        dealsGrid->setZebraStripes(true);
        dealsGrid->setMultiSelect(true);

        dealsGrid->addColumn("Client",      150.f, true);
        dealsGrid->addColumn("Company",     130.f, true);
        dealsGrid->addColumn("Stage",       110.f, true);
        dealsGrid->addColumn("Value",       100.f, true);
        dealsGrid->addColumn("Probability",  80.f, true);
        dealsGrid->addColumn("Close Date",  100.f, true);
        dealsGrid->addColumn("Owner",       130.f, true);

        populateDealsGrid(dealsGrid, 300);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  TAB 3: Tasks
    // ═══════════════════════════════════════════════════════════════════════
    {
        auto* tasksPage = tabs->addTab<BoxLayout>("Tasks (150)", LayoutDir::Vertical);
        auto* vbox = tasksPage->createChild<BoxLayout>(LayoutDir::Vertical);
        vbox->setSpacing(6);
        vbox->setPadding(8);
        vbox->setStretch(1);

        auto* taskFilter = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
        taskFilter->setSpacing(8);
        taskFilter->createChild<Label>("Priority:");
        taskFilter->createChild<ComboBox>()->setItems({"All", "High", "Medium", "Low"});
        taskFilter->createChild<Label>("Assignee:");
        taskFilter->createChild<ComboBox>()->setItems({"All", "Me", "Unassigned"});
        taskFilter->createChild<Spacer>(0.f)->setStretch(1);
        taskFilter->createChild<Button>("+ New Task");

        vbox->createChild<Line>();

        auto* taskGrid = vbox->createChild<DataGrid>();
        taskGrid->setStretch(1);
        taskGrid->setShowCheckboxes(true);
        taskGrid->setZebraStripes(true);

        taskGrid->addColumn("Task",          250.f, true);
        taskGrid->addColumn("Client",        130.f, true);
        taskGrid->addColumn("Priority",       80.f, true);
        taskGrid->addColumn("Status",         90.f, true);
        taskGrid->addColumn("Due Date",      100.f, true);
        taskGrid->addColumn("Assignee",      120.f, true);

        const char* taskTypes[] = {
            "Follow up call", "Send proposal", "Schedule demo", "Review contract",
            "Update CRM", "Prepare presentation", "Send invoice", "Negotiate terms",
            "Onboarding call", "Quarterly review", "Collect feedback", "Renew contract",
            "Upsell opportunity", "Technical support", "Data migration"
        };
        const char* priorities[] = {"High", "Medium", "Low"};
        const char* taskStatuses[] = {"To Do", "In Progress", "Blocked", "Done"};
        int nFirst = sizeof(firstNames) / sizeof(firstNames[0]);
        int nLast  = sizeof(lastNames) / sizeof(lastNames[0]);

        for (int i = 0; i < 150; ++i) {
            char client[64];
            snprintf(client, sizeof(client), "%s %s",
                     firstNames[crmRand(nFirst)], lastNames[crmRand(nLast)]);
            char assignee[64];
            snprintf(assignee, sizeof(assignee), "%s %s",
                     firstNames[crmRand(nFirst)], lastNames[crmRand(nLast)]);
            char dueDate[12];
            snprintf(dueDate, sizeof(dueDate), "2026-%02d-%02d",
                     4 + crmRand(3), 1 + crmRand(28));

            taskGrid->addRow({
                taskTypes[crmRand(15)],
                client,
                priorities[crmRand(3)],
                taskStatuses[crmRand(4)],
                dueDate,
                assignee
            });
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  TAB 4: Reports / Charts
    // ═══════════════════════════════════════════════════════════════════════
    {
        auto* reportsPage = tabs->addTab<BoxLayout>("Reports", LayoutDir::Vertical);
        auto* vbox = reportsPage->createChild<BoxLayout>(LayoutDir::Vertical);
        vbox->setSpacing(10);
        vbox->setPadding(12);
        vbox->setStretch(1);

        vbox->createChild<Label>("Sales Overview")->setColor(Color(140, 180, 220, 255));
        vbox->createChild<Line>();

        // Two charts side by side
        auto* chartRow = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
        chartRow->setSpacing(12);
        chartRow->setStretch(1);

        // Revenue over months (line chart)
        auto* leftChart = chartRow->createChild<BoxLayout>(LayoutDir::Vertical);
        leftChart->setSpacing(4);
        leftChart->setStretch(1);
        leftChart->createChild<Label>("Monthly Revenue ($K)");
        auto* plot = leftChart->createChild<PlotWidget>();
        plot->setStretch(1);
        {
            std::vector<float> rev2025 = {120,135,128,142,155,148,162,170,158,175,180,195};
            std::vector<float> rev2026 = {145,160,172,185,0,0,0,0,0,0,0,0};
            int s0 = plot->addSeries("2025", Color(100, 150, 220, 255));
            plot->setPoints(s0, rev2025);
            int s1 = plot->addSeries("2026", Color(100, 220, 120, 255));
            plot->setPoints(s1, rev2026);
        }

        // Deal stage histogram
        auto* rightChart = chartRow->createChild<BoxLayout>(LayoutDir::Vertical);
        rightChart->setSpacing(4);
        rightChart->setStretch(1);
        rightChart->createChild<Label>("Deal Distribution by Value");
        auto* hist = rightChart->createChild<HistogramWidget>();
        hist->setStretch(1);
        {
            std::vector<float> dealValues;
            for (int i = 0; i < 300; ++i)
                dealValues.push_back(static_cast<float>(5000 + crmRand(200000)));
            hist->setData(dealValues);
            hist->setBinCount(20);
        }

        // Bottom: Progress bars for team targets
        vbox->createChild<Line>();
        vbox->createChild<Label>("Team Targets Q2 2026");

        auto makeTarget = [](BoxLayout* parent, const char* name, float pct) {
            auto* row = parent->createChild<BoxLayout>(LayoutDir::Horizontal);
            row->setSpacing(8);
            row->setPadding(Edges(0, 2, 0, 2));
            auto* lbl = row->createChild<Label>(name);
            lbl->setSize(120, 0);
            auto* pb = row->createChild<ProgressBar>();
            pb->setValue(pct);
            pb->setStretch(1);
            char txt[16]; snprintf(txt, sizeof(txt), "%.0f%%", pct * 100.f);
            auto* val = row->createChild<Label>(txt);
            val->setSize(50, 0);
        };

        makeTarget(vbox, "Team Alpha",  0.82f);
        makeTarget(vbox, "Team Beta",   0.64f);
        makeTarget(vbox, "Team Gamma",  0.91f);
        makeTarget(vbox, "Team Delta",  0.45f);
        makeTarget(vbox, "Team Epsilon", 0.73f);
    }

    // ── Status Bar ───────────────────────────────────────────────────────
    auto* statusBar = outer->createChild<StatusBar>();
    statusBar->createChild<Label>("Ready");
    statusBar->createChild<Spacer>(0.f)->setStretch(1);
    statusBar->createChild<Label>("500 clients  |  300 deals  |  150 tasks");
}
