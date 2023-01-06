-- Copyright (C) 2019-2020  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

function expect_eq(a, b)
  if a ~= b then
    print(debug.traceback())
    print('Expected A == B but:')
    print(' - Value A = ' .. tostring(a))
    print(' - Value B = ' .. tostring(b))
    assert(a == b)
  end
end

local function dump_img(image)
  local w = image.width
  local h = image.height
  print('Image(' .. tostring(w) .. 'x' .. tostring(h) .. ') = {')
  for v=0,h-1 do
    local lineStr = '  '
    for u=0,w-1 do
      lineStr = lineStr .. image:getPixel(u, v) .. ','
    end
    print(lineStr)
  end
  print('}')
end

function expect_img(image, expectedPixels)
  local w = image.width
  local h = image.height
  if w*h ~= #expectedPixels then
    print(debug.traceback())
    print('Expected pixels: #=' .. #expectedPixels)
    print('Image size: w=' .. w .. ' h=' .. h .. ' #=' .. w*h)
    dump_img(image)
    assert(w*h == #expectedPixels)
  end
  for y=0,h-1 do
    for x=0,w-1 do
      local value = image:getPixel(x, y)
      local expected = expectedPixels[1+y*w+x]
      if value ~= expected then
	dump_img(image)
        print('In pixel (' .. x .. ', ' .. y .. '):')

	local a = value
	local b = expected
	print(debug.traceback())
	print('Expected A == B but:')
	if image.colorMode == ColorMode.RGB then
	  print(string.format(' - Value A = rgba(%d,%d,%d,%d)',
			      app.pixelColor.rgbaR(a),
			      app.pixelColor.rgbaG(a),
			      app.pixelColor.rgbaB(a),
			      app.pixelColor.rgbaA(a)))
	  print(string.format(' - Value B = rgba(%d,%d,%d,%d)',
			      app.pixelColor.rgbaR(b),
			      app.pixelColor.rgbaG(b),
			      app.pixelColor.rgbaB(b),
			      app.pixelColor.rgbaA(b)))
	elseif image.colorMode == ColorMode.GRAY then
	  print(string.format(' - Value A = gray(%d,%d)',
			      app.pixelColor.grayaV(a),
			      app.pixelColor.grayaA(a)))
	  print(string.format(' - Value B = gray(%d,%d)',
			      app.pixelColor.grayaV(b),
			      app.pixelColor.grayaA(b)))
	else
	  print(' - Value A = ' .. tostring(a))
	  print(' - Value B = ' .. tostring(b))
	end
	assert(a == b)
      end
    end
  end
end

function expect_img_msg(image, expectedPixels, msg)
  local status, err = pcall(expect_img, image, expectedPixels)
  if not status then
    print(msg)
    error(err)
  end
end

function array_to_pixels(array, image)
  local w = image.width
  local h = image.height
  assert(w*h == #array)
  local i = 1
  for y=0,h-1 do
    for x=0,w-1 do
      image:drawPixel(x, y, array[i])
      i = i+1
    end
  end
end

-- Returns true if the given array (layers) of layers
function expect_rendered_layers(expectedImage, sprite, layerNames, frame)
  function contains_layer(name)
    for _,n in ipairs(layerNames) do
      if name == n then return true end
    end
    return false
  end

  if frame == nil then frame = 1 end
  local render = Image(sprite.spec)
  function render_layers(prefix, layers)
    for _,layer in ipairs(layers) do
      if layer.isGroup then
	render_layers(layer.name.."/", layer.layers)
      end
      if contains_layer(prefix..layer.name) then
	local cel = layer:cel(frame)
	if cel then
	  render:drawImage(cel.image, cel.position)
	end
      end
    end
  end

  render_layers("", sprite.layers)
  if not expectedImage:isEqual(render) then
    print("Rendering:")
    for _,n in ipairs(layerNames) do
      print("  - " .. n)
    end
    error("render doesn't match to the expected image")
  end
end

-- Asserts that the passed sprites are equals without taking
-- into account:
-- * User data custom properties.
-- * Filename.
-- * Pixel ratio.
-- * Grid bounds.
function assert_sprites_eq(expectedSprite, sprite)
  assert(expectedSprite.width == sprite.width)
  assert(expectedSprite.height == sprite.height)
  assert(expectedSprite.colorMode == sprite.colorMode)
  assert(expectedSprite.transparentColor == sprite.transparentColor)
  assert_layers_eq(expectedSprite.layers, sprite.layers)
  assert_frames_eq(expectedSprite.frames, sprite.frames)
  assert_tags_eq(expectedSprite.tags, sprite.tags)
  assert_slices_eq(expectedSprite.slices, sprite.slices)
end

function assert_layers_eq(expectedLayers, layers)
  assert(#expectedLayers == #layers)
  for i = 1,#layers do
    assert_layer_eq(expectedLayers[i], layers[i])
  end
end

function assert_layer_eq(expectedLayer, layer)
  assert(expectedLayer.name == layer.name)
  assert(expectedLayer.opacity == layer.opacity)
  assert(expectedLayer.blendMode == layer.blendMode)
  assert(expectedLayer.stackIndex == layer.stackIndex)
  assert(expectedLayer.isGroup == layer.isGroup)
  assert(expectedLayer.isImage == layer.isImage)
  assert(expectedLayer.isTransparent == layer.isTransparent)
  assert(expectedLayer.isGroup == layer.isGroup)
  assert(expectedLayer.isBackground == layer.isBackground)
  assert(expectedLayer.isContinuous == layer.isContinuous)
  assert(expectedLayer.isReference == layer.isReference)
  assert(expectedLayer.color == layer.color)
  assert(expectedLayer.data == layer.data)

  if expectedLayer.isGroup then
    assert_layers_eq(expectedLayer.layers, layer.layers);
  else
    assert_cels_eq(expectedLayer.cels, layer.cels)
  end
end

function assert_frames_eq(expectedFrames, frames)
  assert(#expectedFrames == #frames)

  for i = 1,#frames do
    assert(expectedFrames[i].frameNumber == frames[i].frameNumber)
    assert(expectedFrames[i].duration == frames[i].duration)
  end
end

function assert_cels_eq(expectedCels, cels)
  assert(#expectedCels == #cels)

  for i = 1,#cels do
    local expCel = expectedCels[i]
    local cel = cels[i]
    assert(expCel.frameNumber == cel.frameNumber)
    assert(expCel.bounds.x == cel.bounds.x)
    assert(expCel.bounds.y == cel.bounds.y)
    assert(expCel.bounds.width == cel.bounds.width)
    assert(expCel.bounds.height == cel.bounds.height)
    assert(expCel.position.x == cel.position.x)
    assert(expCel.position.y == cel.position.y)
    assert(expCel.opacity == cel.opacity)
    assert(expCel.color == cel.color)
    assert(expCel.data == cel.data)
    assert(expCel.image:isEqual(cel.image))
  end
end

function assert_tags_eq(expectedTags, tags)
  assert(#expectedTags == #tags)
  for i = 1,#tags do
    local expTag = expectedTags[i]
    local tag = tags[i]
    local a = expTag.fromFrame
    local b = tag.fromFrame

    assert(expTag.fromFrame.frameNumber == tag.fromFrame.frameNumber)
    assert(expTag.toFrame.frameNumber == tag.toFrame.frameNumber)
    assert(expTag.name == tag.name)
    assert(expTag.aniDir == tag.aniDir)
    assert(expTag.color == tag.color)
    assert(expTag.data == tag.data)
  end
end

function assert_slices_eq(expectedSlices, slices)
  assert(#expectedSlices == #slices)
  for i = 1,#slices do
    local expSlice = expectedSlices[i]
    local slice = slices[i]
    assert(expSlice.bounds.x == slice.bounds.x)
    assert(expSlice.bounds.y == slice.bounds.y)
    assert(expSlice.bounds.width == slice.bounds.width)
    assert(expSlice.bounds.height == slice.bounds.height)
    assert(expSlice.color == slice.color)
    assert(expSlice.data == slice.data)
    assert(expSlice.name == slice.name)

    if expSlice.center == nil or slice.center == nil then
      assert(expSlice.center == slice.center)
    else
      assert(expSlice.center.x == slice.center.x)
      assert(expSlice.center.y == slice.center.y)
      assert(expSlice.center.width == slice.center.width)
      assert(expSlice.center.height == slice.center.height)
    end

    if expSlice.pivot == nil or slice.pivot == nil then
      assert(expSlice.pivot == slice.pivot)
    else
      assert(expSlice.pivot.x == slice.pivot.x)
      assert(expSlice.pivot.y == slice.pivot.y)
    end
  end
end
