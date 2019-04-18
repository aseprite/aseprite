-- Copyright (C) 2019  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local rgba = app.pixelColor.rgba

local a = Image(32, 64)
assert(a.width == 32)
assert(a.height == 64)
assert(a.colorMode == ColorMode.RGB) -- RGB by default
assert(a:isEmpty())
assert(a:isPlain(rgba(0, 0, 0, 0)))
assert(a:isPlain(0))

do
  local b = Image(32, 64, ColorMode.INDEXED)
  assert(b.width == 32)
  assert(b.height == 64)
  assert(b.colorMode == ColorMode.INDEXED)

  local c = Image{ width=32, height=64, colorMode=ColorMode.INDEXED }
  assert(c.width == 32)
  assert(c.height == 64)
  assert(c.colorMode == ColorMode.INDEXED)
end

-- Get/put RGBA pixels
do
  for y=0,a.height-1 do
    for x=0,a.width-1 do
      a:putPixel(x, y, rgba(x, y, x+y, x-y))
    end
  end
  assert(not a:isEmpty())
end

-- Clone
do
  local c = Image(a)
  local d = a:clone()
  assert(c.width == 32)
  assert(c.height == 64)
  assert(c.colorMode == ColorMode.RGB)
  assert(c.width == d.width)
  assert(c.height == d.height)
  assert(c.colorMode == d.colorMode)

  -- Get RGB pixels
  for y=0,c.height-1 do
    for x=0,c.width-1 do
      local expectedColor = rgba(x, y, x+y, x-y)
      assert(c:getPixel(x, y) == expectedColor)
      assert(d:getPixel(x, y) == expectedColor)
    end
  end
end

-- Patch
do
  local spr = Sprite(256, 256)
  local image = app.site.image
  local copy = image:clone()
  assert(image:getPixel(0, 0) == 0)
  for y=0,copy.height-1 do
    for x=0,copy.width-1 do
      copy:putPixel(x, y, rgba(255-x, 255-y, 0, 255))
    end
  end
  image:putImage(copy)
  assert(image:getPixel(0, 0) == rgba(255, 255, 0, 255))
  assert(image:getPixel(255, 255) == rgba(0, 0, 0, 255))
  app.undo()
  assert(image:getPixel(0, 0) == rgba(0, 0, 0, 0))
  assert(image:getPixel(255, 255) == rgba(0, 0, 0, 0))
end

-- Load/Save
do
  local a = Image{ fromFile="sprites/1empty3.aseprite" }
  assert(a.width == 32)
  assert(a.height == 32)
  a:saveAs("_test_oneframe.png")

  local b = Image{ fromFile="_test_oneframe.png" }
  assert(b.width == 32)
  assert(b.height == 32)
  for y=0,a.height-1 do
    for x=0,a.width-1 do
      assert(a:getPixel(x, y) == b:getPixel(x, y))
    end
  end
end
