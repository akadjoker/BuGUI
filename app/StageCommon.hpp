#pragma once

#include "WidgetApp.hpp"
#include "Widgets.hpp"
#include "WidgetSerializer.hpp"
#include "Color.hpp"
#include <cstdio>

// ═════════════════════════════════════════════════════════════════════════════
//  Shared helpers & palette (used by all stages)
// ═════════════════════════════════════════════════════════════════════════════

namespace Clr {
    inline const Color dim       {120, 120, 125, 255};
    inline const Color section   {170, 170, 175, 255};
    inline const Color panelDark { 38,  40,  44, 255};
    inline const Color canvasBg  { 22,  22,  26, 255};
    inline const Color accent    {100, 160, 220, 255};
}

inline Label* sectionLabel(BoxLayout* parent, const char* text)
{
    auto* lbl = parent->createChild<Label>(text);
    lbl->setColor(Clr::section);
    lbl->setMargins(10, 0, 2, 0);
    return lbl;
}

inline Label* dimLabel(BoxLayout* parent, const char* text)
{
    auto* lbl = parent->createChild<Label>(text);
    lbl->setColor(Clr::dim);
    return lbl;
}
