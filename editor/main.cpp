#include "WidgetApp.hpp"
#include "Widgets.hpp"
#include "WidgetSerializer.hpp"
#include "Batch.hpp"

#include <string>
#include <vector>
#include <functional>
#include <stdexcept>

namespace {

Widget* buildEmptyDocument(Widget* parent)
{
    auto* root = parent->createChild<Panel>();
    root->setId("doc_root");
    root->setBgColor(Color(24, 26, 30, 255));
    return root;
}

void fillHierarchy(ListBox* list, Widget* root, std::vector<Widget*>& outOrder)
{
    if (!list) return;
    list->clear();
    outOrder.clear();
    if (!root) return;

    std::function<void(Widget*, int)> walk = [&](Widget* w, int depth)
    {
        outOrder.push_back(w);

        std::string item;
        for (int i = 0; i < depth; ++i) item += "  ";

        item += WidgetSerializer::typeName(w);
        if (!w->id().empty())
            item += " #" + w->id();

        list->addItem(item);

        for (auto* c : w->children())
            walk(c, depth + 1);
    };

    walk(root, 0);
}

bool isDescendantOf(Widget* w, Widget* root)
{
    Widget* cur = w;
    while (cur)
    {
        if (cur == root) return true;
        cur = cur->parent();
    }
    return false;
}

void fitPreviewToHost(Widget* preview, Widget* host)
{
    if (!preview || !host) return;

    Rect want{8.0f, 8.0f, host->rect().w - 16.0f, host->rect().h - 16.0f};
    if (want.w < 0) want.w = 0;
    if (want.h < 0) want.h = 0;

    const Rect& cur = preview->rect();
    if (cur.x != want.x || cur.y != want.y || cur.w != want.w || cur.h != want.h)
        preview->setRect(want);
}

bool tryParseFloat(const std::string& s, float& out)
{
    try
    {
        size_t used = 0;
        out = std::stof(s, &used);
        return used == s.size();
    }
    catch (const std::exception&)
    {
        return false;
    }
}

std::string f2(float v)
{
    char b[64];
    snprintf(b, sizeof(b), "%.2f", v);
    return b;
}

Widget* createWidgetByType(const std::string& type, Widget* parent)
{
    if (!parent) return nullptr;

    if (type == "Panel")       return parent->createChild<Panel>();
    if (type == "BoxLayout")   return parent->createChild<BoxLayout>(LayoutDir::Vertical);
    if (type == "Label")       return parent->createChild<Label>("Label");
    if (type == "Button")      return parent->createChild<Button>("Button");
    if (type == "TextInput")   return parent->createChild<TextInput>("TextInput");
    if (type == "CheckBox")    return parent->createChild<CheckBox>("CheckBox");
    if (type == "Slider")      return parent->createChild<Slider>(0.0f, 100.0f, 50.0f);
    if (type == "ProgressBar") return parent->createChild<ProgressBar>(0.0f, 100.0f, 50.0f);
    if (type == "Line")        return parent->createChild<Line>(1.0f);
    if (type == "Spacer")      return parent->createChild<Spacer>(10.0f);

    return nullptr;
}

std::string widgetTextValue(Widget* w)
{
    if (!w) return "";
    if (auto* x = dynamic_cast<Label*>(w)) return x->text();
    if (auto* x = dynamic_cast<Button*>(w)) return x->text();
    if (auto* x = dynamic_cast<TextInput*>(w)) return x->text();
    if (auto* x = dynamic_cast<CheckBox*>(w)) return x->text();
    return "";
}

void setWidgetTextValue(Widget* w, const std::string& text)
{
    if (!w) return;
    if (auto* x = dynamic_cast<Label*>(w)) { x->setText(text); return; }
    if (auto* x = dynamic_cast<Button*>(w)) { x->setText(text); return; }
    if (auto* x = dynamic_cast<TextInput*>(w)) { x->setText(text); return; }
    if (auto* x = dynamic_cast<CheckBox*>(w)) { x->setText(text); return; }
}

} // namespace

int main()
{
    auto& app = WidgetApp::instance();
    if (!app.init("BuGUI Editor", 1400, 860))
        return 1;

    Widget* stage = app.addStage("editor");

    auto* outer = stage->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setPadding(10, 10, 10, 10);
    outer->setSpacing(8);

    auto* title = outer->createChild<Label>("BuGUI Editor (MVP)");
    title->setColor(Color(130, 210, 255, 255));

    auto* toolbar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    toolbar->setSpacing(8);

    auto* pathInput = toolbar->createChild<TextInput>("editor_layout.json");
    pathInput->setSize(420, 30);

    auto* newBtn = toolbar->createChild<Button>("New");
    auto* loadBtn = toolbar->createChild<Button>("Load");
    auto* saveBtn = toolbar->createChild<Button>("Save");
    auto* refreshBtn = toolbar->createChild<Button>("Refresh Tree");

    auto* status = outer->createChild<Label>("Ready.");

    auto* body = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    body->setSpacing(10);
    body->setStretch(1.0f);

    auto* hierarchyPanel = body->createChild<Panel>();
    hierarchyPanel->setStretch(1.0f);
    auto* hCol = hierarchyPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    hCol->setPadding(8, 8, 8, 8);
    hCol->setSpacing(6);
    hCol->createChild<Label>("Hierarchy");
    auto* hierarchy = hCol->createChild<ListBox>();
    hierarchy->setStretch(1.0f);

    auto* previewPanel = body->createChild<Panel>();
    previewPanel->setStretch(3.0f);
    auto* pCol = previewPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    pCol->setPadding(8, 8, 8, 8);
    pCol->setSpacing(6);
    pCol->createChild<Label>("Preview (runtime)");
    auto* previewHost = pCol->createChild<Panel>();
    previewHost->setStretch(1.0f);
    previewHost->setId("preview_host");

    auto* inspectorPanel = body->createChild<Panel>();
    inspectorPanel->setStretch(1.2f);
    auto* iCol = inspectorPanel->createChild<BoxLayout>(LayoutDir::Vertical);
    iCol->setPadding(8, 8, 8, 8);
    iCol->setSpacing(6);
    iCol->createChild<Label>("Inspector");

    auto* palette = iCol->createChild<ComboBox>();
    palette->addItem("Panel");
    palette->addItem("BoxLayout");
    palette->addItem("Label");
    palette->addItem("Button");
    palette->addItem("TextInput");
    palette->addItem("CheckBox");
    palette->addItem("Slider");
    palette->addItem("ProgressBar");
    palette->addItem("Line");
    palette->addItem("Spacer");
    palette->setSelectedIndex(0);

    auto* addChildBtn = iCol->createChild<Button>("Add Child");
    auto* deleteBtn = iCol->createChild<Button>("Delete Selected");
    iCol->createChild<Line>(1.0f);

    iCol->createChild<Label>("ID");
    auto* idInput = iCol->createChild<TextInput>("");
    iCol->createChild<Label>("Text (Label/Button/TextInput/CheckBox)");
    auto* textInput = iCol->createChild<TextInput>("");
    iCol->createChild<Label>("X");
    auto* xInput = iCol->createChild<TextInput>("0");
    iCol->createChild<Label>("Y");
    auto* yInput = iCol->createChild<TextInput>("0");
    iCol->createChild<Label>("W");
    auto* wInput = iCol->createChild<TextInput>("100");
    iCol->createChild<Label>("H");
    auto* hInput = iCol->createChild<TextInput>("40");
    auto* applyBtn = iCol->createChild<Button>("Apply Properties");

    Widget* previewDoc = buildEmptyDocument(previewHost);

    Widget* selectedWidget = previewDoc;
    std::vector<Widget*> hierarchyOrder;

    auto rebuildHierarchy = [hierarchy, &previewDoc, &hierarchyOrder, &selectedWidget]() {
        fillHierarchy(hierarchy, previewDoc, hierarchyOrder);
        if (!selectedWidget)
        {
            hierarchy->setSelectedIndex(-1);
            return;
        }
        int idx = -1;
        for (int i = 0; i < (int)hierarchyOrder.size(); ++i)
        {
            if (hierarchyOrder[i] == selectedWidget)
            {
                idx = i;
                break;
            }
        }
        hierarchy->setSelectedIndex(idx);
    };

    auto setStatus = [status](const std::string& text) {
        status->setText(text);
    };

    auto refreshInspector = [&selectedWidget, idInput, textInput, xInput, yInput, wInput, hInput]() {
        if (!selectedWidget)
        {
            idInput->setText("");
            textInput->setText("");
            xInput->setText("0");
            yInput->setText("0");
            wInput->setText("0");
            hInput->setText("0");
            return;
        }

        const Rect& r = selectedWidget->rect();
        idInput->setText(selectedWidget->id());
        textInput->setText(widgetTextValue(selectedWidget));
        xInput->setText(f2(r.x));
        yInput->setText(f2(r.y));
        wInput->setText(f2(r.w));
        hInput->setText(f2(r.h));
    };

    rebuildHierarchy();
    refreshInspector();

    hierarchy->selectionChanged.connect([&hierarchyOrder, &selectedWidget, setStatus, refreshInspector](int idx) {
        if (idx < 0 || idx >= (int)hierarchyOrder.size())
            return;
        selectedWidget = hierarchyOrder[idx];
        setStatus("Selected: " + WidgetSerializer::typeName(selectedWidget));
        refreshInspector();
    });

    newBtn->clicked.connect([previewHost, &previewDoc, &selectedWidget, rebuildHierarchy, setStatus, refreshInspector]() {
        if (previewDoc && previewDoc->parent() == previewHost)
            previewHost->removeChild(previewDoc);
        previewDoc = buildEmptyDocument(previewHost);
        selectedWidget = previewDoc;
        rebuildHierarchy();
        refreshInspector();
        setStatus("New empty document created.");
    });

    loadBtn->clicked.connect([previewHost, pathInput, &previewDoc, &selectedWidget, rebuildHierarchy, setStatus, refreshInspector]() {
        const std::string path = pathInput->text();
        if (path.empty())
        {
            setStatus("Load failed: empty path.");
            return;
        }

        Widget* loaded = WidgetSerializer::loadFromFile(path, previewHost);
        if (!loaded)
        {
            setStatus("Load failed: " + path);
            return;
        }

        if (previewDoc && previewDoc->parent() == previewHost)
            previewHost->removeChild(previewDoc);

        previewDoc = loaded;
        selectedWidget = previewDoc;
        rebuildHierarchy();
        refreshInspector();
        setStatus("Loaded: " + path);
    });

    saveBtn->clicked.connect([pathInput, &previewDoc, setStatus]() {
        const std::string path = pathInput->text();
        if (!previewDoc)
        {
            setStatus("Save failed: no document.");
            return;
        }
        if (path.empty())
        {
            setStatus("Save failed: empty path.");
            return;
        }

        if (WidgetSerializer::saveToFile(previewDoc, path))
            setStatus("Saved: " + path);
        else
            setStatus("Save failed: " + path);
    });

    refreshBtn->clicked.connect([rebuildHierarchy, setStatus]() {
        rebuildHierarchy();
        setStatus("Hierarchy refreshed.");
    });

    addChildBtn->clicked.connect([palette, &selectedWidget, &previewDoc, rebuildHierarchy, refreshInspector, setStatus]() {
        Widget* parent = selectedWidget ? selectedWidget : previewDoc;
        if (!parent)
        {
            setStatus("Add failed: no parent.");
            return;
        }
        int idx = palette->selectedIndex();
        if (idx < 0 || idx >= palette->itemCount())
        {
            setStatus("Add failed: no type selected.");
            return;
        }
        Widget* nw = createWidgetByType(palette->itemText(idx), parent);
        if (!nw)
        {
            setStatus("Add failed for type.");
            return;
        }
        selectedWidget = nw;
        rebuildHierarchy();
        refreshInspector();
        setStatus("Added: " + WidgetSerializer::typeName(nw));
    });

    deleteBtn->clicked.connect([&selectedWidget, &previewDoc, rebuildHierarchy, refreshInspector, setStatus]() {
        if (!selectedWidget)
        {
            setStatus("Delete failed: nothing selected.");
            return;
        }
        if (selectedWidget == previewDoc)
        {
            setStatus("Delete blocked: document root.");
            return;
        }
        Widget* victim = selectedWidget;
        Widget* p = victim->parent();
        if (!p)
        {
            setStatus("Delete failed: no parent.");
            return;
        }
        selectedWidget = p;
        p->removeChild(victim);
        rebuildHierarchy();
        refreshInspector();
        setStatus("Deleted widget.");
    });

    applyBtn->clicked.connect([&selectedWidget, idInput, textInput, xInput, yInput, wInput, hInput, rebuildHierarchy, refreshInspector, setStatus]() {
        if (!selectedWidget)
        {
            setStatus("Apply failed: nothing selected.");
            return;
        }

        Rect r = selectedWidget->rect();
        float v;
        if (tryParseFloat(xInput->text(), v)) r.x = v;
        if (tryParseFloat(yInput->text(), v)) r.y = v;
        if (tryParseFloat(wInput->text(), v)) r.w = v;
        if (tryParseFloat(hInput->text(), v)) r.h = v;
        selectedWidget->setRect(r);
        selectedWidget->setId(idInput->text());
        setWidgetTextValue(selectedWidget, textInput->text());

        rebuildHierarchy();
        refreshInspector();
        setStatus("Applied properties.");
    });

    app.onAny("press", [&previewDoc, &selectedWidget, rebuildHierarchy, refreshInspector](Widget* w) {
        if (!previewDoc || !w) return;
        if (!isDescendantOf(w, previewDoc)) return;
        selectedWidget = w;
        rebuildHierarchy();
        refreshInspector();
    });

    app.setOverlayCallback([&selectedWidget](PaintContext& ctx) {
        if (!selectedWidget || !selectedWidget->isVisible()) return;
        Rect r = selectedWidget->absoluteRect();
        ctx.line.SetColor(255, 210, 90, 255);
        ctx.lineRect(r.x, r.y, r.w, r.h);
    });

    app.setOnIdle([previewHost, &previewDoc](float) {
        fitPreviewToHost(previewDoc, previewHost);
    });

    app.setStage("editor");
    return app.run();
}
