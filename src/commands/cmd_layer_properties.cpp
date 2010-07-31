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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "app.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// layer_properties

class LayerPropertiesCommand : public Command
{
public:
  LayerPropertiesCommand();
  Command* clone() { return new LayerPropertiesCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

LayerPropertiesCommand::LayerPropertiesCommand()
  : Command("layer_properties",
	    "Layer Properties",
	    CmdRecordableFlag)
{
}

bool LayerPropertiesCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    sprite->getCurrentLayer() != NULL;
}

void LayerPropertiesCommand::onExecute(Context* context)
{
  JWidget box1, box2, box3, label_name, entry_name;
  JWidget button_ok, button_cancel, label_bm, view_bm, list_bm;
  CurrentSpriteWriter sprite(context);
  Layer* layer = sprite->getCurrentLayer();
  bool with_blend_modes = (layer->is_image() && sprite->getImgType() != IMAGE_INDEXED);

  FramePtr window(new Frame(false, _("Layer Properties")));
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL);
  box3 = jbox_new(JI_HORIZONTAL + JI_HOMOGENEOUS);
  label_name = new Label(_("Name:"));
  entry_name = jentry_new(256, layer->get_name().c_str());
  button_ok = jbutton_new(_("&OK"));
  button_cancel = jbutton_new(_("&Cancel"));

  if (with_blend_modes) {
    label_bm = new Label(_("Blend mode:"));
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

    jlistbox_select_index(list_bm, static_cast<LayerImage*>(layer)->get_blend_mode());

    jview_attach(view_bm, list_bm);
    jwidget_set_min_size(view_bm, 128*jguiscale(), 64*jguiscale());
    jwidget_expansive(view_bm, true);
  }

  jwidget_set_min_size(entry_name, 128, 0);
  jwidget_expansive(entry_name, true);

  jwidget_add_child(box2, label_name);
  jwidget_add_child(box2, entry_name);
  jwidget_add_child(box1, box2);
  if (with_blend_modes) {
    jwidget_add_child(box1, label_bm);
    jwidget_add_child(box1, view_bm);
  }
  jwidget_add_child(box3, button_ok);
  jwidget_add_child(box3, button_cancel);
  jwidget_add_child(box1, box3);
  jwidget_add_child(window, box1);

  jwidget_magnetic(entry_name, true);
  jwidget_magnetic(button_ok, true);

  window->open_window_fg();

  if (window->get_killer() == button_ok) {
    layer->set_name(entry_name->getText());

    if (with_blend_modes)
      static_cast<LayerImage*>(layer)->set_blend_mode(jlistbox_get_selected_index(list_bm));

    update_screen_for_sprite(sprite);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_layer_properties_command()
{
  return new LayerPropertiesCommand;
}
