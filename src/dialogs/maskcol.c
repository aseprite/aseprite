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

#ifndef USE_PRECOMPILED_HEADER

#include "jinete/jbox.h"
#include "jinete/jbutton.h"
#include "jinete/jhook.h"
#include "jinete/jlabel.h"
#include "jinete/jslider.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "modules/color.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "util/misc.h"
#include "widgets/colbar.h"
#include "widgets/colbut.h"

#endif

static JWidget button_color, slider_fuzziness, check_preview;

static void button_1_command(JWidget widget);
static void button_2_command(JWidget widget);
static int color_change_hook(JWidget widget, int user_data);
static int slider_change_hook(JWidget widget, int user_data);
static int preview_change_hook(JWidget widget, int user_data);

static Mask *gen_mask(void);
static void mask_preview(void);

void dialogs_mask_color(void)
{
  JWidget window, box1, box2, box3, box4;
  JWidget label_color, button_1, button_2;
  JWidget label_fuzziness;
  JWidget button_ok, button_cancel;
  Sprite *sprite = current_sprite;
  Image *image;

  if (!is_interactive () || !sprite)
    return;

  image = GetImage(current_sprite);
  if (!image)
    return;

  window = jwindow_new(_("Mask by Color"));
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL);
  box3 = jbox_new(JI_HORIZONTAL);
  box4 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  label_color = jlabel_new(_("Color:"));
  button_color = color_button_new
   (get_config_string("MaskColor", "Color",
		      color_bar_get_color(app_get_color_bar(), 0)),
    sprite->imgtype);
  button_1 = jbutton_new("1");
  button_2 = jbutton_new("2");
  label_fuzziness = jlabel_new(_("Fuzziness:"));
  slider_fuzziness =
    jslider_new(0, 255, get_config_int("MaskColor", "Fuzziness", 0));
  check_preview = jcheck_new(_("&Preview"));
  button_ok = jbutton_new(_("&OK"));
  button_cancel = jbutton_new(_("&Cancel"));

  if (get_config_bool("MaskColor", "Preview", TRUE))
    jwidget_select(check_preview);

  jbutton_add_command(button_1, button_1_command);
  jbutton_add_command(button_2, button_2_command);

  HOOK(button_color, SIGNAL_COLOR_BUTTON_CHANGE, color_change_hook, 0);
  HOOK(slider_fuzziness, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);
  HOOK(check_preview, JI_SIGNAL_CHECK_CHANGE, preview_change_hook, 0);

  jwidget_magnetic(button_ok, TRUE);
  jwidget_expansive(button_color, TRUE);
  jwidget_expansive(slider_fuzziness, TRUE);
  jwidget_expansive(box2, TRUE);

  jwidget_add_child(window, box1);
  jwidget_add_childs(box1, box2, box3, check_preview, box4, NULL);
  jwidget_add_childs(box2, label_color, button_color, button_1, button_2, NULL);
  jwidget_add_childs(box3, label_fuzziness, slider_fuzziness, NULL);
  jwidget_add_childs(box4, button_ok, button_cancel, NULL);

  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* mask first preview */
  mask_preview();

  /* load window configuration */
  load_window_pos(window, "MaskColor");

  /* open the window */
  jwindow_open_fg(window);

  if (jwindow_get_killer (window) == button_ok) {
    Mask *mask;

    /* undo */
    if (undo_is_enabled (sprite->undo))
      undo_set_mask(sprite->undo, sprite);

    /* change the mask */
    mask = gen_mask();
    sprite_set_mask(sprite, mask);
    mask_free(mask);

    set_config_string("MaskColor", "Color",
		      color_button_get_color(button_color));

    set_config_int("MaskColor", "Fuzziness",
		   jslider_get_value(slider_fuzziness));

    set_config_bool("MaskColor", "Preview",
		    jwidget_is_selected(check_preview));
  }

  /* update boundaries and editors */
  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);

  /* save window configuration */
  save_window_pos(window, "MaskColor");

  jwidget_free(window);
}

static void button_1_command(JWidget widget)
{
  color_button_set_color(button_color,
			 color_bar_get_color(app_get_color_bar(), 0));
  mask_preview();
}

static void button_2_command(JWidget widget)
{
  color_button_set_color(button_color,
			 color_bar_get_color(app_get_color_bar(), 1));
  mask_preview();
}

static int color_change_hook(JWidget widget, int user_data)
{
  mask_preview();
  return FALSE;
}

static int slider_change_hook(JWidget widget, int user_data)
{
  mask_preview();
  return FALSE;
}

static int preview_change_hook(JWidget widget, int user_data)
{
  mask_preview();
  return FALSE;
}

static Mask *gen_mask(void)
{
  int xpos, ypos, color, fuzziness;
  const char *color_text;
  Sprite *sprite;
  Image *image;
  Mask *mask;

  sprite = current_sprite;
  image = GetImage2 (sprite, &xpos, &ypos, NULL);

  color_text = color_button_get_color(button_color);
  color = get_color_for_image(sprite->imgtype, color_text);
  fuzziness = jslider_get_value(slider_fuzziness);

  mask = mask_new();
  mask_by_color(mask, image, color, fuzziness);
  mask_move(mask, xpos, ypos);

  return mask;
}

static void mask_preview(void)
{
  if (jwidget_is_selected (check_preview)) {
    Sprite *sprite = current_sprite;
    Mask *mask = gen_mask();
    Mask *old_mask = sprite->mask;

    sprite->mask = mask;

    sprite_generate_mask_boundaries(sprite);
    update_screen_for_sprite(sprite);

    sprite->mask = old_mask;
    mask_free(mask);
  }
}
