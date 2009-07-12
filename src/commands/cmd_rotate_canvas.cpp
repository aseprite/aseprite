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

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "core/job.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undoable.h"
#include "widgets/colbar.h"

class RotateCanvasJob : public Job
{
  SpriteWriter m_sprite;
  int m_angle;

public:

  RotateCanvasJob(const SpriteReader& sprite, int angle)
    : Job("Rotate Canvas")
    , m_sprite(sprite)
  {
    m_angle = angle;
  }

protected:

  /**
   * [working thread]
   */
  virtual void on_job()
  {
    Undoable undoable(m_sprite, "Rotate Canvas");

    // get all sprite cels
    JList cels = jlist_new();
    sprite_get_cels(m_sprite, cels);

    // for each cel...
    JLink link;
    JI_LIST_FOR_EACH(cels, link) {
      Cel* cel = (Cel*)link->data;
      Image* image = stock_get_image(m_sprite->stock, cel->image);

      // change it location
      switch (m_angle) {
	case 180:
	  undoable.set_cel_position(cel,
				    m_sprite->w - cel->x - image->w,
				    m_sprite->h - cel->y - image->h);
	  break;
	case 90:
	  undoable.set_cel_position(cel, m_sprite->h - cel->y - image->h, cel->x);
	  break;
	case -90:
	  undoable.set_cel_position(cel, cel->y, m_sprite->w - cel->x - image->w);
	  break;
      }
    }
    jlist_free(cels);

    // for each stock's image
    for (int i=0; i<m_sprite->stock->nimage; ++i) {
      Image* image = stock_get_image(m_sprite->stock, i);
      if (!image)
	continue;

      // rotate the image
      Image* new_image = image_new(image->imgtype,
				   m_angle == 180 ? image->w: image->h,
				   m_angle == 180 ? image->h: image->w);
      image_rotate(image, new_image, m_angle);

      undoable.replace_stock_image(i, new_image);

      job_progress((float)i / m_sprite->stock->nimage);

      // cancel all the operation?
      if (is_canceled())
	return;	       // Undoable destructor will undo all operations
    }

    // rotate mask
    if (m_sprite->mask->bitmap) {
      Mask* new_mask = mask_new();
      int x, y;

      switch (m_angle) {
	case 180:
	  x = m_sprite->w - m_sprite->mask->x - m_sprite->mask->w;
	  y = m_sprite->h - m_sprite->mask->y - m_sprite->mask->h;
	  break;
	case 90:
	  x = m_sprite->h - m_sprite->mask->y - m_sprite->mask->h;
	  y = m_sprite->mask->x;
	  break;
	case -90:
	  x = m_sprite->mask->y;
	  y = m_sprite->w - m_sprite->mask->x - m_sprite->mask->w;
	  break;
      }

      // create the new rotated mask
      mask_replace(new_mask, x, y,
		   m_angle == 180 ? m_sprite->mask->w: m_sprite->mask->h,
		   m_angle == 180 ? m_sprite->mask->h: m_sprite->mask->w);
      image_rotate(m_sprite->mask->bitmap, new_mask->bitmap, m_angle);

      // copy new mask
      undoable.copy_to_current_mask(new_mask);
      mask_free(new_mask);

      // regenerate mask
      sprite_generate_mask_boundaries(m_sprite);
    }

    // change the sprite's size
    if (m_angle != 180)
      undoable.set_sprite_size(m_sprite->h, m_sprite->w);

    // commit changes
    undoable.commit();
  }

};

static bool cmd_rotate_canvas_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return sprite != NULL;
}

static void cmd_rotate_canvas_execute(const char *argument)
{
  int angle = ustrtol(argument, NULL, 10);
  CurrentSpriteReader sprite;

  {
    RotateCanvasJob job(sprite, angle);
    job.do_job();
  }

  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);
}

Command cmd_rotate_canvas = {
  CMD_ROTATE_CANVAS,
  cmd_rotate_canvas_enabled,
  NULL,
  cmd_rotate_canvas_execute,
};
