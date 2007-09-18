-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_ReloadMenus()
  if jalert(_("Warning<<If you try to reload the root-menu<<you will lost the current one.<<Do you want continue?||&Load||&Cancel"))
     == 1 then
   rebuild_root_menu()
  end
end
