// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/selection_mode_field.h"

#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/skin/skin_theme.h"
#include "ui/tooltips.h"

namespace app {

using namespace app::skin;
using namespace ui;

SelectionModeField::SelectionModeField()
  : ButtonSet(4)
{
  auto theme = static_cast<SkinTheme*>(this->theme());

  addItem(theme->parts.selectionReplace());
  addItem(theme->parts.selectionAdd());
  addItem(theme->parts.selectionSubtract());
  addItem(theme->parts.selectionIntersect());

  setSelectedItem((int)Preferences::instance().selection.mode());
}

void SelectionModeField::setupTooltips(TooltipManager* tooltipManager)
{
  tooltipManager->addTooltipFor(
    at(0), "Replace selection", BOTTOM);

  tooltipManager->addTooltipFor(
    at(1), key_tooltip("Add to selection", KeyAction::AddSelection), BOTTOM);

  tooltipManager->addTooltipFor(
    at(2), key_tooltip("Subtract from selection", KeyAction::SubtractSelection), BOTTOM);

  tooltipManager->addTooltipFor(
    at(3), key_tooltip("Intersect selection", KeyAction::IntersectSelection), BOTTOM);
}

gen::SelectionMode SelectionModeField::selectionMode()
{
  return (gen::SelectionMode)selectedItem();
}

void SelectionModeField::setSelectionMode(gen::SelectionMode mode)
{
  setSelectedItem((int)mode, false);
  invalidate();
}

void SelectionModeField::onItemChange(Item* item)
{
  ButtonSet::onItemChange(item);
  onSelectionModeChange(selectionMode());
}

} // namespace app
