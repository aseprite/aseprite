-- Copyright (C) 2023  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local a = Uuid()
local b = a
assert(a == b)
assert(a ~= Uuid())

local s = tostring(a) -- e.g. "74341b56-4f7f-4ab1-9495-58cf5bce0e1c"
assert(a == Uuid(s))  -- Check that Uuid(string) is working
assert(#s == 36)

-- Test __index
local t = string.format("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                        a[1], a[2], a[3], a[4],
                        a[5], a[6],
                        a[7], a[8],
                        a[9], a[10],
                        a[11], a[12], a[13], a[14], a[15], a[16])
assert(s == t)

-- Out of bounds
assert(nil == a[0])
assert(nil == a[17])
