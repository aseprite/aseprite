-- Copyright (C) 2022  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local spr = Sprite(32, 32, ColorMode.INDEXED)
  spr.palettes[1]:resize(3)
  spr.palettes[1]:setColor(0, Color(0, 0, 0, 0))
  spr.palettes[1]:setColor(1, Color(0, 0, 0, 255))
  spr.palettes[1]:setColor(2, Color(255, 0, 0, 255))

  app.useTool{ color=1, brush=Brush(1),
               tool="filled_ellipse", points={{0,0},{31,31}} }
  app.useTool{ color=2, brush=Brush(1),
               tool="filled_ellipse", points={{4,4},{27,27}} }
  spr.filename = "_test_a.aseprite"
  app.command.SaveFile()
  assert(spr.filename == "_test_a.aseprite")

  app.command.SaveFileAs{ filename="_test_b.png" }
  assert(spr.filename == "_test_b.png")

  app.command.SaveFileCopyAs{ filename="_test_c.png" }
  assert(spr.filename == "_test_b.png")

  -- Scale
  for _,fn in ipairs({ "_test_c_scaled.png",
                       "_test_c_scaled.gif",
                       "_test_c_scaled.fli",
                       "_test_c_scaled.tga",
                       "_test_c_scaled.bmp" }) do
    for _,scale in ipairs({ 1, 2, 4 }) do
      app.activeSprite = spr
      app.command.SaveFileCopyAs{ filename=fn, scale=scale }
      local c = app.open(fn)
      assert(c.width == spr.width*scale)
      assert(c.height == spr.height*scale)

      local testImg = Image(spr.cels[1].image)
      testImg:resize(testImg.width*scale, testImg.height*scale)
      assert(c.cels[1].image:isEqual(testImg))
    end
  end

  -- Scale + Slices
  local slice = spr:newSlice(Rectangle(1, 2, 8, 15))
  slice.name = "small_slice"
  for _,fn in ipairs({ "_test_c_small_slice.png",
                       -- Slices aren't supported in gif/fli yet
                       --"_test_c_small_slice.gif",
                       --"_test_c_small_slice.fli",
                       "_test_c_small_slice.tga",
                       "_test_c_small_slice.bmp" }) do
    for _,scale in ipairs({ 1, 2, 4 }) do
      app.activeSprite = spr
      app.command.SaveFileCopyAs{ filename=fn, slice="small_slice", scale=scale }
      local c = app.open(fn)
      assert(c.width == 8*scale)
      assert(c.height == 15*scale)

      local testImg = Image(spr.cels[1].image, spr.slices[1].bounds)
      testImg:resize(testImg.width*scale, testImg.height*scale)
      assert(c.cels[1].image:isEqual(testImg))
    end
  end

end
