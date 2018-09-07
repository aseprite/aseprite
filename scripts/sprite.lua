-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local a = Sprite(32, 64)
assert(a.width == 32)
assert(a.height == 64)
assert(a.colorMode == ColorMode.RGB) -- RGB by default

-- Crop and resize
a.selection:select(2, 3, 4, 5)
a:crop()
assert(a.width == 4)
assert(a.height == 5)
a:resize(6, 8)
assert(a.width == 6)
assert(a.height == 8)
a:crop{x=-1, y=-1, width=20, height=30}
assert(a.width == 20)
assert(a.height == 30)

-- Resize sprite setting width/height
a.width = 8
a.height = 10
assert(a.width == 8)
assert(a.height == 10)

local b = Sprite(4, 4, ColorMode.INDEXED)
assert(b.width == 4)
assert(b.height == 4)
assert(b.colorMode == ColorMode.INDEXED)
