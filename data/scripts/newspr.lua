-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

local _sprite_counter = 0

function NewSprite(imgtype, w, h)
  -- new sprite
  local sprite = sprite_new_with_layer(imgtype, w, h)
  if not sprite then return nil end

  sprite_mount(sprite)
  set_current_sprite(sprite)
  return sprite
end

-- shows the "New Sprite" dialog
function GUI_NewSprite()
  -- load the window widget
  local window = ji_load_widget("newspr.jid", "new_sprite")
  if not window then return end

  local width = jwidget_find_name(window, "width")
  local height = jwidget_find_name(window, "height")
  local radio1 = jwidget_find_name(window, "radio1")
  local radio2 = jwidget_find_name(window, "radio2")
  local radio3 = jwidget_find_name(window, "radio3")
  local button_ok = jwidget_find_name(window, "ok_button")
  local bg_box = jwidget_find_name(window, "bg_box")
  local imgtype, w, h, bg

  -- default values: Indexed, 320x200, Transparent
  imgtype = get_config_int("NewSprite", "Type", IMAGE_INDEXED)
  imgtype = MID(IMAGE_RGB, imgtype, IMAGE_INDEXED)
  w = get_config_int("NewSprite", "Width", 320)
  h = get_config_int("NewSprite", "Height", 200)
  bg = get_config_int("NewSprite", "Background", 0)

  jwidget_set_text(width, w)
  jwidget_set_text(height, h)

  -- select image-type
  if imgtype == IMAGE_RGB then
    jwidget_select(radio1)
  elseif imgtype == IMAGE_GRAYSCALE then
    jwidget_select(radio2)
  elseif imgtype == IMAGE_INDEXED then
    jwidget_select(radio3)
  end

  -- select background color
  jlistbox_select_index(bg_box, bg)

  -- open the window
  jwindow_open_fg(window)

  if jwindow_get_killer(window) == button_ok then
    -- get the options
    if jwidget_is_selected(radio1) then
      imgtype = IMAGE_RGB
    elseif jwidget_is_selected(radio2) then
      imgtype = IMAGE_GRAYSCALE
    elseif jwidget_is_selected(radio3) then
      imgtype = IMAGE_INDEXED
    end

    w = tonumber(jwidget_get_text(width))
    h = tonumber(jwidget_get_text(height))
    bg = jlistbox_get_selected_index(bg_box)

    w = MID(1, w, 9999)
    h = MID(1, h, 9999)

    -- select the color
    local color = nil

    if bg >= 0 and bg <= 3 then
      local bg_table = {
	"mask",
	"rgb{0,0,0}",
	"rgb{255,255,255}",
	"rgb{255,0,255}" }
      color = bg_table[bg+1]
    else
      local default_color = get_config_string("NewSprite", "BackgroundCustom",
					       "rgb{0,0,0}")

      color = ji_color_select(imgtype, default_color)
      if color then
	set_config_string("NewSprite", "BackgroundCustom", color)
      end
    end

    if color then
      -- save the configuration
      set_config_int("NewSprite", "Type", imgtype)
      set_config_int("NewSprite", "Width", w)
      set_config_int("NewSprite", "Height", h)
      set_config_int("NewSprite", "Background", bg)

      -- create the new sprite
      local sprite = NewSprite(imgtype, w, h)

      _sprite_counter = _sprite_counter + 1
      sprite_set_filename(sprite, "Sprite-" .. _sprite_counter)

      image_clear(GetImage(), get_color_for_image(imgtype, color))

      sprite_show(sprite)
    end
  end

  jwidget_free(window)
end
