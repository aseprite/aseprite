/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#include <allegro.h>

#include "jinete.h"

#include "core/app.h"
#include "core/core.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/render.h"
#include "raster/raster.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"

#endif

#define JUST_ONE	1
#define FIT_ON_SCREEN	2

static void view_sprite(int flags);

void view_tiled(void) { view_sprite(0); }
void view_normal(void) { view_sprite(JUST_ONE); }
void view_fullscreen(void) { view_sprite(JUST_ONE | FIT_ON_SCREEN); }

static void view_sprite(int flags)
{
  JWidget widget = current_editor;

  if (is_interactive() && widget && editor_get_sprite(widget)) {
    Editor *editor = editor_data (widget);
    Sprite *sprite = editor_get_sprite (widget);
    JWidget view = jwidget_get_view (widget);
    int old_mouse_x, old_mouse_y;
    int scroll_x, scroll_y;
    int u, v, x, y, w, h;
    int shiftx, shifty;
    Image *image;
    BITMAP *bmp;
    int redraw;
    JRect vp;
    int bg_color, index_bg_color = -1;

    jmanager_free_mouse();

    vp = jview_get_viewport_position(view);
    jview_get_scroll(view, &scroll_x, &scroll_y);

    old_mouse_x = ji_mouse_x(0);
    old_mouse_y = ji_mouse_y(0);

    bmp = create_bitmap (sprite->w, sprite->h);
    if (bmp) {
      /* print a informative text */
      status_bar_set_text(app_get_status_bar(), 1, _("Rendering..."));
      jwidget_flush_redraw(app_get_status_bar());
      jmanager_dispatch_messages();

      ji_mouse_set_cursor(JI_CURSOR_NULL);
      ji_mouse_set_position(JI_SCREEN_W/2, JI_SCREEN_H/2);

      /* render the sprite in the bitmap */
      image = render_sprite(sprite, 0, 0, sprite->w, sprite->h,
			    sprite->frpos, 0);
      if (image) {
	image_to_allegro(image, bmp, 0, 0);
	image_free(image);
      }

      if (flags & JUST_ONE)
	bg_color = palette_color[index_bg_color=0];
      else
	bg_color = makecol(128, 128, 128);

      shiftx = - scroll_x + vp->x1 + editor->offset_x;
      shifty = - scroll_y + vp->y1 + editor->offset_y;

      w = sprite->w << editor->zoom;
      h = sprite->h << editor->zoom;

      redraw = TRUE;
      do {
	/* update scroll */
	if (ji_mouse_poll()) {
	  shiftx += ji_mouse_x(0) - JI_SCREEN_W/2;
	  shifty += ji_mouse_y(0) - JI_SCREEN_H/2;
	  ji_mouse_set_position(JI_SCREEN_W/2, JI_SCREEN_H/2);
	  ji_mouse_poll();

	  redraw = TRUE;
	}

	if (redraw) {
	  redraw = FALSE;

	  /* fit on screen */
	  if (flags & FIT_ON_SCREEN) {
	    double sx, sy, scale, outw, outh;

	    sx = (double)JI_SCREEN_W / (double)bmp->w;
	    sy = (double)JI_SCREEN_H / (double)bmp->h;
	    scale = MIN (sx, sy);

	    outw = (double)bmp->w * (double)scale;
	    outh = (double)bmp->h * (double)scale;

	    stretch_blit(bmp, ji_screen, 0, 0, bmp->w, bmp->h, 0, 0, outw, outh);
	    rectfill_exclude(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1,
			     0, 0, outw-1, outh-1, bg_color);
	  }
	  /* draw in normal size */
	  else {
	    if (flags & JUST_ONE) {
	      x = shiftx;
	      y = shifty;
	    }
	    else {
	      x = SGN(shiftx) * (ABS(shiftx)%w);
	      y = SGN(shifty) * (ABS(shifty)%h);
	    }

	    if (flags & JUST_ONE) {
/* 	      rectfill_exclude(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1, */
/* 			       x, y, x+w-1, y+h-1, bg_color); */
	      clear_to_color(ji_screen, bg_color);
	    }

	    if (!editor->zoom) {
	      /* in the center */
	      if (flags & JUST_ONE)
		draw_sprite(ji_screen, bmp, x, y);
	      /* tiled */
	      else
		for (v=y-h; v<JI_SCREEN_H+h; v+=h)
		  for (u=x-w; u<JI_SCREEN_W+w; u+=w)
		    blit(bmp, ji_screen, 0, 0, u, v, w, h);
	    }
	    else {
	      /* in the center */
	      if (flags & JUST_ONE)
		masked_stretch_blit(bmp, ji_screen, 0, 0, bmp->w, bmp->h, x, y, w, h);
	      /* tiled */
	      else
		for (v=y-h; v<JI_SCREEN_H+h; v+=h)
		  for (u=x-w; u<JI_SCREEN_W+w; u+=w)
		    stretch_blit(bmp, ji_screen, 0, 0, bmp->w, bmp->h, u, v, w, h);
	    }
	  }
	}

	gui_feedback();

	if (keypressed()) {
	  int c = readkey()>>8;

	  /* change frame */
	  if (editor_keys_toset_frpos(widget, c)) {
	    /* redraw */
	    redraw = TRUE;

	    /* render the sprite in the bitmap */
	    image = render_sprite(sprite, 0, 0, sprite->w, sprite->h,
				  sprite->frpos, 0);
	    if (image) {
	      image_to_allegro(image, bmp, 0, 0);
	      image_free(image);
	    }
	  }
	  /* change background color */
	  else if (c == KEY_PLUS_PAD) {
	    if (index_bg_color < 255) {
	      bg_color = palette_color[++index_bg_color];
	      redraw = TRUE;
	    }
	  }
	  else if (c == KEY_MINUS_PAD) {
	    if (index_bg_color > 0) {
	      bg_color = palette_color[--index_bg_color];
	      redraw = TRUE;
	    }
	  }
	  else
	    break;
	}
      } while (!ji_mouse_b(0));

      destroy_bitmap(bmp);
    }

    do {
      ji_mouse_poll();
      gui_feedback();
    } while (ji_mouse_b(0));
    clear_keybuf();

    ji_mouse_set_position(old_mouse_x, old_mouse_y);
    ji_mouse_set_cursor(JI_CURSOR_NORMAL);

    jmanager_refresh_screen();
    jrect_free(vp);
  }
}
