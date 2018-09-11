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

  s:deleteSlice(b)
  assert(a == s.slices[1])
  assert(c == s.slices[2])
end
