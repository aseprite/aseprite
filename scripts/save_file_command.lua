-- Copyright (C) 2022  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local spr = Sprite(32, 32)
  spr.filename = "_test_a.png"
  app.command.SaveFile()
  assert(spr.filename == "_test_a.png")

  app.command.SaveFileAs{ filename="_test_b.png" }
  assert(spr.filename == "_test_b.png")

  app.command.SaveFileCopyAs{ filename="_test_c.png" }
  assert(spr.filename == "_test_b.png")
end
