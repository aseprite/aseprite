-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local p = Palette()
assert(#p == 256)
for i = 0,#p-1 do
   assert(p:getColor(i) == Color(0, 0, 0))
end

p = Palette(32)
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
