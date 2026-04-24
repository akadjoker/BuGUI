#pragma once

#include "StageCommon.hpp"

// ═════════════════════════════════════════════════════════════════════════════
//  FileDialog demo stage
// ═════════════════════════════════════════════════════════════════════════════

inline void buildFileDialogStage(WidgetApp& app)
{
    Widget* root = app.addStage("filedialog");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Navigation ───────────────────────────────────────────────────────
    auto* nav = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    nav->setSize(0, 28);
    nav->setSpacing(8);
    nav->setPadding(16, 16, 0, 16);
    auto* back = nav->createChild<Button>("<  TreeGrid");
    back->clicked.connect([]{ WidgetApp::instance().setStage("treegrid", TransitionType::SlideRight, EaseType::OutBack); });
    nav->createChild<Label>("FileDialog")->setColor(Clr::accent);

    // ── Body ─────────────────────────────────────────────────────────────
    auto* body = outer->createChild<BoxLayout>(LayoutDir::Vertical);
    body->setStretch(1);
    body->setSpacing(12);
    body->setPadding(16);

    body->createChild<Label>("Click buttons below to open file dialogs as floating windows.")->setColor(Clr::dim);

    // Result display
    auto* resultLabel = body->createChild<Label>("Selected: (none)");
    resultLabel->setColor(Clr::dim);

    // ── Buttons ──────────────────────────────────────────────────────────
    auto* btns = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    btns->setSpacing(8);
    btns->setSize(0, 36);

    auto* openBtn = btns->createChild<Button>("Open File");
    openBtn->setSize(120, 0);
    openBtn->clicked.connect([resultLabel, &app] {
        auto* fd = app.addFloat<FileDialog>("Open File");
        fd->setMode(FileDialog::Mode::Open);
        fd->setFloatPos(80, 60);
        fd->accepted.connect([resultLabel](const std::string& path) {
            resultLabel->setText("Opened: " + path);
            resultLabel->setColor(Color(100, 220, 100, 255));
        });
        fd->cancelled.connect([resultLabel] {
            resultLabel->setText("Cancelled");
            resultLabel->setColor(Clr::dim);
        });
    });

    auto* saveBtn = btns->createChild<Button>("Save File");
    saveBtn->setSize(120, 0);
    saveBtn->clicked.connect([resultLabel, &app] {
        auto* fd = app.addFloat<FileDialog>("Save File");
        fd->setMode(FileDialog::Mode::Save);
        fd->setFloatPos(100, 80);
        fd->accepted.connect([resultLabel](const std::string& path) {
            resultLabel->setText("Saved: " + path);
            resultLabel->setColor(Color(100, 180, 255, 255));
        });
    });

    auto* folderBtn = btns->createChild<Button>("Select Folder");
    folderBtn->setSize(130, 0);
    folderBtn->clicked.connect([resultLabel, &app] {
        auto* fd = app.addFloat<FileDialog>("Select Folder");
        fd->setMode(FileDialog::Mode::SelectFolder);
        fd->setFloatPos(120, 100);
        fd->accepted.connect([resultLabel](const std::string& path) {
            resultLabel->setText("Folder: " + path);
            resultLabel->setColor(Color(220, 180, 100, 255));
        });
    });

    auto* filterBtn = btns->createChild<Button>("Open C++ Files");
    filterBtn->setSize(140, 0);
    filterBtn->clicked.connect([resultLabel, &app] {
        auto* fd = app.addFloat<FileDialog>("Open C++ Source");
        fd->setMode(FileDialog::Mode::Open);
        fd->setFilter("*.cpp;*.hpp;*.h;*.c");
        fd->setFloatPos(90, 70);
        fd->addBookmark("Project", "/media/ctw04578/data/projects/zen/BuGUI");
        fd->accepted.connect([resultLabel](const std::string& path) {
            resultLabel->setText("Source: " + path);
            resultLabel->setColor(Color(100, 220, 100, 255));
        });
    });

    // ── Row 2 ─────────────────────────────────────────────────────────────
    auto* btns2 = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    btns2->setSpacing(8);
    btns2->setSize(0, 36);

    auto* imgBtn = btns2->createChild<Button>("Open Image");
    imgBtn->setSize(120, 0);
    imgBtn->clicked.connect([resultLabel, &app] {
        auto* fd = app.addFloat<FileDialog>("Open Image");
        fd->setMode(FileDialog::Mode::OpenImage);
        fd->setFloatPos(60, 50);
        fd->accepted.connect([resultLabel](const std::string& path) {
            resultLabel->setText("Image: " + path);
            resultLabel->setColor(Color(180, 100, 255, 255));
        });
    });

    auto* saveImgBtn = btns2->createChild<Button>("Save Image");
    saveImgBtn->setSize(120, 0);
    saveImgBtn->clicked.connect([resultLabel, &app] {
        auto* fd = app.addFloat<FileDialog>("Save Image");
        fd->setMode(FileDialog::Mode::SaveImage);
        fd->setFloatPos(80, 70);
        fd->accepted.connect([resultLabel](const std::string& path) {
            resultLabel->setText("Save Image: " + path);
            resultLabel->setColor(Color(255, 180, 100, 255));
        });
    });

    auto* multiBtn = btns2->createChild<Button>("Multi-Select");
    multiBtn->setSize(130, 0);
    multiBtn->clicked.connect([resultLabel, &app] {
        auto* fd = app.addFloat<FileDialog>("Select Files (Multi)");
        fd->setMode(FileDialog::Mode::Open);
        fd->setMultiSelect(true);
        fd->setFloatPos(100, 60);
        fd->acceptedMulti.connect([resultLabel](const std::vector<std::string>& paths) {
            resultLabel->setText("Selected " + std::to_string(paths.size()) + " files");
            resultLabel->setColor(Color(100, 220, 180, 255));
        });
        fd->cancelled.connect([resultLabel] {
            resultLabel->setText("Cancelled");
            resultLabel->setColor(Clr::dim);
        });
    });

    // ── Row 3: MessageBox / InputBox / Toast ──────────────────────────────
    body->createChild<Line>();
    body->createChild<Label>("Dialogs & Notifications")->setColor(Clr::accent);

    auto* btns3 = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    btns3->setSpacing(8);
    btns3->setSize(0, 36);

    auto* mbInfoBtn = btns3->createChild<Button>("MessageBox Info");
    mbInfoBtn->setSize(140, 0);
    mbInfoBtn->clicked.connect([resultLabel, &app] {
        auto* mb = app.addFloat<MessageBox>("Info", "This is an informational message.",
                                              MessageBox::Ok, MessageBox::Info);
        mb->result.connect([resultLabel](MessageBox::Result r) {
            resultLabel->setText("MessageBox: OK");
            resultLabel->setColor(Clr::dim);
        });
    });

    auto* mbYesNoBtn = btns3->createChild<Button>("Confirm YesNo");
    mbYesNoBtn->setSize(140, 0);
    mbYesNoBtn->clicked.connect([resultLabel, &app] {
        auto* mb = app.addFloat<MessageBox>("Confirm", "Delete this file permanently?",
                                              MessageBox::YesNo, MessageBox::Warning);
        mb->result.connect([resultLabel](MessageBox::Result r) {
            if (r == MessageBox::ResultYes)
                resultLabel->setText("Confirmed: Yes");
            else
                resultLabel->setText("Confirmed: No");
            resultLabel->setColor(Color(220, 180, 100, 255));
        });
    });

    auto* ibBtn = btns3->createChild<Button>("InputBox");
    ibBtn->setSize(120, 0);
    ibBtn->clicked.connect([resultLabel, &app] {
        auto* ib = app.addFloat<InputBox>("Rename", "Enter new filename:");
        ib->setText("document.txt");
        ib->accepted.connect([resultLabel](const std::string& text) {
            resultLabel->setText("Input: " + text);
            resultLabel->setColor(Color(100, 220, 180, 255));
        });
        ib->cancelled.connect([resultLabel] {
            resultLabel->setText("Input: Cancelled");
            resultLabel->setColor(Clr::dim);
        });
    });

    // ── Row 4: Toast demos ────────────────────────────────────────────────
    auto* btns4 = body->createChild<BoxLayout>(LayoutDir::Horizontal);
    btns4->setSpacing(8);
    btns4->setSize(0, 36);

    auto* tInfo = btns4->createChild<Button>("Toast Info");
    tInfo->setSize(100, 0);
    tInfo->clicked.connect([] { Toast::show("This is an info toast.", Toast::Info); });

    auto* tOk = btns4->createChild<Button>("Toast Success");
    tOk->setSize(120, 0);
    tOk->clicked.connect([] { Toast::show("File saved successfully!", Toast::Success); });

    auto* tWarn = btns4->createChild<Button>("Toast Warning");
    tWarn->setSize(120, 0);
    tWarn->clicked.connect([] { Toast::show("Disk space running low.", Toast::Warning, 5.0f); });

    auto* tErr = btns4->createChild<Button>("Toast Error");
    tErr->setSize(110, 0);
    tErr->clicked.connect([] { Toast::show("Connection failed!", Toast::Error, 4.0f); });
}
