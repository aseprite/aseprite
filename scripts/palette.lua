-- Copyright (C) 2019  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local p = Palette()
  assert(#p == 256)
  for i = 0,#p-1 do
    assert(p:getColor(i) == Color(0, 0, 0))
  end
end

do
  local p = Palette(32)
  assert(#p == 32)
  for i = 0,#p-1 do
    assert(p:getColor(i) == Color(0, 0, 0))
  end

  p:resize(4)
  assert(#p == 4)
  p:setColor(0, Color(255, 8, 32))
  p:setColor(1, Color(250, 4, 30))
  p:setColor(2, Color(240, 3, 20))
  p:setColor(3, Color(210, 2, 10))
  assert(p:getColor(0) == Color(255, 8, 32))
  assert(p:getColor(1) == Color(250, 4, 30))
  assert(p:getColor(2) == Color(240, 3, 20))
  assert(p:getColor(3) == Color(210, 2, 10))
end

-- Load/save
do
  local p = Palette{ fromFile="sprites/abcd.aseprite" }
  assert(#p == 5)
  assert(p:getColor(0) == Color(0, 0, 0))
  assert(p:getColor(1) == Color(25, 0, 255))
  assert(p:getColor(2) == Color(255, 0, 0))
  assert(p:getColor(3) == Color(255, 255, 0))
  assert(p:getColor(4) == Color(0, 128, 0))

  p:saveAs("_test_.gpl")

  local q = Palette{ fromFile="_test_.gpl" }
  assert(#p == #q)
  for i=0,#q-1 do
    assert(p:getColor(i) == q:getColor(i))
  end
end
