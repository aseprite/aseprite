-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite(32, 64)
  local l = s.layers[1]
  assert(l.parent == s)
  assert(l.opacity == 255)
  assert(l.blendMode == BlendMode.NORMAL)

  l.name = "My Layer"
  l.opacity = 128
  l.blendMode = BlendMode.MULTIPLY
  assert(l.name == "My Layer")
  assert(l.opacity == 128)
  assert(l.blendMode == BlendMode.MULTIPLY)

  l.data = "Data"
  assert(l.data == "Data")
end
