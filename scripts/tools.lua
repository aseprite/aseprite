-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

local rgba = app.pixelColor.rgba
local gray = app.pixelColor.graya

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

-- default brush is a circle of 1x1 when there is no UI
assert(app.activeBrush.type == BrushType.CIRCLE)
assert(app.activeBrush.size == 1)
assert(app.activeBrush.angle == 0)

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
  app.useTool{ color=red, layer=bgCel2.layer, frame=bgCel2.frame, points={ Point(1, 0) }}

  -- After using the tool in bgCel2, the activeFrame is the frame
  -- number 2.
  assert(bgCel2.frame == app.activeFrame)
  assert(bgCel2.frame == fgCel2.frame)

  app.activeFrame = fgCel1.frame
  app.useTool{ color=yellow, layer=fgCel1.layer, points={ Point(1, 1) }}
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

----------------------------------------------------------------------
-- draw with brushes
----------------------------------------------------------------------

function drawing_with_simple_brushes(colorMode, a, b, c)
  print("drawing_with_simple_brushes", colorMode)

  local expectedImages = {
    { 0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0 },
    { 0, 0, 0, 0,
      0, a, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0 },
    { 0, 0, 0, 0,
      0, b, b, 0,
      0, b, b, 0,
      0, 0, 0, 0 },
    { c, c, c, 0,
      c, c, c, c,
      c, c, c, c,
      0, c, c, c }
  }

  local s = Sprite(4, 4, colorMode)
  assert(s == app.activeSprite)
  assert(s.cels[1] == app.activeCel)

  function expect_cel_is_image(imageIndex)
    local a = Image(s.spec)
    a:drawSprite(s, 1, Point(0, 0))
    local b = expectedImages[imageIndex]
    expect_img(a, b)
  end

  expect_cel_is_image(1)
  app.useTool{ tool='pencil', color=a, points={ Point(1, 1) } }
  assert(#s.cels == 1)
  expect_cel_is_image(2)
  app.undo()

  expect_cel_is_image(1)
  app.useTool{ tool='pencil',
               brush=Brush{ size=2, type=BrushType.SQUARE },
               color=b, points={ Point(2, 2) } }
  expect_cel_is_image(3)
  app.undo()

  expect_cel_is_image(1)
  app.useTool{ tool='pencil',
               brush=Brush{ size=2, type=BrushType.SQUARE, center=Point(0, 0) },
               color=b, points={ Point(1, 1) } }
  expect_cel_is_image(3)
  app.undo()

  expect_cel_is_image(1)
  app.useTool{ tool='line',
               brush={ size=3, type=BrushType.SQUARE },
               color=c, points={ Point(1, 1), Point(2, 2) } }
  expect_cel_is_image(4)
  app.undo()

end

do
  drawing_with_simple_brushes(ColorMode.RGB, red.rgbaPixel, blue.rgbaPixel, yellow.rgbaPixel)
  drawing_with_simple_brushes(ColorMode.GRAY, gray(255), gray(128), gray(32))
  drawing_with_simple_brushes(ColorMode.INDEXED, 1, 2, 3)
end

----------------------------------------------------------------------
-- draw with special image brushes + patterns
----------------------------------------------------------------------

function drawing_with_image_brushes(imageColorMode, colorInImage,
                                    brushColorMode, colorInBrush, palette)
  print("drawing_with_image_brushes", imageColorMode, brushColorMode)
  local s = Sprite(4, 4, imageColorMode)
  local c = colorInImage
  cel = s.cels[1]

  if palette then
    s:setPalette(palette)
  end

  -- Brush image with BrushPattern.ORIGIN
  local bi = Image(2, 2, brushColorMode)
  bi:clear(0)
  bi:putPixel(0, 0, colorInBrush)
  bi:putPixel(1, 1, colorInBrush)
  local b = Brush { image=bi,
                    center=Point(0, 0),
                    pattern=BrushPattern.ORIGIN,
                    patternOrigin=Point(0, 0) }

  expect_img(app.activeImage,
             { 0, 0, 0, 0,
               0, 0, 0, 0,
               0, 0, 0, 0,
               0, 0, 0, 0 })

  app.useTool{ tool=pencil, brush=b, points={ Point(0, 0) } }
  assert(cel.bounds == Rectangle(0, 0, 2, 2))
  expect_img(app.activeImage,
             { c, 0,
               0, c })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(0, 0), Point(1, 1) } }
  assert(cel.bounds == Rectangle(0, 0, 3, 3))
  expect_img(app.activeImage,
             { c, 0, 0,
               0, c, 0,
               0, 0, c })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(0, 1) } }
  assert(cel.bounds == Rectangle(0, 1, 2, 2))
  expect_img(app.activeImage,
             { 0, c,
               c, 0 })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(0, 0), Point(2, 0),
                                              Point(0, 0), Point(0, 1) } }
  assert(cel.bounds == Rectangle(0, 0, 4, 3))
  expect_img(app.activeImage,
             { c, 0, c, 0,
               0, c, 0, c,
               c, 0, 0, 0 })
  app.undo()

  app.useTool{ tool='paint_bucket', brush=b, points={ Point(0, 0) } }
  assert(cel.bounds == Rectangle(0, 0, 4, 4))
  expect_img(app.activeImage,
             { c, 0, c, 0,
               0, c, 0, c,
               c, 0, c, 0,
               0, c, 0, c })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(1, 0) } }
  assert(app.activeImage ~= nil)
  assert(cel.bounds == Rectangle(1, 0, 2, 2))
  expect_img(app.activeImage,
             { 0, c,
               c, 0 })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(1, 0),
                                              Point(1, 0)} }
  assert(app.activeImage ~= nil)
  assert(cel.bounds == Rectangle(1, 0, 2, 2))
  expect_img(app.activeImage,
             { 0, c,
               c, 0 })
  app.undo()

  -- Change brush pattern to BrushPattern.TARGET

  b = Brush { image=bi,
              center=Point(0, 0),
              pattern=BrushPattern.TARGET,
              patternOrigin=Point(0, 0) }

  app.useTool{ tool=pencil, brush=b, points={ Point(1, 0) } }
  assert(cel.bounds == Rectangle(1, 0, 2, 2))
  expect_img(app.activeImage,
             { c, 0,
               0, c })
  app.undo()

end

do
  drawing_with_image_brushes(ColorMode.RGB, rgba(255, 0, 0),
                             ColorMode.RGB, rgba(255, 0, 0))
end

----------------------------------------------------------------------
-- draw with symmetry
----------------------------------------------------------------------

function drawing_with_symmetry(imageColorMode, colorInImage,
			       brushColorMode, colorInBrush, palette)
  print("drawing_with_symmetry", imageColorMode, brushColorMode)
  local s = Sprite(8, 3, imageColorMode)
  local c = colorInImage
  cel = s.cels[1]

  if palette then
    s:setPalette(palette)
  end

  -- Enable symmetry
  local pref = app.preferences
  local docPref = pref.document(s)
  pref.symmetry_mode.enabled = true
  docPref.symmetry.mode   = 1 -- TODO use SymmetryMode.HORIZONTAL when it's available
  docPref.symmetry.x_axis = 4

  expect_eq(cel.bounds, Rectangle(0, 0, 8, 3))
  expect_img(cel.image,
             { 0, 0, 0, 0, 0, 0, 0, 0,
	       0, 0, 0, 0, 0, 0, 0, 0,
	       0, 0, 0, 0, 0, 0, 0, 0 })

  local b = Brush { size=1 }
  app.fgColor = c
  app.useTool{ tool=pencil, brush=b, points={ Point(0, 0) } }
  expect_eq(cel.bounds, Rectangle(0, 0, 8, 1))
  expect_img(cel.image,
             { c, 0, 0, 0, 0, 0, 0, c })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(2, 0) } }
  expect_eq(cel.bounds, Rectangle(2, 0, 4, 1))
  expect_img(cel.image,
             { c, 0, 0, c })
  app.undo()

  -- Brush size 2x2 center=(1,1)
  b = Brush { size=2 }
  assert(b.center.x == 1)
  assert(b.center.y == 1)
  app.useTool{ tool=pencil, brush=b, points={ Point(1, 1) } }
  expect_eq(cel.bounds, Rectangle(0, 0, 8, 2))
  expect_img(cel.image,
             { c, c, 0, 0, 0, 0, c, c,
               c, c, 0, 0, 0, 0, c, c })
  app.undo()

  -- Brush size 2x2 center=(0,0)
  b = Brush { size=2, center=Point(0, 0) }
  assert(b.center.x == 0)
  assert(b.center.y == 0)
  app.useTool{ tool=pencil, brush=b, points={ Point(1, 0) } }
  expect_eq(cel.bounds, Rectangle(1, 0, 6, 2))
  expect_img(cel.image,
             { c, c, 0, 0, c, c,
               c, c, 0, 0, c, c })
  app.undo()

  -- Brush size 3x3
  b = Brush { size=3 }
  app.useTool{ tool=pencil, brush=b, points={ Point(1, 1) } }
  expect_eq(cel.bounds, Rectangle(0, 0, 8, 3))
  expect_img(cel.image,
             { 0, c, 0, 0, 0, 0, c, 0,
	       c, c, c, 0, 0, c, c, c,
               0, c, 0, 0, 0, 0, c, 0 })
  app.undo()

  -- Brush size 3x3
  b = Brush { size=3, center=Point(1, 1) }
  app.useTool{ tool=pencil, brush=b, points={ Point(2, 1) } }
  expect_eq(cel.bounds, Rectangle(1, 0, 6, 3))
  expect_img(cel.image,
             { 0, c, 0, 0, c, 0,
	       c, c, c, c, c, c,
               0, c, 0, 0, c, 0 })
  app.undo()

  -- Brush size 4x4 center=(2,2)
  b = Brush { size=4 }
  assert(b.center.x == 2)
  assert(b.center.y == 2)
  app.useTool{ tool=pencil, brush=b, points={ Point(1, 1) } }
  expect_eq(cel.bounds, Rectangle(0, 0, 8, 3))
  expect_img(cel.image,
             { c, c, c, 0, 0, c, c, c,
               c, c, c, 0, 0, c, c, c,
               c, c, 0, 0, 0, 0, c, c })
  app.undo()

  -- Brush size 4x4 center=(1,1)
  b = Brush { size=4, center=Point(1, 1) }
  app.useTool{ tool=pencil, brush=b, points={ Point(1, 0) } }
  expect_eq(cel.bounds, Rectangle(0, 0, 8, 3))
  expect_img(cel.image,
             { c, c, c, c, c, c, c, c,
               c, c, c, c, c, c, c, c,
               0, c, c, 0, 0, c, c, 0 })
  app.undo()

  -- Odd symmetry
  docPref.symmetry.x_axis = 4.5

  b = Brush { size=1 }
  app.useTool{ tool=pencil, brush=b, points={ Point(4, 0) } }
  expect_eq(cel.bounds, Rectangle(4, 0, 1, 1))
  expect_img(cel.image,
             { c })
  app.undo()

  b = Brush { size=1 }
  app.useTool{ tool=pencil, brush=b, points={ Point(3, 0) } }
  expect_eq(cel.bounds, Rectangle(3, 0, 3, 1))
  expect_img(cel.image,
             { c, 0, c })
  app.undo()

  b = Brush { size=2 }
  app.useTool{ tool=pencil, brush=b, points={ Point(2, 0) } }
  expect_eq(cel.bounds, Rectangle(1, 0, 7, 1))
  expect_img(cel.image,
             { c, c, 0, 0, 0, c, c })
  app.undo()

end

do
  drawing_with_symmetry(ColorMode.RGB, rgba(255, 0, 0),
			ColorMode.RGB, rgba(255, 0, 0))
end
