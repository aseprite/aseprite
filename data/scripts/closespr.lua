-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_CloseSprite()
  if not current_sprite then return end

  -- see if the sprite has changes
  while sprite_is_modified(current_sprite) do
    -- ask what want to do the user with the changes in the sprite
    local ret = jalert(_("Warning")..
		       "<<".._("Saving changes in:")..
			 "<<"..get_filename(current_sprite.filename)..
			 "||".._("&Save||&Discard||&Cancel"))

    if ret == 1 then
      -- "save": save the changes
      GUI_SaveSprite()		-- call this routine from savespr.lua
    elseif ret != 2 then
      -- "cancel" or <esc>
      return			-- we back doing nothing
    else
      -- "discard"
      break
    end
  end

  CloseSprite()
end
