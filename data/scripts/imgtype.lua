-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_ImageType()
  if not current_sprite then
    return
  end

  -- load the window widget
  local window = ji_load_widget("imgtype.jid", "image_type")
  if not window then return end

  local from, radio1, radio2, radio3, dither1, dither2 =
    jwidget_find_name(window, "from"),
    jwidget_find_name(window, "imgtype1"),
    jwidget_find_name(window, "imgtype2"),
    jwidget_find_name(window, "imgtype3"),
    jwidget_find_name(window, "dither1"),
    jwidget_find_name(window, "dither2")

  if current_sprite.imgtype == IMAGE_RGB then
    jwidget_set_text(from, _("RGB"))
    jwidget_disable(radio1)
    jwidget_select(radio3)	-- to Indexed by default
  elseif current_sprite.imgtype == IMAGE_GRAYSCALE then
    jwidget_set_text(from, _("Grayscale"))
    jwidget_disable(radio2)
    jwidget_select(radio1)	-- to RGB by default
  elseif current_sprite.imgtype == IMAGE_INDEXED then
    jwidget_set_text(from, _("Indexed"))
    jwidget_disable(radio3)
    jwidget_select(radio1)	-- to RGB by default
  end

  if get_config_bool("Options", "Dither", false) then
    jwidget_select(dither2)
  else
    jwidget_select(dither1)
  end

  -- open the window
  jwindow_open_fg(window)

  if jwindow_get_killer(window) == jwidget_find_name(window, "ok") then
    local destimgtype, dithermethod

    if jwidget_is_selected(radio1) then
      destimgtype = IMAGE_RGB
    elseif jwidget_is_selected(radio2) then
      destimgtype = IMAGE_GRAYSCALE
    elseif jwidget_is_selected(radio3) then
      destimgtype = IMAGE_INDEXED
    end

    if jwidget_is_selected(dither1) then
      dithermethod = DITHERING_NONE
    elseif jwidget_is_selected(dither2) then
      dithermethod = DITHERING_ORDERED
    end

    use_current_sprite_rgb_map()
    sprite_set_imgtype(current_sprite, destimgtype, dithermethod)
    restore_rgb_map()

    app_refresh_screen()
  end

  jwidget_free(window)
end
