-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_LoadSession()
  local filename = ji_file_select(_("Load .ses File"), "", "ses")
  if filename then
    if load_session(filename) then
      jalert(_("Session Manager<<Session loaded succefully<<Go to the \"List\" menu to see the loaded sprites||&OK"))
    else
      jalert(_("Session Manager<<Error loading session||&OK"))
    end
  end
end

function GUI_SaveSession()
  local filename = ji_file_select(_("Save .ses File"), "", "ses")
  if filename then
    if save_session(filename) then
      jalert(_("Session Manager<<Session saved succefully||&OK"))
    else
      jalert(_("Session Manager<<Error saving session||&OK"))
    end
  end
end

if is_interactive() and
   is_backup_session() and
   get_config_bool("Options", "AskBkpSes", true) then
  local ret = jalert(_("Session Manager<<There is a backed up session(maybe a previous crash).<<Do you want to load it?||Never &ask||&No||&Yes"))

  -- never ask again
  if ret == 1 then
    set_config_bool("Options", "AskBkpSes", false)
    -- try to load it
  elseif ret == 3 then
    set_config_string("FileSelect", "CurrentDirectory", "data/session")
    GUI_LoadSession()
  end
end
