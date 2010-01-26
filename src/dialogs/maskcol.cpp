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

#include "jinete/jbox.h"
#include "jinete/jbutton.h"
#include "jinete/jhook.h"
#include "jinete/jlabel.h"
#include "jinete/jslider.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

#include "app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "core/core.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "util/misc.h"
#include "widgets/colbar.h"
#include "widgets/colbut.h"

static JWidget button_color, slider_fuzziness, check_preview;

static void button_1_command(JWidget widget);
static void button_2_command(JWidget widget);
static bool color_change_hook(JWidget widget, void *data);
static bool slider_change_hook(JWidget widget, void *data);
static bool preview_change_hook(JWidget widget, void *data);

static Mask *gen_mask(Sprite* sprite);
static void mask_preview(Sprite* sprite);

void dialogs_mask_color(Sprite* sprite)
{
  JWidget box1, box2, box3, box4;
  JWidget label_color, button_1, button_2;
  JWidget label_fuzziness;
  JWidget button_ok, button_cancel;
  Image *image;

  if (!is_interactive () || !sprite)
    return;

  image = GetImage(sprite);
  if (!image)
    return;

  FramePtr window(new Frame(false, _("Mask by Color")));
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL);
  box3 = jbox_new(JI_HORIZONTAL);
  box4 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  label_color = jlabel_new(_("Color:"));
  button_color = colorbutton_new
   (get_config_color("MaskColor", "Color",
		     colorbar_get_fg_color(app_get_colorbar())),
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

  button_1->user_data[1] = sprite;
  button_2->user_data[1] = sprite;

  jbutton_add_command(button_1, button_1_command);
  jbutton_add_command(button_2, button_2_command);

  HOOK(button_color, SIGNAL_COLORBUTTON_CHANGE, color_change_hook, sprite);
  HOOK(slider_fuzziness, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, sprite);
  HOOK(check_preview, JI_SIGNAL_CHECK_CHANGE, preview_change_hook, sprite);

  jwidget_magnetic(button_ok, TRUE);
  jwidget_expansive(button_color, TRUE);
  jwidget_expansive(slider_fuzziness, TRUE);
  jwidget_expansive(box2, TRUE);

  jwidget_add_child(window, box1);
  jwidget_add_children(box1, box2, box3, check_preview, box4, NULL);
  jwidget_add_children(box2, label_color, button_color, button_1, button_2, NULL);
  jwidget_add_children(box3, label_fuzziness, slider_fuzziness, NULL);
  jwidget_add_children(box4, button_ok, button_cancel, NULL);

  /* default position */
  window->remap_window();
  window->center_window();

  /* mask first preview */
  mask_preview(sprite);

  /* load window configuration */
  load_window_pos(window, "MaskColor");

  /* open the window */
  window->open_window_fg();

  if (window->get_killer() == button_ok) {
    Mask *mask;

    /* undo */
    if (undo_is_enabled(sprite->undo)) {
      undo_set_label(sprite->undo, "Mask by Color");
      undo_set_mask(sprite->undo, sprite);
    }

    /* change the mask */
    mask = gen_mask(sprite);
    sprite_set_mask(sprite, mask);
    mask_free(mask);

    set_config_color("MaskColor", "Color",
		     colorbutton_get_color(button_color));

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
}

static void button_1_command(JWidget widget)
{
  colorbutton_set_color(button_color,
			colorbar_get_fg_color(app_get_colorbar()));
  mask_preview((Sprite*)widget->user_data[1]);
}

static void button_2_command(JWidget widget)
{
  colorbutton_set_color(button_color,
			colorbar_get_bg_color(app_get_colorbar()));
  mask_preview((Sprite*)widget->user_data[1]);
}

static bool color_change_hook(JWidget widget, void *data)
{
  mask_preview((Sprite*)data);
  return FALSE;
}

static bool slider_change_hook(JWidget widget, void *data)
{
  mask_preview((Sprite*)data);
  return FALSE;
}

static bool preview_change_hook(JWidget widget, void *data)
{
  mask_preview((Sprite*)data);
  return FALSE;
}

static Mask *gen_mask(Sprite* sprite)
{
  int xpos, ypos, color, fuzziness;

  Image* image = GetImage2(sprite, &xpos, &ypos, NULL);

  color = get_color_for_image(sprite->imgtype,
			      colorbutton_get_color(button_color));
  fuzziness = jslider_get_value(slider_fuzziness);

  Mask* mask = mask_new();
  mask_by_color(mask, image, color, fuzziness);
  mask_move(mask, xpos, ypos);

  return mask;
}

static void mask_preview(Sprite* sprite)
{
  if (jwidget_is_selected(check_preview)) {
    Mask *mask = gen_mask(sprite);
    Mask *old_mask = sprite->mask;

    sprite->mask = mask;

    sprite_generate_mask_boundaries(sprite);
    update_screen_for_sprite(sprite);

    sprite->mask = old_mask;
    mask_free(mask);
  }
}
