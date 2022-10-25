-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

assert("" ~= os.getenv("PATH"))
print("PATH", os.getenv("PATH"))

local start_clock = os.clock()
print("Start ", start_clock)

local end_clock = os.clock()
print("End ", end_clock, " Elapsed ", end_clock - start_clock)
assert(start_clock < end_clock)
