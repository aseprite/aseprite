-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local none = ColorSpace()         -- None
local srgb = ColorSpace{ sRGB }
assert(none ~= srgb)

local spr = Sprite(32, 32)
local cs1 = spr.colorSpace
local cs2 = spr.spec.colorSpace
assert(cs1.name == "sRGB")    -- Default color profile: sRGB
assert(cs1 == cs2)
assert(cs1 ~= none)
assert(cs1 == srgb)

local spr3 = Sprite(32, 32)
local cs3 = spr.spec.colorSpace
assert(cs1 == cs3)
