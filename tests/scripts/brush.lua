-- Copyright (C) 2019-2024  Igara Studio S.A.
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

-- Image brush
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

  -- Test copy image brushes
  local brush2 = Brush(brush)
  expect_img(brush2.image, { b, g, g, b })
  brush2:setFgColor(r)
  expect_img(brush2.image, { b, r, r, b })
  brush2:setBgColor(r)
  expect_img(brush2.image, { r, r, r, r })
  expect_img(brush.image, { b, g, g, b }) -- First brush wasn't modified
end

-- Tests with Image Brushes
-- Brush in a certain pixel format used on different sprites of
-- all available pixel formats.
do
  -- RGB sprite
  local sprRGB = Sprite(2, 2, ColorMode.RGB)
  local cel = sprRGB.cels[1]
  expect_img(cel.image, { 0, 0,
                          0, 0})
  local pal = Palette(4)
  pal:setColor(1, Color{ r=255, g=0, b=0, a=128 })
  pal:setColor(2, Color{ r=0, g=255, b=0, a=128 })
  pal:setColor(3, Color{ r=0, g=0, b=255, a=128 })
  sprRGB:setPalette(pal)

  -- Test Sprite RGB with RGB brush
  local brushImg = Image(2, 2, ColorMode.RGB)
  array_to_pixels({ pal:getColor(1), pal:getColor(2),
                    pal:getColor(3), pal:getColor(0) }, brushImg)
  local bruRGB = Brush { image=brushImg }

  app.useTool{ tool=pencil, brush=bruRGB, points={ Point(1, 1) } }
  expect_img(cel.image,
             { pal:getColor(1).rgbaPixel, pal:getColor(2).rgbaPixel,
               pal:getColor(3).rgbaPixel, pal:getColor(0).rgbaPixel })
  app.undo()

  -- Test Sprite RGB with INDEXED brush
  local brushImg = Image(2, 2, ColorMode.INDEXED)
  array_to_pixels({ 1, 2,
                    3, 0 }, brushImg)
  local bruINDEXED = Brush { image=brushImg }

  app.useTool{ tool=pencil, brush=bruINDEXED, points={ Point(1, 1) } }
  expect_img(cel.image,
             { pal:getColor(1).rgbaPixel, pal:getColor(2).rgbaPixel,
               pal:getColor(3).rgbaPixel, 0 })
  app.undo()

  -- Test Sprite RGB with GRAYSCALE brush
  local brushImg = Image(2, 2, ColorMode.GRAYSCALE)
  array_to_pixels({ Color{ gray=255, alpha=128 }, Color{ gray=128, alpha=128 },
                    Color{ gray=64, alpha=255 }, Color{ gray=0, alpha=255 } }, brushImg)
  local bruGRAYSCALE = Brush { image=brushImg }

  app.useTool{ tool=pencil, brush=bruGRAYSCALE, points={ Point(1, 1) } }
  expect_img(cel.image,
             { Color{ gray=255, alpha=128 }.rgbaPixel, Color{ gray=128, alpha=128 }.rgbaPixel,
               Color{ gray=64, alpha=255 }.rgbaPixel, Color{ gray=0, alpha=255 }.rgbaPixel })

  -- -- -- -- -- -- --
  -- INDEXED sprite
  local sprINDEXED = Sprite(2, 2, ColorMode.INDEXED)
  local cel = sprINDEXED.cels[1]
  expect_img(cel.image, { 0, 0,
                          0, 0 })
  local pal = Palette(4)
  pal:setColor(1, Color{ r=255, g=0, b=0, a=128 })
  pal:setColor(2, Color{ r=0, g=255, b=0, a=128 })
  pal:setColor(3, Color{ r=0, g=0, b=255, a=128 })
  sprINDEXED:setPalette(pal)

  -- Test Sprite INDEXED with RGB brush
  local brushImg = Image(2, 2, ColorMode.RGB)
  array_to_pixels({ pal:getColor(1), pal:getColor(2),
                    pal:getColor(3), app.pixelColor.rgba(0, 0, 0, 0) }, brushImg)
  local bruRGB = Brush { image=brushImg }

  app.useTool{ tool=pencil, brush=bruRGB, points={ Point(1, 1) } }
  expect_img(cel.image,
             { 1, 2,
               3, 3 })
  app.undo()

  -- Test Sprite INDEXED with INDEXED brush
  local brushImg = Image(2, 2, ColorMode.INDEXED)
  array_to_pixels({ 1, 2,
                    3, 0 }, brushImg)
  local bruINDEXED = Brush { image=brushImg }

  app.useTool{ tool=pencil, brush=bruINDEXED, points={ Point(1, 1) } }
  expect_img(cel.image,
             { 1, 2,
               3, 0 })
  app.undo()

  -- Test Sprite INDEXED with INDEXED brush
  -- (INDEXED brush with one out of bounds index)
  local brushImg = Image(2, 2, ColorMode.INDEXED)
  array_to_pixels({ 1, 5,
                    3, 0 }, brushImg)
  local bruINDEXED = Brush { image=brushImg }

  app.useTool{ tool=pencil, brush=bruINDEXED, points={ Point(1, 1) } }
  expect_img(cel.image,
             { 1, 3,
               3, 0 })
  app.undo()

  -- Test Sprite INDEXED with GRAYSCALE brush
  local brushImg = Image(2, 2, ColorMode.GRAYSCALE)
  array_to_pixels({ Color{ gray=255, alpha=128 }, Color{ gray=128, alpha=128 },
                    Color{ gray=64, alpha=255 }, Color{ gray=0, alpha=255 } }, brushImg)
  local bruGRAYSCALE = Brush { image=brushImg }

  app.useTool{ tool=pencil, brush=bruGRAYSCALE, points={ Point(1, 1) } }
  expect_img(cel.image,
             { 2, 3,
               3, 3 })

  -- -- -- -- -- -- --
  -- GRAYSCALE sprite
  local sprGRAYSCALE = Sprite(2, 2, ColorMode.GRAYSCALE)
  local cel = sprGRAYSCALE.cels[1]
  expect_img(cel.image, { 0, 0,
                          0, 0 })
  local pal = Palette(4)
  pal:setColor(1, Color{ gray=128, alpha=128 }.grayPixel)
  pal:setColor(2, Color{ gray=64, alpha=128 }.grayPixel)
  pal:setColor(3, Color{ gray=32, alpha=255 }.grayPixel)
  print(pal:getColor(1).grayPixel)
  print(pal:getColor(2).grayPixel)
  print(pal:getColor(3).grayPixel)
  sprGRAYSCALE:setPalette(pal)

  -- Test Sprite GRAYSCALE with RGB brush
  local brushImg = Image(2, 2, ColorMode.RGB)
  array_to_pixels({ Color{ r=255, g=0, b=0, a=128 }, Color{ r=0, g=255, b=0, a=128 },
                    Color{ r=0, g=0, b=255, a=128 }, app.pixelColor.rgba(0, 0, 0, 0) }, brushImg)
  local bruRGB = Brush { image=brushImg }

  app.useTool{ tool=pencil, brush=bruRGB, points={ Point(1, 1) } }
  expect_img(cel.image,
             { Color{ gray=54, alpha=128 }.grayPixel, Color{ gray=182, alpha=128 }.grayPixel,
               Color{ gray=18, alpha=128 }.grayPixel, 0 })
  app.undo()

  -- Test Sprite GRAYSCALE with INDEXED brush
  -- (INDEXED brush with out of bound index)
  local brushImg = Image(2, 2, ColorMode.INDEXED)
  array_to_pixels({ 1, 5,
                    3, 0 }, brushImg)
  local bruINDEXED = Brush { image=brushImg }

  app.useTool{ tool=pencil, brush=bruINDEXED, points={ Point(1, 1) } }
  expect_img(cel.image,
             { Color{ gray=128, alpha=128 }.grayPixel,
               Color{ gray=32, alpha=255 }.grayPixel })
  app.undo()

  -- Test Sprite GRAYSCALE with GRAYSCALE brush
  local brushImg = Image(2, 2, ColorMode.GRAYSCALE)
  array_to_pixels({ Color{ gray=128, alpha=128 }, Color{ gray=222, alpha=222 },
                    Color{ gray=32, alpha=255 }, Color{ gray=0, alpha=255 } }, brushImg)
  local bruGRAYSCALE = Brush { image=brushImg }

  app.useTool{ tool=pencil, brush=bruGRAYSCALE, points={ Point(1, 1) } }
  expect_img(cel.image,
             { pal:getColor(1).grayPixel, Color{ gray=222, alpha=222 }.grayPixel,
               pal:getColor(3).grayPixel, Color{ gray=0, alpha=255 }.grayPixel })
end
