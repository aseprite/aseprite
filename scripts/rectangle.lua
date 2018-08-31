-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local rc = Rectangle()
assert(rc.x == 0)
assert(rc.y == 0)
assert(rc.width == 0)
assert(rc.height == 0)
assert(rc.isEmpty)

rc = Rectangle(1, 2, 3, 4)
assert(rc.x == 1)
assert(rc.y == 2)
assert(rc.width == 3)
assert(rc.height == 4)
assert(not rc.isEmpty)

local rc2 = Rectangle(rc)
assert(rc2.x == 1)
assert(rc2.y == 2)
assert(rc2.width == 3)
assert(rc2.height == 4)

rc.x = 5;
rc.y = 6;
rc.width = 7;
rc.height = 8;
assert(rc.x == 5)
assert(rc.y == 6)
assert(rc.width == 7)
assert(rc.height == 8)

rc = Rectangle{x=2, y=3, width=4, height=5}
assert(rc.x == 2)
assert(rc.y == 3)
assert(rc.width == 4)
assert(rc.height == 5)
