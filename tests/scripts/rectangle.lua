-- Copyright (C) 2019-2023  Igara Studio S.A.
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
assert(rc.w == 3)               -- Short w/h form
assert(rc.h == 4)
assert(rc.width == 3)           -- Long width/height form
assert(rc.height == 4)
assert(rc.origin == Point(1, 2))
assert(rc.size == Size(3, 4))
assert(not rc.isEmpty)
assert("Rectangle{ x=1, y=2, width=3, height=4 }" == tostring(rc))

local rc2 = Rectangle(rc)
assert(rc2.x == 1)
assert(rc2.y == 2)
assert(rc2.width == 3)
assert(rc2.height == 4)

rc.x = 5
rc.y = 6
rc.width = 7
rc.height = 8
assert(rc.x == 5)
assert(rc.y == 6)
assert(rc.width == 7)
assert(rc.height == 8)

rc.origin = Point(10, 12)
assert(rc.x == 10)
assert(rc.y == 12)
assert(rc.width == 7)
assert(rc.height == 8)

rc.size = Size(32, 64)
assert(rc.x == 10)
assert(rc.y == 12)
assert(rc.width == 32)
assert(rc.height == 64)

rc = Rectangle{x=2, y=3, width=4, height=5}
assert(rc.x == 2)
assert(rc.y == 3)
assert(rc.width == 4)
assert(rc.height == 5)

rc = Rectangle{x=0, y=1, w=2, h=3}
assert(rc.x == 0)
assert(rc.y == 1)
assert(rc.w == 2)
assert(rc.h == 3)

rc = Rectangle{6, 7, 8, 9}
assert(rc.x == 6)
assert(rc.y == 7)
assert(rc.w == 8)
assert(rc.h == 9)

rc = Rectangle(Point(2, 3), Size(4, 5))
assert(rc.x == 2)
assert(rc.y == 3)
assert(rc.width == 4)
assert(rc.height == 5)

-- Rectangle:contains

local a = Rectangle{x=2, y=3, width=4, height=5}
local b = Rectangle{x=3, y=4, width=1, height=1}
assert(a:contains(b))
assert(not b:contains(a))

-- Rectangle:intersect

assert(a:intersects(b))
assert(b == a:intersect(b))

a = Rectangle{x=2, y=3, width=4, height=5}
b = Rectangle{x=3, y=4, width=4, height=5}
c = Rectangle{x=3, y=4, width=3, height=4}
assert(c == a:intersect(b))
assert(c == b:intersect(a))
assert((a & b) == c)

-- Rectangle:union

c = Rectangle{x=2, y=3, width=5, height=6}
assert(c == a:union(b))
assert(c == (a | b))
