-- ASE - Allegro Sprite Editor
-- Copyright (C) 2001-2005, 2007 by David A. Capello

local function MakeDacap()
  local x, y, sprite, image, render
  local w, h = 256, 128

  sprite = sprite_new_with_layer(IMAGE_RGB, w, h)
  sprite_mount(sprite)
  set_current_sprite(sprite)

  -- disable undo
  undo_disable(sprite.undo)

  sprite_set_filename(sprite, "dacap")

  -- Background layer
  image = GetImage()
  image_clear(image, _rgba(255, 255, 255, 255))

  render = RenderText("verdana10.pcx", 22, _rgba(255,0,0,255), "David A. Capello")
  if render then
    x = image.w/2-render.w/2+2
    y = image.h/2-render.h/2+2
    image_merge(image, render, x, y, 255, BLEND_MODE_NORMAL)
    ConvolutionMatrixRGB("smooth-3x3")
    image_free(render)
  end

  -- Foreground layer
  NewLayer("Foreground", 0, 0, 0, 0)

  image = GetImage()
  image_clear(image, _rgba(0,0,0,0))

  render = RenderText("verdana10.pcx", 22, _rgba(0,0,0,255), "David A. Capello")
  if render then
    x = image.w/2-render.w/2
    y = image.h/2-render.h/2
    image_merge(image, render, x, y, 255, BLEND_MODE_NORMAL)
    image_free(render)
  end

  -- flatten both layers
  FlattenLayers()

  -- enable undo
  undo_enable(sprite.undo)
  return sprite
end

local sprite = MakeDacap()
if sprite then
  sprite_quantize(sprite)
  use_sprite_rgb_map(sprite)
  sprite_set_imgtype(sprite, IMAGE_INDEXED, DITHERING_NONE)
  restore_rgb_map()
  sprite_show(sprite)
end
