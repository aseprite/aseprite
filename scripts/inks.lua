-- Copyright (C) 2020  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

local pencil = "pencil"

function test_inks(colorMode)
  -- Test ink over a transparent sprite
  local s = Sprite(3, 3, colorMode)
  local p, a, b, c, d
  if colorMode == ColorMode.GRAY then
    local function gray(g)
      return app.pixelColor.graya(g, 255)
    end
    p = s.palettes[1]
    a, b, c, d = gray(0), gray(64), gray(128), gray(255)
  else
    p = Palette()
    p:resize(4)
    p:setColor(0, Color(0, 0, 0))
    p:setColor(1, Color(64, 64, 64))
    p:setColor(2, Color(128, 128, 128))
    p:setColor(3, Color(255, 255, 255))
    s:setPalette(p)

    a, b, c, d = 0, 1, 2, 3
    if colorMode == ColorMode.RGB then
      a = p:getColor(a).rgbaPixel
      b = p:getColor(b).rgbaPixel
      c = p:getColor(c).rgbaPixel
      d = p:getColor(d).rgbaPixel
    end
  end

  -- With simple ink opacity doesn't have affect (always the color)
  local opacities = { 0, 128, 255 }
  for i = 1,#opacities do
    expect_img(app.activeImage,
               { 0, 0, 0,
                 0, 0, 0,
                 0, 0, 0 })
    app.useTool{ tool=pencil, color=d, points={ Point(0, 0), Point(2, 2) },
                 ink=Ink.SIMPLE, opacity=opacities[i] }
    expect_img(app.activeImage,
               { d, 0, 0,
                 0, d, 0,
                 0, 0, d })
    if i < #opacities then app.undo() end
  end

  -- Check that painting with transparent index (color) on a
  -- transparent layer (using any value of opacity) with alpha
  -- compositing doesn't modify pixels
  for i = 1,#opacities do
    app.useTool{ tool=pencil, color=0, points={ Point(1, 1) },
                 ink=Ink.ALPHA_COMPOSITING, opacity=opacities[i] }
    expect_img(app.activeImage,
               { d, 0, 0,
                 0, d, 0,
                 0, 0, d })
  end

  -- Convert to background layer
  app.command.BackgroundFromLayer()

  app.useTool{ tool=pencil, color=d, points={ Point(0, 1), Point(2, 1) },
               ink=Ink.ALPHA_COMPOSITING, opacity=64 }
  expect_img(app.activeImage,
             { d, a, a,
               b, d, b,
               a, a, d })

  app.useTool{ tool=pencil, color=d, points={ Point(0, 1) },
               ink=Ink.ALPHA_COMPOSITING, opacity=86 }
  expect_img(app.activeImage,
             { d, a, a,
               c, d, b,
               a, a, d })
end

function test_alpha_compositing_on_indexed_with_full_opacity_and_repeated_colors_in_palette()
  local s = Sprite(1, 1, ColorMode.INDEXED)
  local p = Palette()
  p:resize(5)
  p:setColor(0, Color(0, 0, 0))
  p:setColor(1, Color(64, 64, 64))
  p:setColor(2, Color(128, 128, 128))
  p:setColor(3, Color(128, 128, 128))
  p:setColor(4, Color(255, 255, 255))
  s:setPalette(p)

  app.command.BackgroundFromLayer()

  local inks = { Ink.SIMPLE, Ink.ALPHA_COMPOSITING }
  for i = 1,2 do
    for c = 0,4 do
      expect_img(app.activeImage, { 0 })
      app.useTool{ tool="pencil", color=c, points={ Point(0, 0) },
                   ink=inks[i], opacity=255 }
      expect_img(app.activeImage, { c })
      app.undo()
    end
  end
end

test_inks(ColorMode.RGB)
test_inks(ColorMode.GRAY)
test_inks(ColorMode.INDEXED)
test_alpha_compositing_on_indexed_with_full_opacity_and_repeated_colors_in_palette()
