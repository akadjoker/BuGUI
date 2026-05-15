#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "ScrollWidgets.hpp"
using namespace BuGUI;

void registerMenuStage(WidgetApp& app)
{
    auto* root = app.addStage("menu");

    auto* sv = root->createChild<ScrollView>();
    sv->setStretch(1);

    auto* vbox = sv->setContent<BoxLayout>(LayoutDir::Vertical);
    vbox->setSpacing(10.0f);
    vbox->setPadding(40.0f);

    vbox->createChild<Label>("BuGUI Demo");
    vbox->createChild<Spacer>(20.0f);

    auto* btn1 = vbox->createChild<Button>("01 - Basic Widgets");
    btn1->clicked.connect([&app]() {
        app.setStage("basic", TransitionType::CoverLeft);
    });

    auto* btn2 = vbox->createChild<Button>("02 - Input Widgets");
    btn2->clicked.connect([&app]() {
        app.setStage("inputs", TransitionType::CoverLeft);
    });

    auto* btn3 = vbox->createChild<Button>("03 - Layouts");
    btn3->clicked.connect([&app]() {
        app.setStage("layouts", TransitionType::CoverLeft);
    });

    auto* btn4 = vbox->createChild<Button>("04 - Scroll & Lists");
    btn4->clicked.connect([&app]() {
        app.setStage("scroll", TransitionType::CoverLeft);
    });

    auto* btn5 = vbox->createChild<Button>("05 - Controls");
    btn5->clicked.connect([&app]() {
        app.setStage("controls", TransitionType::CoverLeft);
    });

    auto* btn6 = vbox->createChild<Button>("06 - Text & Combo");
    btn6->clicked.connect([&app]() {
        app.setStage("inputs2", TransitionType::CoverLeft);
    });

    auto* btn7 = vbox->createChild<Button>("07 - Menus & Context");
    btn7->clicked.connect([&app]() {
        app.setStage("menus", TransitionType::CoverLeft);
    });

    auto* btn8 = vbox->createChild<Button>("08 - Dialogs & Toasts");
    btn8->clicked.connect([&app]() {
        app.setStage("dialogs", TransitionType::CoverLeft);
    });

    auto* btn9 = vbox->createChild<Button>("09 - Tree / Property / Color");
    btn9->clicked.connect([&app]() {
        app.setStage("treepropcolor", TransitionType::CoverLeft);
    });

    auto* btn10 = vbox->createChild<Button>("10 - DataGrid / TreeGrid");
    btn10->clicked.connect([&app]() {
        app.setStage("datagrid", TransitionType::CoverLeft);
    });

    auto* btn11 = vbox->createChild<Button>("11 - FloatWindow");
    btn11->clicked.connect([&app]() {
        app.setStage("floatwindow", TransitionType::CoverLeft);
    });

    auto* btn12 = vbox->createChild<Button>("12 - Gizmo2D + Gizmo3D");
    btn12->clicked.connect([&app]() {
        app.setStage("gizmos", TransitionType::CoverLeft);
    });

    auto* btn13 = vbox->createChild<Button>("13 - AnchorLayout + Panel + Overlay");
    btn13->clicked.connect([&app]() {
        app.setStage("anchorlayouts", TransitionType::CoverLeft);
    });

    auto* btn14 = vbox->createChild<Button>("14 - Carousel");
    btn14->clicked.connect([&app]() {
        app.setStage("carousel", TransitionType::CoverLeft);
    });

    auto* btn15 = vbox->createChild<Button>("15 - SlidePanel / Drawer");
    btn15->clicked.connect([&app]() {
        app.setStage("slidepanel", TransitionType::CoverLeft);
    });

    auto* btn16 = vbox->createChild<Button>("16 - Charts (Gradient · Plot · Histogram · Curve)");
    btn16->clicked.connect([&app]() {
        app.setStage("charts", TransitionType::CoverLeft);
    });

    auto* btn17 = vbox->createChild<Button>("17 - Node Editor");
    btn17->clicked.connect([&app]() {
        app.setStage("nodeeditor", TransitionType::CoverLeft);
    });

    auto* btn18 = vbox->createChild<Button>("18 - Timeline");
    btn18->clicked.connect([&app]() {
        app.setStage("timeline", TransitionType::CoverLeft);
    });

    auto* btn19 = vbox->createChild<Button>("19 - Audio Widgets");
    btn19->clicked.connect([&app]() {
        app.setStage("audio", TransitionType::CoverLeft);
    });

    auto* btn20 = vbox->createChild<Button>("20 - Thumbnail Grid");
    btn20->clicked.connect([&app]() {
        app.setStage("thumbnail", TransitionType::CoverLeft);
    });

    auto* btn21 = vbox->createChild<Button>("21 - File Dialog");
    btn21->clicked.connect([&app]() {
        app.setStage("filedialog", TransitionType::CoverLeft);
    });

    auto* btn22 = vbox->createChild<Button>("22 - Console");
    btn22->clicked.connect([&app]() {
        app.setStage("console", TransitionType::CoverLeft);
    });

    auto* btn23 = vbox->createChild<Button>("23 - Dock Panel");
    btn23->clicked.connect([&app]() {
        app.setStage("dock", TransitionType::CoverLeft);
    });

    auto* btn24 = vbox->createChild<Button>("24 - New Features (Toolbar / SpinBox / Markers / DnD)");
    btn24->clicked.connect([&app]() {
        app.setStage("newfeatures", TransitionType::CoverLeft);
    });

    auto* btn25 = vbox->createChild<Button>("25 - Date & Time Pickers");
    btn25->clicked.connect([&app]() {
        app.setStage("datetime", TransitionType::CoverLeft);
    });

    auto* btn26 = vbox->createChild<Button>("26 - Automotive Cluster");
    btn26->clicked.connect([&app]() {
        app.setStage("cluster", TransitionType::CoverLeft);
    });

    auto* btn27 = vbox->createChild<Button>("27 - Animation + Model/View");
    btn27->clicked.connect([&app]() {
        app.setStage("animmodel", TransitionType::CoverLeft);
    });

    vbox->createChild<Spacer>(20.0f);
    vbox->createChild<Label>("Stress Tests")->setColor(Color(200, 150, 80, 255));

    auto* btn28 = vbox->createChild<Button>("28 - Fake Blender (3D Editor)");
    btn28->clicked.connect([&app]() {
        app.setStage("fakeblender", TransitionType::CoverLeft);
    });

    auto* btn29 = vbox->createChild<Button>("29 - CRM (Client Manager, 500+ rows)");
    btn29->clicked.connect([&app]() {
        app.setStage("crm", TransitionType::CoverLeft);
    });

    auto* btn30 = vbox->createChild<Button>("30 - Fake IDE (Code Editor)");
    btn30->clicked.connect([&app]() {
        app.setStage("fakeide", TransitionType::CoverLeft);
    });

    auto* btn31 = vbox->createChild<Button>("31 - Fake Unity 3D (Game Engine)");
    btn31->clicked.connect([&app]() {
        app.setStage("fakeunity", TransitionType::CoverLeft);
    });

    vbox->createChild<Spacer>(10.0f);
    vbox->createChild<Label>("Widget Demos")->setColor(Color(100, 180, 200, 255));

    auto* btn32 = vbox->createChild<Button>("32 - ToolBar (Icon Grid / Drag Reorder)");
    btn32->clicked.connect([&app]() {
        app.setStage("toolbar", TransitionType::CoverLeft);
    });

    auto* btn33 = vbox->createChild<Button>("33 - CodeEditor (Syntax Highlight / Fold / Bracket Match)");
    btn33->clicked.connect([&app]() {
        app.setStage("codeeditor", TransitionType::CoverLeft);
    });

    auto* btn34 = vbox->createChild<Button>("34 - App Widgets (Breadcrumbs / Search / Outliner / RichText / Viewport3D)");
    btn34->clicked.connect([&app]() {
        app.setStage("appwidgets", TransitionType::CoverLeft);
    });

    auto* btn35 = vbox->createChild<Button>("35 - Focus & Keyboard Navigation (Tab cycling / onFocusGained / Context Menu)");
    btn35->clicked.connect([&app]() {
        app.setStage("focus", TransitionType::CoverLeft);
    });
}
