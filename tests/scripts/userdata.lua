-- Copyright (C) 2022  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local spr = Sprite(1, 1)

  spr.properties.a = true
  spr.properties.b = 1
  spr.properties.c = "hi"
  spr.properties.d = 2.3
  assert(spr.properties.a == true)
  assert(spr.properties.b == 1)
  assert(spr.properties.c == "hi")
  -- TODO we don't have too much precision saving fixed points in properties
  assert(math.abs(spr.properties.d - 2.3) < 0.00001)

  spr.properties.a = false
  assert(spr.properties.a == false)

  spr.properties.b = nil
  assert(spr.properties.b == nil)
end
