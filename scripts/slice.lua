-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite(32, 32)

  local a = s:newSlice(0, 0, 32, 32)
  assert(a.bounds == Rectangle(0, 0, 32, 32))
  assert(a.sprite == s)

  assert(a.name == "Slice")
  a.name = "Slice A"
  assert(a.name == "Slice A")

  assert(a.center == nil)
  a.center = Rectangle(2, 3, 28, 20)
  assert(a.center == Rectangle(2, 3, 28, 20))

  assert(a.pivot == nil)
  a.pivot = Point(16, 17)
  assert(a.pivot == Point(16, 17))

  a.data = "Data"
  assert(a.data == "Data")
end
