-- Copyright (C) 2019-2023  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

local rgba = app.pixelColor.rgba

local a = Image(32, 64)
assert(a.id > 0)
assert(a.width == 32)
assert(a.height == 64)
assert(a.colorMode == ColorMode.RGB) -- RGB by default
assert(a.rowStride == 32*4)
assert(a:isEmpty())
assert(a:isPlain(rgba(0, 0, 0, 0)))
assert(a:isPlain(0))

do
  local b = Image(32, 64, ColorMode.INDEXED)
  assert(b.width == 32)
  assert(b.height == 64)
  assert(b.colorMode == ColorMode.INDEXED)
  assert(b.rowStride == 32*1)

  local c = Image{ width=32, height=64, colorMode=ColorMode.INDEXED }
  assert(c.width == 32)
  assert(c.height == 64)
  assert(c.colorMode == ColorMode.INDEXED)
end

-- Get/put RGBA pixels
do
  for y=0,a.height-1 do
    for x=0,a.width-1 do
      a:putPixel(x, y, rgba(x, y, x+y, x-y))
    end
  end
  assert(not a:isEmpty())
end

-- Clear
do
  local spec = ImageSpec{
    width=2, height=2,
    colorMode=ColorMode.INDEXED,
    transparentColor=1 }

  local img = Image(spec)
  img:clear()
  expect_img(img, { 1, 1,
                    1, 1 })

  img:clear(img.bounds)
  expect_img(img, { 1, 1,
                    1, 1 })

  img:clear(Rectangle(1, 0, 1, 2), 2)
  expect_img(img, { 1, 2,
                    1, 2 })
end

-- Clone
do
  local c = Image(a)
  local d = a:clone()
  assert(c.width == 32)
  assert(c.height == 64)
  assert(c.colorMode == ColorMode.RGB)
  assert(c.width == d.width)
  assert(c.height == d.height)
  assert(c.colorMode == d.colorMode)
  assert(c.id ~= d.id) -- The clone must have different ID

  -- Get RGB pixels
  for y=0,c.height-1 do
    for x=0,c.width-1 do
      local expectedColor = rgba(x, y, x+y, x-y)
      assert(c:getPixel(x, y) == expectedColor)
      assert(d:getPixel(x, y) == expectedColor)
    end
  end

  -- Clone a rectangle
  local e = Image(c, Rectangle(1, 1, 4, 5))
  print(e.width)
  print(e.height)
  assert(e.width == 4)
  assert(e.height == 5)

  -- Empty clone
  assert(nil == Image(c, Rectangle(1, 1, 0, 0)))
end

-- Patch
do
  local spr = Sprite(256, 256)
  local image = app.site.image
  local imageID = image.id
  assert(image.id > 0)
  assert(image.version == 0)
  local copy = image:clone()
  assert(image:getPixel(0, 0) == 0)
  for y=0,copy.height-1 do
    for x=0,copy.width-1 do
      copy:putPixel(x, y, rgba(255-x, 255-y, 0, 255))
    end
  end
  image:putImage(copy)
  assert(image.version == 1)
  assert(image:getPixel(0, 0) == rgba(255, 255, 0, 255))
  assert(image:getPixel(255, 255) == rgba(0, 0, 0, 255))
  app.undo()
  assert(image.version == 2)
  assert(image.id == imageID) -- the ID doesn't change
  assert(image:getPixel(0, 0) == rgba(0, 0, 0, 0))
  assert(image:getPixel(255, 255) == rgba(0, 0, 0, 0))
end

-- Load/Save
do
  local a = Image{ fromFile="sprites/1empty3.aseprite" }
  assert(a.width == 32)
  assert(a.height == 32)
  a:saveAs("_test_oneframe.png")

  local b = Image{ fromFile="_test_oneframe.png" }
  assert(b.width == 32)
  assert(b.height == 32)
  for y=0,a.height-1 do
    for x=0,a.width-1 do
      assert(a:getPixel(x, y) == b:getPixel(x, y))
    end
  end
end

-- Save indexed image and load and check that the palette is the same
do
  local spr = Sprite{ fromFile="sprites/abcd.aseprite" }
  local img = Image{ fromFile="sprites/abcd.aseprite" }
  spr.cels[1].image:saveAs("_test_palette_a.png")
  img:saveAs("_test_palette_b.png") -- This file will contain a black palette
  img:saveAs{ filename="_test_palette_c.png", palette=spr.palettes[1] }

  local a = Sprite{ fromFile="_test_palette_a.png" }
  local b = Sprite{ fromFile="_test_palette_b.png" }
  local c = Sprite{ fromFile="_test_palette_c.png" }

  assert(a.width == 5)
  assert(a.height == 7)
  assert(b.width == 32)
  assert(b.height == 32)
  assert(c.width == 32)
  assert(c.height == 32)
  local bimg = b.cels[1].image
  local cimg = c.cels[1].image
  for y=0,31 do
    for x=0,31 do
      assert(bimg:getPixel(x, y) == cimg:getPixel(x, y))
    end
  end

  local apal = a.palettes[1]
  local bpal = b.palettes[1]
  local cpal = c.palettes[1]
  -- Same palette in a and c
  assert(#apal == #cpal)
  for i=0,#apal-1 do
    assert(apal:getColor(i) == cpal:getColor(i))
  end
  -- b should contain a complete black palette
  assert(bpal:getColor(0) == Color(0, 0, 0, 0))
  for i=1,#bpal-1 do
    assert(bpal:getColor(i) == Color(0, 0, 0, 255))
  end
end

-- Resize image
do
  local a = Sprite(3, 2)
  local cel = a.cels[1]
  assert(cel.bounds == Rectangle(0, 0, 3, 2))
  local img = cel.image
  local cols = { rgba(10, 60, 1), rgba(20, 50, 2), rgba(30, 40, 3),
                 rgba(40, 30, 4), rgba(50, 20, 5), rgba(60, 10, 6) }
  array_to_pixels(cols, img)
  expect_img(img, cols)

  -- Test resize of a cel with origin=0,0

  img:resize(img.width*2, img.height*2)

  expect_eq(cel.bounds, Rectangle(0, 0, 6, 4))
  expect_eq(img.width, 6)
  expect_eq(img.height, 4)
  local cols2 = { cols[1],cols[1], cols[2],cols[2], cols[3],cols[3],
                  cols[1],cols[1], cols[2],cols[2], cols[3],cols[3],
                  cols[4],cols[4], cols[5],cols[5], cols[6],cols[6],
                  cols[4],cols[4], cols[5],cols[5], cols[6],cols[6] }
  expect_img(img, cols2)

  -- Undo
  function undo()
    app.undo()
    img = cel.image -- TODO img shouldn't be invalidated, the resize operation should kept the image ID
  end
  undo()

  -- Test a resize when cel origin > 0,0
  cel.position = Point(2, 1)
  expect_eq(cel.bounds, Rectangle(2, 1, 3, 2))
  expect_img(img, cols)
  img:resize{ width=6, height=4 }
  expect_eq(cel.bounds, Rectangle(2, 1, 6, 4)) -- Position is not modified
  expect_eq(img.width, 6)
  expect_eq(img.height, 4)

  undo()
  img:resize{ size=Size(6, 4), pivot=Point(1, 1) }
  expect_eq(cel.bounds, Rectangle(1, 0, 6, 4))

  undo()
  img:resize{ size=Size(6, 4), pivot=Point(3, 2) }
  expect_eq(cel.bounds, Rectangle(-1, -1, 6, 4))

  -- Test resize without cel

  local img2 = Image(img)
  expect_img(img2, cols2)
  img2:resize(3, 2)
  expect_img(img2, cols)
end

-- Bounds & shrink bounds
do
  local a = Image(5, 4, ColorMode.INDEXED)
  array_to_pixels({ 0, 0, 0, 0, 0,
                    0, 1, 0, 0, 0,
                    0, 0, 0, 1, 0,
                    0, 0, 0, 0, 0, }, a)

  expect_eq(a.bounds, Rectangle(0, 0, 5, 4))
  expect_eq(a:shrinkBounds(), Rectangle(1, 1, 3, 2))
  expect_eq(a:shrinkBounds(1), Rectangle(0, 0, 5, 4))

  array_to_pixels({ 2, 2, 2, 2, 2,
                    0, 1, 0, 0, 2,
                    0, 0, 0, 1, 2,
                    0, 0, 0, 0, 2, }, a)

  expect_eq(a:shrinkBounds(), Rectangle(0, 0, 5, 4))
  expect_eq(a:shrinkBounds(1), Rectangle(0, 0, 5, 4))
  expect_eq(a:shrinkBounds(2), Rectangle(0, 1, 4, 3))
end

-- Test v1.2.17 crashes
do
  local defSpec = ImageSpec{ width=1, height=1, colorMode=ColorMode.RGB }

  local img = Image() -- we create a 1x1 RGB image
  assert(img ~= nil)
  assert(img.spec == defSpec)

  img = Image(nil)
  assert(img ~= nil)
  assert(img.spec == defSpec)

  local spr = Sprite(32, 32, ColorMode.INDEXED)
  spr.cels[1].image:putPixel(15, 15, 129)
  img = Image(spr)    -- we create a sprite render of the first frame
  assert(img ~= nil)
  assert(img.spec == spr.spec)
  assert(img:getPixel(15, 15) == 129)
end

-- Fix drawImage() when drawing in a cel image
do
  local a = Image(3, 2, ColorMode.INDEXED)
  array_to_pixels({ 0, 1, 2,
                    2, 3, 4 }, a)

  local function test(b)
    expect_img(b, { 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0 })

    b:drawImage(a, Point(2, 1))
    expect_img(b, { 0, 0, 0, 0, 0,
                    0, 0, 0, 1, 2,
                    0, 0, 2, 3, 4,
                    0, 0, 0, 0, 0 })

    b:drawImage(a, Point(0, 1))
    expect_img(b, { 0, 0, 0, 0, 0,
                    0, 1, 2, 1, 2,
                    2, 3, 4, 3, 4,
                    0, 0, 0, 0, 0 })

    b:drawImage(a, Point(-1, 2))
    expect_img(b, { 0, 0, 0, 0, 0,
                    0, 1, 2, 1, 2,
                    1, 2, 4, 3, 4,
                    3, 4, 0, 0, 0 })

    b:drawImage(a, Point(0, 3))
    expect_img(b, { 0, 0, 0, 0, 0,
                    0, 1, 2, 1, 2,
                    1, 2, 4, 3, 4,
                    0, 1, 2, 0, 0 })

    b:drawImage(a, Point(0, 3)) -- Do nothing
    expect_img(b, { 0, 0, 0, 0, 0,
                    0, 1, 2, 1, 2,
                    1, 2, 4, 3, 4,
                    0, 1, 2, 0, 0 })
  end

  local b = Image(5, 4, ColorMode.INDEXED)
  test(b)

  local s = Sprite(5, 4, ColorMode.INDEXED)
  test(app.activeCel.image)

end

-- Tests using Image:drawImage() with opacity and blend modes
do
  local spr = Sprite(3, 3, ColorMode.RGB)
  local back = Image(3, 3)
  local r_255 = Color(255, 0, 0).rgbaPixel
  local g_255 = Color(0, 255, 0).rgbaPixel
  local g_127 = Color(0, 255, 0, 127).rgbaPixel
  local g_064 = Color(0, 255, 0, 64).rgbaPixel
  local r_g127 = Color(128, 127, 0, 255).rgbaPixel -- result of g_127 over r_255 (NORMAL blend mode)
  local r_g064 = Color(191, 64, 0, 255).rgbaPixel  -- result of g_064 over r_255 (NORMAL blend mode)
  local mask = Color(0, 0, 0, 0).rgbaPixel
  back:clear(r_255)
  spr:newCel(spr.layers[1], 1, back, Point(0, 0))

  local image = Image(2, 2)
  image:drawPixel(0, 0, g_127)
  image:drawPixel(1, 0, g_064)
  image:drawPixel(0, 1, mask)
  image:drawPixel(1, 1, g_255)

  local c = spr.layers[1]:cel(1).image

  expect_img(c, { r_255, r_255, r_255,
                  r_255, r_255, r_255,
                  r_255, r_255, r_255 })

  spr.layers[1]:cel(1).image:drawImage(image, Point(1, 1), 255, BlendMode.NORMAL)

  c = spr.layers[1]:cel(1).image

  expect_img(c, { r_255, r_255, r_255,
                  r_255, r_g127, r_g064,
                  r_255, r_255, g_255 })
  undo()
  c = spr.layers[1]:cel(1).image
  expect_img(c, { r_255, r_255, r_255,
                  r_255, r_255, r_255,
                  r_255, r_255, r_255 })

  -- Image:drawImage() with 50% opacity
  spr.layers[1]:cel(1).image:drawImage(image, Point(1, 1), 128, BlendMode.NORMAL)
  local r_g032 = Color(223, 32, 0, 255).rgbaPixel -- result of g_064 @oopacity=128 over r_255 (NORMAL blend mode)
  local r_g128 = Color(127, 128, 0, 255).rgbaPixel  -- result of g_255 o@oopacity=128 ver r_255 (NORMAL blend mode)
  c = spr.layers[1]:cel(1).image

  expect_img(c, { r_255, r_255, r_255,
                  r_255, r_g064, r_g032,
                  r_255, r_255, r_g128 })
  undo()
  c = spr.layers[1]:cel(1).image
  expect_img(c, { r_255, r_255, r_255,
                  r_255, r_255, r_255,
                  r_255, r_255, r_255 })

  -- Image:drawImage() without undo information (destination image
  -- not related with a cel on a sprite)
  local back2 = Image(3, 3, ColorMode.RGB)
  back2:clear(r_255)
  local image2 = Image(2, 2, ColorMode.RGB)
  image2:drawPixel(0, 0, g_127)
  image2:drawPixel(1, 0, g_064)
  image2:drawPixel(0, 1, mask)
  image2:drawPixel(1, 1, g_255)

  expect_img(back2, { r_255, r_255, r_255,
                      r_255, r_255, r_255,
                      r_255, r_255, r_255 })

  back2:drawImage(image2, Point(1, 1), 255, BlendMode.NORMAL)
  expect_img(back2, { r_255, r_255, r_255,
                      r_255, r_g127, r_g064,
                      r_255, r_255, g_255 })
  undo()
  expect_img(back2, { r_255, r_255, r_255,
                      r_255, r_g127, r_g064,
                      r_255, r_255, g_255 })
end

-- Tests for Image:flip()
function test_image_flip(img)
  local r = Color(255, 0, 0).rgbaPixel
  local g = Color(0, 255, 0).rgbaPixel
  img:clear(0)
  img:drawPixel(0, 0, g)
  img:drawPixel(1, 1, r)
  img:drawPixel(2, 2, r)
  expect_img(img, { g, 0, 0,
                    0, r, 0,
                    0, 0, r })
  img:flip()
  expect_img(img, { 0, 0, g,
                    0, r, 0,
                    r, 0, 0 })

  -- Without sprite, don't test undo
  if not app.sprite then return end

  app.undo()
  expect_img(img, { g, 0, 0,
                    0, r, 0,
                    0, 0, r })
  img:flip(FlipType.HORIZONTAL)
  expect_img(img, { 0, 0, g,
                    0, r, 0,
                    r, 0, 0 })
  app.undo()
  img:flip(FlipType.VERTICAL)
  expect_img(img, { 0, 0, r,
                    0, r, 0,
                    g, 0, 0 })
  img:flip(FlipType.VERTICAL)
  expect_img(img, { g, 0, 0,
                    0, r, 0,
                    0, 0, r })
  app.undo()
  app.undo()
  expect_img(img, { g, 0, 0,
                    0, r, 0,
                    0, 0, r })
end

local spr = Sprite(3, 3)   -- Test with sprite (with transactions & undo/redo)
test_image_flip(app.image)
app.sprite = nil           -- Test without sprite (without transactions)
test_image_flip(Image(3, 3))
