-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_LoadScriptFile()
  local filename = ji_file_select(_("Load Script File(.lua)"), "", "lua")
  if filename then
    dofile(filename)
  end
end
