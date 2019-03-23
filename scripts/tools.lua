-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

----------------------------------------------------------------------
-- activeTool
----------------------------------------------------------------------

local pencil = app.activeTool -- pencil is the default tool
assert(pencil ~= nil)
assert(pencil.id == 'pencil')
app.activeTool = 'line'
assert(app.activeTool.id == 'line')
app.activeTool = pencil
assert(app.activeTool.id == 'pencil')

----------------------------------------------------------------------
-- create sprite for testing
----------------------------------------------------------------------

local spr = Sprite(4, 4)
local cel = spr.cels[1]
assert(cel.bounds == Rectangle(0, 0, 4, 4))

----------------------------------------------------------------------
-- pencil and eraser
----------------------------------------------------------------------

app.useTool{
  tool='pencil',
  color=Color{ r=0, g=0, b=0 },
  points={ Point(2, 2),
           Point(3, 2) }}
assert(cel.bounds == Rectangle(2, 2, 2, 1))

app.useTool{
  tool='eraser',
  points={ Point(2, 2) }}
assert(cel.bounds == Rectangle(3, 2, 1, 1))

app.useTool{
  tool='eraser',
  points={ Point(3, 2) }}
-- This must fail because cel is pointing to an invalid cel now.
-- TODO: In a future this could change if this issue:
--         https://github.com/aseprite/aseprite/issues/1833
--       is implemented.
assert(not pcall(function() print(cel.bounds) end))

----------------------------------------------------------------------
-- line
----------------------------------------------------------------------

local red = Color{ r=255, g=0, b=0 }
app.useTool{
  tool='line',
  color=red,
  points={ Point(0, 0), Point(3, 3) }}
local cel = spr.cels[1]
assert(cel.bounds == Rectangle(0, 0, 4, 4))
do
  local r = red.rgbaPixel
  local expected = { r, 0, 0, 0,
                     0, r, 0, 0,
                     0, 0, r, 0,
                     0, 0, 0, r }
  assert(cel.image.width == 4)
  assert(cel.image.height == 4)
  for v=0,3 do
    for u=0,3 do
      print(u, v, cel.image:getPixel(u, v), expected[1+v*4+u])
      assert(cel.image:getPixel(u, v) == expected[1+v*4+u])
    end
  end
end

----------------------------------------------------------------------
-- paint_bucket
----------------------------------------------------------------------

app.useTool{
  tool='paint_bucket',
  color=red,
  points={ Point(3, 0) }}
local cel = spr.cels[1]
do
  local r = red.rgbaPixel
  local expected = { r, r, r, r,
                     0, r, r, r,
                     0, 0, r, r,
                     0, 0, 0, r }
  assert(cel.image.width == 4)
  assert(cel.image.height == 4)
  for v=0,3 do
    for u=0,3 do
      print(u, v, cel.image:getPixel(u, v), expected[1+v*4+u])
      assert(cel.image:getPixel(u, v) == expected[1+v*4+u])
    end
  end
end

----------------------------------------------------------------------
-- rectangle
----------------------------------------------------------------------

local blue = Color{ r=0, g=0, b=255 }
app.useTool{
  tool='rectangle',
  color=blue,
  points={ Point(0, 0), Point(3, 3) }}
local cel = spr.cels[1]
do
  local r = red.rgbaPixel
  local b = blue.rgbaPixel
  local expected = { b, b, b, b,
                     b, r, r, b,
                     b, 0, r, b,
                     b, b, b, b }
  assert(cel.image.width == 4)
  assert(cel.image.height == 4)
  for v=0,3 do
    for u=0,3 do
      print(u, v, cel.image:getPixel(u, v), expected[1+v*4+u])
      assert(cel.image:getPixel(u, v) == expected[1+v*4+u])
    end
  end
end

----------------------------------------------------------------------
-- ellipse
----------------------------------------------------------------------

local yellow = Color{ r=255, g=255, b=0 }
app.useTool{
  tool='ellipse',
  color=yellow,
  points={ Point(0, 0), Point(3, 3) }}
local cel = spr.cels[1]
do
  local r = red.rgbaPixel
  local b = blue.rgbaPixel
  local y = yellow.rgbaPixel
  local expected = { b, y, y, b,
                     y, r, r, y,
                     y, 0, r, y,
                     b, y, y, b }
  assert(cel.image.width == 4)
  assert(cel.image.height == 4)
  for v=0,3 do
    for u=0,3 do
      print(u, v, cel.image:getPixel(u, v), expected[1+v*4+u])
      assert(cel.image:getPixel(u, v) == expected[1+v*4+u])
    end
  end
end

----------------------------------------------------------------------
-- draw in several cels
----------------------------------------------------------------------

do
  local spr2 = Sprite(4, 4)
  spr2:newFrame()

  local bgLay = spr2.layers[1]
  local fgLay = spr2:newLayer()
  local bgCel1 = spr2:newCel(fgLay, 1, Image(spr2.spec))
  local fgCel1 = spr2:newCel(bgLay, 1, Image(spr2.spec))
  local bgCel2 = spr2:newCel(fgLay, 2, Image(spr2.spec))
  local fgCel2 = spr2:newCel(bgLay, 2, Image(spr2.spec))
  assert(fgCel1.bounds == Rectangle(0, 0, 4, 4))
  assert(bgCel1.bounds == Rectangle(0, 0, 4, 4))
  assert(fgCel2.bounds == Rectangle(0, 0, 4, 4))
  assert(bgCel2.bounds == Rectangle(0, 0, 4, 4))

  -- After each useTool(), the cels will be shrunken to the minimum
  -- required size.
  app.activeTool = 'pencil'
  app.useTool{ color=red, cel=bgCel1, points={ Point(0, 0) }}
  app.useTool{ color=red, cel=bgCel2, points={ Point(1, 0) }}
  app.useTool{ color=yellow, cel=fgCel1, points={ Point(1, 1) }}
  app.useTool{ color=yellow, cel=fgCel2, points={ Point(2, 1) }}

  assert(bgCel1.bounds == Rectangle(0, 0, 1, 1))
  assert(bgCel2.bounds == Rectangle(1, 0, 1, 1))
  assert(fgCel1.bounds == Rectangle(1, 1, 1, 1))
  assert(fgCel2.bounds == Rectangle(2, 1, 1, 1))

  assert(bgCel1.image:getPixel(0, 0) == red.rgbaPixel)
  assert(bgCel2.image:getPixel(0, 0) == red.rgbaPixel)
  assert(fgCel1.image:getPixel(0, 0) == yellow.rgbaPixel)
  assert(fgCel2.image:getPixel(0, 0) == yellow.rgbaPixel)
end
