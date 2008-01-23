/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007, 2008  David A. Capello
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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "jinete/jinete.h"
#include "jinete/jintern.h"

#include "console/console.h"
#include "core/cfg.h"
#include "core/dirs.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "script/import.h"
#include "util/mapgen.h"
#include "util/misc.h"

#endif

#ifndef M_SQRT2
#define M_SQRT2        1.41421356237309504880
#endif

static JWidget entry_width, entry_height, entry_seed, entry_factor;
static JWidget preview_map, check_preview;

static void regen_map(int forced);
static void random_seed_command(JWidget widget);
static void sqrt2_factor_command(JWidget widget);
static int change_hook(JWidget widget, int user_data);

static JWidget image_viewer_new(Image *image);
static bool image_viewer_msg_proc(JWidget widget, JMessage msg);

void dialogs_mapgen(void)
{
  JWidget window, view_map, check_tiled, button_ok;
  Sprite *sprite, *old_sprite = current_sprite;
  PALETTE old_palette;
  RGB *new_palette = NULL;
  double factor;
  char buf[256];
  int w, h, seed;

  window = load_widget("mapgen.jid", "mapgen");
  if (!window)
    return;

  if (!get_widgets (window,
		    "mapview", &view_map,
		    "width", &entry_width,
		    "height", &entry_height,
		    "seed", &entry_seed,
		    "fractal_factor", &entry_factor,
		    "preview", &check_preview,
		    "tiled", &check_tiled,
		    "button_ok", &button_ok, NULL)) {
    jwidget_free(window);
    return;
  }

  /* get current palette */
  palette_copy(old_palette, current_palette);

  /* load terrain palette */
  {
    DIRS *dir, *dirs = filename_in_datadir("palettes/terrain1.col");
    for (dir=dirs; dir; dir=dir->next) {
      new_palette = palette_load(dir->path);
      if (new_palette)
	break;
    }
    dirs_free(dirs);
  }

  sprite = sprite_new_with_layer(IMAGE_INDEXED, 256, 256);
  set_current_sprite(sprite);

  preview_map = image_viewer_new(GetImage(current_sprite));

  /* set palette */
  if (new_palette)
    sprite_set_palette(sprite, new_palette, 0);
  set_current_palette(sprite_get_palette(sprite, 0), FALSE);
  jmanager_refresh_screen();

  w = get_config_int("MapGen", "Width", 256);
  sprintf(buf, "%d", MID(1, w, 9999));
  jwidget_set_text(entry_width, buf);

  h = get_config_int("MapGen", "Height", 256);
  sprintf(buf, "%d", MID(1, h, 9999));
  jwidget_set_text(entry_height, buf);

  seed = get_config_int("MapGen", "Seed", 662355502);
  sprintf(buf, "%d", seed);
  jwidget_set_text(entry_seed, buf);

  factor = get_config_float("MapGen", "FractalFactor", M_SQRT2);
  sprintf(buf, "%.16g", factor);
  jwidget_set_text(entry_factor, buf);

  if (get_config_bool("MapGen", "Preview", TRUE))
    jwidget_select(check_preview);

  jwidget_select(check_tiled);
  jwidget_disable(check_tiled);

  HOOK(check_preview, JI_SIGNAL_CHECK_CHANGE, change_hook, 0);
  HOOK(entry_seed, JI_SIGNAL_ENTRY_CHANGE, change_hook, 0);
  HOOK(entry_factor, JI_SIGNAL_ENTRY_CHANGE, change_hook, 0);

  jbutton_add_command(jwidget_find_name(window, "random_seed"),
		      random_seed_command);
  jbutton_add_command(jwidget_find_name(window, "sqrt2_factor"),
		      sqrt2_factor_command);

  jview_attach(view_map, preview_map);

  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* load window configuration */
  load_window_pos(window, "MapGen");

  /* open the window */
  regen_map(FALSE);
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_ok) {
    /* generate the map */
    regen_map(TRUE);

    /* show the sprite */
    sprite_show(sprite);
  }
  else {
    set_current_sprite(old_sprite);
    set_current_palette(old_palette, FALSE);
    jmanager_refresh_screen();

    sprite_free(sprite);
  }

  /* save window configuration */
  save_window_pos(window, "MapGen");

  jwidget_free(window);
}

static void regen_map(int forced)
{
  int w, h, seed;
  double factor;

  if (forced || jwidget_is_selected(check_preview)) {
    w = strtol(jwidget_get_text(entry_width), NULL, 10);
    h = strtol(jwidget_get_text(entry_height), NULL, 10);
    seed = strtol(jwidget_get_text(entry_seed), NULL, 10);
    factor = strtod(jwidget_get_text(entry_factor), NULL);

    if (forced) {
      set_config_int("MapGen", "Width", w);
      set_config_int("MapGen", "Height", h);
      set_config_int("MapGen", "Seed", seed);
      set_config_float("MapGen", "FractalFactor", factor);
    }

    /* generate the map */
    mapgen(GetImage(current_sprite), seed, factor);
    jwidget_dirty(preview_map);
  }
}

static void random_seed_command(JWidget widget)
{
  char buf[256];
  sprintf(buf, "%d", rand());
  jwidget_set_text(entry_seed, buf);
  regen_map(FALSE);
}

static void sqrt2_factor_command(JWidget widget)
{
  char buf[256];
  sprintf(buf, "%.16g", M_SQRT2);
  jwidget_set_text(entry_factor, buf);
  regen_map(FALSE);
}

static int change_hook(JWidget widget, int user_data)
{
  regen_map(FALSE);
  return FALSE;
}

/**********************************************************************/
/* image viewer (simple preview) */

static JWidget image_viewer_new(Image *image)
{
  JWidget widget = jwidget_new(JI_WIDGET);
  jwidget_add_hook(widget, JI_WIDGET, image_viewer_msg_proc, NULL);
  widget->user_data[0] = image;
  return widget;
}

static bool image_viewer_msg_proc(JWidget widget, JMessage msg)
{
  Image *image = widget->user_data[0];

  switch (msg->type) {

    case JM_REQSIZE:
      msg->reqsize.w = image->w;
      msg->reqsize.h = image->h;
      return TRUE;

    case JM_DRAW: {
      BITMAP *bmp = create_bitmap(image->w, image->h);
      image_to_allegro(image, bmp, 0, 0);

      _ji_theme_rectfill_exclude
	(ji_screen,
	 widget->rc->x1, widget->rc->y1,
	 widget->rc->x2-1, widget->rc->y2-1,
	 widget->rc->x1, widget->rc->y1,
	 widget->rc->x1+bmp->w-1,
	 widget->rc->y1+bmp->h-1, jwidget_get_bg_color(widget));

      blit(bmp, ji_screen,
	   0, 0, widget->rc->x1, widget->rc->y1, bmp->w, bmp->h);
      destroy_bitmap(bmp);
      return TRUE;
    }

    case JM_BUTTONPRESSED: {
      JWidget view = jwidget_get_view(widget);
      if (view) {
	jwidget_hard_capture_mouse(widget);
	jmouse_set_cursor(JI_CURSOR_MOVE);
	return TRUE;
      }
      break;
    }

    case JM_MOTION: {
      JWidget view = jwidget_get_view(widget);
      if (view && jwidget_has_capture(widget)) {
	JRect vp = jview_get_viewport_position(view);
	int scroll_x, scroll_y;

	jview_get_scroll(view, &scroll_x, &scroll_y);
	jview_set_scroll(view,
			 scroll_x + jmouse_x(1) - jmouse_x(0),
			 scroll_y + jmouse_y(1) - jmouse_y(0));

	jmouse_control_infinite_scroll(vp);
	jrect_free(vp);
      }
      break;
    }

    case JM_BUTTONRELEASED: {
      JWidget view = jwidget_get_view(widget);
      if (view && jwidget_has_capture(widget)) {
	jwidget_release_mouse(widget);
	jmouse_set_cursor(JI_CURSOR_NORMAL);
	return TRUE;
      }
      break;
    }
  }

  return FALSE;
}
