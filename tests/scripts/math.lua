-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

assert(100 == math.abs(-100))
assert(100 == math.min(100, 200, 300))
assert(300 == math.max(100, 200, 300))
assert(50 == math.fmod(250, 100))
assert(3141 == math.floor(1000*math.pi))

-- Add tests for integer <-> number comparisons
assert(math.floor(0.5) == 0)
assert(math.floor(0.5) == 0.0)
assert(math.floor(0.5) ~= 0.5)
assert(math.floor(0.5)+0.1-0.1 ~= 0.5)
