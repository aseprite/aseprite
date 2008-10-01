/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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
#include <assert.h>
#include <string.h>

#include "jinete/jlist.h"
#include "jinete/jmanager.h"
#include "jinete/jsystem.h"
#include "jinete/jwidget.h"

#include "core/app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "core/dirs.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/sprites.h"
#include "raster/raster.h"
#include "util/functions.h"
#include "util/misc.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"

Image *GetImage(Sprite *sprite)
{
  Image *image = NULL;

  if (sprite && sprite->layer && layer_is_image(sprite->layer)) {
    Cel *cel = layer_get_cel(sprite->layer, sprite->frame);

    if (cel) {
      if ((cel->image >= 0) &&
	  (cel->image < sprite->stock->nimage))
	image = sprite->stock->image[cel->image];
    }
  }

  return image;
}

Image *GetImage2(Sprite *sprite, int *x, int *y, int *opacity)
{
  Image *image = NULL;

  if (sprite != NULL &&
      sprite->layer != NULL &&
      layer_is_image(sprite->layer)) {
    Cel *cel = layer_get_cel(sprite->layer, sprite->frame);

    if (cel) {
      if ((cel->image >= 0) &&
	  (cel->image < sprite->stock->nimage))
	image = sprite->stock->image[cel->image];

      if (x) *x = cel->x;
      if (y) *y = cel->y;
      if (opacity) *opacity = MID(0, cel->opacity, 255);
    }
  }

  return image;
}

void LoadPalette(const char *filename)
{
  if (current_sprite) {
    DIRS *dir, *dirs;
    char buf[512];

    dirs = dirs_new();
    dirs_add_path(dirs, filename);

    usprintf(buf, "palettes/%s", filename);
    dirs_cat_dirs(dirs, filename_in_datadir (buf));

    for (dir=dirs; dir; dir=dir->next) {
      if (exists(dir->path)) {
	Palette *pal = palette_load(dir->path);
	if (pal != NULL) {
	  /* set the palette calling the hooks */
	  set_current_palette(pal, FALSE);

	  /* just one palette */
	  sprite_reset_palettes(current_sprite);
	  sprite_set_palette(current_sprite, pal, 0);

	  /* redraw the entire screen */
	  jmanager_refresh_screen();
	}
	break;
      }
    }

    dirs_free(dirs);
  }
}

/* clears the mask region in the current sprite with the BG color */
void ClearMask()
{
  Sprite *sprite = current_sprite;
  int x, y, u, v, putx, puty;
  ase_uint8 *address;
  Image *image;
  div_t d;

  if (sprite != NULL) {
    image = GetImage2(sprite, &x, &y, NULL);
    if (image) {
      int bgcolor = app_get_color_to_clear_layer(sprite->layer);

      /* if the mask is empty then we have to clear the entire image
	 in the cel */
      if (mask_is_empty(sprite->mask)) {
	/* if the layer is the background then we clear the image */
	if (layer_is_background(sprite->layer)) {
	  if (undo_is_enabled(sprite->undo))
	    undo_image(sprite->undo, image, 0, 0, image->w, image->h);

	  /* clear all */
	  image_clear(image, bgcolor);
	}
	/* if the layer is transparent we can remove the cel (and it's
	   associated image) */
	else {
	  assert(layer_get_cel(sprite->layer, sprite->frame) != NULL);

	  RemoveCel(sprite->layer, layer_get_cel(sprite->layer,
						 sprite->frame));
	}
      }
      else {
	int x1 = MAX(0, sprite->mask->x);
	int y1 = MAX(0, sprite->mask->y);
	int x2 = MIN(image->w-1, sprite->mask->x+sprite->mask->w-1);
	int y2 = MIN(image->h-1, sprite->mask->y+sprite->mask->h-1);

	/* do nothing */
	if (x1 > x2 || y1 > y2)
	  return;

	if (undo_is_enabled(sprite->undo))
	  undo_image(sprite->undo, image, x1, y1, x2-x1+1, y2-y1+1);

	/* clear the masked zones */
	for (v=0; v<sprite->mask->h; v++) {
	  d = div(0, 8);
	  address = ((ase_uint8 **)sprite->mask->bitmap->line)[v]+d.quot;

	  for (u=0; u<sprite->mask->w; u++) {
	    if ((*address & (1<<d.rem))) {
	      putx = u+sprite->mask->x-x;
	      puty = v+sprite->mask->y-y;
	      image_putpixel(image, putx, puty, bgcolor);
	    }

	    _image_bitmap_next_bit(d, address);
	  }
	}
      }
    }
  }
}

/* returns a new layer created from the current mask in the current
   sprite, the layer isn't added to the sprite */
Layer *NewLayerFromMask(Sprite *src_sprite, Sprite *dst_sprite)
{
  ase_uint8 *address;
  int x, y, u, v, getx, gety;
  Image *dst, *src = GetImage2(src_sprite, &x, &y, NULL);
  Layer *layer;
  Cel *cel;
  div_t d;

  if (!src_sprite || !src_sprite->mask || !src_sprite->mask->bitmap || !src)
    return NULL;

  dst = image_new(dst_sprite->imgtype,
		  src_sprite->mask->w,
		  src_sprite->mask->h);
  if (!dst)
    return NULL;

  /* clear the new image */
  image_clear(dst, 0);

  /* copy the masked zones */
  for (v=0; v<src_sprite->mask->h; v++) {
    d = div(0, 8);
    address = ((ase_uint8 **)src_sprite->mask->bitmap->line)[v]+d.quot;

    for (u=0; u<src_sprite->mask->w; u++) {
      if ((*address & (1<<d.rem))) {
	getx = u+src_sprite->mask->x-x;
	gety = v+src_sprite->mask->y-y;

	if ((getx >= 0) && (getx < src->w) &&
	    (gety >= 0) && (gety < src->h))
	  dst->method->putpixel(dst, u, v,
				src->method->getpixel(src, getx, gety));
      }

      _image_bitmap_next_bit(d, address);
    }
  }

  layer = layer_new(dst_sprite);
  if (!layer) {
    image_free(dst);
    return NULL;
  }

  layer_set_blend_mode(layer, BLEND_MODE_NORMAL);

  cel = cel_new(dst_sprite->frame, stock_add_image(dst_sprite->stock, dst));
  cel_set_position(cel, dst_sprite->mask->x, dst_sprite->mask->y);

  layer_add_cel(layer, cel);

  return layer;
}

Image *GetLayerImage(Layer *layer, int *x, int *y, int frame)
{
  Image *image = NULL;

  if (layer_is_image (layer)) {
    Cel *cel = layer_get_cel(layer, frame);

    if (cel) {
      if ((cel->image >= 0) &&
	  (cel->image < layer->sprite->stock->nimage))
	image = layer->sprite->stock->image[cel->image];

      if (x) *x = cel->x;
      if (y) *y = cel->y;
    }
  }

  return image;
}

/* Gives to the user the possibility to move the sprite's layer in the
   current editor, returns TRUE if the position was changed.  */
int interactive_move_layer(int mode, bool use_undo, int (*callback)())
{
  JWidget editor = current_editor;
  Sprite *sprite = editor_get_sprite (editor);
  Layer *layer = sprite->layer;
  Cel *cel = layer_get_cel(layer, sprite->frame);
  int start_x, new_x;
  int start_y, new_y;
  int start_b;
  int ret;
  int update = FALSE;
  int quiet_clock = -1;
  int first_time = TRUE;
  int begin_x;
  int begin_y;

  if (!cel)
    return FALSE;

  begin_x = cel->x;
  begin_y = cel->y;

  hide_drawing_cursor(editor);
  jmouse_set_cursor(JI_CURSOR_MOVE);

  editor_click_start(editor, mode, &start_x, &start_y, &start_b);

  do {
    if (update) {
      cel->x = begin_x - start_x + new_x;
      cel->y = begin_y - start_y + new_y;

      /* update layer-bounds */
      jwidget_dirty(editor);

      /* update status bar */
      statusbar_set_text
	(app_get_statusbar(), 0,
	 "Pos %3d %3d Offset %3d %3d",
	 (int)cel->x,
	 (int)cel->y,
	 (int)(cel->x - begin_x),
	 (int)(cel->y - begin_y));

      /* update clock */
      quiet_clock = ji_clock;
      first_time = FALSE;
    }

    /* call the user's routine */
    if (callback)
      (*callback)();

    /* redraw dirty widgets */
    jwidget_flush_redraw(ji_get_default_manager());
    jmanager_dispatch_messages(ji_get_default_manager());

    gui_feedback();
  } while (editor_click(editor, &new_x, &new_y, &update, NULL));

  new_x = cel->x;
  new_y = cel->y;
  cel_set_position(cel, begin_x, begin_y);
  
  /* the position was changed */
  if (!editor_click_cancel(editor)) {
    if (use_undo && undo_is_enabled(sprite->undo)) {
      undo_set_label(sprite->undo, "Cel Movement");
      undo_open(sprite->undo);
      undo_int(sprite->undo, (GfxObj *)cel, &cel->x);
      undo_int(sprite->undo, (GfxObj *)cel, &cel->y);
      undo_close(sprite->undo);
    }

    cel_set_position(cel, new_x, new_y);
    ret = TRUE;
  }
  /* the position wasn't changed */
  else {
    ret = FALSE;
  }

  /* redraw the sprite in all editors */
  update_screen_for_sprite(sprite);

  /* restore the cursor */
  show_drawing_cursor(editor);

  editor_click_done(editor);

  return ret;
}

