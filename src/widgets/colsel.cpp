/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "app.h"
#include "core/color.h"
#include "modules/gui.h"
#include "modules/gfx.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "widgets/colsel.h"
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
  JWidget (*create)();
} Model;

typedef struct ColorSelector
{
  color_t color;
  bool palette_locked;
  bool editable_palette;
} ColorSelector;

static JWidget create_rgb_container();
static JWidget create_hsv_container();
static JWidget create_gray_container();
static JWidget create_mask_container();

static int colorselector_type();
static ColorSelector* colorselector_data(JWidget widget);
static bool colorselector_msg_proc(JWidget widget, JMessage msg);
static void colorselector_update_lock_button(JWidget widget);
static void colorselector_set_color2(JWidget widget, color_t color,
				     bool update_index_entry,
				     bool select_index_entry,
				     Model* exclude_this_model);
static void colorselector_set_paledit_index(JWidget widget, int index,
					    bool select_index_entry);

static void select_tab_callback(JWidget tabs, void* data, int button);
static bool slider_change_hook(JWidget widget, void* data);
static bool button_mask_select_hook(JWidget widget, void* data);
static bool paledit_change_hook(JWidget widget, void* data);
static bool lock_button_select_hook(JWidget widget, void* data);

static Model models[] = {
  { "RGB",	MODEL_RGB,	COLOR_TYPE_RGB,		create_rgb_container },
  { "HSV",	MODEL_HSV,	COLOR_TYPE_RGB,		create_hsv_container },
  { "Gray",	MODEL_GRAY,	COLOR_TYPE_GRAY,	create_gray_container },
  { "Mask",	MODEL_MASK,	COLOR_TYPE_MASK,	create_mask_container },
  { NULL,	0,		0,			NULL }
};

Frame* colorselector_new(bool editable_palette)
{
  Frame* window = new TipWindow("");
  JWidget grid1 = jgrid_new(3, false);
  JWidget grid2 = jgrid_new(5, false);
  JWidget tabs = tabs_new(select_tab_callback);
  PalEdit* pal = new PalEdit(false);
  JWidget idx = jlabel_new("None");
  JWidget lock = jbutton_new("");
  JWidget child;
  ColorSelector* colorselector = jnew(ColorSelector, 1);
  Model* m;

  pal->setName("pal");
  idx->setName("idx");
  lock->setName("lock");
  tabs->setName("tabs");
  grid2->setName("grid2");

  /* color selector */
  colorselector->color = color_mask();
  colorselector->palette_locked = true;
  colorselector->editable_palette = editable_palette;

  /* palette */
  jwidget_add_tooltip_text(pal, _("Use SHIFT or CTRL to select ranges"));

  /* lock button */
  add_gfxicon_to_button(lock, GFX_BOX_LOCK, JI_CENTER | JI_MIDDLE);

  /* tabs */
  tabs->setBgColor(window->getBgColor());

  /* data for a better layout */
  grid1->child_spacing = 0;
  grid2->border_width.t = 3 * jguiscale();
  jwidget_expansive(grid2, true);

  /* append a tab for each color-model */
  for (m=models; m->text!=NULL; ++m) {
    tabs_append_tab(tabs, _(m->text), (void*)m);

    child = (*m->create)();
    child->setName(m->text);
    jgrid_add_child(grid2, child, 1, 1, JI_HORIZONTAL | JI_TOP);
  }

  /* add children */
  jgrid_add_child(grid2, pal, 1, 1, JI_RIGHT | JI_TOP);
  jgrid_add_child(grid1, tabs, 1, 1, JI_HORIZONTAL | JI_BOTTOM);
  jgrid_add_child(grid1, idx, 1, 1, JI_RIGHT | JI_BOTTOM);
  jgrid_add_child(grid1, lock, 1, 1, JI_RIGHT | JI_BOTTOM);
  jgrid_add_child(grid1, grid2, 3, 1, JI_HORIZONTAL | JI_VERTICAL);
  jwidget_add_child(window, grid1);

  /* hooks */
  jwidget_add_hook(window,
		   colorselector_type(),
		   colorselector_msg_proc, colorselector);

  HOOK(pal, SIGNAL_PALETTE_EDITOR_CHANGE, paledit_change_hook, 0);
  HOOK(lock, JI_SIGNAL_BUTTON_SELECT, lock_button_select_hook, 0);

  /* update the lock button */
  colorselector_update_lock_button(window);

  jwidget_init_theme(window);
  return window;
}

void colorselector_set_color(JWidget widget, color_t color)
{
  colorselector_set_color2(widget, color, true, true, NULL);
}

color_t colorselector_get_color(JWidget widget)
{
  ColorSelector* colorselector = colorselector_data(widget);

  return colorselector->color;
}

JWidget colorselector_get_paledit(JWidget widget)
{
  return jwidget_find_name(widget, "pal");
}

static JWidget create_rgb_container()
{
  JWidget grid = jgrid_new(2, false);
  JWidget rlabel = jlabel_new("R");
  JWidget glabel = jlabel_new("G");
  JWidget blabel = jlabel_new("B");
  JWidget rslider = jslider_new(0, 255, 0);
  JWidget gslider = jslider_new(0, 255, 0);
  JWidget bslider = jslider_new(0, 255, 0);
  jgrid_add_child(grid, rlabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, rslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, glabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, gslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, blabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, bslider, 1, 1, JI_HORIZONTAL);

  rslider->setName("rgb_r");
  gslider->setName("rgb_g");
  bslider->setName("rgb_b");

  HOOK(rslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);
  HOOK(gslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);
  HOOK(bslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);

  return grid;
}

static JWidget create_hsv_container()
{
  JWidget grid = jgrid_new(2, false);
  JWidget hlabel = jlabel_new("H");
  JWidget slabel = jlabel_new("S");
  JWidget vlabel = jlabel_new("V");
  JWidget hslider = jslider_new(0, 255, 0);
  JWidget sslider = jslider_new(0, 255, 0);
  JWidget vslider = jslider_new(0, 255, 0);
  jgrid_add_child(grid, hlabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, hslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, slabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, sslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, vlabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, vslider, 1, 1, JI_HORIZONTAL);

  hslider->setName("hsv_h");
  sslider->setName("hsv_s");
  vslider->setName("hsv_v");

  HOOK(hslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);
  HOOK(sslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);
  HOOK(vslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);

  return grid;
}

static JWidget create_gray_container()
{
  JWidget grid = jgrid_new(2, false);
  JWidget klabel = jlabel_new("V");
  JWidget vslider = jslider_new(0, 255, 0);
  jgrid_add_child(grid, klabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, vslider, 1, 1, JI_HORIZONTAL);

  vslider->setName("gray_v");

  HOOK(vslider, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);

  return grid;
}

static JWidget create_mask_container()
{
  JWidget button = jbutton_new("Mask Color");
  HOOK(button, JI_SIGNAL_BUTTON_SELECT, button_mask_select_hook, 0);
  return button;
}

static int colorselector_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static ColorSelector* colorselector_data(JWidget widget)
{
  return reinterpret_cast<ColorSelector*>
    (jwidget_get_data(widget, colorselector_type()));
}

static bool colorselector_msg_proc(JWidget widget, JMessage msg)
{
  ColorSelector* colorselector = colorselector_data(widget);

  switch (msg->type) {

    case JM_DESTROY:
      jfree(colorselector);
      break;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_INIT_THEME) {
	Widget* idx = widget->findChild("idx");
	PalEdit* pal = static_cast<PalEdit*>(widget->findChild("pal"));
	Widget* grid2 = widget->findChild("grid2");
	int idxlen = ji_font_text_len(idx->getFont(), "Index=888");

	jwidget_set_min_size(idx, idxlen, 0);
	pal->setBoxSize(4*jguiscale());
	jwidget_set_min_size(grid2, 200*jguiscale(), 0);
      }
      break;

  }

  return false;
}

static void colorselector_update_lock_button(JWidget widget)
{
  ColorSelector* colorselector = colorselector_data(widget);
  JWidget lock = jwidget_find_name(widget, "lock");

  if (colorselector->palette_locked) {
    set_gfxicon_in_button(lock, GFX_BOX_LOCK);
    jwidget_add_tooltip_text(lock, _("Press here to edit the palette"));
  }
  else {
    set_gfxicon_in_button(lock, GFX_BOX_UNLOCK);
    jwidget_add_tooltip_text(lock, _("Press here to lock the palette"));
  }
}

static void colorselector_set_color2(JWidget widget, color_t color,
				     bool update_index_entry,
				     bool select_index_entry,
				     Model* exclude_this_model)
{
  ColorSelector* colorselector = colorselector_data(widget);
  JWidget tabs = jwidget_find_name(widget, "tabs");
  int imgtype = app_get_current_image_type();
  Model* m = reinterpret_cast<Model*>(tabs_get_selected_tab(tabs));
  JWidget rgb_rslider = jwidget_find_name(widget, "rgb_r");
  JWidget rgb_gslider = jwidget_find_name(widget, "rgb_g");
  JWidget rgb_bslider = jwidget_find_name(widget, "rgb_b");
  JWidget hsv_hslider = jwidget_find_name(widget, "hsv_h");
  JWidget hsv_sslider = jwidget_find_name(widget, "hsv_s");
  JWidget hsv_vslider = jwidget_find_name(widget, "hsv_v");
  JWidget gray_vslider = jwidget_find_name(widget, "gray_v");

  colorselector->color = color;

  if (exclude_this_model != models+MODEL_RGB) {
    jslider_set_value(rgb_rslider, color_get_red(color));
    jslider_set_value(rgb_gslider, color_get_green(color));
    jslider_set_value(rgb_bslider, color_get_blue(color));
  }
  if (exclude_this_model != models+MODEL_HSV) {
    jslider_set_value(hsv_hslider, color_get_hue(color));
    jslider_set_value(hsv_sslider, color_get_saturation(color));
    jslider_set_value(hsv_vslider, color_get_value(color));
  }
  if (exclude_this_model != models+MODEL_GRAY) {
    jslider_set_value(gray_vslider, color_get_value(color));
  }
  
  switch (color_type(color)) {
    case COLOR_TYPE_MASK:
      m = models+MODEL_MASK;
      break;
    case COLOR_TYPE_RGB:
      m = models+MODEL_RGB;
      break;
    case COLOR_TYPE_INDEX:
      if (m != models+MODEL_RGB &&
	  m != models+MODEL_HSV) {
	m = models+MODEL_RGB;
      }
      break;
    case COLOR_TYPE_HSV:
      m = models+MODEL_HSV;
      break;
    case COLOR_TYPE_GRAY:
      m = models+MODEL_GRAY;
      break;
    default:
      assert(false);
  }

  tabs_select_tab(tabs, m);
  select_tab_callback(tabs, m, 1);

  if (update_index_entry) {
    switch (color_type(color)) {
      case COLOR_TYPE_INDEX:
	colorselector_set_paledit_index(widget, color_get_index(color), select_index_entry);
	break;
      case COLOR_TYPE_MASK:
	colorselector_set_paledit_index(widget, 0, true);
	break;
      default: {
	int r = color_get_red  (color);
	int g = color_get_green(color);
	int b = color_get_blue (color);
	int i = get_current_palette()->findBestfit(r, g, b);
	if (i >= 0 && i < 256)
	  colorselector_set_paledit_index(widget, i, true);
	else
	  colorselector_set_paledit_index(widget, -1, true);
	break;
      }
    }
  }
}

static void colorselector_set_paledit_index(JWidget widget, int index, bool select_index_entry)
{
  ColorSelector* colorselector = colorselector_data(widget);
  PalEdit* pal = static_cast<PalEdit*>(widget->findChild("pal"));
  Widget* idx = widget->findChild("idx");
  Widget* lock = widget->findChild("lock");
  char buf[256];

  if (index >= 0) {
    if (select_index_entry)
      pal->selectColor(index);

    sprintf(buf, "Index=%d", index);

    if (colorselector->editable_palette)
      jwidget_enable(lock);
    else
      jwidget_disable(lock);
  }
  else {
    if (select_index_entry)
      pal->selectRange(-1, -1, PALETTE_EDITOR_RANGE_NONE);

    sprintf(buf, "None");

    jwidget_disable(lock);
  }

  idx->setText(buf);
}

static void select_tab_callback(JWidget tabs, void* data, int button)
{
  Frame* window = static_cast<Frame*>(tabs->getRoot());
  Model* selected_model = (Model*)data;
  JWidget child;
  Model* m;
  bool something_change = false;

  for (m=models; m->text!=NULL; ++m) {
    child = jwidget_find_name(window, m->text);

    if (m == selected_model) {
      if (child->flags & JI_HIDDEN) {
	jwidget_show(child);
	something_change = true;
      }
    }
    else {
      if (!(child->flags & JI_HIDDEN)) {
	jwidget_hide(child);
	something_change = true;
      }
    }
  }

  if (something_change)
    jwidget_relayout(window);
}

static bool slider_change_hook(JWidget widget, void* data)
{
  Frame* window = static_cast<Frame*>(widget->getRoot());
  ColorSelector* colorselector = colorselector_data(window);
  JWidget tabs = jwidget_find_name(window, "tabs");
  PalEdit* pal = static_cast<PalEdit*>(window->findChild("pal"));
  Model* m = reinterpret_cast<Model*>(tabs_get_selected_tab(tabs));
  color_t color = colorselector->color;
  int i, r, g, b;
  
  switch (m->model) {
    case MODEL_RGB: {
      JWidget rslider = jwidget_find_name(window, "rgb_r");
      JWidget gslider = jwidget_find_name(window, "rgb_g");
      JWidget bslider = jwidget_find_name(window, "rgb_b");
      int r = jslider_get_value(rslider);
      int g = jslider_get_value(gslider);
      int b = jslider_get_value(bslider);
      color = color_rgb(r, g, b);
      break;
    }
    case MODEL_HSV: {
      JWidget hslider = jwidget_find_name(window, "hsv_h");
      JWidget sslider = jwidget_find_name(window, "hsv_s");
      JWidget vslider = jwidget_find_name(window, "hsv_v");
      int h = jslider_get_value(hslider);
      int s = jslider_get_value(sslider);
      int v = jslider_get_value(vslider);
      color = color_hsv(h, s, v);
      break;
    }
    case MODEL_GRAY: {
      JWidget vslider = jwidget_find_name(window, "gray_v");
      int v = jslider_get_value(vslider);
      color = color_gray(v);
      break;
    }
  }

  r = color_get_red  (color);
  g = color_get_green(color);
  b = color_get_blue (color);
  
  /* if the palette is locked then we have to search for the closest
     color to the RGB values */
  if (colorselector->palette_locked) {
    i = get_current_palette()->findBestfit(r, g, b);
    if (i >= 0 && i < 256)
      colorselector_set_paledit_index(window, i, true);
  }
  /* the palette is unlocked, we have to modify the select entries */
  else {
    bool array[256];

    pal->getSelectedEntries(array);
    for (i=0; i<256; ++i)
      if (array[i])
	set_current_color(i, r, g, b);

    jwidget_dirty(pal);

    i = pal->get2ndColor();
    if (i >= 0)
      color = color_index(i);
  }

  colorselector_set_color2(window, color, false, false, m);
  jwidget_emit_signal(window, SIGNAL_COLORSELECTOR_COLOR_CHANGED);
  return 0;
}

static bool button_mask_select_hook(JWidget widget, void* data)
{
  Frame* window = static_cast<Frame*>(widget->getRoot());
  colorselector_set_color(window, color_mask());
  jwidget_emit_signal(window, SIGNAL_COLORSELECTOR_COLOR_CHANGED);
  return true;
}

static bool paledit_change_hook(Widget* widget, void* data)
{
  Frame* window = static_cast<Frame*>(widget->getRoot());
  PalEdit* paledit = static_cast<PalEdit*>(widget);
  bool array[256];
  color_t color = colorselector_get_color(window);
  int i;

  paledit->getSelectedEntries(array);
  for (i=0; i<256; ++i)
    if (array[i]) {
      color = color_index(i);
      break;
    }

  colorselector_set_color2(window, color, true, false, NULL);
  jwidget_emit_signal(window, SIGNAL_COLORSELECTOR_COLOR_CHANGED);
  return 0;
}

static bool lock_button_select_hook(JWidget widget, void* data)
{
  Frame* window = static_cast<Frame*>(widget->getRoot());
  ColorSelector* colorselector = colorselector_data(window);

  colorselector->palette_locked = !colorselector->palette_locked;
  colorselector_update_lock_button(window);
  return true;
}
