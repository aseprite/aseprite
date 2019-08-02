-- Copyright (C) 2019  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

do -- Undo/Redo commands (like app.undo/redo())
  local s = Sprite(32, 32)
  assert(s.width == 32)
  assert(s.height == 32)

  s:resize(50, 40)
  assert(s.width == 50)
  assert(s.height == 40)

  app.command.Undo()
  assert(s.width == 32)
  assert(s.height == 32)

  app.command.Redo()
  assert(s.width == 50)
  assert(s.height == 40)
end

do -- NewSprite
  local s1 = app.activeSprite
  app.command.NewFile{ }
  assert(s1 == app.activeSprite)
  app.command.NewFile{ width=256, height=128, colorMode=ColorMode.INDEXED }
  local s2 = app.activeSprite
  assert(s1 ~= s2)
  assert(s2.width == 256)
  assert(s2.height == 128)
  assert(s2.colorMode == ColorMode.INDEXED)
end

do -- NewLayer/RemoveLayer
  local s = Sprite(32, 32)
  assert(#s.layers == 1)
  local lay = s.layers[1]
  app.command.NewLayer{top=true}
  local lay2 = app.activeLayer
  assert(#s.layers == 2)
  assert(s.layers[2].isImage)

  app.command.NewLayer{top=true, group=true}
  local lay3 = app.activeLayer
  assert(#s.layers == 3)
  assert(s.layers[3].isGroup)

  assert(app.activeLayer == lay3)
  app.command.RemoveLayer()
  assert(app.activeLayer == lay2)
  assert(#s.layers == 2)

  app.command.RemoveLayer()
  assert(app.activeLayer == lay)
  assert(#s.layers == 1)
end

do -- Background/Transparent layers
  local s = Sprite(32, 32)
  assert(s.layers[1].isTransparent)
  assert(s.cels[1].image:getPixel(0, 0) == app.pixelColor.rgba(0, 0, 0, 0))

  app.bgColor = Color(32, 64, 128)
  app.command.BackgroundFromLayer() -- the layer will be filled with app.bgColor
  assert(s.layers[1].isBackground)
  assert(s.layers[1] == s.backgroundLayer)
  assert(s.cels[1].image:getPixel(0, 0) == app.pixelColor.rgba(32, 64, 128, 255))

  app.command.LayerFromBackground()
  assert(s.layers[1].isTransparent)
  assert(s.cels[1].image:getPixel(0, 0) == app.pixelColor.rgba(32, 64, 128, 255))
end

do -- Crop and Trim
  local s = Sprite(32, 32)
  s.selection:select(4, 5, 8, 10)
  assert(s.cels[1].bounds == Rectangle(0, 0, 32, 32))

  -- Crop

  app.command.CropSprite()
  assert(s.width == 8)
  assert(s.height == 10)
  assert(s.cels[1].bounds == Rectangle(-4, -5, 32, 32))

  -- Autocrop (Trim)

  app.command.AutocropSprite() -- Trim does nothing when we should remove all pixels
  assert(s.width == 8)
  assert(s.height == 10)

  s.cels[1].image:putPixel(5, 5, Color(255, 0, 0))
  s.cels[1].image:putPixel(4, 6, Color(255, 0, 0))
  app.command.AutocropSprite()
  assert(s.width == 2)
  assert(s.height == 2)
end

do -- Cel Opacity
  local s = Sprite(32, 32)
  local c = s.cels[1]
  assert(c.opacity == 255)

  app.command.CelOpacity{opacity=128}
  assert(c.opacity == 128)

  s.cels[1].opacity = 255
  assert(c.opacity == 255)

  app.undo()
  assert(c.opacity == 128)
  app.undo()
  assert(c.opacity == 255)
end

do -- PaletteSize
  local s = Sprite(32, 32)
  assert(#s.palettes[1] == 256)
  app.command.PaletteSize{ size=32 }
  assert(#s.palettes[1] == 32)
  app.command.PaletteSize{ size=8 }
  assert(#s.palettes[1] == 8)
end

do -- CanvasSize
  local s = Sprite(32, 32)
  assert(s.bounds == Rectangle(0, 0, 32, 32))
  app.command.CanvasSize{ left=2 }
  assert(s.bounds == Rectangle(0, 0, 34, 32))
  app.command.CanvasSize{ top=2, right=4, bottom=8 }
  assert(s.bounds == Rectangle(0, 0, 38, 42))
end

do -- ReplaceColor
  local s = Sprite(4, 4)
  local cel = app.activeCel
  local red = Color(255, 0, 0)
  local yellow = Color(255, 255, 0)
  local blue = Color(0, 0, 255)
  local r = red.rgbaPixel
  local y = yellow.rgbaPixel
  local b = blue.rgbaPixel

  expect_eq(cel.bounds, Rectangle(0, 0, 4, 4))
  expect_img(cel.image,
             { 0, 0, 0, 0,
               0, 0, 0, 0,
               0, 0, 0, 0,
               0, 0, 0, 0 })

  app.useTool{ brush=Brush(1), color=red, points={Point(1,1)} }
  app.useTool{ brush=Brush(1), color=yellow, points={Point(2,2)} }
  expect_eq(cel.bounds, Rectangle(1, 1, 2, 2))
  expect_img(cel.image,
             { r, 0,
               0, y })

  app.command.ReplaceColor{ from=red, to=blue }
  expect_eq(cel.bounds, Rectangle(1, 1, 2, 2))
  expect_img(cel.image,
             { b, 0,
               0, y })
end

do -- Invert
  local s = Sprite(2, 2)
  local cel = app.activeCel
  local aa = Color(255, 128, 128).rgbaPixel
  local na = Color(0, 127, 127).rgbaPixel
  local bb = Color(128, 128, 255).rgbaPixel
  local nb = Color(127, 127, 0).rgbaPixel
  local i = cel.image

  i:drawPixel(0, 0, aa)
  i:drawPixel(1, 0, bb)
  i:drawPixel(0, 1, bb)
  i:drawPixel(1, 1, aa)
  expect_img(cel.image,
             { aa, bb,
               bb, aa })

  app.command.InvertColor()
  expect_img(cel.image,
             { na, nb,
               nb, na })

  local Na = Color(0, 127, 128).rgbaPixel
  local Ng = Color(127, 127, 255).rgbaPixel
  app.command.InvertColor{ channels=FilterChannels.BLUE }
  expect_img(cel.image,
             { Na, Ng,
               Ng, Na })
end

do -- Outline
  local s = Sprite(4, 4)
  local cel = app.activeCel
  local red = Color(255, 0, 0)
  local yellow = Color(255, 255, 0)
  local blue = Color(0, 0, 255)
  local r = red.rgbaPixel
  local y = yellow.rgbaPixel
  local b = blue.rgbaPixel

  expect_eq(cel.bounds, Rectangle(0, 0, 4, 4))
  expect_img(cel.image,
             { 0, 0, 0, 0,
               0, 0, 0, 0,
               0, 0, 0, 0,
               0, 0, 0, 0 })

  app.useTool{ brush=Brush(1), color=red, points={Point(1,1)} }
  app.useTool{ brush=Brush(1), color=yellow, points={Point(2,2)} }
  expect_eq(cel.bounds, Rectangle(1, 1, 2, 2))
  expect_img(cel.image,
             { r, 0,
               0, y })

  app.command.Outline{ color=blue }
  expect_eq(cel.bounds, Rectangle(0, 0, 4, 4))
  expect_img(cel.image,
             { 0, b, 0, 0,
               b, r, b, 0,
               0, b, y, b,
               0, 0, b, 0 })

  -- Test "bgColor", "matrix", "place" params

  app.useTool{ tool='filled_rectangle', brush=Brush(1), color=blue, points={Point(0,0), Point(3,3)} }
  app.useTool{ tool='filled_rectangle', brush=Brush(1), color=yellow, points={Point(1,1), Point(2,2)} }
  expect_eq(cel.bounds, Rectangle(0, 0, 4, 4))
  expect_img(cel.image,
             { b, b, b, b,
               b, y, y, b,
               b, y, y, b,
               b, b, b, b })

  app.command.Outline{ color=red, bgColor=blue, matrix='circle' }
  expect_eq(cel.bounds, Rectangle(0, 0, 4, 4))
  expect_img(cel.image,
             { b, r, r, b,
               r, y, y, r,
               r, y, y, r,
               b, r, r, b })
  app.undo()

  app.command.Outline{ color=red, bgColor=blue, matrix='square' }
  expect_eq(cel.bounds, Rectangle(0, 0, 4, 4))
  expect_img(cel.image,
             { r, r, r, r,
               r, y, y, r,
               r, y, y, r,
               r, r, r, r })
  app.undo()

  app.command.Outline{ color=red, bgColor=blue, place='inside' }
  expect_eq(cel.bounds, Rectangle(0, 0, 4, 4))
  expect_img(cel.image,
             { b, b, b, b,
               b, r, r, b,
               b, r, r, b,
               b, b, b, b })

end

do -- BrightnessContrast
  local rgba = app.pixelColor.rgba
  local rgbaR = app.pixelColor.rgbaR
  local rgbaG = app.pixelColor.rgbaG
  local rgbaB = app.pixelColor.rgbaB
  local s = Sprite(2, 2)
  local cel = app.activeCel
  local c = { rgba(255, 128, 64), rgba(250, 225, 110),
              rgba( 30,  60,  0), rgba(200, 100,  50), }
  local i = cel.image

  i:drawPixel(0, 0, c[1])
  i:drawPixel(1, 0, c[2])
  i:drawPixel(0, 1, c[3])
  i:drawPixel(1, 1, c[4])
  expect_img(i,
             { c[1], c[2],
               c[3], c[4] })

  app.command.BrightnessContrast() -- Do nothing by default
  expect_img(i,
             { c[1], c[2],
               c[3], c[4] })

  local d = {}
  for k,v in ipairs(c) do
    d[k] = rgba(math.min(255, rgbaR(v)*1.5),
                math.min(255, rgbaG(v)*1.5),
                math.min(255, rgbaB(v)*1.5), 255)
  end
  app.command.BrightnessContrast{ brightness=50 } -- Do nothing by default
  expect_img(i,
             { d[1], d[2],
               d[3], d[4] })
end

do -- Despeckle
  local s = Sprite(5, 5)
  local white = Color(255, 255, 255)
  local red = Color(255, 0, 0)

  app.bgColor = white
  app.command.BackgroundFromLayer()

  local cel = app.activeCel
  local i = cel.image
  local b = white.rgbaPixel
  local c = red.rgbaPixel
  expect_img(i,
             { b,b,b,b,b,
               b,b,b,b,b,
               b,b,b,b,b,
               b,b,b,b,b,
               b,b,b,b,b })

  app.useTool{ tool='filled_rectangle', brush=Brush(1), color=red,
               points={Point(1,1),Point(3,3)} }
  expect_img(i,
             { b,b,b,b,b,
               b,c,c,c,b,
               b,c,c,c,b,
               b,c,c,c,b,
               b,b,b,b,b })

  app.command.Despeckle()
  expect_img(i,
             { b,b,b,b,b,
               b,b,c,b,b,
               b,c,c,c,b,
               b,b,c,b,b,
               b,b,b,b,b })

  app.command.Despeckle()
  expect_img(i,
             { b,b,b,b,b,
               b,b,b,b,b,
               b,b,c,b,b,
               b,b,b,b,b,
               b,b,b,b,b })
end

do -- HueSaturation
  local s = Sprite(1, 1)
  local cel = app.activeCel
  local i = cel.image
  local b = Color(255, 0, 0).rgbaPixel

  i:drawPixel(0, 0, b)
  expect_img(i, { b })

  app.command.HueSaturation{ hue=360 } -- Do nothing (change hue to a full 360 circle)
  expect_img(i, { b })

  app.command.HueSaturation{ hue=60 }
  b = Color(255, 255, 0).rgbaPixel
  expect_img(i, { b })

  app.command.HueSaturation{ hue=60 }
  b = Color(0, 255, 0).rgbaPixel
  expect_img(i, { b })

  app.command.HueSaturation{ saturation=-50 }
  b = Color(64, 191, 64).rgbaPixel
  expect_img(i, { b })

  app.undo()
  app.command.HueSaturation{ saturation=-100 }
  b = Color(128, 128, 128).rgbaPixel
  expect_img(i, { b })

  app.undo()
  app.command.HueSaturation{ saturation=-50, mode='hsv' }
  b = Color(128, 255, 128).rgbaPixel
  expect_img(i, { b })

  app.undo()
  app.command.HueSaturation{ value=75 }
  b = Color(191, 255, 191).rgbaPixel
  expect_img(i, { b })

  app.undo()
  app.command.HueSaturation{ alpha=-50 }
  b = Color(0, 255, 0, 127).rgbaPixel
  expect_img(i, { b })

end

do -- ColorCurve
  local rgba = app.pixelColor.rgba
  local s = Sprite(2, 1)
  local cel = app.activeCel
  local i = cel.image

  i:drawPixel(0, 0, rgba(255, 128, 0))
  i:drawPixel(1, 0, rgba(64, 0, 32))
  expect_img(i, { rgba(255, 128, 0), rgba(64, 0, 32) })

  app.command.ColorCurve() -- Do nothing
  expect_img(i, { rgba(255, 128, 0), rgba(64, 0, 32) })

  app.command.ColorCurve{ curve={{0,0},{255,128}} }
  expect_img(i, { rgba(128, 64, 0), rgba(32, 0, 16) })

  app.command.ColorCurve{ channels=FilterChannels.ALPHA, curve={{0,0},{255,128}} }
  expect_img(i, { rgba(128, 64, 0, 128), rgba(32, 0, 16, 128) })

  app.command.ColorCurve{ channels=FilterChannels.RGBA, curve={{0,255},{255,255}} }
  expect_img(i, { rgba(255, 255, 255), rgba(255, 255, 255) })

  app.command.ColorCurve{ channels=FilterChannels.GREEN, curve={{0,0},{255,0}} }
  expect_img(i, { rgba(255, 0, 255), rgba(255, 0, 255) })

  app.command.ColorCurve{ channels=FilterChannels.BLUE, curve="0,128,255,128" }
  expect_img(i, { rgba(255, 0, 128), rgba(255, 0, 128) })
end

do -- ConvolutionMatrix
  local rgba = app.pixelColor.rgba
  local s = Sprite(3, 3)
  local cel = app.activeCel
  local i = cel.image
  local b = rgba(0, 0, 0)
  local w = rgba(255, 255, 255)

  app.bgColor = Color(255, 255, 255)
  app.command.BackgroundFromLayer()
  i:drawPixel(1, 1, b)
  expect_img(i, { w, w, w,
                  w, b, w,
                  w, w, w })

  local u = rgba(239, 239, 239)
  local v = rgba(223, 223, 223)
  local w = rgba(191, 191, 191)
  app.command.ConvolutionMatrix{ fromResource="blur-3x3" }
  expect_img(i, { u, v, u,
                  v, w, v,
                  u, v, u })
end
