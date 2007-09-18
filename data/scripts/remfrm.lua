-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function RemoveFrame(layer, frame)
  local sprite = current_sprite
  local frpos, it, used, image

  if sprite and layer_is_image(layer) and frame then
    -- find if the image that use the frame to remove, is used by
    -- another frames
    used = false
    for frpos = 0, sprite.frames-1 do
      it = layer_get_frame(layer, frpos)
      if it and it.id != frame.id and it.image == frame.image then
	used = true
	break
      end
    end

    undo_open(sprite.undo)
    if not used then
      -- if the image is only used by this frame, we can remove the
      -- image from the stock
      image = stock_get_image(layer.stock, frame.image)
      undo_remove_image(sprite.undo, layer.stock, image)
      stock_remove_image(layer.stock, image)
      image_free(image)
    end
    undo_remove_frame(sprite.undo, layer, frame)
    undo_close(sprite.undo)

    -- remove the frame
    layer_remove_frame(layer, frame)
    frame_free(frame)
  end
end

function GUI_RemoveFrame()
  local sprite = current_sprite
  if sprite then
    local frame = layer_get_frame(sprite.layer, sprite.frpos)
    if frame then
      RemoveFrame(sprite.layer, frame)
      GUI_Refresh(sprite)
    end
  end
end
