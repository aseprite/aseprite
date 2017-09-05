// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app_render.h"

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
  DocumentPreferences& docPref = Preferences::instance().document(doc);
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
    case app::gen::BgType::CHECKED_1x1:
      bgType = render::BgType::CHECKED;
      tile = gfx::Size(1, 1);
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
