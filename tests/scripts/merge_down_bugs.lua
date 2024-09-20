-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local red = Color{ r=255, g=0, b=0 }
local blue = Color{ r=0, g=0, b=255 }

-- Reproduces the bug reported in https://community.aseprite.org/t/2894
do
  local s = Sprite(32, 32)
  local a = s.layers[1]
  app.useTool{ color=red, layer=a, points={ Point(2, 2) }}

  local b = s:newLayer()
  app.useTool{ color=blue, layer=b, points={ Point(1, 1) }}

  a.isContinuous = true
  b.isContinuous = true

  app.command.NewFrame()
  app.activeLayer = b
  app.command.MergeDownLayer()

  assert(#s.cels == 2)
  assert(s.cels[1].image:isEqual(s.cels[2].image))
end

-- Checks that merge down accounts for z-index (issue #4468)
do
  local s = Sprite(32, 32)
  local a = s.layers[1]
  app.useTool{ color=red, layer=a, points={ Point(1, 1) }}

  local b = s:newLayer()
  app.useTool{ color=blue, layer=b, points={ Point(1, 1) }}

  a.cels[1].zIndex = 1
  b.cels[1].zIndex = 0

  local before = s.cels[1].image:clone()

  app.activeLayer = b
  app.command.MergeDownLayer()

  local after = s.cels[1].image
  assert(before:isEqual(after))
end
