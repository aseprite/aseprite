-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local none = ColorSpace()         -- None
local srgb = ColorSpace{ sRGB=true }
local none2 = ColorSpace{ sRGB=false }
assert(none ~= srgb)
assert(none == none2)

local spr = Sprite(32, 32)
local cs1 = spr.colorSpace
local cs2 = spr.spec.colorSpace
assert(cs1 == cs2)
assert(cs1 ~= none)
assert(cs1 == srgb)    -- Default color profile: sRGB

local spr3 = Sprite(32, 32)
local cs3 = spr.spec.colorSpace
assert(cs1 == cs3)
