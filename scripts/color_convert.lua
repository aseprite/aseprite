-- Copyright (C) 2019-2022  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

-- this tests and ensures that the alpha-ness of a background
-- layer is maintained when sprite conversion takes place from
-- Indexed to RGB

do
  local sprite = Sprite(2, 2, ColorMode.INDEXED)
  local pal = sprite.palettes[1]

  assert(sprite.layers[1].isTransparent)
  assert(sprite.colorMode == ColorMode.INDEXED)
  assert(#pal == 256)

  local color = pal:getColor(3)
  color.alpha = 200
  pal:setColor(3, color)

  local image = sprite.cels[1].image
  image:drawPixel(0, 0, pal:getColor(2))
  image:drawPixel(0, 1, pal:getColor(3))
  image:drawPixel(1, 0, pal:getColor(4))
  image:drawPixel(1, 1, pal:getColor(5))

  app.bgColor = Color{ index=0 }
  app.command.BackgroundFromLayer()

  local layer = sprite.layers[1]
  assert(layer.isTransparent == false)
  assert(layer.name == "Background")

  -- change color mode from Indexed to RGB
  app.command.ChangePixelFormat {
    format="rgb"
  }

  assert(sprite.colorMode == ColorMode.RGB)
  image = sprite.cels[1].image

  for x=0, 1, 1 do
    for y=0, 1, 1 do
      local pixel = image:getPixel(x, y)
      assert(app.pixelColor.rgbaA(pixel) == 255)
    end
  end
end
