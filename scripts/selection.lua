-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

-- Isolated selection
local a = Selection()
assert(a.bounds.x == 0)
assert(a.bounds.y == 0)
assert(a.bounds.width == 0)
assert(a.bounds.height == 0)
assert(a.isEmpty)

a:select(1, 2, 3, 4)
assert(a.bounds.x == 1)
assert(a.bounds.y == 2)
assert(a.bounds.width == 3)
assert(a.bounds.height == 4)
assert(not a.isEmpty)
assert(a:contains(1, 2))
assert(a:contains(1+3-1, 2+4-1))
assert(not a:contains(0, 1))
assert(not a:contains(1+3, 2+4))

a:select{x=5, y=6, width=7, height=8}
assert(a.bounds.x == 5)
assert(a.bounds.y == 6)
assert(a.bounds.width == 7)
assert(a.bounds.height == 8)

a:deselect()
assert(a.bounds.x == 0)
assert(a.bounds.y == 0)
assert(a.bounds.width == 0)
assert(a.bounds.height == 0)
assert(a.isEmpty)
assert(not a:contains(0, 0))
