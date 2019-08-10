-- Copyright (C) 2019  Igara Studio S.A.
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
