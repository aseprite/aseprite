-- Copyright (C) 2022  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite(32, 32)

  local a = s:newSlice()
  local b = s:newSlice(0, 2, 8, 10)
  local c = s:newSlice{ x=0, y=0, width=32, height=32 }
  assert(a.bounds == nil)
  assert(b.bounds == Rectangle(0, 2, 8, 10))
  assert(c.bounds == Rectangle(0, 0, 32, 32))

  local bounds = { Rectangle(0, 0, 32, 32), Rectangle(0, 2, 8, 10), nil }

  local i = 1
  for k,v in ipairs(s.slices) do
    assert(i == k)
    assert(v == s.slices[k])
    assert(bounds[i] == s.slices[k].bounds)
    i = i+1
  end

  s:deleteSlice(b)
  assert(c == s.slices[1])
  assert(a == s.slices[2])

  assert(2 == #s.slices)
  app.undo()
  assert(3 == #s.slices)
  app.undo()
  assert(2 == #s.slices)
  app.undo()
  assert(1 == #s.slices)
  app.undo()
  assert(0 == #s.slices)
end
