-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

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

do -- NewLayer/RemoveLayer
  local s = Sprite(32, 32)
  assert(#s.layers == 1)
  local lay = s.layers[1]
  app.command.NewLayer{top=true}
  assert(#s.layers == 2)
  assert(s.layers[2].isImage)

  app.command.NewLayer{top=true, group=true}
  assert(#s.layers == 3)
  assert(s.layers[3].isGroup)

  app.command.RemoveLayer()
  assert(#s.layers == 2)

  app.command.RemoveLayer()
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
