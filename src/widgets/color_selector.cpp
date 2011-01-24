/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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
#include <vector>

#include "app.h"
#include "app/color.h"
#include "base/bind.h"
#include "gui/jinete.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "widgets/color_selector.h"
#include "widgets/paledit.h"

enum {
  MODEL_RGB,
  MODEL_HSV,
  MODEL_GRAY,
  MODEL_MASK
};

struct Model
{
  const char *text;
  int model;
  Widget* (*create)();
};

struct ColorSelector
{
  Color color;
  Model* selected_model;
  std::vector<Widget*> model_buttons;
};

static Widget* create_rgb_container();
static Widget* create_hsv_container();
static Widget* create_gray_container();
static Widget* create_mask_container();

static int colorselector_type();
static ColorSelector* colorselector_data(JWidget widget);
static bool colorselector_msg_proc(JWidget widget, JMessage msg);
static void colorselector_set_color2(JWidget widget, const Color& color,
				     bool update_index_entry,
				     bool select_index_entry,
				     Model* exclude_this_model);
static void colorselector_set_paledit_index(JWidget widget, int index,
					    bool select_index_entry);

static bool select_model_hook(Frame* frame, Model* selected_model);
static void slider_change_hook(Slider* widget);
static bool paledit_change_hook(JWidget widget, void* data);

static Model models[] = {
  { "RGB",	MODEL_RGB,	create_rgb_container },
  { "HSV",	MODEL_HSV,	create_hsv_container },
  { "Gray",	MODEL_GRAY,	create_gray_container },
  { "Mask",	MODEL_MASK,	create_mask_container },
  { NULL,	0,		NULL }
};

Frame* colorselector_new()
{
  Frame* window = new PopupFrame(NULL, false);
  Widget* grid1 = jgrid_new(2, false);
  Widget* grid2 = jgrid_new(5, false);
  Box* models_box = new Box(JI_HORIZONTAL);
  PalEdit* pal = new PalEdit(false);
  Label* idx = new Label("None");
  Widget* child;
  ColorSelector* colorselector = new ColorSelector;
  Model* m;

  pal->setName("pal");
  idx->setName("idx");
  grid2->setName("grid2");

  /* color selector */
  colorselector->color = Color::fromMask();
  colorselector->selected_model = &models[0];

  /* palette */
  jwidget_add_tooltip_text(pal, "Use SHIFT or CTRL to select ranges");

  /* data for a better layout */
  grid1->child_spacing = 0;
  grid2->border_width.t = 3 * jguiscale();
  jwidget_expansive(grid2, true);

  jwidget_noborders(models_box);

  // Append one button for each color-model
  for (m=models; m->text!=NULL; ++m) {
    // Create the color-model button to select it
    RadioButton* model_button = new RadioButton(m->text, 1, JI_BUTTON);
    colorselector->model_buttons.push_back(model_button);
    setup_mini_look(model_button);
    model_button->Click.connect(Bind<bool>(&select_model_hook, window, m));
    jwidget_add_child(models_box, model_button);
    
    // Create the color-model container
    child = (*m->create)();
    child->setName(m->text);
    jgrid_add_child(grid2, child, 1, 1, JI_HORIZONTAL | JI_TOP);
  }

  /* add children */
  jgrid_add_child(grid2, pal, 1, 1, JI_RIGHT | JI_TOP);
  jgrid_add_child(grid1, models_box, 1, 1, JI_HORIZONTAL | JI_BOTTOM);
  jgrid_add_child(grid1, idx, 1, 1, JI_RIGHT | JI_BOTTOM);
  jgrid_add_child(grid1, grid2, 2, 1, JI_HORIZONTAL | JI_VERTICAL);
  jwidget_add_child(window, grid1);

  /* hooks */
  jwidget_add_hook(window,
		   colorselector_type(),
		   colorselector_msg_proc, colorselector);

  HOOK(pal, SIGNAL_PALETTE_EDITOR_CHANGE, paledit_change_hook, 0);

  jwidget_init_theme(window);
  return window;
}

void colorselector_set_color(JWidget widget, const Color& color)
{
  colorselector_set_color2(widget, color, true, true, NULL);
}

Color colorselector_get_color(JWidget widget)
{
  ColorSelector* colorselector = colorselector_data(widget);

  return colorselector->color;
}

JWidget colorselector_get_paledit(JWidget widget)
{
  return jwidget_find_name(widget, "pal");
}

static Widget* create_rgb_container()
{
  Widget* grid = jgrid_new(2, false);
  Label* rlabel = new Label("R");
  Label* glabel = new Label("G");
  Label* blabel = new Label("B");
  Slider* rslider = new Slider(0, 255, 0);
  Slider* gslider = new Slider(0, 255, 0);
  Slider* bslider = new Slider(0, 255, 0);
  jgrid_add_child(grid, rlabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, rslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, glabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, gslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, blabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, bslider, 1, 1, JI_HORIZONTAL);

  rslider->setName("rgb_r");
  gslider->setName("rgb_g");
  bslider->setName("rgb_b");

  rslider->Change.connect(Bind<void>(&slider_change_hook, rslider));
  gslider->Change.connect(Bind<void>(&slider_change_hook, gslider));
  bslider->Change.connect(Bind<void>(&slider_change_hook, bslider));

  return grid;
}

static Widget* create_hsv_container()
{
  Widget* grid = jgrid_new(2, false);
  Label* hlabel = new Label("H");
  Label* slabel = new Label("S");
  Label* vlabel = new Label("V");
  Slider* hslider = new Slider(0, 360, 0);
  Slider* sslider = new Slider(0, 100, 0);
  Slider* vslider = new Slider(0, 100, 0);
  jgrid_add_child(grid, hlabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, hslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, slabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, sslider, 1, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, vlabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, vslider, 1, 1, JI_HORIZONTAL);

  hslider->setName("hsv_h");
  sslider->setName("hsv_s");
  vslider->setName("hsv_v");

  hslider->Change.connect(Bind<void>(&slider_change_hook, hslider));
  sslider->Change.connect(Bind<void>(&slider_change_hook, sslider));
  vslider->Change.connect(Bind<void>(&slider_change_hook, vslider));

  return grid;
}

static Widget* create_gray_container()
{
  Widget* grid = jgrid_new(2, false);
  Label* klabel = new Label("V");
  Slider* vslider = new Slider(0, 255, 0);
  jgrid_add_child(grid, klabel, 1, 1, JI_RIGHT);
  jgrid_add_child(grid, vslider, 1, 1, JI_HORIZONTAL);

  vslider->setName("gray_v");

  vslider->Change.connect(Bind<void>(&slider_change_hook, vslider));

  return grid;
}

static Widget* create_mask_container()
{
  return new Label("Mask color selected");
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
      delete colorselector;
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

static void colorselector_set_color2(JWidget widget, const Color& color,
				     bool update_index_entry,
				     bool select_index_entry,
				     Model* exclude_this_model)
{
  ColorSelector* colorselector = colorselector_data(widget);
  Model* m = colorselector->selected_model;
  Slider* rgb_rslider = widget->findChildT<Slider>("rgb_r");
  Slider* rgb_gslider = widget->findChildT<Slider>("rgb_g");
  Slider* rgb_bslider = widget->findChildT<Slider>("rgb_b");
  Slider* hsv_hslider = widget->findChildT<Slider>("hsv_h");
  Slider* hsv_sslider = widget->findChildT<Slider>("hsv_s");
  Slider* hsv_vslider = widget->findChildT<Slider>("hsv_v");
  Slider* gray_vslider = widget->findChildT<Slider>("gray_v");

  colorselector->color = color;

  if (exclude_this_model != models+MODEL_RGB) {
    rgb_rslider->setValue(color.getRed());
    rgb_gslider->setValue(color.getGreen());
    rgb_bslider->setValue(color.getBlue());
  }
  if (exclude_this_model != models+MODEL_HSV) {
    hsv_hslider->setValue(color.getHue());
    hsv_sslider->setValue(color.getSaturation());
    hsv_vslider->setValue(color.getValue());
  }
  if (exclude_this_model != models+MODEL_GRAY) {
    gray_vslider->setValue(color.getGray());
  }
  
  switch (color.getType()) {
    case Color::MaskType:
      m = models+MODEL_MASK;
      break;
    case Color::RgbType:
      m = models+MODEL_RGB;
      break;
    case Color::IndexType:
      if (m != models+MODEL_RGB &&
	  m != models+MODEL_HSV) {
	m = models+MODEL_RGB;
      }
      break;
    case Color::HsvType:
      m = models+MODEL_HSV;
      break;
    case Color::GrayType:
      m = models+MODEL_GRAY;
      break;
    default:
      ASSERT(false);
  }

  // // Select the RGB button
  // jwidget_select(colorselector->rgb_button);
  colorselector->model_buttons[m->model]->setSelected(true);

  // Call the hook
  select_model_hook(dynamic_cast<Frame*>(widget->getRoot()), m);

  if (update_index_entry) {
    switch (color.getType()) {
      case Color::IndexType:
	colorselector_set_paledit_index(widget, color.getIndex(), select_index_entry);
	break;
      case Color::MaskType:
	colorselector_set_paledit_index(widget, 0, true);
	break;
      default: {
	int r = color.getRed();
	int g = color.getGreen();
	int b = color.getBlue();
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
  PalEdit* pal = static_cast<PalEdit*>(widget->findChild("pal"));
  Widget* idx = widget->findChild("idx");
  char buf[256];

  if (index >= 0) {
    if (select_index_entry)
      pal->selectColor(index);

    sprintf(buf, "Index=%d", index);
  }
  else {
    if (select_index_entry)
      pal->selectRange(-1, -1, PALETTE_EDITOR_RANGE_NONE);

    sprintf(buf, "None");
  }

  idx->setText(buf);
}

static bool select_model_hook(Frame* frame, Model* selected_model)
{
  ASSERT(frame != NULL);

  ColorSelector* colorselector = colorselector_data(frame);
  JWidget child;
  Model* m;
  bool something_change = false;

  colorselector->selected_model = selected_model;
  
  for (m=models; m->text!=NULL; ++m) {
    child = jwidget_find_name(frame, m->text);

    if (m == selected_model) {
      if (child->flags & JI_HIDDEN) {
	child->setVisible(true);
	something_change = true;
      }
    }
    else {
      if (!(child->flags & JI_HIDDEN)) {
	child->setVisible(false);
	something_change = true;
      }
    }
  }

  if (something_change) {
    // Select the mask color
    if (selected_model->model == MODEL_MASK) {
      colorselector_set_color2(frame, Color::fromMask(), false, false, NULL);
      jwidget_emit_signal(frame, SIGNAL_COLORSELECTOR_COLOR_CHANGED);
    }

    jwidget_relayout(frame);
  }

  return true;
}

static void slider_change_hook(Slider* widget)
{
  Frame* window = static_cast<Frame*>(widget->getRoot());
  ColorSelector* colorselector = colorselector_data(window);
  Model* m = colorselector->selected_model;
  Color color = colorselector->color;
  int i, r, g, b;
  
  switch (m->model) {
    case MODEL_RGB: {
      Slider* rslider = window->findChildT<Slider>("rgb_r");
      Slider* gslider = window->findChildT<Slider>("rgb_g");
      Slider* bslider = window->findChildT<Slider>("rgb_b");
      int r = rslider->getValue();
      int g = gslider->getValue();
      int b = bslider->getValue();
      color = Color::fromRgb(r, g, b);
      break;
    }
    case MODEL_HSV: {
      Slider* hslider = window->findChildT<Slider>("hsv_h");
      Slider* sslider = window->findChildT<Slider>("hsv_s");
      Slider* vslider = window->findChildT<Slider>("hsv_v");
      int h = hslider->getValue();
      int s = sslider->getValue();
      int v = vslider->getValue();
      color = Color::fromHsv(h, s, v);
      break;
    }
    case MODEL_GRAY: {
      Slider* vslider = window->findChildT<Slider>("gray_v");
      int v = vslider->getValue();
      color = Color::fromGray(v);
      break;
    }
  }

  r = color.getRed();
  g = color.getGreen();
  b = color.getBlue();
  
  // Search for the closest color to the RGB values
  i = get_current_palette()->findBestfit(r, g, b);
  if (i >= 0 && i < 256)
    colorselector_set_paledit_index(window, i, true);

  colorselector_set_color2(window, color, false, false, m);
  jwidget_emit_signal(window, SIGNAL_COLORSELECTOR_COLOR_CHANGED);
}

static bool paledit_change_hook(Widget* widget, void* data)
{
  Frame* window = static_cast<Frame*>(widget->getRoot());
  PalEdit* paledit = static_cast<PalEdit*>(widget);
  bool array[256];
  Color color = colorselector_get_color(window);
  int i;

  paledit->getSelectedEntries(array);
  for (i=0; i<256; ++i)
    if (array[i]) {
      color = Color::fromIndex(i);
      break;
    }

  colorselector_set_color2(window, color, true, false, NULL);
  jwidget_emit_signal(window, SIGNAL_COLORSELECTOR_COLOR_CHANGED);
  return 0;
}
