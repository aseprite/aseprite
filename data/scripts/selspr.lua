-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

-- shows a sprite by its ID
function ShowSpriteByID(sprite_id)
  local sprite = nil

  -- is not the null sprite?
  if sprite_id != 0 then
    local clipboard = get_clipboard_sprite()

    -- the clipboard?
    if clipboard and sprite_id == clipboard.id then
      sprite = clipboard
    -- some sprite in the list?
    else
      sprite = get_first_sprite()

      while sprite do
	if sprite.id == sprite_id then
	  break
	end
	sprite = get_next_sprite(sprite)
      end
    end
  end

  set_sprite_in_current_editor(sprite)
end
