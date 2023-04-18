-- Copyright (c) 2023  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local mod = require 'require_module'
assert(mod.a == 5)
mod.a = 6

local mod2 = require 'require_module'
assert(mod2.a == 6) -- Now mod and mod2 points to the same object
