-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function FlattenLayers()
  local sprite = current_sprite
  if not sprite then
    return
  end

  -- generate the flat_layer
  local flat_layer = layer_flatten(sprite.set,
				   sprite.imgtype, 0, 0, sprite.w, sprite.h,
				   0, sprite.frames-1)
  if not flat_layer then
    print("Not enough memory")
    return nil
  end

  -- open undo, and add the new layer
  if undo_is_enabled(sprite.undo) then
    undo_open(sprite.undo)
    undo_add_layer(sprite.undo, sprite.set, flat_layer)
  end

  layer_add_layer(sprite.set, flat_layer)

  -- select the new layer
  if undo_is_enabled(sprite.undo) then
    undo_set_layer(sprite.undo, sprite)
  end

  sprite_set_layer(sprite, flat_layer)

  -- remove old layers
  local it = sprite.set.layers
  local next

  while it do
    next = it.next

    if it.id != flat_layer.id then
      -- undo
      if undo_is_enabled(sprite.undo) then
	undo_remove_layer(sprite.undo, it)
      end

      -- remove and destroy "it" layer
      layer_remove_layer(sprite.set, it)
      layer_free(it)
    end

    it = next
  end

  -- close the undo
  if undo_is_enabled(sprite.undo) then
    undo_close(sprite.undo)
  end

  GUI_Refresh(sprite)

  return flat_layer
end
