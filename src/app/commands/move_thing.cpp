// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/move_thing.h"

#include "app/commands/params.h"
#include "app/i18n/strings.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "fmt/format.h"

#include <algorithm>

namespace app {

void MoveThing::onLoadParams(const Params& params)
{
  std::string v = params.get("direction");
  if (v == "left") direction = Left;
  else if (v == "right") direction = Right;
  else if (v == "up") direction = Up;
  else if (v == "down") direction = Down;

  v = params.get("units");
  if (v == "pixel") units = Pixel;
  else if (v == "tile-width") units = TileWidth;
  else if (v == "tile-height") units = TileHeight;
  else if (v == "zoomed-pixel") units = ZoomedPixel;
  else if (v == "zoomed-tile-width") units = ZoomedTileWidth;
  else if (v == "zoomed-tile-height") units = ZoomedTileHeight;
  else if (v == "viewport-width") units = ViewportWidth;
  else if (v == "viewport-height") units = ViewportHeight;

  int q = params.get_as<int>("quantity");
  quantity = std::max<int>(1, q);
}

std::string MoveThing::getFriendlyString() const
{
  std::string dim, dir;

  switch (units) {
    case Pixel: dim = Strings::commands_Move_Pixel(); break;
    case TileWidth: dim = Strings::commands_Move_TileWidth(); break;
    case TileHeight: dim = Strings::commands_Move_TileHeight(); break;
    case ZoomedPixel: dim = Strings::commands_Move_ZoomedPixel(); break;
    case ZoomedTileWidth: dim = Strings::commands_Move_ZoomedTileWidth(); break;
    case ZoomedTileHeight: dim = Strings::commands_Move_ZoomedTileHeight(); break;
    case ViewportWidth: dim = Strings::commands_Move_ViewportWidth(); break;
    case ViewportHeight: dim = Strings::commands_Move_ViewportHeight(); break;
  }

  switch (direction) {
    case Left:  dir = Strings::commands_Move_Left(); break;
    case Right: dir = Strings::commands_Move_Right(); break;
    case Up:    dir = Strings::commands_Move_Up(); break;
    case Down:  dir = Strings::commands_Move_Down(); break;
  }

  return fmt::format(Strings::commands_Move_Thing(),
                     quantity, dim, dir);
}

gfx::Point MoveThing::getDelta(Context* context) const
{
  gfx::Point delta(0, 0);

  DocView* view = static_cast<UIContext*>(context)->activeView();
  if (!view)
    return delta;

  Editor* editor = view->editor();
  gfx::Rect vp = view->viewWidget()->viewportBounds();
  gfx::Rect gridBounds = view->document()->sprite()->gridBounds();
  int pixels = 0;

  switch (units) {
    case Pixel:
      pixels = 1;
      break;
    case TileWidth:
      pixels = gridBounds.w;
      break;
    case TileHeight:
      pixels = gridBounds.h;
      break;
    case ZoomedPixel:
      pixels = editor->zoom().apply(1);
      break;
    case ZoomedTileWidth:
      pixels = editor->zoom().apply(gridBounds.w);
      break;
    case ZoomedTileHeight:
      pixels = editor->zoom().apply(gridBounds.h);
      break;
    case ViewportWidth:
      pixels = vp.h;
      break;
    case ViewportHeight:
      pixels = vp.w;
      break;
  }

  switch (direction) {
    case Left:  delta.x = -quantity * pixels; break;
    case Right: delta.x = +quantity * pixels; break;
    case Up:    delta.y = -quantity * pixels; break;
    case Down:  delta.y = +quantity * pixels; break;
  }

  return delta;
}

} // namespace app
