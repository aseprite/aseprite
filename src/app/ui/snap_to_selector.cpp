// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/snap_to_grid.h"
#include "app/ui/snap_to_selector.h"

#include "app/i18n/strings.h"

namespace app {

SnapToSelector::SnapToSelector()
{
  // addItem() must match the PreferSnapTo enum
  static_assert(int(PreferSnapTo::ClosestGridVertex) == 0 &&
                int(PreferSnapTo::BoxCenter) == 1,
                "Unexpected app::PreferSnapTo values");

  addItem(Strings::snap_to_selector_default());
  addItem(Strings::snap_to_selector_box_center());
  snapTo(app::PreferSnapTo::ClosestGridVertex);
}

PreferSnapTo SnapToSelector::snapTo()
{
  return (PreferSnapTo)getSelectedItemIndex();
}

void SnapToSelector::snapTo(const PreferSnapTo snapTo)
{
  setSelectedItemIndex((int)snapTo);
}

} // namespace app
