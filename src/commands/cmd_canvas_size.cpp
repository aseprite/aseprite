/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"
#include "undoable.h"
#include "widgets/colbar.h"

class CanvasSizeCommand : public Command
{
  int m_left, m_right, m_top, m_bottom;

public:
  CanvasSizeCommand();
  Command* clone() const { return new CanvasSizeCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

CanvasSizeCommand::CanvasSizeCommand()
  : Command("canvas_size",
	    "Canvas Size",
	    CmdRecordableFlag)
{
  m_left = m_right = m_top = m_bottom = 0;
}

bool CanvasSizeCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void CanvasSizeCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  if (context->is_ui_available()) {
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
    jwidget_show(window);
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
    int bgcolor = get_color_for_image(sprite->getImgType(),
				      context->getSettings()->getBgColor());
    bgcolor = fixup_color_for_background(sprite->getImgType(), bgcolor);

    undoable.crop_sprite(x1, y1, x2-x1, y2-y1, bgcolor);
    undoable.commit();
  }
  sprite->generateMaskBoundaries();
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_canvas_size_command()
{
  return new CanvasSizeCommand;
}
