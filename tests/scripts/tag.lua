-- Copyright (C) 2020-2023  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite(32, 32)
  for i = 1,7 do s:newFrame() end
  assert(#s.frames == 8)

  a = s:newTag(1, 8)
  assert(a.sprite == s)
  assert(a.fromFrame.frameNumber == 1)
  assert(a.toFrame.frameNumber == 8)
  assert(a.frames == 8)

  assert(app.tag == a)

  a.fromFrame = 2
  a.toFrame = 5
  assert(a.fromFrame.frameNumber == 2)
  assert(a.toFrame.frameNumber == 5)

  assert(a.name == "Tag")
  a.name = "Tag A"
  assert(a.name == "Tag A")

  assert(a.aniDir == AniDir.FORWARD) -- Default AniDir is FORWARD
  a.aniDir = AniDir.REVERSE
  assert(a.aniDir == AniDir.REVERSE)
  a.aniDir = AniDir.PING_PONG
  assert(a.aniDir == AniDir.PING_PONG)
  a.aniDir = AniDir.FORWARD
  assert(a.aniDir == AniDir.FORWARD)

  assert(a.color == Color(0, 0, 0))
  a.color = Color(255, 0, 0)
  assert(a.color == Color(255, 0, 0))

  assert(a.repeats == 0)
  a.repeats = 1
  assert(a.repeats == 1)

  a.data = "Data"
  assert(a.data == "Data")
end
