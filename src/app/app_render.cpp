/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app_render.h"

#include "app/app.h"
#include "app/color_utils.h"
#include "app/pref/preferences.h"
#include "render/render.h"

namespace app {

AppRender::AppRender()
{
}

AppRender::AppRender(app::Document* doc, doc::PixelFormat pixelFormat)
{
  setupBackground(doc, pixelFormat);
}

void AppRender::setupBackground(app::Document* doc, doc::PixelFormat pixelFormat)
{
  DocumentPreferences& docPref = App::instance()->preferences().document(doc);
  render::BgType bgType;

  gfx::Size tile;
  switch (docPref.bg.type()) {
    case app::gen::BgType::CHECKED_16x16:
      bgType = render::BgType::CHECKED;
      tile = gfx::Size(16, 16);
      break;
    case app::gen::BgType::CHECKED_8x8:
      bgType = render::BgType::CHECKED;
      tile = gfx::Size(8, 8);
      break;
    case app::gen::BgType::CHECKED_4x4:
      bgType = render::BgType::CHECKED;
      tile = gfx::Size(4, 4);
      break;
    case app::gen::BgType::CHECKED_2x2:
      bgType = render::BgType::CHECKED;
      tile = gfx::Size(2, 2);
      break;
    default:
      bgType = render::BgType::TRANSPARENT;
      break;
  }

  setBgType(bgType);
  setBgZoom(docPref.bg.zoom());
  setBgColor1(color_utils::color_for_image(docPref.bg.color1(), pixelFormat));
  setBgColor2(color_utils::color_for_image(docPref.bg.color2(), pixelFormat));
  setBgCheckedSize(tile);
}

}
