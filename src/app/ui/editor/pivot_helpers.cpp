// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/editor/pivot_helpers.h"

#include "app/pref/preferences.h"
#include "app/transformation.h"

namespace app {

void set_pivot_from_preferences(Transformation& t)
{
  auto corners = t.transformedCorners();
  gfx::PointT<double> nw(corners[Transformation::Corners::LEFT_TOP]);
  gfx::PointT<double> ne(corners[Transformation::Corners::RIGHT_TOP]);
  gfx::PointT<double> sw(corners[Transformation::Corners::LEFT_BOTTOM]);
  gfx::PointT<double> se(corners[Transformation::Corners::RIGHT_BOTTOM]);
  gfx::PointT<double> pivotPos((nw + se) / 2);

  app::gen::PivotPosition pivot = Preferences::instance().selection.pivotPosition();
  switch (pivot) {
    case app::gen::PivotPosition::NORTHWEST: pivotPos = nw; break;
    case app::gen::PivotPosition::NORTH:     pivotPos = (nw + ne) / 2.0; break;
    case app::gen::PivotPosition::NORTHEAST: pivotPos = ne; break;
    case app::gen::PivotPosition::WEST:      pivotPos = (nw + sw) / 2.0; break;
    case app::gen::PivotPosition::EAST:      pivotPos = (ne + se) / 2.0; break;
    case app::gen::PivotPosition::SOUTHWEST: pivotPos = sw; break;
    case app::gen::PivotPosition::SOUTH:     pivotPos = (sw + se) / 2.0; break;
    case app::gen::PivotPosition::SOUTHEAST: pivotPos = se; break;
  }

  t.displacePivotTo(gfx::PointF(pivotPos));
}

} // namespace app
