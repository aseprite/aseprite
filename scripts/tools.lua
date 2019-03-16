-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local spr = Sprite(4, 4)
local cel = spr.cels[1]
assert(cel.bounds == Rectangle(0, 0, 4, 4))

app.toolStroke{
  tool='pencil',
  color=Color{ r=0, g=0, b=0 },
  points={ Point(2, 2),
           Point(3, 2) }}
assert(cel.bounds == Rectangle(2, 2, 2, 1))

app.toolStroke{
  tool='eraser',
  points={ Point(2, 2) }}
assert(cel.bounds == Rectangle(3, 2, 1, 1))
