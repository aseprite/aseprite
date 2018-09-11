-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local a = Sprite(32, 64)
  local l = a.layers[1]
  assert(l.opacity == 255)
  assert(l.blendMode == BlendMode.NORMAL)

  l.name = "My Layer"
  l.opacity = 128
  l.blendMode = BlendMode.MULTIPLY
  assert(l.name == "My Layer")
  assert(l.opacity == 128)
  assert(l.blendMode == BlendMode.MULTIPLY)
end
