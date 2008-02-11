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
#include <stdio.h>
#include <string.h>

#include "jinete/jinete.h"

#include "core/cfg.h"
#include "dialogs/filesel.h"
#include "modules/color.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "util/quantize.h"
#include "widgets/colview.h"
#include "widgets/paledit.h"

static PALETTE *palettes;

static JWidget slider_R, slider_G, slider_B;
static JWidget slider_H, slider_S, slider_V;
static JWidget color_viewer;
static JWidget palette_editor;
static JWidget slider_frame;

static void select_all_command(JWidget widget);
static void load_command(JWidget widget);
static void save_command(JWidget widget);
static void ramp_command(JWidget widget);
static void quantize_command(JWidget widget);

static int sliderRGB_change_signal(JWidget widget, int user_data);
static int sliderHSV_change_signal(JWidget widget, int user_data);
static int slider_columns_change_signal(JWidget widget, int user_data);
static int slider_frame_change_signal(JWidget widget, int user_data);
static int palette_editor_change_signal(JWidget widget, int user_data);

static void set_new_palette(RGB *palette);

void dialogs_palette_editor(void)
{
  JWidget window, color_viewer_box, palette_editor_view;
  JWidget slider_columns, button_ok;
  JWidget button_select_all;
  JWidget button_undo, button_redo;
  JWidget button_load, button_save;
  JWidget button_ramp, button_quantize;
  int frame, columns;
  PALETTE palette;
  int imgtype = current_sprite ? current_sprite->imgtype: IMAGE_INDEXED;
  int frame_bak = current_sprite ? current_sprite->frame : 0;

  if (imgtype == IMAGE_GRAYSCALE) {
    jalert(_("Error<<You can't edit grayscale palette||&OK"));
    return;
  }

  /* load widgets */
  window = load_widget("paledit.jid", "palette_editor");
  if (!window)
    return;

  if (!get_widgets (window,
		    "red", &slider_R,
		    "green", &slider_G,
		    "blue", &slider_B,
		    "hue", &slider_H,
		    "saturation", &slider_S,
		    "value", &slider_V,
		    "columns", &slider_columns,
		    "frame", &slider_frame,
		    "select_all", &button_select_all,
		    "undo", &button_undo,
		    "redo", &button_redo,
		    "load", &button_load,
		    "save", &button_save,
		    "ramp", &button_ramp,
		    "quantize", &button_quantize,
		    "button_ok", &button_ok,
		    "color_viewer", &color_viewer_box,
		    "palette_editor", &palette_editor_view, NULL)) {
    jwidget_free(window);
    return;
  }

  /* create current_sprite->frames palettes */
  if (current_sprite) {
    palettes = jmalloc(sizeof(PALETTE) * current_sprite->frames);
    if (!palettes) {
      jalert(_("Error<<Not enough memory||&OK"));
      return;
    }
    for (frame=0; frame<current_sprite->frames; frame++)
      memcpy(palettes[frame],
	     sprite_get_palette(current_sprite, frame), sizeof(PALETTE));
  }
  else 
    palettes = NULL;

  /* get current palette */
  palette_copy(palette, current_palette);

  /* get configuration */
  columns = get_config_int("PaletteEditor", "Columns", 16);
  columns = MID(1, columns, 256);

  /* custom widgets */
  color_viewer = color_viewer_new("index{0}", IMAGE_INDEXED);
  palette_editor = palette_editor_new(palette, TRUE, 6);

  jwidget_expansive(color_viewer, TRUE);
  jwidget_add_child(color_viewer_box, color_viewer);

  jwidget_disable(button_undo);
  jwidget_disable(button_redo);

  jview_attach(palette_editor_view, palette_editor);
  jview_maxsize(palette_editor_view);

  /* set columns */
  jslider_set_value(slider_columns, columns);
  palette_editor_set_columns(palette_editor, columns);

  /* frame */
  if (current_sprite) {
    jslider_set_range(slider_frame, 0, current_sprite->frames-1);
    jslider_set_value(slider_frame, current_sprite->frame);
  }
  else
    jwidget_disable(slider_frame);

  /* hook signals */
  HOOK(slider_R, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_signal, 0);
  HOOK(slider_G, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_signal, 0);
  HOOK(slider_B, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_signal, 0);
  HOOK(slider_H, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_signal, 0);
  HOOK(slider_S, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_signal, 0);
  HOOK(slider_V, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_signal, 0);
  HOOK(slider_columns, JI_SIGNAL_SLIDER_CHANGE, slider_columns_change_signal, 0);
  HOOK(slider_frame, JI_SIGNAL_SLIDER_CHANGE, slider_frame_change_signal, 0);
  HOOK(palette_editor, SIGNAL_PALETTE_EDITOR_CHANGE, palette_editor_change_signal, 0);

  jbutton_add_command(button_select_all, select_all_command);
  jbutton_add_command(button_load, load_command);
  jbutton_add_command(button_save, save_command);
  jbutton_add_command(button_ramp, ramp_command);
  jbutton_add_command(button_quantize, quantize_command);

  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* load window configuration */
  load_window_pos(window, "PaletteEditor");

  /* open and run the window */
  jwindow_open_fg(window);

  /* check the killer widget */
  if (jwindow_get_killer (window) == button_ok) {
    if (current_sprite) {
      palette_copy(palettes[jslider_get_value(slider_frame)],
		   current_palette);

      sprite_reset_palettes(current_sprite);
      for (frame=0; frame<current_sprite->frames; frame++) {
	if (frame == 0 ||
	    palette_diff(palettes[frame], palettes[frame-1], NULL, NULL))
	  sprite_set_palette(current_sprite, palettes[frame], frame);
      }
    }
    /* change the system palette */
    else
      set_default_palette(palette);

    set_current_palette(palette, TRUE);
  }
  /* cancel or ESC */
  else {
    /* restore the system palette */
    if (current_sprite) {
      current_sprite->frame = frame_bak;
      set_current_palette(sprite_get_palette(current_sprite, frame_bak), TRUE);
    }
    else {
      set_current_palette(NULL, TRUE);
    }
  }

  /* redraw the entire screen */
  jmanager_refresh_screen();

  /* save columns configuration */
  columns = jslider_get_value(slider_columns);
  set_config_int("PaletteEditors", "Columns", MID(1, columns, 256));

  /* save window configuration */
  save_window_pos(window, "PaletteEditor");

  jwidget_free(window);

  if (palettes)
    jfree(palettes);
}

static void select_all_command(JWidget widget)
{
  palette_editor_select_range(palette_editor, 0, 255,
			      PALETTE_EDITOR_RANGE_LINEAL);
}

static void load_command(JWidget widget)
{
  RGB *palette;
  char *filename;

  filename = ase_file_selector(_("Load Palette"), "", "pcx,bmp,tga,lbm,col");
  if (filename) {
    palette = palette_load(filename);
    if (!palette) {
      jalert(_("Error<<Loading palette file||&Close"));
    }
    else {
      set_new_palette(palette);
      jfree(palette);
    }
    jfree(filename);
  }
}

static void save_command(JWidget widget)
{
  char *filename;
  int ret;

 again:
  filename = ase_file_selector(_("Save Palette"), "", "pcx,bmp,tga,col");
  if (filename) {
    if (exists(filename)) {
      ret = jalert("%s<<%s<<%s||%s",
		   _("Warning"),
		   _("File exists, overwrite it?"),
		   get_filename(filename),
		   _("&Yes||&No||&Cancel"));

      if (ret == 2) {
	jfree(filename);
	goto again;
      }
      else if (ret != 1) {
	jfree(filename);
	return;
      }
    }

    if (palette_save(palette_editor_data(palette_editor)->palette, filename))
      jalert(_("Error<<Saving palette file||&Close"));

    jfree(filename);
  }
}

static void ramp_command(JWidget widget)
{
  PaletteEditor *paledit = palette_editor_data(palette_editor);
  int i1 = paledit->color[0];
  int i2 = paledit->color[1];
  PALETTE palette;
  bool array[256];

  palette_editor_get_selected_entries(palette_editor, array);
  palette_copy(palette, paledit->palette);

  if ((i1 >= 0) && (i2 >= 0)) {
    /* make the ramp */
    if (paledit->range_type == PALETTE_EDITOR_RANGE_LINEAL) {
      /* lineal ramp */
      make_palette_ramp(palette, i1, i2);
    }
    else if (paledit->range_type == PALETTE_EDITOR_RANGE_RECTANGULAR) {
      /* rectangular ramp */
      make_palette_rect_ramp(palette, i1, i2, paledit->columns);
    }
  }

  set_new_palette(palette);
}

static void quantize_command(JWidget widget)
{
  PaletteEditor *paledit = palette_editor_data(palette_editor);
  PALETTE palette;
  bool array[256];

  palette_editor_get_selected_entries(palette_editor, array);
  palette_copy(palette, paledit->palette);

  if (current_sprite && current_sprite->imgtype == IMAGE_RGB) {
    sprite_quantize_ex(current_sprite, palette);
  }
  else {
    jalert("Error<<You can use this command only for RGB sprites||&OK");
  }

  set_new_palette(palette);
}

static int sliderRGB_change_signal(JWidget widget, int user_data)
{
  PaletteEditor *paledit = palette_editor_data(palette_editor);
  int r = jslider_get_value(slider_R)>>2;
  int g = jslider_get_value(slider_G)>>2;
  int b = jslider_get_value(slider_B)>>2;
  float h, s, v;
  bool array[256];
  int c;

  rgb_to_hsv(_rgb_scale_6[r],
	     _rgb_scale_6[g],
	     _rgb_scale_6[b], &h, &s, &v);

  palette_editor_get_selected_entries(palette_editor, array);
  for (c=0; c<256; c++) {
    if (array[c]) {
      paledit->palette[c].r = r;
      paledit->palette[c].g = g;
      paledit->palette[c].b = b;

      set_current_color(c, r, g, b);
    }
  }

  jslider_set_value(slider_H, 255.0 * h / 360.0);
  jslider_set_value(slider_V, 255.0 * v);
  jslider_set_value(slider_S, 255.0 * s);

  jwidget_dirty(palette_editor);
  return FALSE;
}

static int sliderHSV_change_signal(JWidget widget, int user_data)
{
  PaletteEditor *paledit = palette_editor_data(palette_editor);
  int h = jslider_get_value(slider_H);
  int s = jslider_get_value(slider_S);
  int v = jslider_get_value(slider_V);
  bool array[256];
  int c, r, g, b;

  hsv_to_rgb(360.0 * h / 255.0, s / 255.0, v / 255.0, &r, &g, &b);

  r >>= 2;
  g >>= 2;
  b >>= 2;

  palette_editor_get_selected_entries(palette_editor, array);
  for (c=0; c<256; c++) {
    if (array[c]) {
      paledit->palette[c].r = r;
      paledit->palette[c].g = g;
      paledit->palette[c].b = b;

      set_current_color(c, r, g, b);
    }
  }

  jslider_set_value(slider_R, _rgb_scale_6[r]);
  jslider_set_value(slider_G, _rgb_scale_6[g]);
  jslider_set_value(slider_B, _rgb_scale_6[b]);

  jwidget_dirty(palette_editor);
  return FALSE;
}

static int slider_columns_change_signal(JWidget widget, int user_data)
{
  palette_editor_set_columns(palette_editor,
			     (int)jslider_get_value(widget));
  return FALSE;
}

static int slider_frame_change_signal(JWidget widget, int user_data)
{
  int old_frame = current_sprite->frame;
  int new_frame = jslider_get_value(slider_frame);

  palette_copy(palettes[old_frame], current_palette);
  current_sprite->frame = new_frame;
  set_new_palette(palettes[new_frame]);

  return FALSE;
}

static int palette_editor_change_signal(JWidget widget, int user_data)
{
  PaletteEditor *paledit = palette_editor_data(palette_editor);
  int imgtype = color_viewer_get_imgtype(color_viewer);
  char *color = color_index(paledit->color[1]);
  int r = color_get_red(imgtype, color);
  int g = color_get_green(imgtype, color);
  int b = color_get_blue(imgtype, color);
  float h, s, v;

  rgb_to_hsv(r, g, b, &h, &s, &v);

  color_viewer_set_color(color_viewer, color);

  jslider_set_value(slider_R, r);
  jslider_set_value(slider_G, g);
  jslider_set_value(slider_B, b);
  jslider_set_value(slider_H, 255.0 * h / 360.0);
  jslider_set_value(slider_V, 255.0 * v);
  jslider_set_value(slider_S, 255.0 * s);

  jfree(color);
  return FALSE;
}

static void set_new_palette(RGB *palette)
{
  PaletteEditor *paledit = palette_editor_data(palette_editor);

  /* copy the palette */
  palette_copy(paledit->palette, palette);

  /* set the palette calling the hooks */
  set_current_palette(palette, FALSE);

  /* redraw the entire screen */
  jmanager_refresh_screen();
}
