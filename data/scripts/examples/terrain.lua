-- ase -- allegro-sprite-editor: the ultimate sprites factory
-- Copyright (C) 2001-2005 by David A. Capello

local function MakeTerrain()
  local sprite, layer, frame, image, frpos, first_image
  local w, h = 256, 256

  sprite = sprite_new(IMAGE_INDEXED, w, h)
  sprite_mount(sprite)
  set_current_sprite(sprite)

  -- disable undo
  undo_disable(sprite.undo)

  sprite_set_filename(sprite, "terrain")
  sprite_set_frames(sprite, 64)
  sprite_set_speed(sprite, 100)

  layer = layer_new(sprite.imgtype)
  layer_set_name(layer, _("Background"))
  layer_set_blend_mode(layer, BLEND_MODE_NORMAL)
  layer_add_layer(sprite.set, layer)
  sprite_set_layer(sprite, layer)

  -- draw the graphics
  local lastpos = sprite.frames-1

  add_progress(sprite.frames)
  for frpos = 0, lastpos do
    image = image_new(sprite.imgtype, w, h)
    stock_add_image(layer.stock, image)
    frame = frame_new(frpos, stock_add_image(layer.stock, image))
    layer_add_frame(layer, frame)

    if frpos == 0 then
      mapgen(image, 3, sqrt(2))
      first_image = image
    else
      image_copy(image, first_image, 0, 0)
      ColorCurveIndex({ 0,0,
			128,MID(0, 128+128*sin(2*PI*frpos/lastpos), 255),
			255,255 })
    end

    sprite_set_frpos(sprite, frpos)
    do_progress(frpos)
  end
  sprite_set_frpos(sprite, 0)
  del_progress()

  LoadPalette("terrain1.col")

  -- enable undo
  undo_enable(sprite.undo)
  return sprite
end

local sprite = MakeTerrain()
sprite_show(sprite)
