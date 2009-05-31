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

#include "core/cfg.h"
#include "commands/commands.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undoable.h"

#define PERC_FORMAT	"%.1f%%"

static bool lock_ratio_change_hook(JWidget widget, void *data);
static bool width_px_change_hook(JWidget widget, void *data);
static bool height_px_change_hook(JWidget widget, void *data);
static bool width_perc_change_hook(JWidget widget, void *data);
static bool height_perc_change_hook(JWidget widget, void *data);

static bool cmd_sprite_size_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_sprite_size_execute(const char *argument)
{
  JWidget window, width_px, height_px, width_perc, height_perc, lock_ratio, method, ok;
  Sprite* sprite = current_sprite;

  // load the window widget
  window = load_widget("sprsize.jid", "sprite_size");
  if (!window)
    return;

  if (!get_widgets(window,
		   "width_px", &width_px,
		   "height_px", &height_px,
		   "width_perc", &width_perc,
		   "height_perc", &height_perc,
		   "lock_ratio", &lock_ratio,
		   "method", &method,
		   "ok", &ok, NULL)) {
    jwidget_free(window);
    return;
  }

  width_px->textf("%d", sprite->w);
  height_px->textf("%d", sprite->h);

  HOOK(lock_ratio, JI_SIGNAL_CHECK_CHANGE, lock_ratio_change_hook, 0);
  HOOK(width_px, JI_SIGNAL_ENTRY_CHANGE, width_px_change_hook, 0);
  HOOK(height_px, JI_SIGNAL_ENTRY_CHANGE, height_px_change_hook, 0);
  HOOK(width_perc, JI_SIGNAL_ENTRY_CHANGE, width_perc_change_hook, 0);
  HOOK(height_perc, JI_SIGNAL_ENTRY_CHANGE, height_perc_change_hook, 0);

  jcombobox_add_string(method, "Nearest-neighbor", NULL);
  jcombobox_add_string(method, "Bilinear", NULL);
  jcombobox_select_index(method, get_config_int("SpriteSize", "Method", RESIZE_METHOD_NEAREST_NEIGHBOR));

  jwindow_remap(window);
  jwindow_center(window);

  load_window_pos(window, "SpriteSize");
  jwidget_show(window);
  jwindow_open_fg(window);
  save_window_pos(window, "SpriteSize");

  if (jwindow_get_killer(window) == ok) {
    int new_width = width_px->text_int();
    int new_height = height_px->text_int();

    {
      Undoable undoable(sprite, "Sprite Size");
      ResizeMethod resize_method =
	(ResizeMethod)jcombobox_get_selected_index(method);

      set_config_int("SpriteSize", "Method", resize_method);

      // get all sprite cels
      JList cels = jlist_new();
      sprite_get_cels(sprite, cels);

      // for each cel...
      JLink link;
      JI_LIST_FOR_EACH(cels, link) {
	Cel* cel = (Cel*)link->data;

	// change it location
	undoable.set_cel_position(cel,
				  cel->x * new_width / sprite->w,
				  cel->y * new_height / sprite->h);
      }
      jlist_free(cels);

      // for each stock's image
      for (int i=0; i<sprite->stock->nimage; ++i) {
	Image* image = stock_get_image(sprite->stock, i);
	if (!image)
	  continue;

	// resize the image
	int w = image->w * new_width / sprite->w;
	int h = image->h * new_height / sprite->h;
	Image* new_image = image_new(image->imgtype, MAX(1, w), MAX(1, h));

	image_resize(image, new_image,
		     resize_method,
		     get_current_palette(),
		     orig_rgb_map);

	undoable.replace_stock_image(i, new_image);
      }

      // resize sprite
      undoable.set_sprite_size(new_width, new_height);

      // TODO resize mask
      undoable.commit();
    }
    sprite_generate_mask_boundaries(sprite);
    update_screen_for_sprite(sprite);
  }

  jwidget_free(window);
}

static bool lock_ratio_change_hook(JWidget widget, void *data)
{
  if (widget->selected())
    width_px_change_hook(widget->find_sibling("width_px"), NULL);

  return true;
}

static bool width_px_change_hook(JWidget widget, void *data)
{
  int width = widget->text_int();
  double perc = 100.0 * width / current_sprite->w;

  widget->find_sibling("width_perc")->textf(PERC_FORMAT, perc);

  if (widget->find_sibling("lock_ratio")->selected()) {
    widget->find_sibling("height_perc")->textf(PERC_FORMAT, perc);
    widget->find_sibling("height_px")->textf("%d", current_sprite->h * width / current_sprite->w);
  }

  return true;
}

static bool height_px_change_hook(JWidget widget, void *data)
{
  int height = widget->text_int();
  double perc = 100.0 * height / current_sprite->h;

  widget->find_sibling("height_perc")->textf(PERC_FORMAT, perc);

  if (widget->find_sibling("lock_ratio")->selected()) {
    widget->find_sibling("width_perc")->textf(PERC_FORMAT, perc);
    widget->find_sibling("width_px")->textf("%d", current_sprite->w * height / current_sprite->h);
  }

  return true;
}

static bool width_perc_change_hook(JWidget widget, void *data)
{
  double width = widget->text_double();

  widget->find_sibling("width_px")->textf("%d", (int)(current_sprite->w * width / 100));

  if (widget->find_sibling("lock_ratio")->selected()) {
    widget->find_sibling("height_px")->textf("%d", (int)(current_sprite->h * width / 100));
    widget->find_sibling("height_perc")->text(widget->text());
  }

  return true;
}

static bool height_perc_change_hook(JWidget widget, void *data)
{
  double height = widget->text_double();

  widget->find_sibling("height_px")->textf("%d", (int)(current_sprite->h * height / 100));

  if (widget->find_sibling("lock_ratio")->selected()) {
    widget->find_sibling("width_px")->textf("%d", (int)(current_sprite->w * height / 100));
    widget->find_sibling("width_perc")->text(widget->text());
  }

  return true;
}

Command cmd_sprite_size = {
  CMD_SPRITE_SIZE,
  cmd_sprite_size_enabled,
  NULL,
  cmd_sprite_size_execute,
  NULL
};
