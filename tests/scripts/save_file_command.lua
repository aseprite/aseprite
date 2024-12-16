-- Copyright (C) 2022-2023  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

function fix_images(testImg, scale, fileExt, c, cm, c1)
  -- GIF file is loaded as indexed, so we have to convert from indexed
  -- to the ColorMode
  if c.colorMode ~= cm then
    assert(fileExt == "gif" or fileExt == "bmp")

    if cm == ColorMode.RGB then
      app.sprite = c
      app.command.ChangePixelFormat{ format="rgb" }
    elseif cm == ColorMode.GRAYSCALE then
      app.sprite = c
      app.command.ChangePixelFormat{ format="grayscale" }
    else
      assert(false)
    end
  end

  -- With file formats that don't support alpha channel, we
  -- compare totally transparent pixels (alpha=0) with black.
  if fileExt == "tga" and cm == ColorMode.GRAYSCALE then
    local pixel
    if cm == ColorMode.RGB then
      pixel = c1.rgbaPixel
    elseif cm == ColorMode.GRAYSCALE then
      pixel = c1.grayPixel
    else
      pixel = 0                 -- Do nothing in indexed
    end
    for p in testImg:pixels() do
      if p() == 0 then p(pixel) end
    end
  end
  testImg:resize(testImg.width*scale, testImg.height*scale)
end

function compatible_modes(fileExt, cm)
  return
    -- TODO support saving any color mode to FLI files on the fly
    (fileExt ~= "fli" or cm == ColorMode.INDEXED) and
    -- TODO Review grayscale support in bmp files
    (fileExt ~= "bmp" or cm ~= ColorMode.GRAYSCALE) and
    -- TODO Review grayscale/indexed support in webp files
    (fileExt ~= "webp" or cm == ColorMode.RGB)
end

for _,cm in ipairs{ ColorMode.RGB,
                    ColorMode.GRAYSCALE,
                    ColorMode.INDEXED } do
  local spr = Sprite(32, 32, cm)
  spr.palettes[1]:resize(3)
  spr.palettes[1]:setColor(0, Color(0, 0, 0, 0))
  spr.palettes[1]:setColor(1, Color(0, 0, 0, 255))
  spr.palettes[1]:setColor(2, Color(255, 0, 0, 255))

  local c1, c2
  if cm == ColorMode.RGB then
    c1 = Color(0, 0, 0)
    c2 = Color(255, 0, 0)
  elseif cm == ColorMode.GRAYSCALE then
    c1 = Color{ gray=0 }
    c2 = Color{ gray=255 }
  elseif cm == ColorMode.INDEXED then
    c1 = 1
    c2 = 2
  end

  app.useTool{ color=c1, brush=Brush(1),
               tool="filled_ellipse", points={{0,0},{31,31}} }
  app.useTool{ color=c2, brush=Brush(1),
               tool="filled_ellipse", points={{4,4},{27,27}} }
  spr.filename = "_test_a.aseprite"

  app.command.SaveFile()
  assert(spr.filename == "_test_a.aseprite")

  app.command.SaveFileAs{ filename="_test_b.png" }
  assert(spr.filename == "_test_b.png")

  app.command.SaveFileCopyAs{ filename="_test_c.png" }
  assert(spr.filename == "_test_b.png")

  -- Scale
  for _,fn in ipairs{ "_test_c_scaled.bmp",
                      "_test_c_scaled.fli",
                      "_test_c_scaled.gif",
                      "_test_c_scaled.png",
                      "_test_c_scaled.tga",
                      "_test_c_scaled.webp" } do
    local fileExt = app.fs.fileExtension(fn)

    if compatible_modes(fileExt, cm) then
      for _,scale in ipairs({ 0.25, 0.5, 1, 2, 3, 4 }) do
        print(fn, scale, cm)

        app.sprite = spr
        app.command.SaveFileCopyAs{ filename=fn, scale=scale }
        local c = app.open(fn)
        assert(c.width == spr.width*scale)
        assert(c.height == spr.height*scale)

        local testImg = Image(spr.cels[1].image)
        fix_images(testImg, scale, fileExt, c, cm, c1)
        if not c.cels[1].image:isEqual(testImg) then
          c.cels[1].image:saveAs("_testA.png")
          testImg:saveAs("_testB.png")
        end
        assert(c.cels[1].image.colorMode == testImg.colorMode)
        assert(c.cels[1].image:isEqual(testImg))
      end
    end
  end

  -- Scale + Slices
  local slice = spr:newSlice(Rectangle(1, 2, 8, 15))
  slice.name = "small_slice"
  for _,fn in ipairs({ "_test_c_small_slice.bmp",
                       "_test_c_small_slice.fli",
                       "_test_c_small_slice.gif",
                       "_test_c_small_slice.png",
                       "_test_c_small_slice.tga",
                       "_test_c_small_slice.webp" }) do
    local fileExt = app.fs.fileExtension(fn)

    if compatible_modes(fileExt, cm) then
      for _,scale in ipairs({ 0.25, 0.5, 1, 2, 3, 4 }) do
        print(fn, scale, cm)

        app.sprite = spr
        app.command.SaveFileCopyAs{ filename=fn, slice="small_slice", scale=scale }
        local c = app.open(fn)
        assert(c.width == math.floor(slice.bounds.width*scale))
        assert(c.height == math.floor(slice.bounds.height*scale))

        local testImg = Image(spr.cels[1].image, spr.slices[1].bounds)
        fix_images(testImg, scale, fileExt, c, cm, c1)
        if not c.cels[1].image:isEqual(testImg) then
          c.cels[1].image:saveAs("_testA.png")
          testImg:saveAs("_testB.png")
        end
        assert(c.cels[1].image:isEqual(testImg))
      end
    end
  end

end
