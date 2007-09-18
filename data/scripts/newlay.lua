-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_NewLayer()
  -- get current sprite
  local sprite = current_sprite
  if not sprite then return end

  -- load the window widget
  local window = ji_load_widget("newlay.jid", "new_layer")
  if not window then return end

  jwidget_set_text(jwidget_find_name(window, "name"), GetUniqueLayerName())
  jwidget_set_text(jwidget_find_name(window, "width"), tostring(sprite.w))
  jwidget_set_text(jwidget_find_name(window, "height"), tostring(sprite.h))

  jwindow_open_fg(window)

  if jwindow_get_killer(window) == jwidget_find_name(window, "ok") then
    local name = jwidget_get_text(jwidget_find_name(window, "name"))
    local w = MID(1, tonumber(jwidget_get_text(jwidget_find_name(window, "width"))), 9999)
    local h = MID(1, tonumber(jwidget_get_text(jwidget_find_name(window, "height"))), 9999)
    local layer = NewLayer(name, 0, 0, w, h)
    if not layer then
      jalert(_("Error<<Not enough memory||&Close"))
      return
    end

    GUI_Refresh(sprite)
  end

  jwidget_free(window)
end

function GUI_NewLayerSet()
  -- get current sprite
  local sprite = current_sprite
  if not sprite then
    return
  end

  -- load the window widget
  local window = ji_load_widget("newlay.jid", "new_layer_set")
  if not window then return end

  jwindow_open_fg(window)

  if jwindow_get_killer(window) == jwidget_find_name(window, "ok") then
    local name = jwidget_get_text(jwidget_find_name(window, "name"))
    local layer = NewLayerSet(name)
    if not layer then
      jalert(_("Error<<Not enough memory||&Close"))
      return
    end

    GUI_Refresh(sprite)
  end

  jwidget_free(window)
end
