-- Copyright (C) 2019  Igara Studio S.A.
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
assert("Point{ x=1, y=2 }" == tostring(pt))

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

pt = Point{45, 25}
assert(pt.x == 45)
assert(pt.y == 25)

pt = -pt
assert(pt.x == -45)
assert(pt.y == -25)

-- add/sub/mul/div/mod/pow/idiv

pt = Point(1, 2) + 4
pt2 = 4 + Point(1, 2)
assert(pt.x == 5)
assert(pt.y == 6)
assert(pt == pt2)

pt = Point(1, 2) + Point(3, 4)
assert(pt.x == 4)
assert(pt.y == 6)

pt = Point(3, 4) - 1
assert(pt.x == 2)
assert(pt.y == 3)

pt = Point(1, 5) - Point(3, 2)
assert(pt.x == -2)
assert(pt.y == 3)

pt = Point(6, 10) * 2
assert(pt.x == 12)
assert(pt.y == 20)

pt = Point(6, 10) / 2
assert(pt.x == 3)
assert(pt.y == 5)

pt = Point(10, 5) % 2
assert(pt.x == 0)
assert(pt.y == 1)

pt = Point(2, 5) ^ 2
assert(pt.x == 4)
assert(pt.y == 25)

pt = Point(31, 10) // 3
assert(pt.x == 10)
assert(pt.y == 3)
