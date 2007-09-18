-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_SwitchREC()
  if is_rec_screen() then
    rec_screen_off()
  else
    rec_screen_on()
  end
end
