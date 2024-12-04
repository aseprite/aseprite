-- Copyright (C) 2019-2024  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

local rgba = app.pixelColor.rgba
local rgbaR = app.pixelColor.rgbaR
local rgbaG = app.pixelColor.rgbaG
local rgbaB = app.pixelColor.rgbaB

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

do -- ExportSpriteSheet
  local s = Sprite{ fromFile="sprites/2f-index-3x3.aseprite" }
  app.command.ExportSpriteSheet {
    type="horizontal",
    textureFilename="_test_export_spritesheet1.png",
    shapePadding=1
  }
  local i = Image{ fromFile="_test_export_spritesheet1.png" }
  expect_img(i,  {
    11,8,11,21,8,11,11,
    11,8,11,21,11,8,11,
    11,8,11,21,11,11,8,
  })

  local s = Sprite{ fromFile="sprites/4f-index-4x4.aseprite" }
  app.command.ExportSpriteSheet {
    type=SpriteSheetType.PACKED,
    textureFilename="_test_export_spritesheet2.png",
    borderPadding=1,
    shapePadding=1,
    trim=true,
  }
  local i = Image{ fromFile="_test_export_spritesheet2.png" }
  expect_img(i,  {
    0,0,0,0,0,0,0,
    0,1,0,2,0,3,0,
    0,1,0,2,0,3,0,
    0,1,0,0,0,0,0,
    0,0,0,4,4,0,0,
    0,0,0,0,0,0,0,
  })

  app.sprite = s
  app.command.ExportSpriteSheet {
    type=SpriteSheetType.PACKED,
    textureFilename="_test_export_spritesheet3.png",
    borderPadding=2,
    shapePadding=1,
    innerPadding=1,
    trim=true,
  }
  local i = Image{ fromFile="_test_export_spritesheet3.png" }
  expect_img(i,  {
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,
    0,0,0,1, 0,0,0,2, 0,0,0,3, 0,0,0,

    0,0,0,1, 0,0,0,2, 0,0,0,3, 0,0,0,
    0,0,0,1, 0,0,0,0, 0,0,0,0, 0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,

    0,0,0,0, 0,0,0,4, 4,0,0,0, 0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,
  })

  local s = Sprite{ fromFile="sprites/groups3abc.aseprite" }
  local c = app.pixelColor.rgba(75, 105, 47)
  app.command.ExportSpriteSheet {
    type=SpriteSheetType.ROWS,
    textureFilename="_test_export_spritesheet4.png",
    layer="b/b",
    trim=true,
  }
  local i = Image{ fromFile="_test_export_spritesheet4.png" }
  expect_img(i,  {
    c,c,0,
    c,c,0,
    c,c,c,
  })
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

-- MoveColors and CopyColors
do
  local s = Sprite(32, 32, ColorMode.INDEXED)
  local p = Palette(4)
  p:setColor(0, Color(0, 0, 0))
  p:setColor(1, Color(255, 0, 0))
  p:setColor(2, Color(0, 255, 0))
  p:setColor(3, Color(0, 0, 255))
  s:setPalette(p)
  assert(#app.range.colors == 0)
  app.range.colors = { 0, 2 }
  assert(#app.range.colors == 2)
  assert(app.range.colors[1] == 0)
  assert(app.range.colors[2] == 2)
  app.command.MoveColors{ before=0 }
  p = s.palettes[1]
  p:setColor(0, Color(0, 0, 0))
  p:setColor(1, Color(0, 255, 0))
  p:setColor(2, Color(255, 0, 0))
  p:setColor(3, Color(0, 0, 255))

  app.range.colors = { 0, 1 }
  assert(#app.range.colors == 2)
  assert(app.range.colors[1] == 0)
  assert(app.range.colors[2] == 1)
  app.command.CopyColors{ before=4 }
  p = s.palettes[1]
  p:setColor(0, Color(0, 0, 0))
  p:setColor(1, Color(0, 255, 0))
  p:setColor(2, Color(255, 0, 0))
  p:setColor(3, Color(0, 0, 255))
  p:setColor(4, Color(0, 0, 0))
  p:setColor(5, Color(0, 255, 0))
end

-- AddColor
do
  local s = Sprite(32, 32)
  local p = s.palettes[1]

  function testAddColor(color)
    assert(p:getColor(#p-1) ~= color)
    app.command.AddColor{ color=color }
    assert(p:getColor(#p-1) == color)
  end
  testAddColor(Color(255, 0, 0))
  testAddColor(Color(0, 255, 0))
  testAddColor(Color(0, 0, 255))

  local color = Color(128, 0, 0)
  app.preferences.color_bar.fg_color = color
  app.command.AddColor{ source="fg" }
  assert(p:getColor(#p-1) == color)

  local color = Color(0, 0, 128)
  app.preferences.color_bar.bg_color = color
  app.command.AddColor{ source="bg" }
  assert(p:getColor(#p-1) == color)
end

-- Flip
do
  local s = Sprite(4, 2, ColorMode.INDEXED)
  local i = s.cels[1].image
  array_to_pixels({ 0, 1, 2, 3,
                    4, 5, 6, 7 }, i)
  app.command.Flip{ orientation="horizontal" }
  expect_img(i, { 3, 2, 1, 0,
                  7, 6, 5, 4 })

  app.command.Flip{ orientation="vertical" }
  expect_img(i, { 7, 6, 5, 4,
                  3, 2, 1, 0 })

  s.selection:select{ 1, 0, 2, 2 }
  app.command.Flip{ orientation="horizontal", target="mask" }
  expect_img(i, { 7, 5, 6, 4,
                  3, 1, 2, 0 })

  s:newFrame()

  assert(app.activeCel.frameNumber == 2)
  local j = app.activeImage
  app.command.Flip{ orientation="vertical", target="mask" }
  expect_img(i, { 7, 5, 6, 4,
                  3, 1, 2, 0 })
  expect_img(j, { 7, 1, 2, 4,
                  3, 5, 6, 0 })

  app.range.frames = { 1, 2 }
  app.command.Flip{ orientation="horizontal", target="mask" }
  expect_img(i, { 7, 6, 5, 4,
                  3, 2, 1, 0 })
  expect_img(j, { 7, 2, 1, 4,
                  3, 6, 5, 0 })
end

-- Fill
do
  local s = Sprite(4, 2, ColorMode.INDEXED)
  local c = s.cels[1]
  local i = c.image
  i:clear(1)
  array_to_pixels({ 1, 1, 1, 1,
                    1, 1, 1, 1 }, i)
  app.fgColor = Color{ index=0 }
  s.selection = Selection(Rectangle(1, 1, 2, 1))
  app.command.Fill()
  expect_eq(Rectangle(0, 0, 4, 2), c.bounds)
  expect_img(i, { 1, 1, 1, 1,
                  1, 0, 0, 1 })

  c.position = { x=0, y=1 }
  app.fgColor = Color{ index=2 }
  s.selection = Selection(Rectangle(1, 0, 2, 2))
  app.command.Fill()
  expect_eq(Rectangle(0, 0, 4, 3), c.bounds)
  expect_img(i, { 0, 2, 2, 0,
                  1, 2, 2, 1,
                  1, 0, 0, 1 })

  app.fgColor = Color{ index=0 }
  s.selection = Selection(Rectangle(0, 0, 3, 3))
  app.command.Fill()
  expect_eq(Rectangle(3, 1, 1, 2), c.bounds)
  expect_img(i, { 1,
                  1 })

  app.undo() -- undo Fill
  expect_eq(Rectangle(0, 0, 4, 3), c.bounds)
  expect_img(i, { 0, 2, 2, 0,
                  1, 2, 2, 1,
                  1, 0, 0, 1 })

  expect_eq(Rectangle(0, 0, 3, 3), s.selection.bounds)
  app.undo() -- undo selection change
  expect_eq(Rectangle(1, 0, 2, 2), s.selection.bounds)
  app.undo() -- undo Fill
  expect_eq(Rectangle(0, 1, 4, 2), c.bounds)
  expect_img(i, { 1, 1, 1, 1,
                  1, 0, 0, 1 })
end

-- MaskByColor
do
  local s = Sprite(5, 5, ColorMode.INDEXED)
  local c = s.cels[1]
  local i = c.image
  array_to_pixels({
    1, 1, 0, 0, 1,
    1, 1, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 1, 1,
    1, 0, 0, 1, 1,
  }, i)

  app.command.MaskByColor {
    color = Color{ index=1 },
    tolerance = 0,
  }
  app.fgColor = Color{ index=2 }
  app.command.Fill()

  expect_img(i, {
    2, 2, 0, 0, 2,
    2, 2, 0, 0, 2,
    2, 0, 0, 0, 2,
    2, 0, 0, 2, 2,
    2, 0, 0, 2, 2,
  })

  -- Subtract from current selection by color
  app.command.MaskAll {}
  app.command.MaskByColor {
    color = Color{ index=2 },
    tolerance = 0,
    mode = SelectionMode.SUBTRACT,
  }

  app.fgColor = Color{ index=3 }
  app.command.Fill()

  expect_img(i, {
    2, 2, 3, 3, 2,
    2, 2, 3, 3, 2,
    2, 3, 3, 3, 2,
    2, 3, 3, 2, 2,
    2, 3, 3, 2, 2,
  })

  -- Add to current selection by color
  app.command.MaskByColor {
    color = Color{ index=2 },
    tolerance = 0,
    mode = SelectionMode.ADD,
  }
  app.fgColor = Color{ index=4 }
  app.command.Fill()

  expect_img(i, {
    4, 4, 4, 4, 4,
    4, 4, 4, 4, 4,
    4, 4, 4, 4, 4,
    4, 4, 4, 4, 4,
    4, 4, 4, 4, 4,
  })

  -- Reset image for new test
  array_to_pixels({
    1, 1, 0, 0, 1,
    1, 1, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 1, 1,
    1, 0, 0, 1, 1,
  }, i)

  -- Select a centered 3x3 square
  app.command.MaskAll {}
  app.command.ModifySelection {
    modifier = 'contract',
    quantity = 1,
    brush = 'square'
  }
  -- Intersect with current selection by color
  app.command.MaskByColor {
    color = Color{ index=1 },
    tolerance = 0,
    mode = SelectionMode.INTERSECT,
  }

  app.fgColor = Color{ index=2 }
  app.command.Fill()

  expect_img(i, {
    1, 1, 0, 0, 1,
    1, 2, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 2, 1,
    1, 0, 0, 1, 1,
  })
end
