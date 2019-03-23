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

app.toolStroke{
  tool='pencil',
  color=Color{ r=0, g=0, b=0 },
  points={ Point(2, 2),
           Point(3, 2) }}
assert(cel.bounds == Rectangle(2, 2, 2, 1))

app.toolStroke{
  tool='eraser',
  points={ Point(2, 2) }}
assert(cel.bounds == Rectangle(3, 2, 1, 1))

app.toolStroke{
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
app.toolStroke{
  tool='line',
  color=red,
  points={ Point(0, 0), Point(3, 3) }}
local cel = spr.cels[1]
assert(cel.bounds == Rectangle(0, 0, 4, 4))
do
  local r = app.pixelColor.rgba(red.red, red.green, red.blue)
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

app.toolStroke{
  tool='paint_bucket',
  color=red,
  points={ Point(3, 0) }}
local cel = spr.cels[1]
do
  local r = app.pixelColor.rgba(red.red, red.green, red.blue)
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
app.toolStroke{
  tool='rectangle',
  color=blue,
  points={ Point(0, 0), Point(3, 3) }}
local cel = spr.cels[1]
do
  local r = app.pixelColor.rgba(red.red, red.green, red.blue)
  local b = app.pixelColor.rgba(blue.red, blue.green, blue.blue)
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
app.toolStroke{
  tool='ellipse',
  color=yellow,
  points={ Point(0, 0), Point(3, 3) }}
local cel = spr.cels[1]
do
  local r = app.pixelColor.rgba(red.red, red.green, red.blue)
  local b = app.pixelColor.rgba(blue.red, blue.green, blue.blue)
  local y = app.pixelColor.rgba(yellow.red, yellow.green, yellow.blue)
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
