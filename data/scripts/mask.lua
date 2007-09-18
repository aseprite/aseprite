-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

function MaskAll()
  local sprite = current_sprite
  if not sprite then return end

  -- undo
  if undo_is_enabled(sprite.undo) then
    undo_set_mask(sprite.undo, sprite)
  end

  -- change the selection
  mask_replace(sprite.mask, 0, 0, sprite.w, sprite.h)

  sprite_generate_mask_boundaries(sprite)
  GUI_Refresh(sprite)
end

function DeselectMask()
  local sprite = current_sprite
  if not sprite or mask_is_empty(sprite.mask) then return end

  -- destroy the *deselected* mask
  local mask = sprite_request_mask(sprite, "*deselected*")
  if mask then
    sprite_remove_mask(sprite, mask)
    mask_free(mask)
  end

  -- save the selection in the repository
  local mask = mask_new_copy(sprite.mask)
  mask_set_name(mask, "*deselected*")
  sprite_add_mask(sprite, mask)

  -- undo
  if undo_is_enabled(sprite.undo) then
    undo_set_mask(sprite.undo, sprite)
  end

  -- deselect the mask
  mask_none(sprite.mask)

  sprite_generate_mask_boundaries(sprite)
  GUI_Refresh(sprite)
end

function ReselectMask()
  local sprite = current_sprite
  if not sprite then return end

  -- request *deselected* mask
  local mask = sprite_request_mask(sprite, "*deselected*")
  if mask then
    -- undo
    if undo_is_enabled(sprite.undo) then
      undo_set_mask(sprite.undo, sprite)
    end

    -- set the mask
    sprite_set_mask(sprite, mask)

    -- remove the *deselected* mask
    sprite_remove_mask(sprite, mask)
    mask_free(mask)

    sprite_generate_mask_boundaries(sprite)
    GUI_Refresh(sprite)
  end
end

function InvertMask()
  local sprite = current_sprite
  if not sprite then return end

  -- change the selection

  if not sprite.mask.bitmap then
    MaskAll()
  else
    -- undo
    if undo_is_enabled(sprite.undo) then
      undo_set_mask(sprite.undo, sprite)
    end

    -- create a new mask
    local mask = mask_new()

    -- select all the sprite area
    mask_replace(mask, 0, 0, sprite.w, sprite.h)

    -- remove in the new mask the current sprite marked region
    image_rectfill(mask.bitmap,
		    sprite.mask.x, sprite.mask.y,
		    sprite.mask.x + sprite.mask.w-1,
		    sprite.mask.y + sprite.mask.h-1, 0)

    -- invert the current mask in the sprite
    mask_invert(sprite.mask)
    if sprite.mask.bitmap then
      -- copy the inverted region in the new mask
      image_copy(mask.bitmap, sprite.mask.bitmap,
		  sprite.mask.x, sprite.mask.y)
    end

    -- we need only need the area inside the sprite
    mask_intersect(mask, 0, 0, sprite.w, sprite.h)

    -- set the new mask
    sprite_set_mask(sprite, mask)
    mask_free(mask)

    sprite_generate_mask_boundaries(sprite)
    GUI_Refresh(sprite)
  end
end

function StretchMaskBottom()
  local sprite = current_sprite
  if not sprite or not sprite.mask.bitmap then return end

  -- undo
  if undo_is_enabled(sprite.undo) then
    undo_set_mask(sprite.undo, sprite)
  end

  local x, y, bitmap, modified

  bitmap = sprite.mask.bitmap

  for y = 0, bitmap.h-2 do
    modified = false

    for x = 0, bitmap.w-1 do
      if image_getpixel(bitmap, x, y) == 1 and
	 image_getpixel(bitmap, x, y+1) == 0 then
	image_putpixel(bitmap, x, y+1, 1)
	modified = true
      end
    end
    if modified then
      y = y+1
    end
  end

  mask_union(sprite.mask,
	      sprite.mask.x, sprite.mask.y+sprite.mask.h, sprite.mask.w, 1)

  bitmap = sprite.mask.bitmap -- the bitmap could change in the union operation
  y = bitmap.h-2
  for x = 0, bitmap.w-1 do
    if image_getpixel(bitmap, x, y) == 0 then
      image_putpixel(bitmap, x, y+1, 0)
    end
  end

  sprite_generate_mask_boundaries(sprite)
  GUI_Refresh(sprite)
end

function GUI_LoadMask()
  -- get current sprite
  local sprite = current_sprite
  if not sprite then
    return
  end

  local filename = ji_file_select(_("Load .msk File"), "", "msk")
  if filename then
    local mask = load_msk_file(filename)
    if not mask then
      jalert(_("Error<<Error loading .msk file").."<<"
	     ..filename.."||".._("&Close"))
      return
    end

    -- undo
    if undo_is_enabled(sprite.undo) then
      undo_set_mask(sprite.undo, sprite)
    end

    sprite_set_mask(sprite, mask)
    mask_free(mask)

    sprite_generate_mask_boundaries(sprite)
    GUI_Refresh(sprite)
  end
end

function GUI_SaveMask()
  -- get current sprite
  local sprite = current_sprite
  if not sprite then
    return
  end

  local filename = "default.msk"
  local ret

  while true do
    filename = ji_file_select(_("Save .msk File"), filename, "msk")
    if not filename then
      return
    end

    -- does the file exist?
    if file_exists(filename) then
      -- ask if the user wants overwrite the file?
      ret = jalert(_("Warning").."<<".._("File exists, overwrite it?")
		   .."<<"..get_filename(filename)
		     .."||".._("&Yes||&No||&Cancel"))
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

  if save_msk_file(sprite.mask, filename) != 0 then
    jalert("Error<<Error saving .msk file<<"..filename.."||&Close")
  end
end
