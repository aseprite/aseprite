-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

-- Create global variable which will be GC'd on lua_close()
a = { }

-- Call print() on __gc, in previous version this produced a crash at exit
setmetatable(a, { __gc=function() print('gc') end })
