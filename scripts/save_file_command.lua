-- Copyright (C) 2022  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local spr = Sprite(32, 32)
  app.useTool{ color=Color(255, 255, 255), brush=Brush(1),
               tool="filled_ellipse", points={{0,0},{31,31}} }
  spr.filename = "_test_a.aseprite"
  app.command.SaveFile()
  assert(spr.filename == "_test_a.aseprite")

  app.command.SaveFileAs{ filename="_test_b.png" }
  assert(spr.filename == "_test_b.png")

  app.command.SaveFileCopyAs{ filename="_test_c.png" }
  assert(spr.filename == "_test_b.png")

  -- Slices
  local slice = spr:newSlice(Rectangle(1, 2, 8, 15))
  slice.name = "small_slice"
  app.command.SaveFileCopyAs{ filename="_test_c_small_slice.png", slice="small_slice" }
  local c = app.open("_test_c_small_slice.png")
  assert(c.width == 8)
  assert(c.height == 15)
end
