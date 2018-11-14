-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local a = Sprite(32, 64)
  assert(a.width == 32)
  assert(a.height == 64)
  assert(a.colorMode == ColorMode.RGB) -- RGB by default
  assert(a.bounds == Rectangle{x=0, y=0, width=32, height=64})

  -- Crop and resize
  a.selection:select(2, 3, 4, 5)
  a:crop()
  assert(a.width == 4)
  assert(a.height == 5)
  a:resize(6, 8)
  assert(a.width == 6)
  assert(a.height == 8)
  a:crop{x=-1, y=-1, width=20, height=30}
  assert(a.width == 20)
  assert(a.height == 30)

  -- Resize sprite setting width/height
  a.width = 8
  a.height = 10
  assert(a.width == 8)
  assert(a.height == 10)

  local b = Sprite(4, 4, ColorMode.INDEXED)
  assert(b.width == 4)
  assert(b.height == 4)
  assert(b.colorMode == ColorMode.INDEXED)
end

-- Transparent color

do
  local a = Sprite(32, 32, ColorMode.INDEXED)
  assert(a.transparentColor == 0)
  a.transparentColor = 8
  assert(a.transparentColor == 8)
end

-- Palette

do
  local a = Sprite(32, 32, ColorMode.INDEXED)
  assert(#a.palettes == 1)
  assert(#a.palettes[1] == 256)

  local p = Palette(3)
  p:setColor(0, Color(255, 0, 0))
  p:setColor(1, Color(0, 255, 0))
  p:setColor(2, Color(0, 0, 255))
  a:setPalette(p)

  assert(#a.palettes == 1)
  assert(#a.palettes[1] == 3)
  assert(a.palettes[1]:getColor(0) == Color(255, 0, 0))
  assert(a.palettes[1]:getColor(1) == Color(0, 255, 0))
  assert(a.palettes[1]:getColor(2) == Color(0, 0, 255))
end
