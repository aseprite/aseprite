-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_SaveSprite()
  if not current_sprite then
    return
  end

  local filename = current_sprite.filename
  local ret

  while true do
    filename = ji_file_select(_("Save Sprite"), filename,
			       get_writeable_extensions())
    if not filename then
      return
    end

    -- does the file exist?
    if file_exists(filename) then
      -- ask if the user wants overwrite the file?
      ret = jalert(_("Warning")..
		   "<<".._("File exists, overwrite it?")..
		     "<<"..get_filename(filename)..
		     "||".._("&Yes||&No||&Cancel"))
    else
      break
    end

    -- "yes": we must continue with the operation...
    if ret == 1 then
      break
      -- "cancel" or <esc> per example: we back doing nothing
    elseif ret != 2 then
      return
    end
    -- "no": we must back to select other file-name */
  end

  sprite_set_filename(current_sprite, filename)
  rebuild_sprite_list()

  if sprite_save(current_sprite) == 0 then
    recent_file(filename)
    sprite_was_saved(current_sprite)
  else
    unrecent_file(filename)
    print(_("Error saving sprite file: ")..current_sprite.filename)
  end
end
