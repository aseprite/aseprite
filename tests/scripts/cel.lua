-- Copyright (C) 2023  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local s = Sprite(32, 64)
assert(#s.layers == 1)
assert(#s.cels == 1)

local c = s.cels[1]
assert(c.sprite == s)
assert(c.layer == s.layers[1])
assert(c.frame == s.frames[1])
assert(c.frame.frameNumber == 1)
assert(c.frameNumber == 1)
assert(c.image)
assert(c.bounds == Rectangle(0, 0, 32, 64))
assert(c.position == Point(0, 0))
assert(c.color == Color())
assert(c.data == "")

c.color = Color{ r=255, g=100, b=20 }
c.data = "test"
assert(c.color == Color{ r=255, g=100, b=20 })
assert(c.data == "test")

c.position = Point(2, 4)
assert(c.position == Point(2, 4))
assert(c.bounds == Rectangle(2, 4, 32, 64))

assert(c.opacity == 255)
c.opacity = 128
assert(c.opacity == 128)

assert(c.zIndex == 0)
c.zIndex = -2
assert(c.zIndex == -2)
