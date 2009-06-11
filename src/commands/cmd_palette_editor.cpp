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
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/cfg.h"
#include "core/color.h"
#include "dialogs/filesel.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "util/quantize.h"
#include "widgets/colview.h"
#include "widgets/paledit.h"

#define get_sprite(wgt) (*(const SpriteReader*)(jwidget_get_window(wgt))->user_data[0])

static Palette **palettes;

static JWidget slider_R, slider_G, slider_B;
static JWidget slider_H, slider_S, slider_V;
static JWidget colorviewer;
static JWidget palette_editor;
static JWidget slider_frame;
static JWidget check_all_frames;

static void select_all_command(JWidget widget);
static void load_command(JWidget widget);
static void save_command(JWidget widget);
static void ramp_command(JWidget widget);
static void quantize_command(JWidget widget);

static bool sliderRGB_change_hook(JWidget widget, void *data);
static bool sliderHSV_change_hook(JWidget widget, void *data);
static bool slider_columns_change_hook(JWidget widget, void *data);
static bool slider_frame_change_hook(JWidget widget, void *data);
static bool check_all_frames_change_hook(JWidget widget, void *data);
static bool palette_editor_change_hook(JWidget widget, void *data);

static void set_new_palette(Palette *palette);

static void cmd_palette_editor_execute(const char *argument)
{
  JWidget colorviewer_box, palette_editor_view;
  JWidget slider_columns, button_ok;
  JWidget button_select_all;
  JWidget button_undo, button_redo;
  JWidget button_load, button_save;
  JWidget button_ramp, button_quantize;
  int frame, columns;
  Palette *palette = NULL;
  const CurrentSpriteReader sprite;
  int imgtype = sprite ? sprite->imgtype: IMAGE_INDEXED;
  int frame_bak = sprite ? sprite->frame : 0;
  bool all_frames_same_palette = TRUE;

  if (imgtype == IMAGE_GRAYSCALE) {
    jalert(_("Error<<You can't edit grayscale palette||&OK"));
    return;
  }

  /* load widgets */
  JWidgetPtr window(load_widget("paledit.jid", "palette_editor"));
  get_widgets(window,
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
	      "colorviewer", &colorviewer_box,
	      "palette_editor", &palette_editor_view,
	      "all_frames", &check_all_frames, NULL);

  window->user_data[0] = (void*)&sprite;

  /* create current_sprite->frames palettes */
  if (sprite) {
    palettes = (Palette **)jmalloc(sizeof(Palette *) * sprite->frames);
    if (!palettes) {
      jalert(_("Error<<Not enough memory||&OK"));
      return;
    }
    for (frame=0; frame<sprite->frames; ++frame) {
      palettes[frame] = palette_new(frame, MAX_PALETTE_COLORS);
      palette_copy_colors(palettes[frame],
			  sprite_get_palette(sprite, frame));

      if (frame > 0 &&
	  palette_count_diff(palettes[frame-1], palettes[frame], NULL, NULL) > 0) {
	all_frames_same_palette = FALSE;
      }
    }
  }
  else {
    palettes = NULL;
    jwidget_disable(check_all_frames);
  }

  /* get current palette */
  palette = palette_new_copy(get_current_palette());

  /* get configuration */
  columns = get_config_int("PaletteEditor", "Columns", 16);
  columns = MID(1, columns, 256);

  /* custom widgets */
  colorviewer = colorviewer_new(color_index(0), IMAGE_INDEXED);
  palette_editor = paledit_new(palette, TRUE, 6);

  jwidget_expansive(colorviewer, TRUE);
  jwidget_add_child(colorviewer_box, colorviewer);

  jwidget_disable(button_undo);
  jwidget_disable(button_redo);

  jview_attach(palette_editor_view, palette_editor);
  jview_maxsize(palette_editor_view);

  /* set columns */
  jslider_set_value(slider_columns, columns);
  paledit_set_columns(palette_editor, columns);

  /* all frames */
  if (all_frames_same_palette)
    jwidget_select(check_all_frames);
  else
    jwidget_deselect(check_all_frames);
  
  /* frame */
  if (sprite) {
    jslider_set_range(slider_frame, 0, sprite->frames-1);
    jslider_set_value(slider_frame, sprite->frame);
    
    if (jwidget_is_selected(check_all_frames))
      jwidget_disable(slider_frame);
  }
  else {
    jwidget_disable(slider_frame);
    jwidget_disable(button_quantize);
  }

  /* hook signals */
  HOOK(slider_R, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_hook, 0);
  HOOK(slider_G, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_hook, 0);
  HOOK(slider_B, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_hook, 0);
  HOOK(slider_H, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_hook, 0);
  HOOK(slider_S, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_hook, 0);
  HOOK(slider_V, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_hook, 0);
  HOOK(slider_columns, JI_SIGNAL_SLIDER_CHANGE, slider_columns_change_hook, 0);
  HOOK(slider_frame, JI_SIGNAL_SLIDER_CHANGE, slider_frame_change_hook, 0);
  HOOK(check_all_frames, JI_SIGNAL_CHECK_CHANGE, check_all_frames_change_hook, 0);
  HOOK(palette_editor, SIGNAL_PALETTE_EDITOR_CHANGE, palette_editor_change_hook, 0);

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
  if (jwindow_get_killer(window) == button_ok) {
    if (sprite) {
      SpriteWriter sprite_writer(sprite);
      sprite_reset_palettes(sprite_writer);

      /* one palette */
      if (jwidget_is_selected(check_all_frames)) {
	/* copy the current palette in the first frame */
	palette_copy_colors(palettes[0],
			    get_current_palette());

	sprite_set_palette(sprite_writer, palettes[0], TRUE);
      }
      /* various palettes */
      else {
	frame = jslider_get_value(slider_frame);
	palette_copy_colors(palettes[frame],
			    get_current_palette());

	for (frame=0; frame<sprite_writer->frames; ++frame) {
	  if (frame == 0 ||
	      palette_count_diff(palettes[frame],
				 palettes[frame-1], NULL, NULL) > 0) {
	    sprite_set_palette(sprite_writer, palettes[frame], TRUE);
	  }
	}
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
    if (sprite) {
      SpriteWriter sprite_writer(sprite);
      sprite_writer->frame = frame_bak;

      set_current_palette(sprite_get_palette(sprite, frame_bak), TRUE);
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

  if (palettes) {
    assert(sprite);

    for (frame=0; frame<sprite->frames; ++frame)
      palette_free(palettes[frame]);

    jfree(palettes);
  }

  palette_free(palette);
}

static void select_all_command(JWidget widget)
{
  paledit_select_range(palette_editor, 0, 255,
		       PALETTE_EDITOR_RANGE_LINEAL);
}

static void load_command(JWidget widget)
{
  Palette *palette;
  jstring filename = ase_file_selector(_("Load Palette"), "", "pcx,bmp,tga,lbm,col");
  if (!filename.empty()) {
    palette = palette_load(filename.c_str());
    if (!palette) {
      jalert(_("Error<<Loading palette file||&Close"));
    }
    else {
      set_new_palette(palette);
      palette_free(palette);
    }
  }
}

static void save_command(JWidget widget)
{
  jstring filename;
  int ret;

 again:
  filename = ase_file_selector(_("Save Palette"), "", "pcx,bmp,tga,col");
  if (!filename.empty()) {
    if (exists(filename.c_str())) {
      ret = jalert("%s<<%s<<%s||%s",
		   _("Warning"),
		   _("File exists, overwrite it?"),
		   get_filename(filename.c_str()),
		   _("&Yes||&No||&Cancel"));

      if (ret == 2)
	goto again;
      else if (ret != 1)
	return;
    }

    if (palette_save(paledit_get_palette(palette_editor),
		     filename.c_str()))
      jalert(_("Error<<Saving palette file||&Close"));
  }
}

static void ramp_command(JWidget widget)
{
  int range_type = paledit_get_range_type(palette_editor);
  int i1 = paledit_get_1st_color(palette_editor);
  int i2 = paledit_get_2nd_color(palette_editor);
  Palette *palette = palette_new(0, MAX_PALETTE_COLORS);
  bool array[256];

  paledit_get_selected_entries(palette_editor, array);
  palette_copy_colors(palette,
		      paledit_get_palette(palette_editor));

  if ((i1 >= 0) && (i2 >= 0)) {
    /* make the ramp */
    if (range_type == PALETTE_EDITOR_RANGE_LINEAL) {
      /* lineal ramp */
      palette_make_horz_ramp(palette, i1, i2);
    }
    else if (range_type == PALETTE_EDITOR_RANGE_RECTANGULAR) {
      /* rectangular ramp */
      palette_make_rect_ramp(palette, i1, i2, paledit_get_columns(palette_editor));
    }
  }

  set_new_palette(palette);
  palette_free(palette);
}

static void quantize_command(JWidget widget)
{
  const SpriteReader& sprite = get_sprite(widget);
  assert(sprite != NULL);

  Palette* palette = palette_new(0, MAX_PALETTE_COLORS);
  bool array[256];

  paledit_get_selected_entries(palette_editor, array);
  palette_copy_colors(palette,
		      paledit_get_palette(palette_editor));

  if (sprite->imgtype == IMAGE_RGB) {
    SpriteWriter sprite_writer(sprite);
    sprite_quantize_ex(sprite_writer, palette);
  }
  else {
    jalert(_("Error<<You can use this command only for RGB sprites||&OK"));
  }

  set_new_palette(palette);
  palette_free(palette);
}

static bool sliderRGB_change_hook(JWidget widget, void *data)
{
  Palette *palette = paledit_get_palette(palette_editor);
  int r = jslider_get_value(slider_R);
  int g = jslider_get_value(slider_G);
  int b = jslider_get_value(slider_B);
  float h, s, v;
  bool array[256];
  int c;

  rgb_to_hsv(r, g, b, &h, &s, &v);

  paledit_get_selected_entries(palette_editor, array);
  for (c=0; c<256; c++) {
    if (array[c]) {
      palette->color[c] = _rgba(r, g, b, 255);
      set_current_color(c, r, g, b);
    }
  }

  jslider_set_value(slider_H, 255.0 * h / 360.0);
  jslider_set_value(slider_V, 255.0 * v);
  jslider_set_value(slider_S, 255.0 * s);

  jwidget_dirty(palette_editor);
  return FALSE;
}

static bool sliderHSV_change_hook(JWidget widget, void *data)
{
  Palette *palette = paledit_get_palette(palette_editor);
  int h = jslider_get_value(slider_H);
  int s = jslider_get_value(slider_S);
  int v = jslider_get_value(slider_V);
  bool array[256];
  int c, r, g, b;

  hsv_to_rgb(360.0 * h / 255.0, s / 255.0, v / 255.0, &r, &g, &b);

  paledit_get_selected_entries(palette_editor, array);
  for (c=0; c<256; c++) {
    if (array[c]) {
      palette->color[c] = _rgba(r, g, b, 255);
      set_current_color(c, r, g, b);
    }
  }

  jslider_set_value(slider_R, r);
  jslider_set_value(slider_G, g);
  jslider_set_value(slider_B, b);

  jwidget_dirty(palette_editor);
  return FALSE;
}

static bool slider_columns_change_hook(JWidget widget, void *data)
{
  paledit_set_columns(palette_editor,
		      (int)jslider_get_value(widget));
  return FALSE;
}

static bool slider_frame_change_hook(JWidget widget, void *data)
{
  const SpriteReader& sprite = get_sprite(widget);
  assert(sprite != NULL);

  int old_frame = sprite->frame;
  int new_frame = jslider_get_value(slider_frame);

  palette_copy_colors(palettes[old_frame],
		      get_current_palette());
  {
    SpriteWriter sprite_writer(sprite);
    sprite_writer->frame = new_frame;
  }
  set_new_palette(palettes[new_frame]);

  return FALSE;
}

static bool check_all_frames_change_hook(JWidget widget, void *data)
{
  const SpriteReader& sprite = get_sprite(widget);
  assert(sprite != NULL);

  int frame = jslider_get_value(slider_frame);

  palette_copy_colors(palettes[frame],
		      get_current_palette());

  if (jwidget_is_selected(check_all_frames)) {
    bool has_two_or_more_palettes = FALSE;
    int c;

    for (c=1; c<sprite->frames; c++) {
      if (palette_count_diff(palettes[c-1], palettes[c], NULL, NULL) > 0) {
	has_two_or_more_palettes = TRUE;
	break;
      }
    }

    if (has_two_or_more_palettes) {
      if (jalert(PACKAGE
		 "<<There are more than one palette. Only the"
		 "<<current palette will be kept (for all frames)."
		 "<<Do you want to continue?"
		 "||&Yes||&No") != 1) {
	jwidget_deselect(check_all_frames);
	return FALSE;
      }
    }

    jwidget_disable(slider_frame);
  }
  else
    jwidget_enable(slider_frame);

  return FALSE;
}

static bool palette_editor_change_hook(JWidget widget, void *data)
{
  int imgtype = colorviewer_get_imgtype(colorviewer);
  color_t color = color_index(paledit_get_2nd_color(palette_editor));
  int r = color_get_red(imgtype, color);
  int g = color_get_green(imgtype, color);
  int b = color_get_blue(imgtype, color);
  float h, s, v;

  rgb_to_hsv(r, g, b, &h, &s, &v);

  colorviewer_set_color(colorviewer, color);

  jslider_set_value(slider_R, r);
  jslider_set_value(slider_G, g);
  jslider_set_value(slider_B, b);
  jslider_set_value(slider_H, 255.0 * h / 360.0);
  jslider_set_value(slider_V, 255.0 * v);
  jslider_set_value(slider_S, 255.0 * s);
  return FALSE;
}

static void set_new_palette(Palette *palette)
{
  /* copy the palette */
  palette_copy_colors(paledit_get_palette(palette_editor),
		      palette);

  /* set the palette calling the hooks */
  set_current_palette(palette, FALSE);

  /* redraw the entire screen */
  jmanager_refresh_screen();
}

Command cmd_palette_editor = {
  CMD_PALETTE_EDITOR,
  NULL,
  NULL,
  cmd_palette_editor_execute,
  NULL
};
