#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "CodeEditor.hpp"
#include "ConsoleWidget.hpp"
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  Stage — CodeEditor demo
//
//  Demonstrates the CodeEditor with different syntax highlighters.
//  - C++ source with folding and bracket matching
//  - Python script
//  - JavaScript
//  - Toolbar to switch languages and toggle features
// ─────────────────────────────────────────────────────────────────────────────

using namespace BuGUI;

static const char* sampleCpp = R"(#include <iostream>
#include <vector>
#include <string>
using namespace BuGUI;

// A simple class to demonstrate syntax highlighting
class Widget {
public:
    Widget(const std::string& name) : name_(name) {}
    virtual ~Widget() = default;

    void setSize(float w, float h) {
        width_ = w;
        height_ = h;
    }

    float area() const {
        return width_ * height_;
    }

    /* Multi-line comment:
       This is a block comment that spans
       multiple lines to test the lexer. */
    const std::string& name() const { return name_; }

protected:
    std::string name_;
    float width_  = 0.0f;
    float height_ = 0.0f;
};

int main() {
    std::vector<Widget*> widgets;
    auto* w = new Widget("Hello");
    w->setSize(100.0f, 50.0f);
    widgets.push_back(w);

    for (auto* widget : widgets) {
        std::cout << widget->name() << ": "
                  << widget->area() << " px²\n";
    }

    int hex = 0xFF00AA;
    double pi = 3.14159265;
    bool flag = true;

    delete w;
    return 0;
}
)";

static const char* samplePython = R"(#!/usr/bin/env python3
"""
Module docstring: Demonstrates Python highlighting.
Triple-quoted strings, decorators, f-strings.
"""

import os
from pathlib import Path

# Constants
MAX_RETRIES = 3
PI = 3.14159

class Animal:
    """Base class for animals."""

    def __init__(self, name: str, legs: int = 4):
        self.name = name
        self.legs = legs
        self._health = 100

    @property
    def is_alive(self) -> bool:
        return self._health > 0

    def speak(self):
        raise NotImplementedError

class Dog(Animal):
    def speak(self):
        return f"{self.name} says: Woof!"

class Cat(Animal):
    def speak(self):
        return f"{self.name} says: Meow!"

def main():
    animals = [Dog("Rex"), Cat("Whiskers"), Dog("Buddy")]

    for animal in animals:
        print(animal.speak())

    # Dictionary comprehension
    sounds = {a.name: a.speak() for a in animals}

    # Multi-line string
    text = '''
    This is a multi-line
    string literal.
    '''

    count = len(animals)
    result = True if count > 0 else False
    print(f"Total: {count}, has animals: {result}")

if __name__ == "__main__":
    main()
)";

static const char* sampleJs = R"(// JavaScript / TypeScript example

const API_URL = 'https://api.example.com';

/**
 * Fetch data from the API.
 * @param {string} endpoint - The API endpoint.
 * @returns {Promise<object>} The response data.
 */
async function fetchData(endpoint) {
    const url = `${API_URL}/${endpoint}`;

    try {
        const response = await fetch(url, {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json',
            },
        });

        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }

        return await response.json();
    } catch (error) {
        console.error('Fetch failed:', error);
        return null;
    }
}

// Arrow function with destructuring
const processItems = (items) => {
    return items
        .filter(item => item.active !== false)
        .map(({ name, value }) => ({
            label: name.toUpperCase(),
            score: value * 2.5,
        }));
};

class EventEmitter {
    #listeners = new Map();

    on(event, callback) {
        if (!this.#listeners.has(event)) {
            this.#listeners.set(event, []);
        }
        this.#listeners.get(event).push(callback);
    }

    emit(event, ...args) {
        const handlers = this.#listeners.get(event) ?? [];
        handlers.forEach(fn => fn(...args));
    }
}

// Template literal
const message = `Found ${42} items in ${3.14}s`;
let x = undefined;
let y = null;
let z = NaN;
)";

void registerCodeEditorStage(WidgetApp& app)
{
    auto* root = app.addStage("codeeditor");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setStretch(1);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Header ───────────────────────────────────────────────────────────
    auto* header = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    header->setSize(0, 32);
    header->setPadding(Edges(4, 4, 4, 4));
    header->setSpacing(8);
    {
        auto* back = header->createChild<Button>("\u2190 Menu");
        back->setSize(80, 24);
        back->clicked.connect([&app]() {
            app.setStage("menu", TransitionType::CoverRight);
        });
        header->createChild<Line>();
        header->createChild<Label>("CodeEditor —");
        auto* langLabel = header->createChild<Label>("C++");
        langLabel->setId("langLabel");
        header->createChild<Spacer>(0.f)->setStretch(1);
    }

    // ── Toolbar row 1: language + fold + zoom ────────────────────────────
    auto* bar1 = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    bar1->setSize(0, 30);
    bar1->setPadding(Edges(2, 8, 2, 8));
    bar1->setSpacing(4);

    auto* btnCpp = bar1->createChild<Button>("C++");       btnCpp->setSize(50, 24);
    auto* btnPy  = bar1->createChild<Button>("Python");    btnPy->setSize(66, 24);
    auto* btnJs  = bar1->createChild<Button>("JS");        btnJs->setSize(44, 24);
    bar1->createChild<Line>();
    auto* btnFold   = bar1->createChild<Button>("Fold All");   btnFold->setSize(68, 24);
    auto* btnUnfold = bar1->createChild<Button>("Unfold All"); btnUnfold->setSize(78, 24);
    bar1->createChild<Line>();
    auto* btnZoomIn  = bar1->createChild<Button>("+");    btnZoomIn->setSize(26, 24);
    auto* btnZoomOut = bar1->createChild<Button>("-");    btnZoomOut->setSize(26, 24);
    auto* btnZoomRst = bar1->createChild<Button>("100%"); btnZoomRst->setSize(48, 24);
    bar1->createChild<Spacer>(0.f)->setStretch(1);

    // ── Toolbar row 2: edit + toggles ────────────────────────────────────
    auto* bar2 = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    bar2->setSize(0, 30);
    bar2->setPadding(Edges(2, 8, 2, 8));
    bar2->setSpacing(4);

    auto* btnUndo = bar2->createChild<Button>("Undo"); btnUndo->setSize(54, 24);
    auto* btnRedo = bar2->createChild<Button>("Redo"); btnRedo->setSize(54, 24);
    bar2->createChild<Line>();
    auto* btnSelAll = bar2->createChild<Button>("Select All"); btnSelAll->setSize(78, 24);
    auto* btnSelOcc = bar2->createChild<Button>("Sel Occurrences"); btnSelOcc->setSize(112, 24);
    bar2->createChild<Line>();
    auto* chkReadOnly  = bar2->createChild<CheckBox>("Read Only");
    auto* chkWordWrap  = bar2->createChild<CheckBox>("Word Wrap");
    auto* chkMinimap   = bar2->createChild<CheckBox>("Minimap");
    auto* chkWhitespace= bar2->createChild<CheckBox>("Whitespace");
    chkMinimap->setChecked(true);
    chkWhitespace->setChecked(true);
    bar2->createChild<Spacer>(0.f)->setStretch(1);

    // ── Editor ───────────────────────────────────────────────────────────
    auto* editor = outer->createChild<CodeEditor>();
    editor->setStretch(1);
    editor->setHighlighter<CppHighlighter>();
    editor->setText(sampleCpp);
    editor->setShowFolding(true);
    editor->setShowBracketMatch(true);
    editor->setHighlightCurrentLine(true);
    editor->setShowIndentGuides(true);
    editor->setShowWhitespace(true);
    editor->setShowScopeLines(true);
    editor->setTabSize(4);
    editor->setShowMinimap(true);

    // ── Status bar ───────────────────────────────────────────────────────
    auto* statusBar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    statusBar->setSize(0, 22);
    statusBar->setPadding(Edges(2, 8, 2, 8));
    statusBar->setSpacing(12);
    auto* lblCursorPos = statusBar->createChild<Label>("Ln 1, Col 1");
    auto* lblLines     = statusBar->createChild<Label>("Lines: 0");
    auto* lblMode      = statusBar->createChild<Label>("");
    statusBar->createChild<Spacer>(0.f)->setStretch(1);

    // ── Console ──────────────────────────────────────────────────────────
    auto* console = outer->createChild<ConsoleWidget>();
    console->setSize(0, 100);
    console->log(LogLevel::Info, "CodeEditor ready. Edit, Ctrl+F to search, Ctrl+Z/Y undo/redo.");

    // ── Initial status ───────────────────────────────────────────────────
    auto updateStatus = [editor, lblLines, lblCursorPos]() {
        char buf[64];
        auto cp = editor->cursorPos();
        std::snprintf(buf, sizeof(buf), "Ln %d, Col %d", cp.line + 1, cp.col + 1);
        lblCursorPos->setText(buf);
        std::snprintf(buf, sizeof(buf), "Lines: %d", editor->lineCount());
        lblLines->setText(buf);
    };
    updateStatus();

    editor->cursorMoved.connect([updateStatus](TextEdit::TextPos) { updateStatus(); });
    editor->textChanged.connect([updateStatus]() { updateStatus(); });

    // ── Language buttons ─────────────────────────────────────────────────
    auto* langLabel = root->findById<Label>("langLabel");

    btnCpp->clicked.connect([editor, console, langLabel]() {
        editor->setHighlighter<CppHighlighter>();
        editor->setText(sampleCpp);
        if (langLabel) langLabel->setText("C++");
        console->log(LogLevel::Info, "Switched to C++");
    });
    btnPy->clicked.connect([editor, console, langLabel]() {
        editor->setHighlighter<PythonHighlighter>();
        editor->setText(samplePython);
        if (langLabel) langLabel->setText("Python");
        console->log(LogLevel::Info, "Switched to Python");
    });
    btnJs->clicked.connect([editor, console, langLabel]() {
        editor->setHighlighter<JsHighlighter>();
        editor->setText(sampleJs);
        if (langLabel) langLabel->setText("JS");
        console->log(LogLevel::Info, "Switched to JavaScript");
    });

    // ── Fold ─────────────────────────────────────────────────────────────
    btnFold->clicked.connect([editor, console]() {
        editor->foldAll();
        console->log(LogLevel::Info, "Folded all regions");
    });
    btnUnfold->clicked.connect([editor, console]() {
        editor->unfoldAll();
        console->log(LogLevel::Info, "Unfolded all regions");
    });
    editor->foldToggled.connect([console](int line, bool folded) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Line %d %s", line + 1, folded ? "folded" : "unfolded");
        console->log(LogLevel::Info, buf);
    });

    // ── Zoom ─────────────────────────────────────────────────────────────
    btnZoomIn->clicked.connect( [editor]() { editor->zoomIn();    });
    btnZoomOut->clicked.connect([editor]() { editor->zoomOut();   });
    btnZoomRst->clicked.connect([editor]() { editor->zoomReset(); });

    // ── Undo / Redo — disabled when no history ───────────────────────────
    btnUndo->setEnabled(false);
    btnRedo->setEnabled(false);

    editor->undoRedoChanged.connect([btnUndo, btnRedo](bool canUndo, bool canRedo) {
        btnUndo->setEnabled(canUndo);
        btnRedo->setEnabled(canRedo);
    });
    btnUndo->clicked.connect([editor, console]() {
        editor->undo();
        console->log(LogLevel::Info, "Undo");
    });
    btnRedo->clicked.connect([editor, console]() {
        editor->redo();
        console->log(LogLevel::Info, "Redo");
    });

    // ── Select All / Select All Occurrences ──────────────────────────────
    btnSelAll->clicked.connect([editor]() { editor->selectAll(); });
    btnSelOcc->clicked.connect([editor, console]() {
        editor->selectAllOccurrences();
        console->log(LogLevel::Info, "Selected all occurrences of word under cursor");
    });

    // ── Toggles ──────────────────────────────────────────────────────────
    chkReadOnly->toggled.connect([editor, lblMode](bool on) {
        editor->setReadOnly(on);
        lblMode->setText(on ? "[READ ONLY]" : "");
    });
    chkWordWrap->toggled.connect([editor](bool on) {
        editor->setWordWrap(on);
    });
    chkMinimap->toggled.connect([editor](bool on) {
        editor->setShowMinimap(on);
    });
    chkWhitespace->toggled.connect([editor](bool on) {
        editor->setShowWhitespace(on);
    });
}
