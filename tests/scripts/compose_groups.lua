-- Copyright (C) 2024  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

-- Enable experimental compose groups feature
app.preferences.experimental.compose_groups = true

function create_group_layer()
  local s = Sprite(2, 2)
  assert(#s.layers == 1)

  -- create two layers and a group layer
  local a = s.layers[1]   a.name = "a"
  assert(#s.layers == 1)
  local b = s:newLayer()  b.name = "b"
  assert(#s.layers == 2)
  local g = s:newGroup()  g.name = "g"
  assert(#s.layers == 3)

  -- b is child of g
  b.parent = g
  assert(g.parent == s)
  assert(b.parent == g)

  assert(#s.layers == 2)
  assert(s.layers[1] == a)
  assert(s.layers[2] == g)

  return s, a, b, g
end

-- Test groups opacity to impact on children layers
do
  assert(app.preferences.experimental.compose_groups == true)

  local s, a, b, g = create_group_layer()

	-- draw in b layer
	local cel = s:newCel(b, 1, Image(2, 2, ColorMode.RGB))
	local img = cel.image

	img:drawPixel(0, 0, Color(255, 0, 0, 255))
	img:drawPixel(1, 0, Color(0, 255, 0, 255))
	img:drawPixel(0, 1, Color(0, 0, 255, 255))
	img:drawPixel(1, 1, Color(255, 255, 0, 255))

	local r = Image(s.spec) -- render
	r:drawSprite(s, 1, 0, 0)

	expect_clr(r:getPixel(0, 0), app.pixelColor.rgba(255, 0, 0, 255))
	expect_clr(r:getPixel(1, 0), app.pixelColor.rgba(0, 255, 0, 255))
	expect_clr(r:getPixel(0, 1), app.pixelColor.rgba(0, 0, 255, 255))
	expect_clr(r:getPixel(1, 1), app.pixelColor.rgba(255, 255, 0, 255))

	-- Set opacity to 50%
	g.opacity = 128

	r = Image(s.spec)
	r:drawSprite(s, 1, 0, 0)

	print(g.opacity)

	print(r:getPixel(0, 0), app.pixelColor.rgba(255, 0, 0, 255))

	-- Assert that the image is drawn with 50% opacity
	expect_clr(r:getPixel(0, 0), app.pixelColor.rgba(255, 0, 0, 128))
	expect_clr(r:getPixel(1, 0), app.pixelColor.rgba(0, 255, 0, 128))
	expect_clr(r:getPixel(0, 1), app.pixelColor.rgba(0, 0, 255, 128))
	expect_clr(r:getPixel(1, 1), app.pixelColor.rgba(255, 255, 0, 128))

	-- Set opacity to 100%
	g.opacity = 255

	r = Image(s.spec)
	r:drawSprite(s, 1, 0, 0)

	expect_eq(g.opacity, 255)

	-- Assert that the image is drawn with 100% opacity
	expect_clr(r:getPixel(0, 0), app.pixelColor.rgba(255, 0, 0, 255))
	expect_clr(r:getPixel(1, 0), app.pixelColor.rgba(0, 255, 0, 255))
	expect_clr(r:getPixel(0, 1), app.pixelColor.rgba(0, 0, 255, 255))
	expect_clr(r:getPixel(1, 1), app.pixelColor.rgba(255, 255, 0, 255))

	-- Set group opacity to 50% and layer opacity to 50%
	g.opacity = 128
	b.opacity = 128

	r = Image(s.spec)
	r:drawSprite(s, 1, 0, 0)

	assert(g.opacity == 128)
	assert(b.opacity == 128)

	-- Assert that the image is drawn with 25% opacity
	expect_clr(r:getPixel(0, 0), app.pixelColor.rgba(255, 0, 0, 64))
	expect_clr(r:getPixel(1, 0), app.pixelColor.rgba(0, 255, 0, 64))
	expect_clr(r:getPixel(0, 1), app.pixelColor.rgba(0, 0, 255, 64))
	expect_clr(r:getPixel(1, 1), app.pixelColor.rgba(255, 255, 0, 64))

	-- Create a new layer in front of the group and check that it's not affected by the group opacity
	local c = s:newLayer()
	c.name = "c"

	-- draw in c layer
	local cel = s:newCel(c, 1, Image(1, 1, ColorMode.RGB))
	local img = cel.image

	img:drawPixel(0, 0, Color(255, 0, 255, 255))

	r = Image(s.spec)
	r:drawSprite(s, 1, 0, 0)

	-- Assert that the first pixel is drawn with 100% opacity and the remaining ones with 25% opacity
	expect_clr(r:getPixel(0, 0), app.pixelColor.rgba(255, 0, 255, 255))
	expect_clr(r:getPixel(1, 0), app.pixelColor.rgba(0, 255, 0, 64))
	expect_clr(r:getPixel(0, 1), app.pixelColor.rgba(0, 0, 255, 64))
	expect_clr(r:getPixel(1, 1), app.pixelColor.rgba(255, 255, 0, 64))

end

do -- test group blend to impact children layers
  assert(app.preferences.experimental.compose_groups == true)

  local s, a, b, g = create_group_layer()

  -- draw all black in layer a
  local cel = s:newCel(a, 1, Image(2, 2, ColorMode.RGB))
  local img = cel.image

  img:drawPixel(0, 0, Color(0, 0, 0, 255))
  img:drawPixel(1, 0, Color(0, 0, 0, 255))
  img:drawPixel(0, 1, Color(0, 0, 0, 255))
  img:drawPixel(1, 1, Color(0, 0, 0, 255))

  -- draw in b layer
  local cel = s:newCel(b, 1, Image(2, 2, ColorMode.RGB))
  local img = cel.image

  img:drawPixel(0, 0, Color(255, 0, 0, 255))
  img:drawPixel(1, 0, Color(0, 255, 0, 255))
  img:drawPixel(0, 1, Color(0, 0, 255, 255))
  img:drawPixel(1, 1, Color(255, 255, 0, 255))

  local r = Image(s.spec) -- render
  r:drawSprite(s, 1, 0, 0)

  expect_clr(r:getPixel(0, 0), app.pixelColor.rgba(255, 0, 0, 255))
  expect_clr(r:getPixel(1, 0), app.pixelColor.rgba(0, 255, 0, 255))
  expect_clr(r:getPixel(0, 1), app.pixelColor.rgba(0, 0, 255, 255))
  expect_clr(r:getPixel(1, 1), app.pixelColor.rgba(255, 255, 0, 255))

  -- Set blend to MULTIPLY
  g.blendMode = BlendMode.MULTIPLY

  r = Image(s.spec)
  r:drawSprite(s, 1, 0, 0)

  -- Assert that the image is drawn with MULTIPLY blend mode
  expect_clr(r:getPixel(0, 0), app.pixelColor.rgba(0, 0, 0, 255))
  expect_clr(r:getPixel(1, 0), app.pixelColor.rgba(0, 0, 0, 255))
  expect_clr(r:getPixel(0, 1), app.pixelColor.rgba(0, 0, 0, 255))
  expect_clr(r:getPixel(1, 1), app.pixelColor.rgba(0, 0, 0, 255))

  -- Create a new layer in front of the group and check that it's not affected by the group blend mode
  local c = s:newLayer()
  c.name = "c"

  -- draw in c layer
  local cel = s:newCel(c, 1, Image(1, 1, ColorMode.RGB))
  local img = cel.image

  img:drawPixel(0, 0, Color(255, 0, 255, 255))

  r = Image(s.spec)
  r:drawSprite(s, 1, 0, 0)

  -- Assert that the first pixel is drawn with Normal blend mode and the remaining ones with Multiply blend mode
  expect_clr(r:getPixel(0, 0), app.pixelColor.rgba(255, 0, 255, 255))
  expect_clr(r:getPixel(1, 0), app.pixelColor.rgba(0, 0, 0, 255))
  expect_clr(r:getPixel(0, 1), app.pixelColor.rgba(0, 0, 0, 255))
  expect_clr(r:getPixel(1, 1), app.pixelColor.rgba(0, 0, 0, 255))
end