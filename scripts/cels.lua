-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite(32, 32)
  for i = 1,3 do s:newFrame() end
  assert(#s.frames == 4)

  -- Layer a and b
  local a = s.layers[1]
  local b = s:newLayer()

  local ca = { s.cels[1],
               s.cels[2],
               s.cels[3] }

  local cb = { s:newCel(b, 1),
               s:newCel(b, 2, Image(8, 8), 4, 4),
               s:newCel(b, 3, Image(32, 10), 16, 10) }

  assert(cb[1].bounds == Rectangle(0, 0, 32, 32))
  assert(cb[2].bounds == Rectangle(4, 4, 8, 8))
  assert(cb[3].bounds == Rectangle(16, 10, 32, 10))

  -- Check how layer cels are updated when we delete a cel in the middle

  assert(cb[1] == b.cels[1])
  assert(cb[2] == b.cels[2])
  assert(cb[3] == b.cels[3])

  local i = 1
  for k,v in ipairs(b.cels) do
    assert(i == k)
    assert(v == b.cels[k])
    i = i+1
  end

  s:deleteCel(cb[2])
  assert(cb[1] == b.cels[1])
  assert(cb[3] == b.cels[2])
end
