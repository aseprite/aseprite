// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "base/convert_to.h"
#include "ui/view.h"

namespace app {

class ScrollCommand : public Command {
public:
  enum Direction { Left, Up, Right, Down, };
  enum Units {
    Pixel,
    TileWidth,
    TileHeight,
    ZoomedPixel,
    ZoomedTileWidth,
    ZoomedTileHeight,
    ViewportWidth,
    ViewportHeight
  };

  ScrollCommand();
  Command* clone() const override { return new ScrollCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  Direction m_direction;
  Units m_units;
  int m_quantity;
};

ScrollCommand::ScrollCommand()
  : Command("Scroll",
            "Scroll",
            CmdUIOnlyFlag)
{
}

void ScrollCommand::onLoadParams(const Params& params)
{
  std::string direction = params.get("direction");
  if (direction == "left") m_direction = Left;
  else if (direction == "right") m_direction = Right;
  else if (direction == "up") m_direction = Up;
  else if (direction == "down") m_direction = Down;

  std::string units = params.get("units");
  if (units == "pixel") m_units = Pixel;
  else if (units == "tile-width") m_units = TileWidth;
  else if (units == "tile-height") m_units = TileHeight;
  else if (units == "zoomed-pixel") m_units = ZoomedPixel;
  else if (units == "zoomed-tile-width") m_units = ZoomedTileWidth;
  else if (units == "zoomed-tile-height") m_units = ZoomedTileHeight;
  else if (units == "viewport-width") m_units = ViewportWidth;
  else if (units == "viewport-height") m_units = ViewportHeight;

  int quantity = params.get_as<int>("quantity");
  m_quantity = std::max<int>(1, quantity);
}

bool ScrollCommand::onEnabled(Context* context)
{
  ContextWriter writer(context);
  return (writer.document() != NULL);
}

void ScrollCommand::onExecute(Context* context)
{
  DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
  ui::View* view = ui::View::getView(current_editor);
  gfx::Rect vp = view->viewportBounds();
  gfx::Point scroll = view->viewScroll();
  gfx::Rect gridBounds = docPref.grid.bounds();
  gfx::Point delta(0, 0);
  int pixels = 0;

  switch (m_units) {
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
      pixels = current_editor->zoom().apply(1);
      break;
    case ZoomedTileWidth:
      pixels = current_editor->zoom().apply(gridBounds.w);
      break;
    case ZoomedTileHeight:
      pixels = current_editor->zoom().apply(gridBounds.h);
      break;
    case ViewportWidth:
      pixels = vp.h;
      break;
    case ViewportHeight:
      pixels = vp.w;
      break;
  }

  switch (m_direction) {
    case Left:  delta.x = -m_quantity * pixels; break;
    case Right: delta.x = +m_quantity * pixels; break;
    case Up:    delta.y = -m_quantity * pixels; break;
    case Down:  delta.y = +m_quantity * pixels; break;
  }

  current_editor->setEditorScroll(scroll+delta);
}

std::string ScrollCommand::onGetFriendlyName() const
{
  std::string text = "Scroll " + base::convert_to<std::string>(m_quantity);

  switch (m_units) {
    case Pixel:
      text += " pixel";
      break;
    case TileWidth:
      text += " horizontal tile";
      break;
    case TileHeight:
      text += " vertical tile";
      break;
    case ZoomedPixel:
      text += " zoomed pixel";
      break;
    case ZoomedTileWidth:
      text += " zoomed horizontal tile";
      break;
    case ZoomedTileHeight:
      text += " zoomed vertical tile";
      break;
    case ViewportWidth:
      text += " viewport width";
      break;
    case ViewportHeight:
      text += " viewport height";
      break;
  }
  if (m_quantity != 1)
    text += "s";

  switch (m_direction) {
    case Left:  text += " left"; break;
    case Right: text += " right"; break;
    case Up:    text += " up"; break;
    case Down:  text += " down"; break;
  }

  return text;
}

Command* CommandFactory::createScrollCommand()
{
  return new ScrollCommand;
}

} // namespace app
