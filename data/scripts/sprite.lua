-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

-- closes the current sprite
function CloseSprite()
  local sprite = current_sprite
  if sprite then
    sprite_unmount(sprite)
    sprite_free(sprite)
  end
end
