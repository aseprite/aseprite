// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
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
#include "app/site.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/util/conversion_to_surface.h"
#include "doc/blend_funcs.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "gfx/color.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/intern.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <algorithm>

namespace app {

using namespace app::skin;
using namespace gfx;

namespace {

void draw_checkered_grid(ui::Graphics* g,
                         const gfx::Rect& rc,
                         const gfx::Size& tile,
                         const gfx::Color c1,
                         const gfx::Color c2)
{
  if (tile.w < 1 || tile.h < 1)
    return;

  int x, y, u, v;

  u = 0;
  v = 0;
  for (y = rc.y; y < rc.y2() - tile.h; y += tile.h) {
    for (x = rc.x; x < rc.x2() - tile.w; x += tile.w)
      g->fillRect(((u++) & 1) ? c1 : c2, gfx::Rect(x, y, tile.w, tile.h));

    if (x < rc.x2())
      g->fillRect(((u++) & 1) ? c1 : c2, gfx::Rect(x, y, rc.x2() - x, tile.h));

    u = (++v);
  }

  if (y < rc.y2()) {
    for (x = rc.x; x < rc.x2() - tile.w; x += tile.w)
      g->fillRect(((u++) & 1) ? c1 : c2, gfx::Rect(x, y, tile.w, rc.y2() - y));

    if (x < rc.x2())
      g->fillRect(((u++) & 1) ? c1 : c2, gfx::Rect(x, y, rc.x2() - x, rc.y2() - y));
  }
}

} // anonymous namespace

gfx::Color grid_color1()
{
  auto editor = Editor::activeEditor();
  if (ui::is_ui_thread() && editor)
    return color_utils::color_for_ui(editor->docPref().bg.color1());
  else
    return gfx::rgba(128, 128, 128);
}

gfx::Color grid_color2()
{
  auto editor = Editor::activeEditor();
  if (ui::is_ui_thread() && editor)
    return color_utils::color_for_ui(editor->docPref().bg.color2());
  else
    return gfx::rgba(192, 192, 192);
}

void draw_checkered_grid(ui::Graphics* g, const gfx::Rect& rc, const gfx::Size& tile)
{
  draw_checkered_grid(g, rc, tile, grid_color1(), grid_color2());
}

void draw_checkered_grid(ui::Graphics* g,
                         const gfx::Rect& rc,
                         const gfx::Size& tile,
                         DocumentPreferences& docPref)
{
  draw_checkered_grid(g, rc, tile, grid_color1(), grid_color2());
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
      draw_checkered_grid(g, rc, gfx::Size(rc.w / 2, rc.h / 2));
    else
      draw_checkered_grid(g, rc, gfx::Size(rc.w / 4, rc.h / 2));
  }

  if (alpha > 0) {
    if (colorMode == doc::ColorMode::GRAYSCALE) {
      color = app::Color::fromGray(color.getGray(), color.getAlpha());
    }

    if (color.getType() == app::Color::IndexType) {
      int index = color.getIndex();

      if (index >= 0 && index < get_current_palette()->size()) {
        g->fillRect(convertColor(color_utils::color_for_ui(color)), rc);
      }
      else {
        g->fillRect(gfx::rgba(0, 0, 0), rc);
        g->drawLine(gfx::rgba(255, 255, 255),
                    gfx::Point(rc.x + rc.w - 2, rc.y + 1),
                    gfx::Point(rc.x + 1, rc.y + rc.h - 2));
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
  auto theme = SkinTheme::instance();
  ASSERT(theme);
  if (!theme)
    return;

  int scale = ui::guiscale();

  // Draw background (the color)
  draw_color(g,
             Rect(rc.x + 1 * scale, rc.y + 1 * scale, rc.w - 2 * scale, rc.h - 2 * scale),
             color,
             colorMode);

  // Draw opaque border
  theme->drawRect(g,
                  rc,
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
    theme->drawRect(g,
                    gfx::Rect(rc.x, rc.y, rc.w, rc.h - 1 - 1 * scale),
                    theme->parts.colorbarSelection().get());
  }
}

void draw_tile(ui::Graphics* g, const Rect& rc, const Site& site, doc::tile_t tile)
{
  if (rc.w < 1 || rc.h < 1)
    return;

  draw_checkered_grid(g, rc, gfx::Size(rc.w / 2, rc.h / 2));

  if (tile == doc::notile)
    return;

  const doc::Tileset* ts = site.tileset();
  if (!ts)
    return;

  const doc::tile_index ti = doc::tile_geti(tile);
  if (ti < 0 || ti >= ts->size())
    return;

  const doc::ImageRef tileImage = ts->get(ti);
  if (!tileImage)
    return;

  const doc::tile_index tf = doc::tile_getf(tile);

  int w = tileImage->width();
  int h = tileImage->height();
  os::SurfaceRef surface = os::instance()->makeRgbaSurface(w, h);
  convert_image_to_surface(tileImage.get(), get_current_palette(), surface.get(), 0, 0, 0, 0, w, h);

  ui::Paint paint;
  paint.blendMode(os::BlendMode::SrcOver);

  os::Sampling sampling;
  if (w > rc.w && h > rc.h) {
    sampling = os::Sampling(os::Sampling::Filter::Linear, os::Sampling::Mipmap::Nearest);
  }

  gfx::Matrix m;

  if (tf & doc::tile_f_dflip) {
    gfx::Matrix rot;
    rot.setRotate(90.0f);
    rot.postConcat(gfx::Matrix::MakeScale(-1.0f, 1.0f));
    m.preConcat(rot);
    std::swap(w, h);
  }

  if (tf & doc::tile_f_xflip) {
    gfx::Matrix flip;
    flip.setScale(-1.0f, 1.0f, w / float(2.0f), 0.0f);
    m.postConcat(flip);
  }

  if (tf & doc::tile_f_yflip) {
    gfx::Matrix flip;
    flip.setScale(1.0f, -1.0f, 0.0f, h / float(2.0f));
    m.postConcat(flip);
  }

  m.postConcat(gfx::Matrix::MakeScale(float(rc.w) / w, float(rc.h) / h));

  // TODO integrate getInternalDeltaX/Y translation in ui::Graphics
  m.postConcat(
    gfx::Matrix::MakeTrans(rc.x + g->getInternalDeltaX(), rc.y + g->getInternalDeltaY()));

  g->save();
  g->setMatrix(m);
  g->drawRgbaSurface(surface.get(), -g->getInternalDeltaX(), -g->getInternalDeltaY());
  g->restore();
}

void draw_tile_button(ui::Graphics* g,
                      const gfx::Rect& rc,
                      const Site& site,
                      doc::tile_t tile,
                      const bool hot,
                      const bool drag)
{
  auto theme = SkinTheme::instance();
  ASSERT(theme);
  if (!theme)
    return;

  int scale = ui::guiscale();

  // Draw background (the tile)
  draw_tile(g,
            Rect(rc.x + 1 * scale, rc.y + 1 * scale, rc.w - 2 * scale, rc.h - 2 * scale),
            site,
            tile);

  // Draw opaque border
  theme->drawRect(g,
                  rc,
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
    theme->drawRect(g,
                    gfx::Rect(rc.x, rc.y, rc.w, rc.h - 1 - 1 * scale),
                    theme->parts.colorbarSelection().get());
  }
}

void draw_alpha_slider(ui::Graphics* g, const gfx::Rect& rc, const app::Color& color)
{
  const int xmax = std::max(1, rc.w - 1);
  const doc::color_t c = (color.getType() != app::Color::MaskType ?
                            doc::rgba(color.getRed(), color.getGreen(), color.getBlue(), 255) :
                            0);

  for (int x = 0; x < rc.w; ++x) {
    const int a = (255 * x / xmax);
    const doc::color_t c1 = doc::rgba_blender_normal(grid_color1(), c, a);
    const doc::color_t c2 = doc::rgba_blender_normal(grid_color2(), c, a);
    const int mid = rc.h / 2;
    const int odd = (x / rc.h) & 1;
    g->drawVLine(app::color_utils::color_for_ui(app::Color::fromImage(IMAGE_RGB, odd ? c2 : c1)),
                 rc.x + x,
                 rc.y,
                 mid);
    g->drawVLine(app::color_utils::color_for_ui(app::Color::fromImage(IMAGE_RGB, odd ? c1 : c2)),
                 rc.x + x,
                 rc.y + mid,
                 rc.h - mid);
  }
}

// TODO this code is exactly the same as draw_alpha_slider() with a ui::Graphics
void draw_alpha_slider(os::Surface* s, const gfx::Rect& rc, const app::Color& color)
{
  const int xmax = std::max(1, rc.w - 1);
  const doc::color_t c = (color.getType() != app::Color::MaskType ?
                            doc::rgba(color.getRed(), color.getGreen(), color.getBlue(), 255) :
                            0);

  os::Paint paint;
  for (int x = 0; x < rc.w; ++x) {
    const int a = (255 * x / xmax);
    const doc::color_t c1 = doc::rgba_blender_normal(grid_color1(), c, a);
    const doc::color_t c2 = doc::rgba_blender_normal(grid_color2(), c, a);
    const int mid = rc.h / 2;
    const int odd = (x / rc.h) & 1;

    paint.color(app::color_utils::color_for_ui(app::Color::fromImage(IMAGE_RGB, odd ? c2 : c1)));
    s->drawRect(gfx::Rect(rc.x + x, rc.y, 1, mid), paint);

    paint.color(app::color_utils::color_for_ui(app::Color::fromImage(IMAGE_RGB, odd ? c1 : c2)));
    s->drawRect(gfx::Rect(rc.x + x, rc.y + mid, 1, rc.h - mid), paint);
  }
}

} // namespace app
