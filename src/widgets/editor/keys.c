/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro/keyboard.h>

#include "jinete/jsystem.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"

#include "core/app.h"
#include "modules/color.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/tools.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "widgets/colbar.h"
#include "widgets/editor.h"

#endif

static int getpixel_sprite(Sprite *sprite, int x, int y);

int editor_keys_toset_zoom(JWidget widget, int scancode)
{
  Editor *editor = editor_data (widget);

  if ((editor->sprite) &&
      (jwidget_has_mouse (widget)) &&
      !(key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG))) {
    JWidget view = jwidget_get_view(widget);
    JRect vp = jview_get_viewport_position(view);
    int x, y, zoom;

    x = 0;
    y = 0;
    zoom = -1;

    switch (scancode) { /* TODO make these keys configurable */
      case KEY_1: zoom = 0; break;
      case KEY_2: zoom = 1; break;
      case KEY_3: zoom = 2; break;
      case KEY_4: zoom = 3; break;
      case KEY_5: zoom = 4; break;
      case KEY_6: zoom = 5; break;
    }

    /* zoom */
    if (zoom >= 0) {
      screen_to_editor(widget, jmouse_x(0), jmouse_y(0), &x, &y);

      x = editor->offset_x - jrect_w(vp)/2 + ((1<<zoom)>>1) + (x << zoom);
      y = editor->offset_y - jrect_h(vp)/2 + ((1<<zoom)>>1) + (y << zoom);

      if ((editor->zoom != zoom) ||
	  (editor->cursor_editor_x != (vp->x1+vp->x2)/2) ||
	  (editor->cursor_editor_y != (vp->y1+vp->y2)/2)) {
	int use_refresh_region = (editor->zoom == zoom) ? TRUE: FALSE;

	editor->zoom = zoom;

	editor_update(widget);
	editor_set_scroll(widget, x, y, use_refresh_region);

	jmouse_set_position((vp->x1+vp->x2)/2, (vp->y1+vp->y2)/2);
	jrect_free(vp);
    	return TRUE;
      }
    }

    jrect_free(vp);
  }

  return FALSE;
}

int editor_keys_toset_frame(JWidget widget, int scancode)
{
  Editor *editor = editor_data (widget);

  if ((editor->sprite) &&
/*       (jwidget_has_mouse (widget)) && */
      !(key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG))) {
    int old_frame = editor->sprite->frame;

    switch (scancode) {
      /* previous frame */
      case KEY_LEFT:
	editor->sprite->frame--;
	if (editor->sprite->frame < 0)
	  editor->sprite->frame = editor->sprite->frames-1;
	break;

	/* last frame */
      case KEY_UP:
	editor->sprite->frame = editor->sprite->frames-1;
	break;

	/* next frame */
      case KEY_RIGHT:
	editor->sprite->frame++;
	if (editor->sprite->frame >= editor->sprite->frames)
	  editor->sprite->frame = 0;
	break;

	/* first frame */
      case KEY_DOWN:
	editor->sprite->frame = 0;
	break;

	/* nothing */
      default:
	return FALSE;
    }

    if (editor->sprite->frame != old_frame)
      update_screen_for_sprite(editor->sprite);

    editor_update_status_bar_for_standby(widget);
    return TRUE;
  }

  return FALSE;
}

int editor_keys_toset_brushsize(JWidget widget, int scancode)
{
  Editor *editor = editor_data(widget);

  if ((editor->sprite) &&
      (jwidget_has_mouse (widget)) &&
      !(key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG))) {
    /* TODO configurable keys */
    /* set the thickness */
    if (scancode == KEY_MINUS_PAD) {
      if (get_brush_size () > 1)
	set_brush_size (get_brush_size ()-1);
      return TRUE;
    }
    else if (scancode == KEY_PLUS_PAD) {
      if (get_brush_size () < 32)
	set_brush_size (get_brush_size ()+1);
      return TRUE;
    }
  }

  return FALSE;
}

int editor_keys_toget_pixels(JWidget widget, int scancode)
{
  Editor *editor = editor_data (widget);

  if ((editor->sprite) &&
      (jwidget_has_mouse (widget)) &&
      !(key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG))) {
    /* TODO make these keys configurable */
    /* get a color from the sprite */
    if ((scancode == KEY_9) || (scancode == KEY_0)) {
      char *color = NULL;
      int x, y;

      /* pixel position to get */
      screen_to_editor(widget, jmouse_x(0), jmouse_y(0), &x, &y);

      /* get the color from the image */
      color = color_from_image(editor->sprite->imgtype,
			       getpixel_sprite(editor->sprite, x, y));

      /* set the color of the color-bar */
      color_bar_set_color(app_get_color_bar(),
			  scancode == KEY_9 ? 0: 1, color, TRUE);

      return TRUE;
    }
  }

  return FALSE;
}

/**
 * Gets a pixel from the sprite in the specified position. If in the
 * specified coordinates there're background this routine will return
 * the 0 color (the mask-color).
 */
static int getpixel_sprite(Sprite *sprite, int x, int y)
{
  Image *image;
  int color = 0;

  if ((x >= 0) && (y >= 0) && (x < sprite->w) && (y < sprite->h)) {
    int old_bgcolor = sprite->bgcolor;
    sprite->bgcolor = 0;
    
    image = image_new(sprite->imgtype, 1, 1);
    image_clear(image, 0);
    sprite_render(sprite, image, -x, -y);
    color = image_getpixel(image, 0, 0);
    image_free(image);

    sprite->bgcolor = old_bgcolor;
  }

  return color;
}
