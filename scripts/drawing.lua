-- Copyright (C) 2018  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local pc = app.pixelColor

-- Image:clear
local function test_clear(colorMode)
  local i = Image(2, 2, colorMode)
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
  i:clear()  -- Image:clear() clears with 0 by default
  assert(0 == i:getPixel(0, 0))
  assert(0 == i:getPixel(1, 0))
  assert(0 == i:getPixel(0, 1))
  assert(0 == i:getPixel(1, 1))
end
test_clear(ColorMode.RGB)
test_clear(ColorMode.GRAYSCALE)
test_clear(ColorMode.INDEXED)

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
