-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local pc = app.pixelColor
assert(0 == pc.rgba(0, 0, 0, 0))
assert(1 == pc.rgbaR(pc.rgba(1, 2, 3, 4)))
assert(2 == pc.rgbaG(pc.rgba(1, 2, 3, 4)))
assert(3 == pc.rgbaB(pc.rgba(1, 2, 3, 4)))
assert(4 == pc.rgbaA(pc.rgba(1, 2, 3, 4)))
assert(0xff104080 == pc.rgba(0x80, 0x40, 0x10, 0xff))
