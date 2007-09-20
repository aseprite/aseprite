-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005, 2007 by David A. Capello

function GUI_SpriteProperties()
  -- get current sprite
  local sprite = current_sprite
  if not sprite then return end

  -- load the window widget
  local window = ji_load_widget("sprprop.jid", "sprite_properties")
  if not window then return end

  -- update widgets values
  do
    local imgtype_text

    if sprite.imgtype == IMAGE_RGB then
      imgtype_text = "RGB"
    elseif sprite.imgtype == IMAGE_GRAYSCALE then
      imgtype_text = "Grayscale"
    elseif sprite.imgtype == IMAGE_INDEXED then
      imgtype_text = "Indexed"
    else
      imgtype_text = "Unknown"
    end

    jwidget_set_text(jwidget_find_name(window, "name"), sprite.filename)
    jwidget_set_text(jwidget_find_name(window, "type"), imgtype_text)
    jwidget_set_text(jwidget_find_name(window, "size"),
		       sprite.w .. "x" .. sprite.h)
    jwidget_set_text(jwidget_find_name(window, "frames"),
		       tostring(sprite.frames))
  end

  jwindow_remap(window)
  jwindow_center(window)

  while true do
    load_window_pos(window, "SpriteProperties")
    jwindow_open_fg(window)
    save_window_pos(window, "SpriteProperties")

    local killer = jwindow_get_killer(window)

    if killer == jwidget_find_name(window, "ok") then
      local filename = jwidget_get_text(jwidget_find_name(window, "name"))
      local frames = tonumber(jwidget_get_text(jwidget_find_name(window, "frames")))

      frames = MID(1, frames, 9999)

      -- does filename change?
      if filename != sprite.filename then
	sprite_set_filename(sprite, filename)
	rebuild_sprite_list()
      end

      sprite_set_frames(sprite, frames)

      -- update sprite in editors
      GUI_Refresh(sprite)
      break
    elseif killer == jwidget_find_name(window, "speed") then
      dialogs_frame_length(-1)
    else
      break
    end
  end

  jwidget_free(window)
end
