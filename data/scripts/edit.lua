-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_EditClear()
  -- get current sprite
  local sprite = current_sprite
  if not sprite then return end

  -- clear the mask
  ClearMask()

  -- refresh the sprite
  GUI_Refresh(sprite)
end
