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

#include <allegro.h>
#include <string.h>

#include "jinete/jlist.h"
#include "jinete/jmanager.h"
#include "jinete/jsystem.h"
#include "jinete/jwidget.h"

#include "app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/raster.h"
#include "util/misc.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"

Image* NewImageFromMask(const Sprite* src_sprite)
{
  const ase_uint8* address;
  int x, y, u, v, getx, gety;
  Image *dst;
  const Image *src = src_sprite->getCurrentImage(&x, &y);
  div_t d;

  ASSERT(src_sprite);
  ASSERT(src_sprite->getMask());
  ASSERT(src_sprite->getMask()->bitmap);
  ASSERT(src);

  dst = image_new(src_sprite->getImgType(),
		  src_sprite->getMask()->w,
		  src_sprite->getMask()->h);
  if (!dst)
    return NULL;

  /* clear the new image */
  image_clear(dst, 0);

  /* copy the masked zones */
  for (v=0; v<src_sprite->getMask()->h; v++) {
    d = div(0, 8);
    address = ((const ase_uint8 **)src_sprite->getMask()->bitmap->line)[v]+d.quot;

    for (u=0; u<src_sprite->getMask()->w; u++) {
      if ((*address & (1<<d.rem))) {
	getx = u+src_sprite->getMask()->x-x;
	gety = v+src_sprite->getMask()->y-y;

	if ((getx >= 0) && (getx < src->w) &&
	    (gety >= 0) && (gety < src->h))
	  dst->putpixel(u, v, src->getpixel(getx, gety));
      }

      _image_bitmap_next_bit(d, address);
    }
  }

  return dst;
}

/* Gives to the user the possibility to move the sprite's layer in the
   current editor, returns true if the position was changed.  */
int interactive_move_layer(int mode, bool use_undo, int (*callback)())
{
  Editor* editor = current_editor;
  Sprite* sprite = editor->getSprite();

  ASSERT(sprite->getCurrentLayer()->is_image());

  LayerImage* layer = static_cast<LayerImage*>(sprite->getCurrentLayer());
  Cel *cel = layer->get_cel(sprite->getCurrentFrame());
  int start_x, new_x;
  int start_y, new_y;
  int start_b;
  int ret;
  int update = false;
  int quiet_clock = -1;
  int first_time = true;
  int begin_x;
  int begin_y;

  if (!cel)
    return false;

  begin_x = cel->x;
  begin_y = cel->y;

  editor->hide_drawing_cursor();
  jmouse_set_cursor(JI_CURSOR_MOVE);

  editor->editor_click_start(mode, &start_x, &start_y, &start_b);

  do {
    if (update) {
      cel->x = begin_x - start_x + new_x;
      cel->y = begin_y - start_y + new_y;

      /* update layer-bounds */
      jwidget_dirty(editor);

      /* update status bar */
      app_get_statusbar()->setStatusText
	(0,
	 "Pos %3d %3d Offset %3d %3d",
	 (int)cel->x,
	 (int)cel->y,
	 (int)(cel->x - begin_x),
	 (int)(cel->y - begin_y));

      /* update clock */
      quiet_clock = ji_clock;
      first_time = false;
    }

    /* call the user's routine */
    if (callback)
      (*callback)();

    /* redraw dirty widgets */
    jwidget_flush_redraw(ji_get_default_manager());
    jmanager_dispatch_messages(ji_get_default_manager());

    gui_feedback();
  } while (editor->editor_click(&new_x, &new_y, &update, NULL));

  new_x = cel->x;
  new_y = cel->y;
  cel_set_position(cel, begin_x, begin_y);
  
  /* the position was changed */
  if (!editor->editor_click_cancel()) {
    if (use_undo && undo_is_enabled(sprite->getUndo())) {
      undo_set_label(sprite->getUndo(), "Cel Movement");
      undo_open(sprite->getUndo());
      undo_int(sprite->getUndo(), (GfxObj *)cel, &cel->x);
      undo_int(sprite->getUndo(), (GfxObj *)cel, &cel->y);
      undo_close(sprite->getUndo());
    }

    cel_set_position(cel, new_x, new_y);
    ret = true;
  }
  /* the position wasn't changed */
  else {
    ret = false;
  }

  /* redraw the sprite in all editors */
  update_screen_for_sprite(sprite);

  /* restore the cursor */
  editor->show_drawing_cursor();

  editor->editor_click_done();

  return ret;
}

