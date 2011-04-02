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

#include <allegro.h>
#include <string.h>

#include "app.h"
#include "app/color.h"
#include "core/cfg.h"
#include "gui/list.h"
#include "gui/manager.h"
#include "gui/system.h"
#include "gui/widget.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/raster.h"
#include "undo/undo_history.h"
#include "undoers/set_cel_position.h"
#include "util/misc.h"
#include "widgets/editor/editor.h"
#include "widgets/statebar.h"

Image* NewImageFromMask(const Document* srcDocument)
{
  const Sprite* srcSprite = srcDocument->getSprite();
  const Mask* srcMask = srcDocument->getMask();
  const uint8_t* address;
  int x, y, u, v, getx, gety;
  Image *dst;
  const Image *src = srcSprite->getCurrentImage(&x, &y);
  div_t d;

  ASSERT(srcSprite);
  ASSERT(srcMask);
  ASSERT(srcMask->bitmap);
  ASSERT(src);

  dst = image_new(srcSprite->getImgType(), srcMask->w, srcMask->h);
  if (!dst)
    return NULL;

  // Clear the new image
  image_clear(dst, 0);

  // Copy the masked zones
  for (v=0; v<srcMask->h; v++) {
    d = div(0, 8);
    address = ((const uint8_t**)srcMask->bitmap->line)[v]+d.quot;

    for (u=0; u<srcMask->w; u++) {
      if ((*address & (1<<d.rem))) {
	getx = u+srcMask->x-x;
	gety = v+srcMask->y-y;

	if ((getx >= 0) && (getx < src->w) &&
	    (gety >= 0) && (gety < src->h))
	  dst->putpixel(u, v, src->getpixel(getx, gety));
      }

      _image_bitmap_next_bit(d, address);
    }
  }

  return dst;
}

// Gives to the user the possibility to move the sprite's layer in the
// current editor, returns true if the position was changed.
int interactive_move_layer(int mode, bool use_undo, int (*callback)())
{
  Editor* editor = current_editor;
  Document* document = editor->getDocument();
  undo::UndoHistory* undo = document->getUndoHistory();
  Sprite* sprite = document->getSprite();

  ASSERT(sprite->getCurrentLayer()->is_image());

  LayerImage* layer = static_cast<LayerImage*>(sprite->getCurrentLayer());
  Cel *cel = layer->getCel(sprite->getCurrentFrame());
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

  begin_x = cel->getX();
  begin_y = cel->getY();

  editor->hideDrawingCursor();
  jmouse_set_cursor(JI_CURSOR_MOVE);

  editor->editor_click_start(mode, &start_x, &start_y, &start_b);

  do {
    if (update) {
      cel->setPosition(begin_x - start_x + new_x,
		       begin_y - start_y + new_y);

      // Update layer-bounds.
      editor->invalidate();

      // Update status bar.
      app_get_statusbar()->setStatusText
	(0,
	 "Pos %3d %3d Offset %3d %3d",
	 (int)cel->getX(),
	 (int)cel->getY(),
	 (int)(cel->getX() - begin_x),
	 (int)(cel->getY() - begin_y));

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

  new_x = cel->getX();
  new_y = cel->getY();
  cel->setPosition(begin_x, begin_y);
  
  /* the position was changed */
  if (!editor->editor_click_cancel()) {
    if (use_undo && undo->isEnabled()) {
      undo->setLabel("Cel Movement");
      undo->setModification(undo::ModifyDocument);

      undo->pushUndoer(new undoers::SetCelPosition(undo->getObjects(), cel));
    }

    cel->setPosition(new_x, new_y);
    ret = true;
  }
  /* the position wasn't changed */
  else {
    ret = false;
  }

  /* redraw the sprite in all editors */
  update_screen_for_document(document);

  /* restore the cursor */
  editor->showDrawingCursor();

  editor->editor_click_done();

  return ret;
}

