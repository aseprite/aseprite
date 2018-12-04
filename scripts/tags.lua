-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite(32, 32)
  for i = 1,7 do s:newFrame() end
  assert(#s.frames == 8)

  a = s:newTag(1, 2)
  b = s:newTag(3, 4)
  c = s:newTag(5, 8)

  assert(a.frames == 2)
  assert(b.frames == 2)
  assert(c.frames == 4)

  assert(a == s.tags[1])
  assert(b == s.tags[2])
  assert(c == s.tags[3])

  local i = 1
  for k,v in ipairs(s.tags) do
    assert(i == k)
    assert(v == s.tags[k])
    i = i+1
  end

  s:deleteTag(b)
  assert(a == s.tags[1])
  assert(c == s.tags[2])

  assert(c.fromFrame.frameNumber == 5)
  assert(c.toFrame.frameNumber == 8)
end
