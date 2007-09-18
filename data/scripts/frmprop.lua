-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_FrameProperties()
  -- get current sprite
  local sprite = current_sprite
  if not sprite then return end

  -- get selected layer
  local layer = sprite.layer
  if not layer then return end

  -- get current frame
  local frame = layer_get_frame(layer, sprite.frpos)
  if not frame then return end

  local window = ji_load_widget("frmprop.jid", "frame_properties")
  if not window then return end

  local entry_frpos = jwidget_find_name(window, "frpos")
  local entry_xpos = jwidget_find_name(window, "xpos")
  local entry_ypos = jwidget_find_name(window, "ypos")
  local slider_opacity = jwidget_find_name(window, "opacity")
  local button_ok = jwidget_find_name(window, "ok")

  jwidget_set_text(entry_frpos, tostring(frame.frpos))
  jwidget_set_text(entry_xpos, tostring(frame.x))
  jwidget_set_text(entry_ypos, tostring(frame.y))
  jslider_set_value(slider_opacity, frame.opacity)

  repeat
    jwindow_open_fg(window)

    if jwindow_get_killer(window) == button_ok then
      local new_frpos = MID(0, tonumber(jwidget_get_text (entry_frpos)),
			    sprite.frames-1)
      local existent_frame = layer_get_frame(layer, new_frpos)

      if new_frpos != frame.frpos and existent_frame then
	jalert("Error<<You can't change frpos to " .. new_frpos ..
	       "<<Already there is a frame in that pos.||&OK")
      else
	-- WE MUST REMOVE THE FRAME BEFORE CALL frame_set_frpos()
	if undo_is_enabled(sprite.undo) then
	  undo_open(sprite.undo)
	  undo_remove_frame(sprite.undo, layer, frame)
	end

	layer_remove_frame(layer, frame)

	-- change frame properties
	frame_set_frpos(frame, new_frpos)
	frame_set_position(frame,
	   MID(-9999, tonumber(jwidget_get_text(entry_xpos)), 9999),
	   MID(-9999, tonumber(jwidget_get_text(entry_ypos)), 9999))
	frame_set_opacity(frame, jslider_get_value(slider_opacity))

	-- add again the same frame
	if undo_is_enabled(sprite.undo) then
	  undo_add_frame(sprite.undo, layer, frame)
	  undo_close(sprite.undo)
	end

	layer_add_frame(layer, frame)

	-- set the sprite position, refresh and break the loop
	sprite_set_frpos(sprite, new_frpos)
	GUI_Refresh(sprite)
	break
      end
    else
      break
    end
  until false

  jwidget_free(window)
end
