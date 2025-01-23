-- Copyright (C) 2019-2022  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile("./test_utils.lua")

local rgba = app.pixelColor.rgba

----------------------------------------------------------------------
-- app.command.ColorQuantization

do
  -- One sprite with a background layer
  local s = Sprite(2, 2)
  app.command.BackgroundFromLayer()

  local i = s.cels[1].image
  local p = s.palettes[1]
  assert(#p == 256)
  assert(s.colorMode == ColorMode.RGB)
  app.command.ColorQuantization{ algorithm="rgb5a3" }
  assert(#p == 1)

  array_to_pixels({ rgba(255, 255, 0), rgba(255, 255, 0),
                    rgba(255, 255, 0), rgba(255, 255, 0) }, i)
  app.command.ColorQuantization{ algorithm="rgb5a3" }
  assert(#p == 1)
  assert(p:getColor(0) == Color(255, 255, 0))

  array_to_pixels({ rgba(255,   0, 0), rgba(255, 0,   0),
                    rgba(255, 255, 0), rgba(255, 255, 0) }, i)
  app.command.ColorQuantization{ algorithm="rgb5a3" }
  assert(#p == 2)
  assert(p:getColor(0) == Color(255, 0, 0))
  assert(p:getColor(1) == Color(255, 255, 0))

  array_to_pixels({ rgba(255,   0, 0), rgba(255, 0,   0),
                    rgba(255, 255, 0), rgba(0,   0, 255) }, i)
  app.command.ColorQuantization{ algorithm="rgb5a3" }
  assert(#p == 3)
  assert(p:getColor(0) == Color(255, 0, 0))
  assert(p:getColor(1) == Color(255, 255, 0))
  assert(p:getColor(2) == Color(0, 0, 255))

  -- Convert the background layer to a transparent layer

  app.command.LayerFromBackground()
  app.command.ColorQuantization{ algorithm="rgb5a3", withAlpha=false }
  assert(#p == 4) -- One extra color for transparent layer
  assert(p:getColor(0) == Color(0, 0, 0))
  assert(p:getColor(1) == Color(255, 0, 0))
  assert(p:getColor(2) == Color(255, 255, 0))
  assert(p:getColor(3) == Color(0, 0, 255))

  app.command.ColorQuantization()
  assert(#p == 4)
  assert(p:getColor(0) == Color(0, 0, 0, 0))
  assert(p:getColor(1) == Color(255, 0, 0))
  assert(p:getColor(2) == Color(255, 255, 0))
  assert(p:getColor(3) == Color(0, 0, 255))

  array_to_pixels({ rgba(0,     0, 0), rgba(255, 0,   0),
                    rgba(255,   0, 0), rgba(0,   0, 255) }, i)
  app.command.ColorQuantization{ algorithm="rgb5a3", withAlpha=false }
  assert(#p == 4)
  assert(p:getColor(0) == Color(0, 0, 0))
  assert(p:getColor(1) == Color(0, 0, 0))
  assert(p:getColor(2) == Color(255, 0, 0))
  assert(p:getColor(3) == Color(0, 0, 255))

  app.command.ColorQuantization{ algorithm="rgb5a3" }
  assert(#p == 4)
  assert(p:getColor(0) == Color(0, 0, 0, 0))
  assert(p:getColor(1) == Color(0, 0, 0))
  assert(p:getColor(2) == Color(255, 0, 0))
  assert(p:getColor(3) == Color(0, 0, 255))
end

do
  -- One sprite with a transparent layer + a background layer
  local s = Sprite(2, 2)
  local p = s.palettes[1]
  app.command.BackgroundFromLayer()
  local bg = s.cels[1].image
  local fg = s:newCel(s:newLayer(), 1).image

  assert(#s.frames == 1)
  assert(#s.layers == 2)
  assert(#s.cels == 2)

  array_to_pixels({ rgba(0, 0, 0, 0), rgba(0, 255, 0),
                    rgba(255, 0, 0), rgba(0, 0, 0, 0) }, fg)
  array_to_pixels({ rgba(0, 0, 0), rgba(0, 0, 0),
                    rgba(0, 0, 0), rgba(0, 0, 255) }, bg)

  app.command.ColorQuantization{ algorithm="rgb5a3" }
  assert(#p == 5)
  assert(p:getColor(0) == Color(0, 0, 0, 0))
  assert(p:getColor(1) == Color(0, 0, 0))
  assert(p:getColor(2) == Color(0, 255, 0))
  assert(p:getColor(3) == Color(255, 0, 0))
  assert(p:getColor(4) == Color(0, 0, 255))
end

----------------------------------------------------------------------
-- app.command.ChangePixelFormat

do
  local s = Sprite(2, 2, ColorMode.RGB)
  local p = Palette(4)
  p:setColor(0, Color(0, 0, 0))
  p:setColor(1, Color(101, 90, 200))
  p:setColor(2, Color(102, 91, 201))
  p:setColor(3, Color(103, 92, 203))
  s:setPalette(p)

  app.command.BackgroundFromLayer()

  local bg = s.cels[1].image
  array_to_pixels({ rgba(0, 0, 0), rgba(101, 90, 200),
                    rgba(102, 91, 201), rgba(103, 92, 203) }, bg)

  app.command.ChangePixelFormat{ format="indexed", rgbmap="rgb5a3" }
  -- Using the 5-bit precision of RGB5A3 will match everything with
  -- the first palette entry.
  bg = s.cels[1].image
  expect_img(bg, { 0, 1,
                   1, 1 })
  app.undo()

  app.command.ChangePixelFormat{ format="indexed", rgbmap="octree" }
  bg = s.cels[1].image
  expect_img(bg, { 0, 1,
                   2, 3 })
  app.undo()

  p:setColor(0, Color(0, 0, 0, 0))
  bg = s.cels[1].image
  array_to_pixels({ rgba(101, 90, 200, 0), rgba(101, 90, 200),
                    rgba(102, 91, 201), rgba(103, 92, 203, 0) }, bg)
  app.command.ChangePixelFormat{ format="indexed", rgbmap="octree" }
  bg = s.cels[1].image
  expect_img(bg, { 0, 1,
                   2, 0 })

  -- Test fitCriteria param
  s = Sprite(6, 2, ColorMode.RGB)
  p = Palette(4)
  p:setColor(0, Color(0, 0, 0))
  p:setColor(1, Color(255, 0, 0))
  p:setColor(2, Color(199, 0, 0))
  p:setColor(3, Color(155, 0, 0))
  s:setPalette(p)

  local img = s.cels[1].image

  array_to_pixels({
    rgba(72, 0, 0), rgba(109, 0, 0), rgba(145, 0, 0), rgba(182, 0, 0), rgba(218, 0, 0), rgba(255, 0, 0),
    rgba(72, 0, 0), rgba(109, 0, 0), rgba(145, 0, 0), rgba(182, 0, 0), rgba(218, 0, 0), rgba(255, 0, 0)}, img)

  app.command.ChangePixelFormat{ format="indexed",  fitCriteria="linearizedRGB" }

  img = s.cels[1].image
  expect_img(img, { 3, 3, 3, 2, 2, 1,
                   3, 3, 3, 2, 2, 1 })
end

----------------------------------------------------------------------
-- Tests for issue aseprite/aseprite#3207
-- Conversion RGB to INDEXED color mode, in transparent layers, always
-- picks index 0 as transparent color, even if the mask color is present
-- in the palette.

do
  local s = Sprite(2, 3, ColorMode.RGB)
  local p = Palette(4)
  p:setColor(0, Color(0, 0, 0))
  p:setColor(1, Color(255, 0, 0))
  p:setColor(2, Color(0, 0, 0, 0))
  p:setColor(3, Color(0, 255, 0))
  s:setPalette(p)

  local bg = s.cels[1].image
  array_to_pixels({ rgba(0, 0, 0),rgba(255, 0, 0),
                    rgba(255, 0, 0), rgba(0, 0, 0, 0),
                    rgba(0, 0, 0, 0), rgba(0, 0, 0, 0), }, bg)

  app.command.ChangePixelFormat{ format="indexed", rgbmap="rgb5a3" }
  -- Using the 5-bit precision of RGB5A3 will match everything with
  -- the first palette entry.
  bg = s.cels[1].image
  expect_img(bg, { 0, 1,
                   1, 2,
                   2, 2 })
  app.undo()

  app.command.ChangePixelFormat{ format="indexed", rgbmap="octree" }
  bg = s.cels[1].image
  expect_img(bg, { 0, 1,
                   1, 2,
                   2, 2 })
  app.undo()

  p:setColor(2, Color(0, 0, 255))
  s:setPalette(p)
  bg = s.cels[1].image

  app.command.ChangePixelFormat{ format="indexed", rgbmap="rgb5a3" }
  bg = s.cels[1].image
  expect_img(bg, { 2, 1,
                   1, 0,
                   0, 0 })
  app.undo()

  app.command.ChangePixelFormat{ format="indexed", rgbmap="octree" }
  bg = s.cels[1].image
  expect_img(bg, { 2, 1,
                   1, 0,
                   0, 0 })
end
