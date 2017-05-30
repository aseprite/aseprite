// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/color.h"
#include "ui/intern.h"
#include "ui/system.h"
#include "ui/theme.h"

#include "app/app.h"
#include "app/color_utils.h"
#include "app/console.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "doc/image.h"
#include "doc/palette.h"

namespace app {

using namespace app::skin;
using namespace gfx;

static void rectgrid(ui::Graphics* g, const gfx::Rect& rc, const gfx::Size& tile)
{
  if (tile.w < 1 || tile.h < 1)
    return;

  int x, y, u, v;

  // TODO these values are hard-coded in ColorSelector::onPaintAlphaBar() too, use pref.xml values
  gfx::Color c1 = gfx::rgba(128, 128, 128);
  gfx::Color c2 = gfx::rgba(192, 192, 192);

  u = 0;
  v = 0;
  for (y=rc.y; y<rc.y2()-tile.h; y+=tile.h) {
    for (x=rc.x; x<rc.x2()-tile.w; x+=tile.w)
      g->fillRect(((u++)&1)? c1: c2, gfx::Rect(x, y, tile.w, tile.h));

    if (x < rc.x2())
      g->fillRect(((u++)&1)? c1: c2, gfx::Rect(x, y, rc.x2()-x, tile.h));

    u = (++v);
  }

  if (y < rc.y2()) {
    for (x=rc.x; x<rc.x2()-tile.w; x+=tile.w)
      g->fillRect(((u++)&1)? c1: c2, gfx::Rect(x, y, tile.w, rc.y2()-y));

    if (x < rc.x2())
      g->fillRect(((u++)&1)? c1: c2, gfx::Rect(x, y, rc.x2()-x, rc.y2()-y));
  }
}

void draw_color(ui::Graphics* g,
                const Rect& rc,
                const app::Color& _color,
                const doc::ColorMode colorMode)
{
  if (rc.w < 1 || rc.h < 1)
    return;

  app::Color color = _color;

  int alpha = color.getAlpha();

  if (alpha < 255) {
    if (rc.w == rc.h)
      rectgrid(g, rc, gfx::Size(rc.w/2, rc.h/2));
    else
      rectgrid(g, rc, gfx::Size(rc.w/4, rc.h/2));
  }

  if (alpha > 0) {
    if (colorMode == doc::ColorMode::GRAYSCALE) {
      color = app::Color::fromGray(
        color.getGray(),
        color.getAlpha());
    }

    if (color.getType() == app::Color::IndexType) {
      int index = color.getIndex();

      if (index >= 0 && index < get_current_palette()->size()) {
        g->fillRect(color_utils::color_for_ui(color), rc);
      }
      else {
        g->fillRect(gfx::rgba(0, 0, 0), rc);
        g->drawLine(gfx::rgba(255, 255, 255),
                    gfx::Point(rc.x+rc.w-2, rc.y+1),
                    gfx::Point(rc.x+1, rc.y+rc.h-2));
      }
    }
    else {
      g->fillRect(color_utils::color_for_ui(color), rc);
    }
  }
}

void draw_color_button(ui::Graphics* g,
                       const Rect& rc,
                       const app::Color& color,
                       const doc::ColorMode colorMode,
                       const bool hot,
                       const bool drag)
{
  SkinTheme* theme = SkinTheme::instance();
  int scale = ui::guiscale();

  // Draw background (the color)
  draw_color(g,
             Rect(rc.x+1*scale,
                  rc.y+1*scale,
                  rc.w-2*scale,
                  rc.h-2*scale),
             color,
             colorMode);

  // Draw opaque border
  theme->drawRect(
    g, rc,
    theme->parts.colorbar0()->bitmapNW(),
    theme->parts.colorbar0()->bitmapN(),
    theme->parts.colorbar1()->bitmapNE(),
    theme->parts.colorbar1()->bitmapE(),
    theme->parts.colorbar3()->bitmapSE(),
    theme->parts.colorbar2()->bitmapS(),
    theme->parts.colorbar2()->bitmapSW(),
    theme->parts.colorbar0()->bitmapW());

  // Draw hot
  if (hot) {
    theme->drawRect(
      g, gfx::Rect(rc.x, rc.y, rc.w, rc.h-1 - 1*scale),
      theme->parts.colorbarSelection().get());
  }
}

} // namespace app
