-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_Undo()
  if current_sprite and undo_can_undo(current_sprite.undo) then
    undo_undo(current_sprite.undo)
    sprite_generate_mask_boundaries(current_sprite)
    GUI_Refresh(current_sprite)
  end
end

function GUI_Redo()
  if current_sprite and undo_can_redo(current_sprite.undo) then
    undo_redo(current_sprite.undo)
    sprite_generate_mask_boundaries(current_sprite)
    GUI_Refresh(current_sprite)
  end
end
