// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/modules/gfx.h"

#include "app/app.h"
#include "app/color_spaces.h"
#include "app/color_utils.h"
#include "app/console.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "doc/blend_funcs.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "gfx/color.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "os/surface.h"
#include "ui/intern.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace app {

using namespace app::skin;
using namespace gfx;

namespace {

// TODO hard-coded values, use pref.xml values
gfx::Color gridColor1 = gfx::rgba(128, 128, 128);
gfx::Color gridColor2 = gfx::rgba(192, 192, 192);

} // anonymous namespace

static void rectgrid(ui::Graphics* g, const gfx::Rect& rc, const gfx::Size& tile)
{
  if (tile.w < 1 || tile.h < 1)
    return;

  int x, y, u, v;

  const gfx::Color c1 = gridColor1;
  const gfx::Color c2 = gridColor2;

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
  const int alpha = color.getAlpha();

  // Color space conversion
  auto convertColor = convert_from_current_to_screen_color_space();

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
        g->fillRect(convertColor(color_utils::color_for_ui(color)), rc);
      }
      else {
        g->fillRect(gfx::rgba(0, 0, 0), rc);
        g->drawLine(gfx::rgba(255, 255, 255),
                    gfx::Point(rc.x+rc.w-2, rc.y+1),
                    gfx::Point(rc.x+1, rc.y+rc.h-2));
      }
    }
    else {
      g->fillRect(convertColor(color_utils::color_for_ui(color)), rc);
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

void draw_alpha_slider(ui::Graphics* g,
                       const gfx::Rect& rc,
                       const app::Color& color)
{
  const int xmax = MAX(1, rc.w-1);
  const doc::color_t c =
    (color.getType() != app::Color::MaskType ?
     doc::rgba(color.getRed(),
               color.getGreen(),
               color.getBlue(), 255): 0);

  for (int x=0; x<rc.w; ++x) {
    const int a = (255 * x / xmax);
    const doc::color_t c1 = doc::rgba_blender_normal(gridColor1, c, a);
    const doc::color_t c2 = doc::rgba_blender_normal(gridColor2, c, a);
    const int mid = rc.h/2;
    const int odd = (x / rc.h) & 1;
    g->drawVLine(
      app::color_utils::color_for_ui(app::Color::fromImage(IMAGE_RGB, odd ? c2: c1)),
      rc.x+x, rc.y, mid);
    g->drawVLine(
      app::color_utils::color_for_ui(app::Color::fromImage(IMAGE_RGB, odd ? c1: c2)),
      rc.x+x, rc.y+mid, rc.h-mid);
  }
}

// TODO this code is exactly the same as draw_alpha_slider() with a ui::Graphics
void draw_alpha_slider(os::Surface* s,
                       const gfx::Rect& rc,
                       const app::Color& color)
{
  const int xmax = MAX(1, rc.w-1);
  const doc::color_t c =
    (color.getType() != app::Color::MaskType ?
     doc::rgba(color.getRed(),
               color.getGreen(),
               color.getBlue(), 255): 0);

  for (int x=0; x<rc.w; ++x) {
    const int a = (255 * x / xmax);
    const doc::color_t c1 = doc::rgba_blender_normal(gridColor1, c, a);
    const doc::color_t c2 = doc::rgba_blender_normal(gridColor2, c, a);
    const int mid = rc.h/2;
    const int odd = (x / rc.h) & 1;
    s->drawVLine(
      app::color_utils::color_for_ui(app::Color::fromImage(IMAGE_RGB, odd ? c2: c1)),
      rc.x+x, rc.y, mid);
    s->drawVLine(
      app::color_utils::color_for_ui(app::Color::fromImage(IMAGE_RGB, odd ? c1: c2)),
      rc.x+x, rc.y+mid, rc.h-mid);
  }
}

} // namespace app
