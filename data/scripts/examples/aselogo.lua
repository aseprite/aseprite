-- ASE -- Allegro Sprite Editor
-- Copyright (C) 2001-2005, 2007 by David A. Capello

-- create an rectangle in the path
local function path_rectangle(path, x, y, w, h)
  path_moveto(path, x, y)
  path_lineto(path, x+w, y)
  path_lineto(path, x+w, y+h)
  path_lineto(path, x, y+h)
  path_close(path)
end

-- create an ellipse in the path
local function path_ellipse(path, x, y, a, b)
  a = a - 1
  b = b - 1
  path_moveto(path, x, y-b)
  path_curveto(path, x-a*4/3, y-b, x-a*4/3, y+b, x, y+b)
  path_curveto(path, x+a*4/3, y+b, x+a*4/3, y-b, x, y-b)
  path_close(path)
end

function MakeASELogo(color_bg, color_a, color_s, color_e)
  local image_a, image_s, image_e, image_bg
  local layer_a, layer_s, layer_e
  local layer_A, layer_S, layer_E
  local sprite = NewSprite(IMAGE_RGB, 128, 72)

  undo_disable(sprite.undo)

  image_bg = GetImage()
  layer_a = NewLayer() image_a = GetImage()
  layer_s = NewLayer() image_s = GetImage()
  layer_e = NewLayer() image_e = GetImage()

  -- clear the images
  image_clear(image_bg, get_color_for_image(sprite.imgtype, color_bg))

  -- "A" letter shape, a triangle
  local path_a = path_new("A")
  path_moveto(path_a, 19, 48)
  path_lineto(path_a, 49, 48)
  path_lineto(path_a, 34, 19)
  path_close(path_a)

  -- "S" letter shape, a circle
  local path_s = path_new("S")
  path_ellipse(path_s, 63, 34, 17, 17)

  -- "E" letter shape, a rectangle
  local path_e = path_new("E")
  path_rectangle(path_e, 82, 20, 24, 29)

  -- we fill the paths
  path_fill(path_a, image_a, get_color_for_image(sprite.imgtype, color_a))
  path_fill(path_s, image_s, get_color_for_image(sprite.imgtype, color_s))
  path_fill(path_e, image_e, get_color_for_image(sprite.imgtype, color_e))

  path_free(path_a)
  path_free(path_s)
  path_free(path_e)

  -- clean some areas of the paths(some slits in letters shapes)

  image_rectfill(image_a, 21, 41, 33, 41, 0) -- for A
  image_rectfill(image_s, 64, 27, 79, 27, 0) -- for S
  image_rectfill(image_s, 48, 41, 63, 41, 0)
  image_rectfill(image_e, 96, 29, 109, 29, 0) -- for E
  image_rectfill(image_e, 96, 38, 109, 38, 0)

  -- duplicate layers

  sprite_set_layer(sprite, layer_a) layer_A = DuplicateLayer()
  sprite_set_layer(sprite, layer_s) layer_S = DuplicateLayer()
  sprite_set_layer(sprite, layer_e) layer_E = DuplicateLayer()

  -- smooth parts

  sprite_set_layer(sprite, layer_a) ConvolutionMatrixRGBA("smooth-7x7")
  sprite_set_layer(sprite, layer_s) ConvolutionMatrixRGBA("smooth-7x7")
  sprite_set_layer(sprite, layer_e) ConvolutionMatrixRGBA("smooth-7x7")

  local curve = { 0,0, 86,0, 100,64, 137,212, 217,0, 255,136 }

  sprite_set_layer(sprite, layer_a) ColorCurveAlpha(curve)
  sprite_set_layer(sprite, layer_s) ColorCurveAlpha(curve)
  sprite_set_layer(sprite, layer_e) ColorCurveAlpha(curve)

  -- outlines

  sprite_set_layer(sprite, layer_A) ConvolutionMatrixRGBA("smooth-3x3")
  sprite_set_layer(sprite, layer_S) ConvolutionMatrixRGBA("smooth-3x3")
  sprite_set_layer(sprite, layer_E) ConvolutionMatrixRGBA("smooth-3x3")

  local curve = { 0,0, 232,128, 255,0 }

  sprite_set_layer(sprite, layer_A) ColorCurveAlpha(curve)
  sprite_set_layer(sprite, layer_S) ColorCurveAlpha(curve)
  sprite_set_layer(sprite, layer_E) ColorCurveAlpha(curve)

  -- final makeup
  autocrop_sprite()
  FlattenLayers()

  sprite_set_filename(sprite, "ASE logo")

  undo_enable(sprite.undo)

  -- returns the new sprite
  return sprite
end

local sprite = MakeASELogo("rgb{255,255,255}",
			   "rgb{255,0,0}", "rgb{0,100,0}", "rgb{0,0,255}")

undo_disable(sprite.undo)
sprite_quantize(sprite)
use_sprite_rgb_map(sprite)
sprite_set_imgtype(sprite, IMAGE_INDEXED, DITHERING_NONE)
restore_rgb_map()
undo_enable(sprite.undo)

sprite_show(sprite)
