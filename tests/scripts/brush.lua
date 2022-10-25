-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

do
  local b = Brush()
  assert(b.type == BrushType.CIRCLE)
  assert(b.size == 1)
  assert(b.angle == 0)
  assert(b.center == Point{ x=0, y=0 })
  assert(b.pattern == BrushPattern.NONE)
  assert(b.patternOrigin == Point{ x=0, y=0 })

  local c = Brush(3)
  assert(c.type == BrushType.CIRCLE)
  assert(c.size == 3)
  assert(c.center == Point{ x=1, y=1 })

  local d = Brush(4)
  assert(d.type == BrushType.CIRCLE)
  assert(d.size == 4)
  assert(d.center == Point{ x=2, y=2 })
end

do
  local b = Brush{ type=BrushType.SQUARE, size=4 }
  assert(b.type == BrushType.SQUARE)
  assert(b.size == 4)
  assert(b.angle == 0)
end

do
  local b = Brush{ type=BrushType.LINE, size=5, angle=90 }
  assert(b.type == BrushType.LINE)
  assert(b.size == 5)
  assert(b.angle == 90)
end

do
  local b = Brush{ type=BrushType.IMAGE, image=Image(4, 8) }
  local c = Brush{ image=Image(4, 8) }
  assert(b.type == BrushType.IMAGE)
  assert(c.type == BrushType.IMAGE)
  assert(b.image.width == 4)
  assert(b.image.height == 8)
  assert(b.pattern == BrushPattern.NONE)
  assert(b.patternOrigin.x == 0)
  assert(b.patternOrigin.y == 0)
end

do
  local rgba = app.pixelColor.rgba
  local r = rgba(255, 0, 0)
  local g = rgba(0, 128, 0)
  local b = rgba(0, 0, 255)

  local img = Image(2, 2)
  img:putPixel(0, 0, r)
  img:putPixel(1, 0, b)
  img:putPixel(0, 1, b)
  img:putPixel(1, 1, r)

  local brush = Brush{ image=img }
  expect_img(brush.image, { r, b, b, r })

  brush:setFgColor(g)
  expect_img(brush.image, { r, g, g, r })

  brush:setBgColor(b)
  expect_img(brush.image, { b, g, g, b })
end
