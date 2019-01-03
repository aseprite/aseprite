-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local spr = Sprite(32, 32)
local cs = spr.spec.colorSpace
assert(cs.name == "sRGB")    -- Default color profile

local spr2 = Sprite(32, 32)
local cs2 = spr.spec.colorSpace
assert(cs == cs2)
