-- Copyright (C) 2019-2020  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  -- Three layers
  local s = Sprite(32, 32)
  assert(#s.layers == 1)

  app.activeCel = s.cels[1]

  local r = app.range
  assert(#r.layers == 1)
  assert(#r.frames == 1)
  assert(#r.cels == 1)
  assert(r.layers[1] == s.layers[1])
  assert(r.frames[1] == s.frames[1])
  assert(r.cels[1] == s.cels[1])

  s:newLayer()
  assert(#s.layers == 2)

  local r = app.range
  assert(#r.layers == 1)
  assert(#r.frames == 1)
  assert(#r.cels == 0)
  assert(r.layers[1] == s.layers[2])
  assert(r.frames[1] == s.frames[1])
end

-- Test app.range.colors
do
  assert(#app.range.colors == 0)
  app.range.colors = { 2 }
  assert(#app.range.colors == 1)
  assert(app.range.colors[1] == 2)
  app.range.colors = { 1, 4 }
  assert(#app.range.colors == 2)
  assert(app.range.colors[1] == 1)
  assert(app.range.colors[2] == 4)
  app.range.colors = { 5, 2, 10, 8, 0 }
  assert(#app.range.colors == 5)
  -- app.range.colors are always sorted by color index
  assert(app.range.colors[1] == 0)
  assert(app.range.colors[2] == 2)
  assert(app.range.colors[3] == 5)
  assert(app.range.colors[4] == 8)
  assert(app.range.colors[5] == 10)
  assert(app.range:containsColor(0))
  assert(not app.range:containsColor(1))
  assert(app.range:containsColor(2))
  assert(app.range:containsColor(5))
  assert(app.range:containsColor(8))
  assert(app.range:containsColor(10))
end

-- Test setters
do
  local spr = Sprite(32, 32)
  local lay1 = spr.layers[1]
  local r = app.range
  assert(r.type == RangeType.EMPTY)
  assert(#r.layers == 1)
  assert(#r.frames == 1)
  assert(r.layers[1] == lay1)
  assert(r.frames[1].frameNumber == 1)

  local lay2 = spr:newLayer()
  r = app.range
  assert(r.type == RangeType.EMPTY)
  assert(#r.layers == 1)
  assert(#r.frames == 1)
  assert(r.layers[1] == lay2)
  assert(r.frames[1].frameNumber == 1)

  r.layers = { lay1, lay2 }
  assert(r.type == RangeType.LAYERS)
  assert(#r.layers == 2)
  assert(#r.frames == 1)
  assert(r.layers[1] == lay1)
  assert(r.layers[2] == lay2)
  assert(r.frames[1].frameNumber == 1)

  spr:newFrame()
  spr:newFrame()
  r.frames = { 1, 3 }
  assert(r.type == RangeType.FRAMES)
  assert(#r.layers == 2)
  assert(#r.frames == 2)
  assert(r.layers[1] == lay1)
  assert(r.layers[2] == lay2)
  assert(r.frames[1].frameNumber == 1)
  assert(r.frames[2].frameNumber == 3)

  r.layers = { lay2 }
  assert(r.type == RangeType.LAYERS)
  assert(#r.layers == 1)
  assert(#r.frames == 2)
  assert(r.layers[1] == lay2)
  assert(r.frames[1].frameNumber == 1)
  assert(r.frames[2].frameNumber == 3)

  -- Clear range
  r:clear()
  assert(r.type == RangeType.EMPTY)
  assert(#r.layers == 1)
  assert(#r.frames == 1)
  assert(r.layers[1] == app.activeLayer)
  assert(r.frames[1] == app.activeFrame)

  -- Check that Range:clear() reset the selected colors
  r.colors = { 2 }
  assert(#r.colors == 1)
  assert(r.colors[1] == 2)
  r:clear()
  assert(#r.colors == 0)
end
