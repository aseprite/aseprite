/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include <allegro/unicode.h>

#include "gui/gui.h"

#include "commands/command.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"
#include "undoable.h"
#include "widgets/color_bar.h"
#include "app/color_utils.h"

class CanvasSizeCommand : public Command
{
  int m_left, m_right, m_top, m_bottom;

public:
  CanvasSizeCommand();
  Command* clone() const { return new CanvasSizeCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CanvasSizeCommand::CanvasSizeCommand()
  : Command("CanvasSize",
	    "Canvas Size",
	    CmdRecordableFlag)
{
  m_left = m_right = m_top = m_bottom = 0;
}

bool CanvasSizeCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void CanvasSizeCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  if (context->isUiAvailable()) {
    JWidget left, top, right, bottom, ok;

    // load the window widget
    FramePtr window(load_widget("canvas_size.xml", "canvas_size"));
    get_widgets(window,
		"left", &left,
		"top", &top,
		"right", &right,
		"bottom", &bottom,
		"ok", &ok, NULL);

    window->remap_window();
    window->center_window();

    left->setTextf("%d", m_left);
    right->setTextf("%d", m_right);
    top->setTextf("%d", m_top);
    bottom->setTextf("%d", m_bottom);

    load_window_pos(window, "CanvasSize");
    window->setVisible(true);
    window->open_window_fg();
    save_window_pos(window, "CanvasSize");

    if (window->get_killer() != ok)
      return;

    m_left = left->getTextInt();
    m_right = right->getTextInt();
    m_top = top->getTextInt();
    m_bottom = bottom->getTextInt();
  }

  // resize canvas

  int x1 = -m_left;
  int y1 = -m_top;
  int x2 = sprite->getWidth() + m_right;
  int y2 = sprite->getHeight() + m_bottom;

  if (x2 <= x1) x2 = x1+1;
  if (y2 <= y1) y2 = y1+1;

  {
    Undoable undoable(sprite, "Canvas Size");
    int bgcolor = color_utils::color_for_image(context->getSettings()->getBgColor(), sprite->getImgType());
    bgcolor = color_utils::fixup_color_for_background(sprite->getImgType(), bgcolor);

    undoable.cropSprite(x1, y1, x2-x1, y2-y1, bgcolor);
    undoable.commit();
  }
  sprite->generateMaskBoundaries();
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createCanvasSizeCommand()
{
  return new CanvasSizeCommand;
}
