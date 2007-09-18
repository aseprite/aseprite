-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_TestJID()
  local filename = ji_file_select(_("Open a JID file"), "", "jid")
  if filename then
    local window, box1, box2, box3, label, entry, button_ok, button_cancel

    window = jwindow_new(_("Test Widget"))
    box1 = jbox_new(JI_VERTICAL)
    box2 = jbox_new(JI_HORIZONTAL)
    box3 = jbox_new(JI_HORIZONTAL + JI_HOMOGENEOUS)
    label = jlabel_new(_("Widget Name:"))
    entry = jentry_new(256, get_config_string("TestJid", "Name", "my_widget"))
    button_ok = jbutton_new(_("&OK"))
    button_cancel = jbutton_new(_("&Cancel"))

    jwidget_magnetic(button_ok, true)
    jwidget_expansive(box2, true)
    jwidget_expansive(entry, true)

    jwidget_add_child (box2, label)
    jwidget_add_child (box2, entry)
    jwidget_add_child (box1, box2)
    jwidget_add_child (box3, button_ok)
    jwidget_add_child (box3, button_cancel)
    jwidget_add_child (box1, box3)
    jwidget_add_child (window, box1)

    jwindow_open_fg(window)

    if jwindow_get_killer(window) == button_ok then
      local widget_name = jwidget_get_text(entry)
      local loaded_widget = ji_load_widget(filename, widget_name)

      if not loaded_widget then
	jalert(_("Error<<Error loading widget:") .. "<<"
	       .. widget_name .. "||" .. _("&OK"))
      else
	set_config_string("TestJid", "Name", widget_name)

	if jwidget_get_type(loaded_widget) == JI_WINDOW then
	  jwindow_open_bg(loaded_widget)
	else
	  local aux_window = jwindow_new("Widget: "..widget_name)
	  jwidget_add_child (aux_window, loaded_widget)
	  jwindow_open_bg(aux_window)
	end
      end
    end

    jwidget_free(window)
  end
end
