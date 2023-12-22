// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/selection_mode_field.h"

#include "app/i18n/strings.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/skin/skin_theme.h"
#include "ui/tooltips.h"

namespace app {

using namespace app::skin;
using namespace ui;

SelectionModeField::SelectionModeField()
  : ButtonSet(4)
{
  auto theme = SkinTheme::get(this);

  addItem(theme->parts.selectionReplace(), "selection_mode");
  addItem(theme->parts.selectionAdd(), "selection_mode");
  addItem(theme->parts.selectionSubtract(), "selection_mode");
  addItem(theme->parts.selectionIntersect(), "selection_mode");

  setSelectedItem((int)Preferences::instance().selection.mode());
  initTheme();
}

void SelectionModeField::setupTooltips(TooltipManager* tooltipManager)
{
  tooltipManager->addTooltipFor(
    at(0), Strings::selection_mode_replace(), BOTTOM);

  tooltipManager->addTooltipFor(
    at(1), key_tooltip(Strings::selection_mode_add().c_str(),
                       KeyAction::AddSelection), BOTTOM);

  tooltipManager->addTooltipFor(
    at(2), key_tooltip(Strings::selection_mode_subtract().c_str(),
                       KeyAction::SubtractSelection), BOTTOM);

  tooltipManager->addTooltipFor(
    at(3), key_tooltip(Strings::selection_mode_intersect().c_str(),
                       KeyAction::IntersectSelection), BOTTOM);
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
