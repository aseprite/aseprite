-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function sprite_recent_load(filename)
  local sprite = sprite_load(filename)
  if sprite then
    recent_file(filename)
    sprite_mount(sprite)
    sprite_show(sprite)
  else
    unrecent_file(filename)
  end
  return sprite
end

function GUI_OpenSprite()
  local filename = ji_file_select(_("Open Sprite"), "",
				  get_readable_extensions())
  if filename then
    sprite_recent_load(filename)
  end
end
