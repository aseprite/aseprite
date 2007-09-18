-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function DuplicateLayer()
  local sprite = current_sprite

  if not sprite or not sprite.layer then
    return nil
  end

  local layer_copy = layer_new_copy(sprite.layer)
  if not layer_copy then
    print("Not enough memory")
    return nil
  end

  layer_set_name(layer_copy, layer_copy.name .. " Copy")

  if undo_is_enabled(sprite.undo) then
    undo_open(sprite.undo)
    undo_add_layer(sprite.undo, sprite.layer.parent, layer_copy)
  end

  layer_add_layer(sprite.layer.parent, layer_copy)

  if undo_is_enabled(sprite.undo) then
    undo_move_layer(sprite.undo, layer_copy)
    undo_set_layer(sprite.undo, sprite)
    undo_close(sprite.undo)
  end

  layer_move_layer(sprite.layer.parent, layer_copy, sprite.layer)
  sprite_set_layer(sprite, layer_copy)

  return layer_copy
end

function GUI_DuplicateLayer()
  if DuplicateLayer() then
    GUI_Refresh(current_sprite)
  end
end
