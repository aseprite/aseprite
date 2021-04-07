-- Copyright (C) 2019-2021  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

assert(string.sub(tostring(app.version), 1, 1) == "1")
assert(string.sub(tostring(app.version), 2, 2) == ".")
assert(app.version.major == 1)

-- We cannot test the specific app.version from the "main" branch
-- because it's "1.x-dev" (which is converted to "1.0-dev" as Version object)
--assert(app.version > Version("1.2.10-beta4"))

assert(Version("1") == Version("1"))
assert(Version("1.1") > Version("1"))
assert(Version("0.1") < Version("0.2"))
assert(Version("1.0.1") > Version("1"))
assert(Version("1.0.1") < Version("1.1"))
assert(Version("1.0.1") == Version("1.0.1"))
assert(Version("1.0.1") ~= Version("1.0.2"))
assert(Version("1.0.1") > Version("1.0.1-beta"))
assert(Version("1.0.1") > Version("1.0.1-dev"))
assert(Version("1.0.1-beta") > Version("1.0.1-alpha"))
assert(Version("1.0.1-beta50") < Version("1.0.1-beta100"))
assert(Version("1.0.1-beta50") <= Version("1.0.1-beta100"))
assert(Version("1.0.1-beta100") <= Version("1.0.1-beta100"))

local v = Version()
assert(v.major == 0)
assert(v.minor == 0)
assert(v.patch == 0)
assert(v.prereleaseLabel == "")
assert(v.prereleaseNumber == 0)

v = Version("1.2.10-beta4")
assert(v.major == 1)
assert(v.minor == 2)
assert(v.patch == 10)
assert(v.prereleaseLabel == "beta")
assert(v.prereleaseNumber == 4)
