-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_LayerProperties()
  -- get current sprite
  local sprite = current_sprite
  if not sprite then
    return
  end

  -- get selected layer
  local layer = sprite.layer
  if not layer then
    return
  end

  local window = jwindow_new(_("Layer Properties"))
  local box1 = jbox_new(JI_VERTICAL)
  local box2 = jbox_new(JI_HORIZONTAL)
  local box3 = jbox_new(JI_HORIZONTAL + JI_HOMOGENEOUS)
  local label_name = jlabel_new(_("Name:"))
  local entry_name = jentry_new(256, layer.name)
  local button_ok = jbutton_new(_("&OK"))
  local button_cancel = jbutton_new(_("&Cancel"))
  local label_bm, view_bm, list_bm

  if layer.type == GFXOBJ_LAYER_IMAGE then
    label_bm = jlabel_new(_("Blend mode:"))
    view_bm = jview_new()
    list_bm = jlistbox_new()

    jwidget_add_child(list_bm, jlistitem_new(_("Normal")))
    jwidget_add_child(list_bm, jlistitem_new(_("Dissolve")))
    jwidget_add_child(list_bm, jlistitem_new(_("Multiply")))
    jwidget_add_child(list_bm, jlistitem_new(_("Screen")))
    jwidget_add_child(list_bm, jlistitem_new(_("Overlay")))
    jwidget_add_child(list_bm, jlistitem_new(_("Hard Light")))
    jwidget_add_child(list_bm, jlistitem_new(_("Dodge")))
    jwidget_add_child(list_bm, jlistitem_new(_("Burn")))
    jwidget_add_child(list_bm, jlistitem_new(_("Darken")))
    jwidget_add_child(list_bm, jlistitem_new(_("Lighten")))
    jwidget_add_child(list_bm, jlistitem_new(_("Addition")))
    jwidget_add_child(list_bm, jlistitem_new(_("Subtract")))
    jwidget_add_child(list_bm, jlistitem_new(_("Difference")))
    jwidget_add_child(list_bm, jlistitem_new(_("Hue")))
    jwidget_add_child(list_bm, jlistitem_new(_("Saturation")))
    jwidget_add_child(list_bm, jlistitem_new(_("Color")))
    jwidget_add_child(list_bm, jlistitem_new(_("Luminosity")))

    jlistbox_select_index(list_bm, layer.blend_mode)

    jview_attach(view_bm, list_bm)
    jwidget_set_static_size(view_bm, 128, 64)
    jwidget_expansive(view_bm, true)
  end

  jwidget_expansive(entry_name, true)

  jwidget_add_child(box2, label_name)
  jwidget_add_child(box2, entry_name)
  jwidget_add_child(box1, box2)
  if layer.type == GFXOBJ_LAYER_IMAGE then
    jwidget_add_child(box1, label_bm)
    jwidget_add_child(box1, view_bm)
  end
  jwidget_add_child(box3, button_ok)
  jwidget_add_child(box3, button_cancel)
  jwidget_add_child(box1, box3)
  jwidget_add_child(window, box1)

  jwidget_magnetic(button_ok, true)

  jwindow_open_fg(window)

  if jwindow_get_killer(window) == button_ok then
    layer_set_name(layer, jwidget_get_text(entry_name))
    if layer.type == GFXOBJ_LAYER_IMAGE then
      layer_set_blend_mode(layer, jlistbox_get_selected_index(list_bm))
    end

    GUI_Refresh(sprite)
  end

  jwidget_free(window)
end
