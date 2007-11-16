-- ASE - Allegro Sprite Editor
-- Copyright (C) 2001-2005, 2007 by David A. Capello

local function MakeSpiral(bg_color, fg_color)
  local sprite, layer, frame, image, frpos
  local w, h = 256, 256

  sprite = sprite_new(IMAGE_RGB, w, h)
  sprite_mount(sprite)
  set_current_sprite(sprite)

  -- disable undo
  undo_disable(sprite.undo)

  sprite_set_filename(sprite, "spiral")
  sprite_set_frames(sprite, 16)
  sprite_set_speed(sprite, 10)

  layer = layer_new(sprite.imgtype)
  layer_set_name(layer, _("Background"))
  layer_set_blend_mode(layer, BLEND_MODE_NORMAL)
  layer_add_layer(sprite.set, layer)
  sprite_set_layer(sprite, layer)

  -- draw the graphics
  local a, a2, len, dlen, max
  local lastpos = sprite.frames-1

  add_progress(sprite.frames)
  for frpos = 0, lastpos do
    image = image_new(sprite.imgtype, w, h)
    image_clear(image, bg_color)
    stock_add_image(layer.stock, image)
    frame = frame_new(frpos, stock_add_image(layer.stock, image))
    layer_add_frame(layer, frame)

    path = path_new(nil)
    path_moveto(path, w/2, h/2)
    max = 2*PI*8
    len = 0
    dlen = 0.75*w/(max/0.05)
    for a = 0, max, 0.05 do
      a2 = a - PI*2*frpos/(lastpos+1)
      path_lineto(path,
		  w/2 + cos(a2)*len,
		  h/2 - sin(a2)*len)
      len = len + dlen
    end
    path_stroke(path, image, fg_color, 1)
    path_free(path)

    sprite_set_frpos(sprite, frpos)
    do_progress(frpos)
  end
  sprite_set_frpos(sprite, 0)
  del_progress()

  -- enable undo
  undo_enable(sprite.undo)
  return sprite
end

local sprite = MakeSpiral(_rgba(255,255,255,255), _rgba(0,0,0,255))
sprite_show(sprite)
