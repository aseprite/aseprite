-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

-- Preferences for tools
do
  local t = app.preferences.tool('pencil')
  assert(t.opacity == 255)
  assert(t.tolerance == 0)
  assert(t.contiguous == true)
  assert(t.brush.type == BrushType.CIRCLE)
  assert(t.brush.size == 1)
  t.brush.size = 2
  assert(t.brush.size == 2)

  -- Getting the tool again will give us the default configuration
  -- again in batch mode
  t = app.preferences.tool('pencil')
  assert(t.brush.size == 1)
end

-- Preferences for documents
do
  local s = Sprite(32, 32)
  local p = app.preferences.document(s)
  assert(p.grid.bounds == Rectangle(0, 0, 16, 16))
  assert(p.grid.color == Color(0, 0, 255))
  p.grid.color = Color(255, 0, 0)
  assert(p.grid.color == Color(255, 0, 0))
end
