-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_FlipHorizontal()
  if current_sprite then
    flip_horizontal()
    GUI_Refresh(current_sprite)
  end
end

function GUI_FlipVertical()
  if current_sprite then
    flip_vertical()
    GUI_Refresh(current_sprite)
  end
end
