-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

-- shows the JPEG configuration dialog
function ConfigureJPEG()
  -- configuration parameters
  local quality = get_config_int("JPEG", "Quality", 100)
  local smooth = get_config_int("JPEG", "Smooth", 0)
  local method = get_config_int("JPEG", "Method", 0)
  -- widgets
  local window = jwindow_new(_("Configure JPEG Compression"))
  local box1 = jbox_new(JI_VERTICAL)
  local box2 = jbox_new(JI_HORIZONTAL)
  local box3 = jbox_new(JI_VERTICAL + JI_HOMOGENEOUS)
  local box4 = jbox_new(JI_VERTICAL + JI_HOMOGENEOUS)
  local box5 = jbox_new(JI_HORIZONTAL + JI_HOMOGENEOUS)
  local label_quality = jlabel_new(_("Quality:"), JI_LEFT)
  local label_smooth = jlabel_new(_("Smooth:"), JI_LEFT)
  local label_method = jlabel_new(_("Method:"), JI_LEFT)
  local slider_quality = jslider_new(0, 100, quality)
  local slider_smooth = jslider_new(0, 100, smooth)
  local view_method = jview_new()
  local list_method = jlistbox_new()
  local button_ok = jbutton_new(_("&OK"))
  local button_cancel = jbutton_new(_("&Cancel"))
  local ret

  jwidget_add_child (list_method, jlistitem_new(_("Slow")))
  jwidget_add_child (list_method, jlistitem_new(_("Fast")))
  jwidget_add_child (list_method, jlistitem_new(_("Float")))
  jlistbox_select_index(list_method, method)

  jview_attach(view_method, list_method)
  jview_maxsize(view_method)

  jwidget_expansive(box4, true)
  jwidget_expansive(view_method, true)

  jwidget_add_child (box3, label_quality)
  jwidget_add_child (box3, label_smooth)
  jwidget_add_child (box4, slider_quality)
  jwidget_add_child (box4, slider_smooth)
  jwidget_add_child (box2, box3)
  jwidget_add_child (box2, box4)
  jwidget_add_child (box1, box2)
  jwidget_add_child (box1, label_method)
  jwidget_add_child (box1, view_method)
  jwidget_add_child (box5, button_ok)
  jwidget_add_child (box5, button_cancel)
  jwidget_add_child (box1, box5)
  jwidget_add_child (window, box1)

  jwindow_open_fg(window)

  if jwindow_get_killer(window) == button_ok then
    ret = true

    quality = jslider_get_value(slider_quality)
    smooth = jslider_get_value(slider_smooth)
    method = jlistbox_get_selected_index(list_method)

    set_config_int("JPEG", "Quality", quality)
    set_config_int("JPEG", "Smooth", smooth)
    set_config_int("JPEG", "Method", method)
  else
    ret = false
  end

  jwidget_free(window)
  return ret
end
