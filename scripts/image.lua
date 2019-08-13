-- Copyright (C) 2019  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

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

-- Resize image
do
  local a = Sprite(3, 2)
  local cel = a.cels[1]
  assert(cel.bounds == Rectangle(0, 0, 3, 2))
  local img = cel.image
  local cols = { rgba(10, 60, 1), rgba(20, 50, 2), rgba(30, 40, 3),
                 rgba(40, 30, 4), rgba(50, 20, 5), rgba(60, 10, 6) }
  array_to_pixels(cols, img)
  expect_img(img, cols)

  -- Test resize of a cel with origin=0,0

  img:resize(img.width*2, img.height*2)

  expect_eq(cel.bounds, Rectangle(0, 0, 6, 4))
  expect_eq(img.width, 6)
  expect_eq(img.height, 4)
  local cols2 = { cols[1],cols[1], cols[2],cols[2], cols[3],cols[3],
                  cols[1],cols[1], cols[2],cols[2], cols[3],cols[3],
                  cols[4],cols[4], cols[5],cols[5], cols[6],cols[6],
                  cols[4],cols[4], cols[5],cols[5], cols[6],cols[6] }
  expect_img(img, cols2)

  -- Undo
  function undo()
    app.undo()
    img = cel.image -- TODO img shouldn't be invalidated, the resize operation should kept the image ID
  end
  undo()

  -- Test a resize when cel origin > 0,0
  cel.position = Point(2, 1)
  expect_eq(cel.bounds, Rectangle(2, 1, 3, 2))
  expect_img(img, cols)
  img:resize{ width=6, height=4 }
  expect_eq(cel.bounds, Rectangle(2, 1, 6, 4)) -- Position is not modified
  expect_eq(img.width, 6)
  expect_eq(img.height, 4)

  undo()
  img:resize{ size=Size(6, 4), pivot=Point(1, 1) }
  expect_eq(cel.bounds, Rectangle(1, 0, 6, 4))

  undo()
  img:resize{ size=Size(6, 4), pivot=Point(3, 2) }
  expect_eq(cel.bounds, Rectangle(-1, -1, 6, 4))

  -- Test resize without cel

  local img2 = Image(img)
  expect_img(img2, cols2)
  img2:resize(3, 2)
  expect_img(img2, cols)
end
