-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005, 2007 by David A. Capello

function switch_recording_screen()
  if is_rec_screen() then
    rec_screen_off()
  else
    rec_screen_on()
  end
end
