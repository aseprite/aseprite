-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local sz = Size()
assert(sz.width == 0)
assert(sz.height == 0)

sz = Size(3, 4)
assert(sz.width == 3)
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
