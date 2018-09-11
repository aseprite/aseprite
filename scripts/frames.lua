-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local a = Sprite(32, 32)
  assert(#a.frames == 1)
  assert(a.frames[1].frameNumber == 1)
  assert(a.frames[1].duration == 0.1)

  local fr = a:newFrame()
  assert(#a.frames == 2)
  assert(a.frames[1].frameNumber == 1)
  assert(a.frames[2].frameNumber == 2)
  assert(a.frames[1].duration == 0.1)
  assert(a.frames[2].duration == 0.1)

  fr.duration = 0.2
  assert(a.frames[1].duration == 0.1)
  assert(a.frames[2].duration == 0.2)

  a:deleteFrame(1)
  assert(a.frames[1].duration == 0.2)
end
