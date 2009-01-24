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

#include <string.h>

#include "jinete/jbox.h"
#include "jinete/jbutton.h"
#include "jinete/jhook.h"
#include "jinete/jlabel.h"
#include "jinete/jlist.h"
#include "jinete/jlistbox.h"
#include "jinete/jslider.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

#include "commands/commands.h"
#include "commands/fx/effectbg.h"
#include "console/console.h"
#include "core/cfg.h"
#include "core/color.h"
#include "core/core.h"
#include "effect/colcurve.h"
#include "effect/convmatr.h"
#include "effect/effect.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "modules/tools.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/colbut.h"
#include "widgets/curvedit.h"
#include "widgets/preview.h"
#include "widgets/target.h"

static JWidget check_preview, preview;
static JWidget check_tiled;
static JWidget target_button;

static void listbox_fill_convmatg(JWidget listbox);
static void listbox_select_current_convmatr(JWidget listbox);

static bool reload_select_hook(JWidget widget, void *data);
static bool generate_select_hook(JWidget widget, void *data);

static bool list_change_hook(JWidget widget, void *data);
static bool target_change_hook(JWidget widget, void *data);
static bool preview_change_hook(JWidget widget, void *data);
static bool tiled_change_hook(JWidget widget, void *data);
static void make_preview();

static bool cmd_convolution_matrix_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_convolution_matrix_execute(const char *argument)
{
  JWidget window, button_ok;
  JWidget view_convmatr, list_convmatr;
  JWidget box_target;
  JWidget reload, generate;
  Sprite *sprite = current_sprite;
  Image *image;
  Effect *effect;

  image = GetImage(current_sprite);
  if (!image)
    return;

  window = load_widget("convmatr.jid", "convolution_matrix");
  if (!window)
    return;

  if (!get_widgets(window,
		   "preview", &check_preview,
		   "tiled", &check_tiled,
		   "button_ok", &button_ok,
		   "view", &view_convmatr,
		   "target", &box_target,
		   "reload", &reload,
		   "generate", &generate, NULL)) {
    jwidget_free(window);
    return;
  }

  effect = effect_new(sprite, "convolution_matrix");
  if (!effect) {
    console_printf(_("Error creating the effect applicator for this sprite\n"));
    jwidget_free(window);
    return;
  }

  preview = preview_new(effect);
  
  list_convmatr = jlistbox_new();
  listbox_fill_convmatg(list_convmatr);

  target_button = target_button_new(sprite->imgtype, TRUE);
  target_button_set_target(target_button, effect->target);

  if (get_config_bool("ConvolutionMatrix", "Preview", TRUE))
    jwidget_select(check_preview);

  if (get_tiled_mode())
    jwidget_select(check_tiled);

  jview_attach(view_convmatr, list_convmatr);
  jwidget_set_min_size(view_convmatr, 128, 64);

  jwidget_add_child(box_target, target_button);
  jwidget_add_child(window, preview);

  HOOK(list_convmatr, JI_SIGNAL_LISTBOX_CHANGE, list_change_hook, 0);
  HOOK(target_button, SIGNAL_TARGET_BUTTON_CHANGE, target_change_hook, 0);
  HOOK(check_preview, JI_SIGNAL_CHECK_CHANGE, preview_change_hook, 0);
  HOOK(check_tiled, JI_SIGNAL_CHECK_CHANGE, tiled_change_hook, 0);
  HOOK(reload, JI_SIGNAL_BUTTON_SELECT, reload_select_hook, list_convmatr);
  HOOK(generate, JI_SIGNAL_BUTTON_SELECT, generate_select_hook, 0);

  /* TODO enable this someday */
  jwidget_disable(generate);

  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* load window configuration */
  load_window_pos(window, "ConvolutionMatrix");

  /* select default convmatr */
  listbox_select_current_convmatr(list_convmatr);

  /* open the window */
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_ok) {
    effect_apply_to_target_with_progressbar(effect);
  }

  effect_free(effect);

  /* update editors */
  update_screen_for_sprite(sprite);

  /* save window configuration */
  save_window_pos(window, "ConvolutionMatrix");

  jwidget_free(window);
}

static void listbox_fill_convmatg(JWidget listbox)
{
  ConvMatr *convmatr;
  JWidget listitem;
  JLink link;

  JI_LIST_FOR_EACH(get_convmatr_stock(), link) {
    convmatr = reinterpret_cast<ConvMatr *>(link->data);
    listitem = jlistitem_new(convmatr->name);
    listitem->user_data[0] = convmatr;
    jwidget_add_child(listbox, listitem);
  }
}

static void listbox_select_current_convmatr(JWidget listbox)
{
  const char *selected = get_config_string("ConvolutionMatrix",
					   "Selected", "");
  JWidget select_this = reinterpret_cast<JWidget>(jlist_first_data(listbox->children));
  JWidget child = NULL;
  JLink link;

  if (selected && *selected) {
    JI_LIST_FOR_EACH(listbox->children, link) {
      child = reinterpret_cast<JWidget>(link->data);

      if (strcmp(jwidget_get_text(child), selected) == 0) {
	select_this = child;
	break;
      }
    }
  }

  if (select_this) {
    jwidget_select(select_this);
    list_change_hook(listbox, 0);
  }
}

static bool reload_select_hook(JWidget widget, void *data)
{
  JWidget listbox = (JWidget)data;
  JWidget listitem;
  JLink link, next;

  /* clean the list */
  JI_LIST_FOR_EACH_SAFE(listbox->children, link, next) {
    listitem = reinterpret_cast<JWidget>(link->data);
    jwidget_remove_child(listbox, listitem);
    jwidget_free(listitem);
  }

  /* re-load the convolution matrix stock */
  reload_matrices_stock();

  /* re-fill the list */
  listbox_fill_convmatg(listbox);
  listbox_select_current_convmatr(listbox);
  jview_update(jwidget_get_view(listbox));

  return TRUE;			/* do not close */
}

static bool generate_select_hook(JWidget widget, void *data)
{
#if 0
  JWidget window;
  JWidget view_x;
  JWidget view_y;
  JWidget curvedit_x;
  JWidget curvedit_y;
  Curve *curve_x;
  Curve *curve_y;
  JWidget div, div_auto;
  JWidget bias, bias_auto;

  window = load_widget("convmatr.jid", "generate_convolution_matrix");
  if (!window)
    return TRUE;		/* don't close */

  if (!get_widgets(window,
		   "view_x", &view_x,
		   "view_y", &view_y,
		   "div", &div,
		   "bias", &bias,
		   "div_auto", &div_auto,
		   "bias_auto", &bias_auto, NULL)) {
    jwidget_free(window);
    return TRUE;		/* don't close */
  }

  /* curve_x = curve_new(CURVE_SPLINE); */
  /* curve_y = curve_new(CURVE_SPLINE); */
  curve_x = curve_new(CURVE_LINEAR);
  curve_y = curve_new(CURVE_LINEAR);
  curve_add_point(curve_x, curve_point_new(-100, 0));
  curve_add_point(curve_x, curve_point_new(0, +100));
  curve_add_point(curve_x, curve_point_new(+100, 0));
  curve_add_point(curve_y, curve_point_new(-100, 0));
  curve_add_point(curve_y, curve_point_new(0, +100));
  curve_add_point(curve_y, curve_point_new(+100, 0));

  curvedit_x = curve_editor_new(curve_x, -200, -200, 200, 200);
  curvedit_y = curve_editor_new(curve_y, -200, -200, 200, 200);

  jview_attach(view_x, curvedit_x);
  jview_attach(view_y, curvedit_y);

  jwidget_set_min_size(view_x, 64, 64);
  jwidget_set_min_size(view_y, 64, 64);

  /* TODO fix this */
  /* jwidget_get_vtable(div)->request_size = NULL; */
  /* jwidget_get_vtable(bias)->request_size = NULL; */

  jwidget_set_min_size(div, 1, 1);
  jwidget_set_min_size(bias, 1, 1);

  jwindow_open_fg(window);

  /* TODO do something */

  jwidget_free(window);

  curve_free(curve_x);
  curve_free(curve_y);
#endif
  return TRUE;			/* do not close */
}

static bool list_change_hook(JWidget widget, void *data)
{
  JWidget selected = jlistbox_get_selected_child(widget);
  ConvMatr *convmatr = reinterpret_cast<ConvMatr*>(selected->user_data[0]);
  int new_target = convmatr->default_target;

  set_config_string("ConvolutionMatrix", "Selected", convmatr->name);

  set_convmatr(convmatr);

  target_button_set_target(target_button, new_target);
  effect_set_target(preview_get_effect(preview), new_target);

  make_preview();
  return FALSE;
}

static bool target_change_hook(JWidget widget, void *data)
{
  effect_set_target(preview_get_effect(preview),
		    target_button_get_target(widget));
  make_preview();
  return FALSE;
}

static bool preview_change_hook(JWidget widget, void *data)
{
  set_config_bool("ConvolutionMatrix", "Preview",
		  jwidget_is_selected(widget));
  make_preview();
  return FALSE;
}

static bool tiled_change_hook(JWidget widget, void *data)
{
  set_tiled_mode(jwidget_is_selected(widget));
  make_preview();
  return FALSE;
}

static void make_preview()
{
  if (jwidget_is_selected(check_preview))
    preview_restart(preview);
}

Command cmd_convolution_matrix = {
  CMD_CONVOLUTION_MATRIX,
  cmd_convolution_matrix_enabled,
  NULL,
  cmd_convolution_matrix_execute,
  NULL
};
