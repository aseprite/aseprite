-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local pt = Point()
assert(pt.x == 0)
assert(pt.y == 0)

pt = Point(1, 2)
assert(pt.x == 1)
assert(pt.y == 2)

local pt2 = Point(pt)
assert(pt2.x == 1)
assert(pt2.y == 2)

pt.x = 5
pt.y = 6
assert(pt.x == 5)
assert(pt.y == 6)

pt = Point{x=10, y=20}
assert(pt.x == 10)
assert(pt.y == 20)
