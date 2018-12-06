-- Copyright (C) 2018  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local a = Sprite(32, 32)
  assert(#a.frames == 1)
  assert(a.frames[1].frameNumber == 1)
  assert(a.frames[1].duration == 0.1)
  a.frames[1].duration = 0.2
  assert(a.frames[1].duration == 0.2)
  assert(a.layers[1]:cel(1) ~= nil)

  -- Sprite:newEmptyFrame()
  local fr0 = a:newEmptyFrame()
  assert(fr0.frameNumber == 2) -- returned the second frame
  assert(#a.frames == 2)
  assert(a.frames[1].frameNumber == 1)
  assert(a.frames[2].frameNumber == 2)
  assert(a.frames[1].duration == 0.2)
  assert(a.frames[2].duration == 0.2) -- the duration is copied
  assert(a.layers[1]:cel(1) ~= nil)
  assert(a.layers[1]:cel(fr0) == nil) -- no cel
  assert(fr0 == a.frames[2])
  a:deleteFrame(fr0)
  assert(#a.frames == 1)

  -- Sprite:newFrame() without arguments
  local fr = a:newFrame()
  assert(fr.frameNumber == 2) -- returned the second frame
  assert(#a.frames == 2)
  assert(a.frames[1].frameNumber == 1)
  assert(a.frames[2].frameNumber == 2)
  assert(a.frames[1].duration == 0.2)
  assert(a.frames[2].duration == 0.2)
  assert(fr == a.frames[2])

  fr.duration = 0.3
  assert(a.frames[1].duration == 0.2)
  assert(a.frames[2].duration == 0.3)

  local i = 1
  for k,v in ipairs(a.frames) do
    assert(i == k)
    assert(v == a.frames[k])
    i = i+1
  end

  a:deleteFrame(1)
  assert(#a.frames == 1)
  assert(a.frames[1].duration == 0.3)

  -- TODO This is a big issue, if we add/delete frames, we don't
  --      update frame objects, they are still pointing to the same frame
  --      number, which could lead to confusion.
  assert(fr.frameNumber == 2)
  fr.duration = 1   -- This is a do nothing operation

  -- Sprite:newEmptyFrame(n)
  local fr2 = a:newEmptyFrame(1)
  assert(#a.frames == 2)
  assert(fr2.frameNumber == 1) -- returned the first frame
  print(a.frames[1].duration)
  assert(a.frames[1].duration == 0.3) -- the duration is copied from the frame
  assert(a.frames[2].duration == 0.3)
  assert(fr2 == a.frames[1])
  assert(a.layers[1]:cel(1) == nil)
  assert(a.layers[1]:cel(2) ~= nil)

  -- Sprite:newFrame(n)
  local fr3 = a:newFrame(2)
  assert(#a.frames == 3)
  assert(fr3.frameNumber == 2) -- returned the second frame
  assert(a.frames[1].duration == 0.3)
  assert(a.frames[2].duration == 0.3) -- copied duration from old frame 2
  assert(a.frames[3].duration == 0.3)
  local cel1 = a.layers[1]:cel(1)
  local cel2 = a.layers[1]:cel(2)
  local cel3 = a.layers[1]:cel(3)
  assert(cel1 == nil)
  assert(cel2 ~= nil)
  assert(cel3 ~= nil)
  print(cel2.image.spec.colorMode)
  print(cel2.image.spec.width)
  print(cel2.image.spec.height)
  print(cel3.image.spec.colorMode)
  print(cel3.image.spec.width)
  print(cel3.image.spec.height)
  assert(cel2.image.spec == cel3.image.spec)
  assert(fr3.previous == a.frames[1])
  assert(fr3 == a.frames[2])
  assert(fr3.next == a.frames[3])
end
