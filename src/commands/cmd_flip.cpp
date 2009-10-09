/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include "jinete/jlist.h"

#include "commands/command.h"
#include "commands/params.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "util/misc.h"
#include "undoable.h"
#include "sprite_wrappers.h"

class FlipCommand : public Command
{
  bool m_flip_mask;
  bool m_flip_horizontal;
  bool m_flip_vertical;

public:
  FlipCommand();
  Command* clone() const { return new FlipCommand(*this); }

protected:
  void load_params(Params* params);
  bool enabled(Context* context);
  void execute(Context* context);

private:
  static char* read_authors_txt(const char *filename);
};

FlipCommand::FlipCommand()
  : Command("flip",
	    "Flip",
	    CmdRecordableFlag)
{
  m_flip_mask = false;
  m_flip_horizontal = false;
  m_flip_vertical = false;
}

void FlipCommand::load_params(Params* params)
{
  std::string target = params->get("target");
  m_flip_mask = (target == "mask");

  std::string orientation = params->get("orientation");
  m_flip_horizontal = (orientation == "horizontal");
  m_flip_vertical = (orientation == "vertical");
}

bool FlipCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void FlipCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  {
    Undoable undoable(sprite, m_flip_mask ?
			      (m_flip_horizontal ? "Flip Horizontal":
						   "Flip Vertical"):
			      (m_flip_horizontal ? "Flip Canvas Horizontal":
						   "Flip Canvas Vertical"));

    if (m_flip_mask) {
      Image* image;
      int x1, y1, x2, y2;
      int x, y;

      image = GetImage2(sprite, &x, &y, NULL);
      if (!image)
	return;

      // mask is empty?
      if (mask_is_empty(sprite->mask)) {
	// so we flip the entire image
	x1 = 0;
	y1 = 0;
	x2 = image->w-1;
	y2 = image->h-1;
      }
      else {
	// apply the cel offset
	x1 = sprite->mask->x - x;
	y1 = sprite->mask->y - y;
	x2 = sprite->mask->x + sprite->mask->w - 1 - x;
	y2 = sprite->mask->y + sprite->mask->h - 1 - y;

	// clip
	x1 = MID(0, x1, image->w-1);
	y1 = MID(0, y1, image->h-1);
	x2 = MID(0, x2, image->w-1);
	y2 = MID(0, y2, image->h-1);
      }

      undoable.flip_image(image, x1, y1, x2, y2, 
			  m_flip_horizontal, m_flip_vertical);
    }
    else {
      // get all sprite cels
      JList cels = jlist_new();
      sprite_get_cels(sprite, cels);

      // for each cel...
      JLink link;
      JI_LIST_FOR_EACH(cels, link) {
	Cel* cel = (Cel*)link->data;
	Image* image = stock_get_image(sprite->stock, cel->image);

	undoable.set_cel_position(cel,
				  m_flip_horizontal ? sprite->w - image->w - cel->x: cel->x,
				  m_flip_vertical ? sprite->h - image->h - cel->y: cel->y);

	undoable.flip_image(image, 0, 0, image->w-1, image->h-1,
			    m_flip_horizontal, m_flip_vertical);
      }
      jlist_free(cels);
    }

    undoable.commit();
  }

  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_flip_command()
{
  return new FlipCommand;
}
