-- Copyright (C) 2019-2022  Igara Studio S.A.
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
expect_eq(cel.bounds, Rectangle(0, 0, 4, 4))

----------------------------------------------------------------------
-- pencil and eraser
----------------------------------------------------------------------

app.useTool{
  tool='pencil',
  color=Color{ r=0, g=0, b=0 },
  points={ Point(2, 2),
           Point(3, 2) }}
expect_eq(cel.bounds, Rectangle(2, 2, 2, 1))

app.useTool{
  tool='eraser',
  points={ Point(2, 2) }}
expect_eq(cel.bounds, Rectangle(3, 2, 1, 1))

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
expect_eq(cel.bounds, Rectangle(0, 0, 4, 4))
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
  expect_eq(fgCel1.bounds, Rectangle(0, 0, 4, 4))
  expect_eq(bgCel1.bounds, Rectangle(0, 0, 4, 4))
  expect_eq(fgCel2.bounds, Rectangle(0, 0, 4, 4))
  expect_eq(bgCel2.bounds, Rectangle(0, 0, 4, 4))

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

  expect_eq(bgCel1.bounds, Rectangle(0, 0, 1, 1))
  expect_eq(bgCel2.bounds, Rectangle(1, 0, 1, 1))
  expect_eq(fgCel1.bounds, Rectangle(1, 1, 1, 1))
  expect_eq(fgCel2.bounds, Rectangle(2, 1, 1, 1))

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
  expect_eq(cel.bounds, Rectangle(0, 0, 2, 2))
  expect_img(app.activeImage,
             { c, 0,
               0, c })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(0, 0), Point(1, 1) } }
  expect_eq(cel.bounds, Rectangle(0, 0, 3, 3))
  expect_img(app.activeImage,
             { c, 0, 0,
               0, c, 0,
               0, 0, c })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(0, 1) } }
  expect_eq(cel.bounds, Rectangle(0, 1, 2, 2))
  expect_img(app.activeImage,
             { 0, c,
               c, 0 })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(0, 0), Point(2, 0),
                                              Point(0, 0), Point(0, 1) } }
  expect_eq(cel.bounds, Rectangle(0, 0, 4, 3))
  expect_img(app.activeImage,
             { c, 0, c, 0,
               0, c, 0, c,
               c, 0, 0, 0 })
  app.undo()

  app.useTool{ tool='paint_bucket', brush=b, points={ Point(0, 0) } }
  expect_eq(cel.bounds, Rectangle(0, 0, 4, 4))
  expect_img(app.activeImage,
             { c, 0, c, 0,
               0, c, 0, c,
               c, 0, c, 0,
               0, c, 0, c })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(1, 0) } }
  assert(app.activeImage ~= nil)
  expect_eq(cel.bounds, Rectangle(1, 0, 2, 2))
  expect_img(app.activeImage,
             { 0, c,
               c, 0 })
  app.undo()

  app.useTool{ tool=pencil, brush=b, points={ Point(1, 0),
                                              Point(1, 0)} }
  assert(app.activeImage ~= nil)
  expect_eq(cel.bounds, Rectangle(1, 0, 2, 2))
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
  expect_eq(cel.bounds, Rectangle(1, 0, 2, 2))
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

----------------------------------------------------------------------
-- useTool in a transaction
----------------------------------------------------------------------

do
  local s = Sprite(2, 2)
  local r = red.rgbaPixel
  local y = yellow.rgbaPixel
  app.fgColor = r

  local cel = s.cels[1]
  expect_img(cel.image, { 0, 0,
                          0, 0 })
  app.transaction(
    function()
      app.useTool{ tool=pencil,
                   color=r, brush=Brush{ size=1 },
                   points={ Point(0, 0) } }
      app.useTool{ tool=pencil,
                   color=y, brush=Brush{ size=1 },
                   points={ Point(1, 1) } }
    end)
  expect_img(cel.image, { r, 0,
                          0, y })
  app.undo() -- Undo the whole transaction (two useTool grouped in one transaction)
  expect_img(cel.image, { 0, 0,
                          0, 0 })
end

----------------------------------------------------------------------
-- draw with tiled mode + image brush
-- test for: https://community.aseprite.org/t/tile-mode-glitch/1183
----------------------------------------------------------------------

function drawing_with_tiled_mode_and_image_brush()
  print("drawing_with_tiled_mode_and_image_brush")
  local spr = Sprite(8, 3, ColorMode.INDEXED)
  local cel = spr.cels[1]

  -- enable tiled mode
  local pref = app.preferences
  local docPref = pref.document(spr)
  docPref.tiled.mode = 3 -- both

  expect_img(cel.image,
             { 0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0 })

  -- Create brush
  local brushImg = Image(5, 2, ColorMode.INDEXED)
  array_to_pixels({ 1, 2, 3, 2, 1,
                    0, 1, 2, 1, 0 }, brushImg)
  local bru = Brush { image=brushImg }

  -- Without overflow
  app.useTool{ tool=pencil, brush=bru, points={ Point(2, 1) } }
  expect_img(cel.image,
             { 1, 2, 3, 2, 1,
               0, 1, 2, 1, 0 })
  app.undo()

  -- Overflow at the left-side
  app.useTool{ tool=pencil, brush=bru, points={ Point(1, 1) } }
  expect_img(cel.image,
             { 2, 3, 2, 1, 0, 0, 0, 1,
               1, 2, 1, 0, 0, 0, 0, 0 })
  app.undo()

  -- Overflow at the right-side
  app.useTool{ tool=pencil, brush=bru, points={ Point(9, 1) } }
  expect_img(cel.image,
             { 2, 3, 2, 1, 0, 0, 0, 1,
               1, 2, 1, 0, 0, 0, 0, 0 })
  app.undo()

  -- Overflow at the top
  app.useTool{ tool=pencil, brush=bru, points={ Point(0, 0) } }
  expect_img(cel.image,
             { 2, 1, 0, 0, 0, 0, 0, 1,
               0, 0, 0, 0, 0, 0, 0, 0,
               3, 2, 1, 0, 0, 0, 1, 2 })
  app.undo()

  -- Overflow at the bottom
  app.useTool{ tool=pencil, brush=bru, points={ Point(1, 3) } }
  expect_img(cel.image,
             { 1, 2, 1, 0, 0, 0, 0, 0,
               0, 0, 0, 0, 0, 0, 0, 0,
               2, 3, 2, 1, 0, 0, 0, 1 })
  app.undo()

  docPref.tiled.mode = 0 -- none (disable tiled mode)
end
drawing_with_tiled_mode_and_image_brush()

----------------------------------------------------------------------
-- countour with pixel perfect
----------------------------------------------------------------------

do
  local s = Sprite(3, 3, ColorMode.INDEXED)
  local i = app.activeImage
  i:clear(1)
  expect_img(i,
             { 1, 1, 1,
               1, 1, 1,
               1, 1, 1 })
  app.useTool{
    tool='contour',
    brush=Brush(1),
    color=2,
    freehandAlgorithm=1, -- 1=FreehandAlgorithm.PIXEL_PERFECT
    points={ { 1, 1 }, { 2, 1 }, { 2, 2 }
    }
  }
  expect_img(app.activeImage,
             { 1, 1, 1,
               1, 2, 1,
               1, 1, 2 })

  app.undo()

  -- Test one pixel when using one point
  app.useTool{
    tool='contour',
    brush=Brush(1),
    color=2,
    freehandAlgorithm=1, -- 1=FreehandAlgorithm.PIXEL_PERFECT
    points={ { 1, 1 } }
  }
  expect_img(app.activeImage,
             { 1, 1, 1,
               1, 2, 1,
               1, 1, 1 })
  app.undo()

  -- Test bug where one click doesn't draw with the contour tool with
  -- pixel perfect algorith.
  -- Report: https://community.aseprite.org/t/13149
  expect_img(app.activeImage,
             { 1, 1, 1,
               1, 1, 1,
               1, 1, 1 })
  app.useTool{
    tool='contour',
    brush=Brush(1),
    color=2,
    freehandAlgorithm=1, -- 1=FreehandAlgorithm.PIXEL_PERFECT
    -- Two points in the same spot, this happens in the UI, one
    -- created in mouse down, other in mouse up.
    points={ { 1, 1 }, { 1, 1 } }
  }
  expect_img(app.activeImage,
             { 1, 1, 1,
               1, 2, 1,
               1, 1, 1 })
end

----------------------------------------------------------------------
-- Floodfill + 8-Connected + Stop at Grid tests
----------------------------------------------------------------------

do
  -- https://github.com/aseprite/aseprite/issues/3564
  -- 8-Connected Fill Escapes Grid With "Stop At Grid" Checked
  -- Magic Wand Test - Stop At Grid + pixel connectivity '8-connected':
  local spr2 = Sprite(9, 9, ColorMode.INDEXED)
  local p2 = spr2.palettes[1]
  p2:setColor(0, Color{ r=0, g=0, b=0 })
  p2:setColor(1, Color{ r=255, g=255, b=255 })
  p2:setColor(2, Color{ r=255, g=0, b=0 })

  -- Changing grid size:
  spr2.gridBounds = Rectangle(0, 0, 3, 3)

  -- Painting a white background
  app.useTool {
    tool='paint_bucket',
    color=1,
    points={ Point(0, 0) }
  }

  -- Configure magic wand settings
  app.command.ShowGrid()
  app.preferences.tool("magic_wand").floodfill.pixel_connectivity = 1
  app.preferences.tool("magic_wand").floodfill.stop_at_grid = 2
  app.preferences.tool("magic_wand").floodfill.refer_to = 0
  app.preferences.tool("magic_wand").contiguous = true
  app.useTool {
    tool='magic_wand',
    points={ Point(4, 4) }
  }

  -- Painting the selected area
  app.useTool {
    tool='paint_bucket',
    color=0,
    points={ Point(4, 4) }
  }

  expect_img(app.activeImage, { 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 0, 0, 0, 1, 1, 1,
                                1, 1, 1, 0, 0, 0, 1, 1, 1,
                                1, 1, 1, 0, 0, 0, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1 })
  app.undo()
  app.undo()

  -- Paint Bucket Test - Stop At Grid + pixel connectivity '8-connected':
  app.preferences.tool("paint_bucket").floodfill.pixel_connectivity = 1
  app.preferences.tool("paint_bucket").floodfill.stop_at_grid = 1
  app.preferences.tool("paint_bucket").floodfill.refer_to = 0
  app.preferences.tool("paint_bucket").contiguous = true
  app.useTool {
    tool='paint_bucket',
    color=2,
    points={ Point(4, 4) }
  }

  expect_img(app.activeImage, { 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 2, 2, 2, 1, 1, 1,
                                1, 1, 1, 2, 2, 2, 1, 1, 1,
                                1, 1, 1, 2, 2, 2, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 1, 1, 1, 1, 1, 1, 1 })

  app.command.ShowGrid()
  app.preferences.tool("magic_wand").floodfill.pixel_connectivity = 0
  app.preferences.tool("magic_wand").floodfill.stop_at_grid = 0
  app.preferences.tool("magic_wand").floodfill.refer_to = 0
  app.preferences.tool("paint_bucket").floodfill.pixel_connectivity = 0
  app.preferences.tool("paint_bucket").floodfill.stop_at_grid = 0
  app.preferences.tool("paint_bucket").floodfill.refer_to = 0
end
