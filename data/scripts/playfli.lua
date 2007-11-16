-- ASE - Allegro Sprite Editor
-- Copyright (C) 2001-2005, 2007 by David A. Capello

local function file_select_fli()
  return ji_file_select(_("Open a FLI/FLC file"), "", "fli,flc")
end

function GUI_PlayFLI()
  local filename = file_select_fli()
  if filename then
    -- load the window widget
    local window = ji_load_widget("playfli.jid", "play_fli")
    if not window then return end

    local loop = jwidget_find_name(window, "loop")
    local fullscreen = jwidget_find_name(window, "fullscreen")
    local button_open = jwidget_find_name(window, "button_open")
    local button_play = jwidget_find_name(window, "button_play")

    if get_config_bool("PlayFLI", "Loop", true) then
      jwidget_select(loop)
    end
    if get_config_bool("PlayFLI", "FullScreen", false) then
      jwidget_select(fullscreen)
    end

    while true do
      jwindow_open_fg(window)

      if jwindow_get_killer(window) == button_play then
	set_config_bool("PlayFLI", "Loop", jwidget_is_selected(loop))
	set_config_bool("PlayFLI", "FullScreen", jwidget_is_selected(fullscreen))

	play_fli_animation(filename,
			   jwidget_is_selected(loop),
			   jwidget_is_selected(fullscreen))
      elseif jwindow_get_killer(window) == button_open then
	local filename2 = file_select_fli()
	if filename2 then
	  filename = filename2
	end
      else
	break
      end
    end

    jwidget_free(window)
  end
end
