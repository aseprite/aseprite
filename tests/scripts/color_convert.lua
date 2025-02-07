-- Copyright (C) 2019-2024  Igara Studio S.A.
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

-- Test of mask color conversion in a opaque sprite with
-- extra non-opaque layers.
-- Conditions:
-- + There is a background layer
-- + There is an extra layer drawn
-- + The mask color is in the palette and whose index is greater than 0
-- + RGBA->INDEXED conversion
do
  local sprite = Sprite(3, 3, ColorMode.RGB)
  app.command.BackgroundFromLayer()
  local pal = sprite.palettes[1]
  local backgroundLayer = sprite.layers[1]

  assert(sprite.layers[1].isBackground)
  assert(sprite.colorMode == ColorMode.RGB)
  assert(sprite.layers[1]:cel(1).image:getPixel(0, 0) == app.pixelColor.rgba(0,0,0,255))
  assert(#pal == 256)

  pal:setColor(0, Color{ r=255, g=0  , b=0  , a=255 })
  pal:setColor(1, Color{ r=0  , g=0  , b=0  , a=0 })
  pal:setColor(2, Color{ r=0  , g=255, b=0  , a=255 })
  pal:setColor(3, Color{ r=0  , g=0  , b=255, a=255 })
  local layer = sprite:newLayer()
  app.useTool {
    tool='pencil',
    color=pal:getColor(2),
    points={ Point(0, 1), Point(1, 0) },
    layer=Layer
  }

  app.command.ChangePixelFormat {
    format="indexed"
  }

  assert(pal:getColor(1) == Color{r=0  , g=0  , b=0  , a=0})
  assert(layer:cel(1).image:getPixel(0, 0) == 1)
end
