-- Copyright (C) 2020-2021  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

app.activeTool = 'paint_bucket'
assert(app.tool.id == 'paint_bucket')
assert(app.brush.type == BrushType.CIRCLE)
assert(app.activeTool.id == 'paint_bucket')
assert(app.activeBrush.type == BrushType.CIRCLE)
assert(app.activeBrush.size == 1)
assert(app.activeBrush.angle == 0)
assert(app.preferences.tool('paint_bucket').floodfill.pixel_connectivity == 0)

local function test_paint_bucket(colorMode, a, b, c)
  local spr = Sprite(4, 4, colorMode)
  local img = app.activeImage

  array_to_pixels({ a, a, a, a,
                    a, b, b, a,
                    a, a, b, a,
                    a, a, a, b, }, img)

  app.useTool{ points={Point(0, 0)}, color=b }
  expect_img(img, { b, b, b, b,
                    b, b, b, b,
                    b, b, b, b,
                    b, b, b, b, })

  app.undo()
  -- FOUR_CONNECTED=0
  app.preferences.tool('paint_bucket').floodfill.pixel_connectivity = 0
  assert(app.preferences.tool('paint_bucket').floodfill.pixel_connectivity == 0)
  app.useTool{ points={Point(1, 1)}, color=c }
  expect_img(img, { a, a, a, a,
                    a, c, c, a,
                    a, a, c, a,
                    a, a, a, b, })

  app.undo()
  -- EIGHT_CONNECTED=1
  app.preferences.tool('paint_bucket').floodfill.pixel_connectivity = 1
  assert(app.preferences.tool('paint_bucket').floodfill.pixel_connectivity == 1)
  app.useTool{ points={Point(1, 1)}, color=c }
  expect_img(img, { a, a, a, a,
                    a, c, c, a,
                    a, a, c, a,
                    a, a, a, c, })
end

local rgba = app.pixelColor.rgba
local gray = app.pixelColor.graya
test_paint_bucket(ColorMode.RGB, rgba(0, 0, 0), rgba(128, 128, 128), rgba(255, 255, 255))
test_paint_bucket(ColorMode.GRAYSCALE, gray(0), gray(128), gray(255))
test_paint_bucket(ColorMode.INDEXED, 1, 2, 3)
