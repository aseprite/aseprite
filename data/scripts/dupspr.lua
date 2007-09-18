-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_DuplicateSprite()
  local sprite = current_sprite
  if not sprite then return end

  -- load the window widget
  local window = ji_load_widget("dupspr.jid", "duplicate_sprite")
  if not window then return end

  local src_name, dst_name, flatten
    = jwidget_find_name(window, "src_name"),
      jwidget_find_name(window, "dst_name"),
      jwidget_find_name(window, "flatten")

  jwidget_set_text(src_name, get_filename(sprite.filename))
  jwidget_set_text(dst_name, sprite.filename .. " Copy")

  if get_config_bool("DuplicateSprite", "Flatten", false) then
    jwidget_select(flatten)
  end

  -- open the window
  jwindow_open_fg(window)

  if jwindow_get_killer(window) == jwidget_find_name(window, "ok") then
    set_config_bool("DuplicateSprite", "Flatten",
		    jwidget_is_selected(flatten))

    local sprite_copy

    if jwidget_is_selected(flatten) then
      sprite_copy = sprite_new_flatten_copy(sprite)
    else
      sprite_copy = sprite_new_copy(sprite)
    end

    if sprite_copy then
      sprite_set_filename(sprite_copy, jwidget_get_text(dst_name))

      sprite_mount(sprite_copy)
      set_current_sprite(sprite_copy)
      sprite_show(sprite_copy)
    end
  end

  jwidget_free(window)
end
