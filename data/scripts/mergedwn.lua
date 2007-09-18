-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function GUI_MergeDown()
  local sprite, frpos, index
  local src_layer, dst_layer
  local src_frame, dst_frame
  local src_image, dst_image
  local old_id

  sprite = current_sprite
  if not sprite then return end

  src_layer = sprite.layer
  if not src_layer or not layer_is_image(src_layer) then return end

  dst_layer = sprite.layer.prev
  if not dst_layer or not layer_is_image(dst_layer) then return end

  if undo_is_enabled(sprite.undo) then
    undo_open(sprite.undo)
  end

  for frpos = 0, sprite.frames-1 do
    -- get frames
    src_frame = layer_get_frame(src_layer, frpos)
    dst_frame = layer_get_frame(dst_layer, frpos)

    -- get images
    if src_frame then
      src_image = stock_get_image(src_layer.stock, src_frame.image)
    else
      src_image = nil
    end

    if dst_frame then
      dst_image = stock_get_image(dst_layer.stock, dst_frame.image)
    else
      dst_image = nil
    end

    -- with source image?
    if src_image then
      -- no destination image
      if not dst_image then
	-- copy this frame to the destination layer
	dst_image = image_new_copy(src_image)
	index = stock_add_image(dst_layer.stock, dst_image)
	if undo_is_enabled(sprite.undo) then
	  undo_add_image(sprite.undo, dst_layer.stock, dst_image)
	end
	dst_frame = frame_new(frpos, index)
	frame_set_position(dst_frame, src_frame.x, src_frame.y)
	frame_set_opacity(dst_frame, src_frame.opacity)
	if undo_is_enabled(sprite.undo) then
	  undo_add_frame(sprite.undo, dst_layer, dst_frame)
	end
	layer_add_frame(dst_layer, dst_frame)
      -- with destination
      else
	local x1 = MIN(src_frame.x, dst_frame.x)
	local y1 = MIN(src_frame.y, dst_frame.y)
	local x2 = MAX(src_frame.x+src_image.w-1, dst_frame.x+dst_image.w-1)
	local y2 = MAX(src_frame.y+src_image.h-1, dst_frame.y+dst_image.h-1)
	local new_image = image_crop(dst_image,
				     x1-dst_frame.x,
				     y1-dst_frame.y,
				     x2-x1+1, y2-y1+1)

	-- merge src_image in new_image
	image_merge(new_image, src_image,
		     src_frame.x-x1,
		     src_frame.y-y1,
		     src_frame.opacity, src_layer.blend_mode)

	frame_set_position(dst_frame, x1, y1)
	if undo_is_enabled(sprite.undo) then
	  undo_replace_image(sprite.undo, dst_layer.stock, dst_frame.image)
	end
	stock_replace_image(dst_layer.stock, dst_frame.image, new_image)

	image_free(dst_image)
      end
    end
  end

  if undo_is_enabled(sprite.undo) then
    undo_set_layer(sprite.undo, sprite)
    undo_remove_layer(sprite.undo, src_layer)
    undo_close(sprite.undo)
  end

  sprite_set_layer(sprite, dst_layer)
  layer_remove_layer(src_layer.parent, src_layer)
  layer_free(src_layer)

  GUI_Refresh(sprite)
end
