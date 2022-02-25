-- Copyright (C) 2018-2022  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local pc = app.pixelColor

-- Image:clear
local function test_clear(colorMode, transparentColor)
  local i = Image(ImageSpec{ width=2, height=2, colorMode=colorMode, transparentColor=transparentColor })
  i:putPixel(0, 0, 1)
  i:putPixel(1, 0, 2)
  i:putPixel(0, 1, 3)
  i:putPixel(1, 1, 4)
  assert(1 == i:getPixel(0, 0))
  assert(2 == i:getPixel(1, 0))
  assert(3 == i:getPixel(0, 1))
  assert(4 == i:getPixel(1, 1))
  i:clear(5)
  assert(5 == i:getPixel(0, 0))
  assert(5 == i:getPixel(1, 0))
  assert(5 == i:getPixel(0, 1))
  assert(5 == i:getPixel(1, 1))

  print("transparentColor = ", transparentColor)
  assert(transparentColor == i.spec.transparentColor)

  i:clear()  -- Image:clear() clears with i.spec.transparentColor by default
  assert(transparentColor == i:getPixel(0, 0))
  assert(transparentColor == i:getPixel(1, 0))
  assert(transparentColor == i:getPixel(0, 1))
  assert(transparentColor == i:getPixel(1, 1))
end
test_clear(ColorMode.RGB, 0)
test_clear(ColorMode.GRAYSCALE, 0)
test_clear(ColorMode.INDEXED, 0)
test_clear(ColorMode.INDEXED, 255)

-- Image:drawSprite
do
   local spr = Sprite(4, 4)
   local cel = spr.cels[1]
   cel.image:putPixel(0, 0, pc.rgba(255, 0, 0, 255))
   cel.image:putPixel(1, 0, pc.rgba(0, 255, 0, 255))
   cel.image:putPixel(0, 1, pc.rgba(0, 0, 255, 255))
   cel.image:putPixel(1, 1, pc.rgba(255, 255, 0, 255))

   local r = Image(spr.spec) -- render
   r:drawSprite(spr, 1, 0, 0)

   assert(r:getPixel(0, 0) == pc.rgba(255, 0, 0, 255))
   assert(r:getPixel(1, 0) == pc.rgba(0, 255, 0, 255))
   assert(r:getPixel(0, 1) == pc.rgba(0, 0, 255, 255))
   assert(r:getPixel(1, 1) == pc.rgba(255, 255, 0, 255))

   r = Image(3, 3, spr.colorMode)
   r:drawSprite(spr, 1, Point{x=1, y=1})
   assert(r:getPixel(0, 0) == pc.rgba(0, 0, 0, 0))
   assert(r:getPixel(1, 0) == pc.rgba(0, 0, 0, 0))
   assert(r:getPixel(2, 0) == pc.rgba(0, 0, 0, 0))
   assert(r:getPixel(0, 1) == pc.rgba(0, 0, 0, 0))
   assert(r:getPixel(1, 1) == pc.rgba(255, 0, 0, 255))
   assert(r:getPixel(2, 1) == pc.rgba(0, 255, 0, 255))
   assert(r:getPixel(0, 2) == pc.rgba(0, 0, 0, 0))
   assert(r:getPixel(1, 2) == pc.rgba(0, 0, 255, 255))
   assert(r:getPixel(2, 2) == pc.rgba(255, 255, 0, 255))
end


-- Image:drawPixel with indexed color
do
  local a = Sprite(32, 32, ColorMode.INDEXED)
  local i = a.cels[1].image
  assert(i:getPixel(0, 0) == 0)
  assert(i:getPixel(1, 0) == 0)
  assert(i:getPixel(2, 0) == 0)
  i:drawPixel(0, 0, Color{ index=2 })
  i:drawPixel(1, 0, Color{ index=3 }.index)
  i:drawPixel(2, 0, Color(4))
  assert(i:getPixel(0, 0) == 2)
  assert(i:getPixel(1, 0) == 3)
  assert(i:getPixel(2, 0) == 4)
end
