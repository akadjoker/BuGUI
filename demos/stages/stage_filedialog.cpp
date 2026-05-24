#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "FileDialog.hpp"
using namespace BuGUI;

void registerFileDialogStage(WidgetApp& app)
{
    auto* root = app.addStage("filedialog");

    auto* vbox = root->createChild<BoxLayout>(LayoutDir::Vertical);
    vbox->setStretch(1);
    vbox->setSpacing(12);
    vbox->setPadding(40);

    auto* topBar = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    topBar->setSpacing(8);
    auto* backTop = topBar->createChild<Button>("\u2190 Menu");
    backTop->clicked.connect([&app] {
        app.setStage("menu", TransitionType::CoverRight);
    });
    topBar->createChild<Spacer>(1)->setStretch(1);

    vbox->createChild<Label>("FileDialog Demo");
    vbox->createChild<Spacer>(10);

    auto* resultLabel = vbox->createChild<Label>("(no file selected)");

    auto* hbox = vbox->createChild<BoxLayout>(LayoutDir::Horizontal);
    hbox->setSpacing(10);

    auto* btnOpen = hbox->createChild<Button>("Open File...");
    btnOpen->clicked.connect([&app, resultLabel] {
        auto* fd = app.addFloat<FileDialog>("Open File");
        fd->setMode(FileDialog::Mode::Open);
        fd->setMultiSelect(false);
        fd->accepted.connect([resultLabel](const std::string& path) {
            resultLabel->setText("Opened: " + path);
        });
    });

    auto* btnSave = hbox->createChild<Button>("Save File...");
    btnSave->clicked.connect([&app, resultLabel] {
        auto* fd = app.addFloat<FileDialog>("Save File");
        fd->setMode(FileDialog::Mode::Save);
        fd->setFilter("*.cpp;*.hpp;*.txt");
        fd->accepted.connect([resultLabel](const std::string& path) {
            resultLabel->setText("Save to: " + path);
        });
    });

    auto* btnFolder = hbox->createChild<Button>("Select Folder...");
    btnFolder->clicked.connect([&app, resultLabel] {
        auto* fd = app.addFloat<FileDialog>("Select Folder");
        fd->setMode(FileDialog::Mode::SelectFolder);
        fd->accepted.connect([resultLabel](const std::string& path) {
            resultLabel->setText("Folder: " + path);
        });
    });

    auto* btnImage = hbox->createChild<Button>("Open Image...");
    btnImage->clicked.connect([&app, resultLabel] {
        auto* fd = app.addFloat<FileDialog>("Open Image");
        fd->setMode(FileDialog::Mode::OpenImage);
        fd->accepted.connect([resultLabel](const std::string& path) {
            resultLabel->setText("Image: " + path);
        });
    });

    vbox->createChild<Spacer>(10);

    auto* btnBack = vbox->createChild<Button>("<  Back to menu");
    btnBack->clicked.connect([&app] {
        app.setStage("menu", TransitionType::CoverRight);
    });
}
