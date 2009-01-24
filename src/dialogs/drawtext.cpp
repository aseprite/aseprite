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

#include <allegro.h>

#include "jinete/jinete.h"

#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "core/core.h"
#include "core/dirs.h"
#include "dialogs/filesel.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/sprites.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "util/clipbrd.h"
#include "util/misc.h"
#include "widgets/colbar.h"
#include "widgets/colbut.h"

static JWidget font_button;

static Image *render_text(FONT *f, const char *text, int color);

static FONT *my_load_font(const char *filename);
static void button_font_command(JWidget widget);
static void update_button_text();

void dialogs_draw_text()
{
  Image *image, *dest_image;
  JWidget window, button_ok, color_box, color_but;
  JWidget entry_size, entry_text;
  char buf[256];

  if (!is_interactive() || !current_sprite)
    return;

  dest_image = GetImage(current_sprite);
  if (!dest_image)
    return;

  window = load_widget("drawtext.jid", "drawtext_window");
  if (!window)
    return;

  if (!get_widgets(window,
		   "font", &font_button,
		   "text", &entry_text,
		   "size", &entry_size,
		   "color_box", &color_box,
		   "ok", &button_ok, NULL)) {
    jwidget_free(window);
    return;
  }

  /* font button */
  jbutton_add_command(font_button, button_font_command);
  update_button_text();

  /* color button */
  color_but = colorbutton_new
    (get_config_color("DrawText", "Color",
		      colorbar_get_fg_color(app_get_colorbar())),
     current_sprite->imgtype);

  jwidget_add_child(color_box, color_but);

  /* entries */
  usprintf(buf, "%d", get_config_int("DrawText", "Size", 8));
  jwidget_set_text(entry_size, buf);
  jwidget_set_text(entry_text,
		   get_config_string("DrawText", "Text", "ABCabc"));

  /* window */
  jwindow_remap(window);
  jwindow_center(window);
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_ok) {
    color_t color_with_type = colorbutton_get_color(color_but);
    const char *text = jwidget_get_text(entry_text);
    const char *size_str = jwidget_get_text(entry_size);
    const char *font_str = get_config_string("DrawText", "Font",
					     "allegro.pcx");
    int color;
    int size;
    FONT *f;

    /* size */
    size = ustrtol(size_str, NULL, 10);
    size = MID(1, size, 256);

    /* load the font */
    f = my_load_font(font_str);

    /* error loading font */
    if (!f) {
      console_printf(_("Error loading font.\n"));
    }
    /* the font was loaded */
    else {
      /* set font size */
      ji_font_set_size(f, size);

      /* setup color */
      color = get_color_for_image(current_sprite->imgtype,
				  color_with_type);

      /* update configuration */
      set_config_color("DrawText", "Color", color_with_type);
      set_config_string("DrawText", "Text", text);
      set_config_int("DrawText", "Size", size);

      /* render text */
      image = render_text(f, text, color);
      if (image) {
	copy_image_to_clipboard(image);
	paste_from_clipboard();
      }
      else
	console_printf(_("Error rendering text.\n"));

      /* free the font */
      destroy_font(f);
    }
  }

  jwidget_free(window);
}

Image *RenderText(const char *fontname, int size, int color, const char *text)
{
  Image *render;
  FONT *f;

  f = my_load_font(fontname);
  if (!f)
    return NULL;

  ji_font_set_size(f, size);

  render = render_text(f, text, color);

  destroy_font(f);

  return render;
}

static Image *render_text(FONT *f, const char *text, int color)
{
  /* TODO warning this uses Image->dat and not Image->line */
#define DO(type, colfunc)				\
  {							\
    register int c;					\
    ase_uint32 *src = (ase_uint32 *)bmp->dat;		\
    type *dst = (type *)image->dat;			\
    for (i=0; i<pixels; i++) {				\
      c = *src;						\
      *dst = colfunc;					\
      src++;						\
      dst++;						\
    }							\
  }

  int i, pixels, w, h;
  Image *image;
  BITMAP *bmp;

  w = text_length(f, text);
  h = text_height(f);
  pixels = w*h;
  if (!pixels)
    return NULL;

  bmp = create_bitmap_ex(32, w, h);
  if (!bmp)
    return NULL;

  text_mode(-1);
  ji_font_set_aa_mode(f, -1);

  clear_to_color(bmp, makecol32 (255, 0, 255));
  textout(bmp, f, text, 0, 0, makecol32 (255, 255, 255));

  image = image_new(current_sprite->imgtype, w, h);
  if (!image) {
    destroy_bitmap(bmp);
    return NULL;
  }

  image_clear(image, 0);
  acquire_bitmap(bmp);

  switch (image->imgtype) {

    case IMAGE_RGB:
      DO(ase_uint32, _rgba(_rgba_getr(color),
			   _rgba_getg(color),
			   _rgba_getb(color), getg32(c)));
      break;

    case IMAGE_GRAYSCALE:
      DO(ase_uint16, _graya(_graya_getv(color), getg32(c)));
      break;

    case IMAGE_INDEXED:
      use_current_sprite_rgb_map();
      DO(ase_uint8, c == makecol32(255, 0, 255) ? 0: color);
      restore_rgb_map();
      break;
  }

  release_bitmap(bmp);
  destroy_bitmap(bmp);
  return image;
}

static FONT *my_load_font(const char *filename)
{
  DIRS *dirs, *dir;
  FONT *f = NULL;
  char buf[256];

  /* directories to search */
  dirs = dirs_new();

  dirs_add_path(dirs, filename);

  usprintf(buf, "fonts/%s", filename);
  dirs_cat_dirs(dirs, filename_in_datadir(buf));

  usprintf(buf, "fonts/%s", get_filename(filename));
  dirs_cat_dirs(dirs, filename_in_datadir(buf));

  /* search the font */
  for (dir=dirs; dir; dir=dir->next) {
    /* load the font */
    f = ji_font_load(dir->path);
    if (f)
      break;
    else {
      if (exists(dir->path))
	PRINTF("Unknown font format \"%s\"\n", dir->path);
    }
  }

  dirs_free(dirs);

  /* error loading font */
  if (!f)
    console_printf(_("Error loading font.\n"));

  return f;
}

static void button_font_command(JWidget widget)
{
  jstring filename =
    ase_file_selector(_("Open Font (TTF or Allegro bitmap format)"),
		      get_config_string ("DrawText", "Font", ""),
		      "pcx,bmp,tga,lbm,ttf");

  if (!filename.empty()) {
    set_config_string("DrawText", "Font", filename.c_str());
    update_button_text();
  }
}

static void update_button_text()
{
  const char *font_str = get_config_string("DrawText", "Font", "allegro.pcx");

  jwidget_set_text(font_button, get_filename(font_str));
}
