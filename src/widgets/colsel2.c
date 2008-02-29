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

#include <assert.h>
#include <allegro.h>

#include "jinete/jinete.h"

#include "core/app.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "widgets/colsel2.h"
#include "widgets/paledit.h"
#include "widgets/tabs.h"

enum {
  MODEL_RGB,
  MODEL_HSV,
  MODEL_GRAY,
  MODEL_MASK
};

typedef struct Model
{
  const char *text;
  int model;
  int color_type;
  JWidget (*create)(JWidget colsel);
} Model;

typedef struct ColorSelector
{
  color_t color;
} ColorSelector;

static JWidget create_rgb_container(JWidget colsel);
static JWidget create_hsv_container(JWidget colsel);
static JWidget create_gray_container(JWidget colsel);
static JWidget create_mask_container(JWidget colsel);

static int colorselector_type(void);
static ColorSelector *colorselector_data(JWidget widget);
static bool colorselector_msg_proc(JWidget widget, JMessage msg);
static void select_tab_callback(JWidget tabs, void *data);

static int slider_change_signal(JWidget widget, int user_data);

static Model models[] = {
  { "RGB",	MODEL_RGB,	COLOR_TYPE_RGB,		create_rgb_container },
  { "HSV",	MODEL_HSV,	COLOR_TYPE_RGB,		create_hsv_container },
  { "Gray",	MODEL_GRAY,	COLOR_TYPE_GRAY,	create_gray_container },
  { "Mask",	MODEL_MASK,	COLOR_TYPE_MASK,	create_mask_container },
  { NULL,	0,		0,			NULL }
};

JWidget colorselector_new(void)
{
  int guiscale = GUISCALE;
  JWidget window = jtooltip_window_new("");
  JWidget vbox = jbox_new(JI_VERTICAL);
  JWidget grid = jgrid_new(5, FALSE);
  JWidget tabs = tabs_new(select_tab_callback);
  JWidget pal = palette_editor_new(current_palette, FALSE, 4);
  JWidget child;
  ColorSelector *colorselector = jnew(ColorSelector, 1);
  Model *m;

  jwidget_set_bg_color(tabs, window->bg_color);
  vbox->child_spacing = 0;

  grid->border_width.t = 3;
  jwidget_set_min_size(grid, 200*guiscale, 0);

  jwidget_set_name(tabs, "tabs");

  for (m=models; m->text!=NULL; ++m) {
    tabs_append_tab(tabs, _(m->text), (void *)m);

    child = (*m->create)(window);
    jwidget_set_name(child, m->text);
    jgrid_add_child(grid, child, 1, 1, JI_HORIZONTAL | JI_TOP);
  }

  jgrid_add_child(grid, pal, 1, 1, JI_RIGHT | JI_TOP);
  jwidget_expansive(grid, TRUE);

  jwidget_add_child(vbox, tabs);
  jwidget_add_child(vbox, grid);
  jwidget_add_child(window, vbox);

  jwidget_add_hook(window,
		   colorselector_type(),
		   colorselector_msg_proc, colorselector);

  return window;
}

void colorselector_set_color(JWidget widget, color_t color)
{
  ColorSelector *colorselector = colorselector_data(widget);
  JWidget tabs = jwidget_find_name(widget, "tabs");
  int imgtype = app_get_current_image_type();
  Model *m = NULL;
  JWidget rgb_rslider = jwidget_find_name(widget, "rgb_r");
  JWidget rgb_gslider = jwidget_find_name(widget, "rgb_g");
  JWidget rgb_bslider = jwidget_find_name(widget, "rgb_b");
  JWidget rgb_aslider = jwidget_find_name(widget, "rgb_a");
  JWidget hsv_hslider = jwidget_find_name(widget, "hsv_h");
  JWidget hsv_sslider = jwidget_find_name(widget, "hsv_s");
  JWidget hsv_vslider = jwidget_find_name(widget, "hsv_v");
  JWidget hsv_aslider = jwidget_find_name(widget, "hsv_a");
  JWidget gray_vslider = jwidget_find_name(widget, "gray_v");
  JWidget gray_aslider = jwidget_find_name(widget, "gray_a");

  colorselector->color = color;

  jslider_set_value(rgb_rslider, color_get_red(imgtype, color));
  jslider_set_value(rgb_gslider, color_get_green(imgtype, color));
  jslider_set_value(rgb_bslider, color_get_blue(imgtype, color));
  jslider_set_value(rgb_aslider, color_get_alpha(imgtype, color));
  jslider_set_value(hsv_hslider, color_get_hue(imgtype, color));
  jslider_set_value(hsv_sslider, color_get_saturation(imgtype, color));
  jslider_set_value(hsv_vslider, color_get_value(imgtype, color));
  jslider_set_value(hsv_aslider, color_get_alpha(imgtype, color));
  jslider_set_value(gray_vslider, color_get_value(imgtype, color));
  jslider_set_value(gray_aslider, color_get_alpha(imgtype, color));
  
  switch (color_type(color)) {
    case COLOR_TYPE_MASK:
      m = models+MODEL_MASK;
      break;
    case COLOR_TYPE_RGB:
    case COLOR_TYPE_INDEX:
      m = models+MODEL_RGB;
      break;
    case COLOR_TYPE_HSV:
      m = models+MODEL_HSV;
      break;
    case COLOR_TYPE_GRAY:
      m = models+MODEL_GRAY;
      break;
    default:
      assert(FALSE);
  }

  tabs_select_tab(tabs, m);
  select_tab_callback(tabs, m);
}

color_t colorselector_get_color(JWidget widget)
{
  ColorSelector *colorselector = colorselector_data(widget);

  return colorselector->color;
}

static JWidget create_rgb_container(JWidget colsel)
{
  JWidget grid = jgrid_new(2, FALSE);
  JWidget rlabel = jlabel_new("R");
  JWidget glabel = jlabel_new("G");
  JWidget blabel = jlabel_new("B");
  JWidget alabel = jlabel_new("A");
  JWidget rslider = jslider_new(0, 255, 0);
  JWidget gslider = jslider_new(0, 255, 0);
  JWidget bslider = jslider_new(0, 255, 0);
  JWidget aslider = jslider_new(0, 255, 0);
  jgrid_add_child(grid, rlabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, rslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, glabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, gslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, blabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, bslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, alabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, aslider, 1, 1, JI_HORIZONTAL);

  jwidget_set_name(rslider, "rgb_r");
  jwidget_set_name(gslider, "rgb_g");
  jwidget_set_name(bslider, "rgb_b");
  jwidget_set_name(aslider, "rgb_a");

  HOOK(rslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_signal, 0);
  HOOK(gslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_signal, 0);
  HOOK(bslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_signal, 0);
  HOOK(aslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_signal, 0);

  return grid;
}

static JWidget create_hsv_container(JWidget colsel)
{
  JWidget grid = jgrid_new(2, FALSE);
  JWidget hlabel = jlabel_new("H");
  JWidget slabel = jlabel_new("S");
  JWidget vlabel = jlabel_new("V");
  JWidget alabel = jlabel_new("A");
  JWidget hslider = jslider_new(0, 255, 0);
  JWidget sslider = jslider_new(0, 255, 0);
  JWidget vslider = jslider_new(0, 255, 0);
  JWidget aslider = jslider_new(0, 255, 0);
  jgrid_add_child(grid, hlabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, hslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, slabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, sslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, vlabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, vslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, alabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, aslider, 1, 1, JI_HORIZONTAL);

  jwidget_set_name(hslider, "hsv_h");
  jwidget_set_name(sslider, "hsv_s");
  jwidget_set_name(vslider, "hsv_v");
  jwidget_set_name(aslider, "hsv_a");

  HOOK(hslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_signal, 0);
  HOOK(sslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_signal, 0);
  HOOK(vslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_signal, 0);
  HOOK(aslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_signal, 0);

  return grid;
}

static JWidget create_gray_container(JWidget colsel)
{
  JWidget grid = jgrid_new(2, FALSE);
  JWidget klabel = jlabel_new("V");
  JWidget alabel = jlabel_new("A");
  JWidget vslider = jslider_new(0, 255, 0);
  JWidget aslider = jslider_new(0, 255, 0);
  jgrid_add_child(grid, klabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, vslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, alabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, aslider, 1, 1, JI_HORIZONTAL);

  jwidget_set_name(vslider, "gray_v");
  jwidget_set_name(aslider, "gray_a");

  HOOK(vslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_signal, 0);
  HOOK(aslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_signal, 0);

  return grid;
}

static JWidget create_mask_container(JWidget colsel)
{
  return jlabel_new("M");
}

static int colorselector_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static ColorSelector *colorselector_data(JWidget widget)
{
  return (ColorSelector *)jwidget_get_data(widget,
					   colorselector_type());
}

static bool colorselector_msg_proc(JWidget widget, JMessage msg)
{
  ColorSelector *colorselector = colorselector_data(widget);

  switch (msg->type) {

    case JM_DESTROY:
      jfree(colorselector);
      break;

  }

  return FALSE;
}

static void select_tab_callback(JWidget tabs, void *data)
{
  JWidget window = jwidget_get_window(tabs);
  JWidget child;
  Model *selected_model = (Model *)data;
  Model *m;

  for (m=models; m->text!=NULL; ++m) {
    child = jwidget_find_name(window, m->text);

    if (m == selected_model)
      jwidget_show(child);
    else
      jwidget_hide(child);
  }

  jwidget_relayout(window);
}

static int slider_change_signal(JWidget widget, int user_data)
{
  ColorSelector *colorselector;
  color_t color;
  JWidget tabs;
  Model *m;

  widget = jwidget_get_window(widget);
  tabs = jwidget_find_name(widget, "tabs");
  colorselector = colorselector_data(widget);
  m = tabs_get_selected_tab(tabs);
  color = colorselector->color;
  
  switch (m->model) {
    case MODEL_RGB:
      {
	JWidget rslider = jwidget_find_name(widget, "rgb_r");
	JWidget gslider = jwidget_find_name(widget, "rgb_g");
	JWidget bslider = jwidget_find_name(widget, "rgb_b");
	JWidget aslider = jwidget_find_name(widget, "rgb_a");
	int r = jslider_get_value(rslider);
	int g = jslider_get_value(gslider);
	int b = jslider_get_value(bslider);
	int a = jslider_get_value(aslider);
	color = color_rgb(r, g, b, a);
      }
      break;
    case MODEL_HSV:
      {
	JWidget hslider = jwidget_find_name(widget, "hsv_h");
	JWidget sslider = jwidget_find_name(widget, "hsv_s");
	JWidget vslider = jwidget_find_name(widget, "hsv_v");
	JWidget aslider = jwidget_find_name(widget, "hsv_a");
	int h = jslider_get_value(hslider);
	int s = jslider_get_value(sslider);
	int v = jslider_get_value(vslider);
	int a = jslider_get_value(aslider);
	color = color_hsv(h, s, v, a);
      }
      break;
    case MODEL_GRAY:
      {
	JWidget vslider = jwidget_find_name(widget, "gray_v");
	JWidget aslider = jwidget_find_name(widget, "gray_a");
	int v = jslider_get_value(vslider);
	int a = jslider_get_value(aslider);
	color = color_gray(v, a);
      }
      break;
  }

  colorselector_set_color(widget, color);

  jwidget_emit_signal(widget, SIGNAL_COLORSELECTOR_COLOR_CHANGED);
  return 0;
}

