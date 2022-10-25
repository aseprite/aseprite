-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

assert(72 == string.byte("Hello", 1))
assert(101 == string.byte("Hello", 2))
assert(111 == string.byte("Hello", 5))
assert("Hello" == string.char(72, 101, 108, 108, 111))

local s = "Hello"
assert(111 == s:byte(5))
assert(5 == string.len(s))
assert("olleH" == string.reverse(s))
assert("hello" == string.lower(s))
assert("HELLO" == string.upper(s))

assert("Simple int 32" == string.format("Simple int %d", 32))
assert("Simple int 0032" == string.format("Simple int %04d", 32))

assert(7 == string.find("Hello World!", "W"))

assert("-- hi 1000 --" == string.format("-- %s %d --", "hi", 1000))
