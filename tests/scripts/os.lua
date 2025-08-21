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

-- os.rename & remove
assert(app.fs.makeDirectory("_os_tmp"))
assert(os.rename("_os_tmp", "_os_tmp_renamed"))
assert(not os.rename("_os_tmp", "_os_tmp_shouldfail"))
assert(not os.remove("_os_tmp_shouldfail"))
assert(not os.rename("_os_tmp_shouldfail", "_test1.png")) -- Should fail, already exists
assert(os.remove("_os_tmp_renamed"))
assert(not app.fs.isDirectory("_os_tmp"))
assert(not app.fs.isDirectory("_os_tmp_renamed"))
