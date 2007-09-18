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
#include <stdio.h>
#include <string.h>

#include "jinete.h"

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

#endif

static JWidget slider_R, slider_G, slider_B, slider_A;
static JWidget slider_H, slider_S, slider_V;
static JWidget color_viewer;
static JWidget palette_editor;

static int sliderRGB_change_signal(JWidget widget, int user_data);
static int sliderHSV_change_signal(JWidget widget, int user_data);
static int sliderA_change_signal(JWidget widget, int user_data);
static int color_viewer_select_signal(JWidget widget, int user_data);
static int palette_editor_change_signal(JWidget widget, int user_data);
static int window_resize_signal(JWidget widget, int user_data);

char *ji_color_select(int imgtype, const char *color)
{
  JWidget window, color_viewer_box, palette_editor_view, button_ok;
  char *selected_color;
  char buf[256];
  PALETTE palette;

  get_palette(palette);

  /* get current palette */
  window = load_widget("colsel.jid", "color_selection");
  if (!window)
    return NULL;

  /* window title */
  sprintf(buf, "%s (%s)", window->text,
	  imgtype == IMAGE_RGB ? "RGB":
	  imgtype == IMAGE_GRAYSCALE ? "Grayscale":
	  imgtype == IMAGE_INDEXED ? "Indexed": "Unknown");

  jwidget_set_text(window, buf);

  if (!get_widgets(window,
		   "red", &slider_R,
		   "green", &slider_G,
		   "blue", &slider_B,
		   "alpha", &slider_A,
		   "hue", &slider_H,
		   "saturation", &slider_S,
		   "value", &slider_V,
		   "button_ok", &button_ok,
		   "color_viewer", &color_viewer_box,
		   "palette_editor", &palette_editor_view, NULL)) {
    jwidget_free(window);
    return NULL;
  }

  color_viewer = color_viewer_new(color, imgtype);
  palette_editor = palette_editor_new(palette, FALSE, 3);
  palette_editor_set_columns(palette_editor, 32);

  jwidget_expansive(color_viewer, TRUE);
  jwidget_add_child(color_viewer_box, color_viewer);

  jview_attach(palette_editor_view, palette_editor);
  jview_maxsize(palette_editor_view);
  jview_without_bars(palette_editor_view);

  /* hook signals */
  HOOK(slider_R, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_signal, 0);
  HOOK(slider_G, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_signal, 0);
  HOOK(slider_B, JI_SIGNAL_SLIDER_CHANGE, sliderRGB_change_signal, 0);
  HOOK(slider_A, JI_SIGNAL_SLIDER_CHANGE, sliderA_change_signal, 0);
  HOOK(slider_H, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_signal, 0);
  HOOK(slider_S, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_signal, 0);
  HOOK(slider_V, JI_SIGNAL_SLIDER_CHANGE, sliderHSV_change_signal, 0);
  HOOK(color_viewer, SIGNAL_COLOR_VIEWER_SELECT, color_viewer_select_signal, 0);
  HOOK(palette_editor, SIGNAL_PALETTE_EDITOR_CHANGE, palette_editor_change_signal, 0);
  HOOK(window, JI_SIGNAL_WINDOW_RESIZE, window_resize_signal, palette_editor);

  /* initial values */
  switch (color_type(color)) {
    case COLOR_TYPE_MASK:
      color_viewer_select_signal(NULL, 0);
      break;
    case COLOR_TYPE_RGB:
    case COLOR_TYPE_GRAY:
      jslider_set_value(slider_R, color_get_red(imgtype, color));
      jslider_set_value(slider_G, color_get_green(imgtype, color));
      jslider_set_value(slider_B, color_get_blue(imgtype, color));
      jslider_set_value(slider_A, color_get_alpha(imgtype, color));
      sliderRGB_change_signal(NULL, 0);
      break;
    case COLOR_TYPE_INDEX:
      palette_editor_select_color(palette_editor, color_get_index(imgtype, color));
      palette_editor_change_signal(NULL, color_get_index(imgtype, color));
      break;
  }

  /* disable widgets */
  if (imgtype != IMAGE_RGB) {
    if (imgtype == IMAGE_GRAYSCALE) {
      jwidget_disable(slider_R);
      jwidget_disable(slider_G);
      jwidget_disable(slider_B);
      jwidget_disable(slider_H);
      jwidget_disable(slider_S);
    }
    else if (imgtype == IMAGE_INDEXED) {
      jwidget_disable(slider_A);
    }
  }

  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* load window configuration */
  load_window_pos(window, "ColorSelection");

  /* open and run the window */
  jwindow_open(window);
  jwidget_emit_signal(window, JI_SIGNAL_WINDOW_RESIZE);
  jwindow_open_fg(window);

  /* check the killer widget */
  if (jwindow_get_killer(window) == button_ok) {
    /* selected color */
    selected_color = jstrdup(color_viewer_get_color (color_viewer));
  }
  /* cancel or ESC */
  else {
    /* selected color */
    selected_color = NULL;
  }

  /* save window configuration */
  save_window_pos(window, "ColorSelection");

  jwidget_free(window);

  return selected_color;
}

static int sliderRGB_change_signal(JWidget widget, int user_data)
{
  int imgtype = color_viewer_get_imgtype(color_viewer);
  int r = jslider_get_value(slider_R);
  int g = jslider_get_value(slider_G);
  int b = jslider_get_value(slider_B);
  int a = jslider_get_value(slider_A);
  float h, s, v;
  char *color;

  rgb_to_hsv(r, g, b, &h, &s, &v);

  jslider_set_value(slider_H, 255.0 * h / 360.0);
  jslider_set_value(slider_V, 255.0 * v);
  jslider_set_value(slider_S, 255.0 * s);

  color = color_rgb(r, g, b, a);

  if (imgtype == IMAGE_INDEXED) {
    int i = get_color_for_image(imgtype, color);
    palette_editor_select_color(palette_editor, i);
  }
  else if (imgtype == IMAGE_RGB) {
    palette_editor_select_color(palette_editor, -1);
  }

  color_viewer_set_color(color_viewer, color);
  jfree(color);

  return FALSE;
}

static int sliderHSV_change_signal(JWidget widget, int user_data)
{
  int imgtype = color_viewer_get_imgtype(color_viewer);
  int h = jslider_get_value(slider_H);
  int s = jslider_get_value(slider_S);
  int v = jslider_get_value(slider_V);
  int a = jslider_get_value(slider_A);
  int r, g, b;
  char *color;

  hsv_to_rgb(360.0 * h / 255.0, s / 255.0, v / 255.0, &r, &g, &b);

  jslider_set_value(slider_R, r);
  jslider_set_value(slider_G, g);
  jslider_set_value(slider_B, b);

  color = color_rgb(r, g, b, a);

  if (imgtype == IMAGE_INDEXED) {
    int i = get_color_for_image(imgtype, color);
    palette_editor_select_color(palette_editor, i);
  }
  else if (imgtype == IMAGE_RGB) {
    palette_editor_select_color(palette_editor, -1);
  }

  color_viewer_set_color(color_viewer, color);
  jfree(color);

  return FALSE;
}

static int sliderA_change_signal(JWidget widget, int user_data)
{
  const char *input_color = color_viewer_get_color(color_viewer);
  int imgtype = color_viewer_get_imgtype(color_viewer);
  int r = color_get_red(imgtype, input_color);
  int g = color_get_green(imgtype, input_color);
  int b = color_get_blue(imgtype, input_color);
  int a = jslider_get_value(slider_A);
  int type = color_type(input_color);
  char *color = NULL;

  switch (type) {
    case COLOR_TYPE_MASK:
      color = color_rgb(jslider_get_value(slider_R),
			jslider_get_value(slider_G),
			jslider_get_value(slider_B), a);
      break;
    case COLOR_TYPE_RGB:
    case COLOR_TYPE_INDEX:
      color = color_rgb(r, g, b, a);
      break;
    case COLOR_TYPE_GRAY:
      {
	float h, s, v;
	rgb_to_hsv(r, g, b, &h, &s, &v);
	color = color_gray(v * 255.0, a);
      }
      break;
  }

  if (color) {
    color_viewer_set_color(color_viewer, color);
    jfree(color);
  }

  return FALSE;
}

static int color_viewer_select_signal(JWidget widget, int user_data)
{
  int imgtype = color_viewer_get_imgtype(color_viewer);
  char *color = color_mask();
  int r = color_get_red(imgtype, color);
  int g = color_get_green(imgtype, color);
  int b = color_get_blue(imgtype, color);
  float h, s, v;

  rgb_to_hsv(r, g, b, &h, &s, &v);

  color_viewer_set_color(color_viewer, color);

  jslider_set_value(slider_R, r);
  jslider_set_value(slider_G, g);
  jslider_set_value(slider_B, b);
  jslider_set_value(slider_A, color_get_alpha(imgtype, color));
  jslider_set_value(slider_H, 255.0 * h / 360.0);
  jslider_set_value(slider_V, 255.0 * v);
  jslider_set_value(slider_S, 255.0 * s);

  if (imgtype == IMAGE_INDEXED) {
    int i = get_color_for_image(imgtype, color);
    palette_editor_select_color(palette_editor, i);
  }
  else if (imgtype == IMAGE_RGB) {
    palette_editor_select_color(palette_editor, -1);
  }

  jfree(color);
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
  jslider_set_value(slider_A, color_get_alpha(imgtype, color));
  jslider_set_value(slider_H, 255.0 * h / 360.0);
  jslider_set_value(slider_V, 255.0 * v);
  jslider_set_value(slider_S, 255.0 * s);

  jfree(color);
  return FALSE;
}

static int window_resize_signal(JWidget widget, int user_data)
{
  JWidget paledit = (JWidget)user_data;
  JWidget view = jwidget_get_view(paledit);
  JRect vp = jview_get_viewport_position(view);
  int cols, box = 3;

  do {
    box++;
    palette_editor_data(paledit)->boxsize = box;
    cols = (jrect_w(vp)-1) / (box+1);
    palette_editor_set_columns(paledit, cols);
  } while (((jrect_h(vp)-1) / (box+1))*cols > 256);

  box--;
  palette_editor_data(paledit)->boxsize = box;
  cols = (jrect_w(vp)-1) / (box+1);
  palette_editor_set_columns(paledit, cols);

  jwidget_dirty(paledit);
  jrect_free(vp);
  return FALSE;
}
