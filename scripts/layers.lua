-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite(32, 32)
  assert(#s.layers == 1)

  local a = s.layers[1]
  local b = s:newLayer()
  local c = s:newLayer()

  assert(#s.layers == 3)
  assert(s.layers[1] == a)
  assert(s.layers[2] == b)
  assert(s.layers[3] == c)

  s:deleteLayer(b)
  assert(#s.layers == 2)
  assert(s.layers[1] == a)
  assert(s.layers[2] == c)
end
