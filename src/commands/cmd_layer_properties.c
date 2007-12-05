/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007  David A. Capello
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

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/layer.h"
#include "raster/sprite.h"

#endif

static bool cmd_layer_properties_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_layer_properties_execute(const char *argument)
{
  JWidget window, box1, box2, box3, label_name, entry_name;
  JWidget button_ok, button_cancel, label_bm, view_bm, list_bm;
  Sprite *sprite;
  Layer *layer;

  /* get current sprite */
  sprite = current_sprite;
  if (!sprite)
    return;

  /* get selected layer */
  layer = sprite->layer;
  if (!layer)
    return;

  window = jwindow_new(_("Layer Properties"));
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL);
  box3 = jbox_new(JI_HORIZONTAL + JI_HOMOGENEOUS);
  label_name = jlabel_new(_("Name:"));
  entry_name = jentry_new(256, layer->name);
  button_ok = jbutton_new(_("&OK"));
  button_cancel = jbutton_new(_("&Cancel"));

  if (layer->gfxobj.type == GFXOBJ_LAYER_IMAGE) {
    label_bm = jlabel_new(_("Blend mode:"));
    view_bm = jview_new();
    list_bm = jlistbox_new();

    jwidget_add_child(list_bm, jlistitem_new(_("Normal")));
    jwidget_add_child(list_bm, jlistitem_new(_("Dissolve")));
    jwidget_add_child(list_bm, jlistitem_new(_("Multiply")));
    jwidget_add_child(list_bm, jlistitem_new(_("Screen")));
    jwidget_add_child(list_bm, jlistitem_new(_("Overlay")));
    jwidget_add_child(list_bm, jlistitem_new(_("Hard Light")));
    jwidget_add_child(list_bm, jlistitem_new(_("Dodge")));
    jwidget_add_child(list_bm, jlistitem_new(_("Burn")));
    jwidget_add_child(list_bm, jlistitem_new(_("Darken")));
    jwidget_add_child(list_bm, jlistitem_new(_("Lighten")));
    jwidget_add_child(list_bm, jlistitem_new(_("Addition")));
    jwidget_add_child(list_bm, jlistitem_new(_("Subtract")));
      jwidget_add_child(list_bm, jlistitem_new(_("Difference")));
      jwidget_add_child(list_bm, jlistitem_new(_("Hue")));
      jwidget_add_child(list_bm, jlistitem_new(_("Saturation")));
      jwidget_add_child(list_bm, jlistitem_new(_("Color")));
      jwidget_add_child(list_bm, jlistitem_new(_("Luminosity")));

      jlistbox_select_index(list_bm, layer->blend_mode);

      jview_attach(view_bm, list_bm);
      jwidget_set_static_size(view_bm, 128, 64);
      jwidget_expansive(view_bm, TRUE);
  }

  jwidget_expansive(entry_name, TRUE);

  jwidget_add_child(box2, label_name);
  jwidget_add_child(box2, entry_name);
  jwidget_add_child(box1, box2);
  if (layer->gfxobj.type == GFXOBJ_LAYER_IMAGE) {
    jwidget_add_child(box1, label_bm);
    jwidget_add_child(box1, view_bm);
  }
  jwidget_add_child(box3, button_ok);
  jwidget_add_child(box3, button_cancel);
  jwidget_add_child(box1, box3);
  jwidget_add_child(window, box1);

  jwidget_magnetic(button_ok, TRUE);

  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_ok) {
    layer_set_name(layer, jwidget_get_text(entry_name));
    if (layer->gfxobj.type == GFXOBJ_LAYER_IMAGE)
      layer_set_blend_mode(layer, jlistbox_get_selected_index(list_bm));

    update_screen_for_sprite(sprite);
  }

  jwidget_free(window);
}

Command cmd_layer_properties = {
  CMD_LAYER_PROPERTIES,
  cmd_layer_properties_enabled,
  NULL,
  cmd_layer_properties_execute,
  NULL
};
