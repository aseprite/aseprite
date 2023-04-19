-- Copyright (C) 2019-2023  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local sz = Size()
assert(sz.width == 0)
assert(sz.height == 0)

sz = Size(3, 4)
assert(sz.w == 3)               -- Short w/h form
assert(sz.h == 4)
assert(sz.width == 3)           -- Long width/height form
assert(sz.height == 4)
assert("Size{ width=3, height=4 }" == tostring(sz))

local sz2 = Size(sz)
assert(sz2.width == 3)
assert(sz2.height == 4)

sz.width = 7
sz.height = 8
assert(sz.width == 7)
assert(sz.height == 8)

sz = Size{width=10, height=20}
assert(sz.width == 10)
assert(sz.height == 20)

sz = Size{w=30, h=40}
assert(sz.width == 30)
assert(sz.height == 40)

sz = Size{45, 25}
assert(sz.w == 45)
assert(sz.h == 25)

sz = -sz
assert(sz.width == -45)
assert(sz.height == -25)

-- add/sub/mul/div/mod/pow/idiv

sz = Size(1, 2) + 4
sz2 = 4 + Size(1, 2)
assert(sz.width == 5)
assert(sz.height == 6)
assert(sz == sz2)

sz = Size(1, 2) + Size(3, 4)
assert(sz.width == 4)
assert(sz.height == 6)

sz = Size(3, 4) - 1
assert(sz.width == 2)
assert(sz.height == 3)

sz = Size(8, 5) - Size(3, 2)
assert(sz.width == 5)
assert(sz.height == 3)

sz = Size(6, 10) * 2
assert(sz.width == 12)
assert(sz.height == 20)

sz = Size(6, 10) / 2
assert(sz.width == 3)
assert(sz.height == 5)

sz = Size(10, 5) % 2
assert(sz.width == 0)
assert(sz.height == 1)

sz = Size(2, 5) ^ 2
assert(sz.width == 4)
assert(sz.height == 25)

sz = Size(31, 10) // 3
assert(sz.width == 10)
assert(sz.height == 3)

-- Size:union

sz = Size(4, 2):union(Size(1, 8))
assert(sz.width == 4)
assert(sz.height == 8)
