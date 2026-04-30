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

static const char* sampleCpp = R"(#include <iostream>
#include <vector>
#include <string>

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
        auto* titleLabel = header->createChild<Label>("CodeEditor — ");
        (void)titleLabel;
        auto* langLabel = header->createChild<Label>("C++");
        langLabel->setId("langLabel");
        header->createChild<Spacer>(0.f)->setStretch(1);
    }

    // ── Language buttons ─────────────────────────────────────────────────
    auto* langBar = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    langBar->setSize(0, 30);
    langBar->setPadding(Edges(2, 8, 2, 8));
    langBar->setSpacing(4);
    {
        langBar->createChild<Label>("Language:");

        auto* btnCpp = langBar->createChild<Button>("C++");
        btnCpp->setSize(60, 24);
        auto* btnPy = langBar->createChild<Button>("Python");
        btnPy->setSize(70, 24);
        auto* btnJs = langBar->createChild<Button>("JavaScript");
        btnJs->setSize(90, 24);

        langBar->createChild<Line>();

        auto* btnFold   = langBar->createChild<Button>("Fold All");
        btnFold->setSize(70, 24);
        auto* btnUnfold = langBar->createChild<Button>("Unfold All");
        btnUnfold->setSize(80, 24);

        langBar->createChild<Line>();

        auto* btnUndo = langBar->createChild<Button>("Undo");
        btnUndo->setSize(60, 24);
        auto* btnRedo = langBar->createChild<Button>("Redo");
        btnRedo->setSize(60, 24);

        langBar->createChild<Line>();

        auto* btnZoomIn  = langBar->createChild<Button>("+");
        btnZoomIn->setSize(28, 24);
        auto* btnZoomOut = langBar->createChild<Button>("-");
        btnZoomOut->setSize(28, 24);
        auto* btnZoomRst = langBar->createChild<Button>("100%");
        btnZoomRst->setSize(50, 24);

        langBar->createChild<Spacer>(0.f)->setStretch(1);

        // ── Editor ───────────────────────────────────────────────────────
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

        // ── Console ──────────────────────────────────────────────────────
        auto* console = outer->createChild<ConsoleWidget>();
        console->setSize(0, 100);
        console->log(LogLevel::Info, "CodeEditor demo loaded. Switch languages, fold code, search (Ctrl+F).");

        auto* langLabel = root->findById<Label>("langLabel");

        // Wire up language buttons
        btnCpp->clicked.connect([editor, console, langLabel]() {
            editor->setHighlighter<CppHighlighter>();
            editor->setText(sampleCpp);
            if (langLabel) langLabel->setText("C++");
            console->log(LogLevel::Info, "Switched to C++ highlighter");
        });
        btnPy->clicked.connect([editor, console, langLabel]() {
            editor->setHighlighter<PythonHighlighter>();
            editor->setText(samplePython);
            if (langLabel) langLabel->setText("Python");
            console->log(LogLevel::Info, "Switched to Python highlighter");
        });
        btnJs->clicked.connect([editor, console, langLabel]() {
            editor->setHighlighter<JsHighlighter>();
            editor->setText(sampleJs);
            if (langLabel) langLabel->setText("JavaScript");
            console->log(LogLevel::Info, "Switched to JavaScript highlighter");
        });

        // Wire up fold buttons
        btnFold->clicked.connect([editor, console]() {
            editor->foldAll();
            console->log(LogLevel::Info, "Folded all regions");
        });
        btnUnfold->clicked.connect([editor, console]() {
            editor->unfoldAll();
            console->log(LogLevel::Info, "Unfolded all regions");
        });

        // Wire up zoom
        btnZoomIn->clicked.connect([editor]() { editor->zoomIn(); });
        btnZoomOut->clicked.connect([editor]() { editor->zoomOut(); });
        btnZoomRst->clicked.connect([editor]() { editor->zoomReset(); });

        // Wire up undo/redo
        btnUndo->clicked.connect([editor, console]() {
            if (editor->canUndo()) {
                editor->undo();
                console->log(LogLevel::Info, "Undo");
            }
        });
        btnRedo->clicked.connect([editor, console]() {
            if (editor->canRedo()) {
                editor->redo();
                console->log(LogLevel::Info, "Redo");
            }
        });
    }
}
