#pragma once

#include "Widget.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <unordered_map>

using json = nlohmann::json;

// ═════════════════════════════════════════════════════════════════════════════
//  WidgetSerializer — save / load widget trees to/from JSON
// ═════════════════════════════════════════════════════════════════════════════

class WidgetSerializer
{
public:
    // Factory: creates a widget from JSON and adds it as child of parent.
    // The factory should set type-specific properties from the JSON object.
    using WidgetFactory = std::function<Widget*(const json& j, Widget* parent)>;

    // Register a widget type factory
    static void registerType(const std::string& typeName, WidgetFactory factory);

    // Get the type name for a widget (uses typeid)
    static std::string typeName(const Widget* w);

    // ── Serialize ─────────────────────────────────────────────────────────
    // Serialize a widget tree to JSON
    static json save(const Widget* root);

    // Save to file (returns true on success)
    static bool saveToFile(const Widget* root, const std::string& path);

    // ── Deserialize ───────────────────────────────────────────────────────
    // Build widget tree from JSON, adding children to parent.
    // Returns the created widget (or nullptr on error).
    static Widget* load(const json& j, Widget* parent);

    // Load from file. Returns the top-level widget (added as child of parent).
    static Widget* loadFromFile(const std::string& path, Widget* parent);

    // ── Built-in type registration ────────────────────────────────────────
    // Call once to register all standard BuGUI widget types.
    static void registerBuiltinTypes();

private:
    static std::unordered_map<std::string, WidgetFactory>& factories();

    // Helpers
    static json serializeBase(const Widget* w);
    static void deserializeBase(const json& j, Widget* w);

    static json serializeChildren(const Widget* w);
    static void deserializeChildren(const json& j, Widget* w);
};
