-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_Exit()
  local sprite = get_first_sprite()
  local clipboard = get_clipboard_sprite()

  while sprite do
    -- check if this sprite is modified
    if sprite_is_modified(sprite)
      and (not clipboard
	   or sprite.id != clipboard.id) then
      if jalert(_("Warning<<There are sprites with changes.<<Do you want quit anyway?||&Yes||&No"))
         != 1 then
	return
      end
      break
    end
    sprite = get_next_sprite(sprite)
  end

  -- close the window
  jwindow_close(app_get_top_window(), 0)
end
